/*
 * Copyright (C) 2003 Trevor van Bremen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1,
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA
 */

#define NEED_VBINLINE_INT_LOAD 1
#define NEED_VBINLINE_INT_STORE 1
#define NEED_VBINLINE_LONG_LOAD 1
#define NEED_VBINLINE_LONG_STORE 1
#define NEED_VBINLINE_QUAD_LOAD 1
#include	"isinternal.h"

/* Local functions */

static int
itreeload (const int ihandle, const int ikeynumber, const int ilength,
           VB_UCHAR *pckeyvalue, off_t tdupnumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct VBKEY    *pskey;
    struct VBTREE   *pstree;
    struct DICTINFO *psvbptr;
    struct keydesc  *pskptr;
    unsigned int    idelta, iindex;
    int     iresult = 0;

    psvbptr = vb_rtd->psvbfile[ihandle];
    pstree = psvbptr->pstree[ikeynumber];
    pskptr = psvbptr->pskeydesc[ikeynumber];
    if (!pstree) {
        pstree = psvbtreeallocate (ihandle);
        psvbptr->pstree[ikeynumber] = pstree;
        vb_rtd->iserrno = errno;
        if (!pstree) {
            goto treeload_exit;
        }
        pstree->iisroot = 1;
        pstree->iistof = 1;
        pstree->iiseof = 1;
        vb_rtd->iserrno = ivbnodeload (ihandle, ikeynumber, pstree,
                                       pskptr->k_rootnode, -1);
        if (vb_rtd->iserrno) {
            vvbtreeallfree (ihandle, ikeynumber, pstree);
            psvbptr->pstree[ikeynumber] = NULL;
            psvbptr->pskeycurr[ikeynumber] = NULL;
            goto treeload_exit;
        }
    } else if (psvbptr->iindexchanged) {
        vb_rtd->iserrno = ivbnodeload (ihandle, ikeynumber, pstree,
                                       pskptr->k_rootnode, -1);
        if (vb_rtd->iserrno) {
            vvbtreeallfree (ihandle, ikeynumber, pstree);
            psvbptr->pstree[ikeynumber] = NULL;
            psvbptr->pskeycurr[ikeynumber] = NULL;
            goto treeload_exit;
        }
    }
    vb_rtd->iserrno = EBADFILE;
    if (pstree->tnodenumber != pskptr->k_rootnode) {
        goto treeload_exit;
    }
    pstree->iisroot = 1;
    pstree->iistof = 1;
    pstree->iiseof = 1;
    while (1) {
/* The following code takes a 'bisection' type approach for location of the */
/* key entry.  It FAR outperforms the original sequential search code. */
        idelta = 1;
        iindex = pstree->ikeysinnode;
        while (iindex) {
            idelta = idelta << 1;
            iindex = iindex >> 1;
        }
        iindex = idelta;
        while (idelta) {
            idelta = idelta >> 1;
            if (iindex > pstree->ikeysinnode) {
                iindex -= idelta;
                continue;
            }
            pstree->pskeycurr = pstree->pskeylist[iindex - 1];
            if (pstree->pskeycurr->iisdummy) {
                iresult = -1;
            } else {
                iresult = ivbkeycompare (ihandle, ikeynumber, ilength,
                                         pckeyvalue, pstree->pskeycurr->ckey);
            }
            if (iresult == 0) {
                if (tdupnumber > pstree->pskeycurr->tdupnumber) {
                    iresult = 1;
                    iindex += idelta;
                    continue;
                }
                if (tdupnumber < pstree->pskeycurr->tdupnumber) {
                    iresult = -1;
                    iindex -= idelta;
                    continue;
                }
                if (tdupnumber == pstree->pskeycurr->tdupnumber) {
                    break;
                }
            }
            if (iresult < 0) {
                iindex -= idelta;
                continue;
            }
            if (iresult > 0) {
                iindex += idelta;
                continue;
            }
        }
        if (iresult > 0 && pstree->pskeycurr->psnext) {
            pstree->pskeycurr = pstree->pskeylist[iindex];
        }
        if (!pstree->pskeycurr) {
            goto treeload_exit;
        }
        if (pstree->pskeycurr->iisdummy && pstree->pskeycurr->psprev
            && pstree->pskeycurr->psprev->iishigh) {
            pstree->pskeycurr = pstree->pskeycurr->psprev;
        }
        if (!pstree->pskeycurr) {
            goto treeload_exit;
        }
        iresult = ivbkeycompare (ihandle, ikeynumber, ilength, pckeyvalue,
                                 pstree->pskeycurr->ckey);
        if (iresult == 0 && tdupnumber < pstree->pskeycurr->tdupnumber) {
            iresult = -1;
        }
        if (!pstree->ilevel) {
            break;  /* Exit the while loop */
        }
        if (!pstree->pskeycurr) {
            goto treeload_exit;
        }
        if (!pstree->pskeycurr->pschild || psvbptr->iindexchanged) {
            pskey = pstree->pskeycurr;
            if (!pstree->pskeycurr->pschild) {
                pskey->pschild = psvbtreeallocate (ihandle);
                vb_rtd->iserrno = errno;
                if (!pskey->pschild) {
                    goto treeload_exit;
                }
                pskey->pschild->psparent = pskey->psparent;
                if (pskey->psparent->iistof
                    && pskey == pskey->psparent->pskeyfirst) {
                    pskey->pschild->iistof = 1;
                }
                if (pskey->psparent->iiseof
                    && pskey == pskey->psparent->pskeylast->psprev) {
                    pskey->pschild->iiseof = 1;
                }
            }
            vb_rtd->iserrno = ivbnodeload (ihandle, ikeynumber, pstree->pskeycurr->pschild,
                                           pstree->pskeycurr->trownode, (int)pstree->ilevel);
            if (vb_rtd->iserrno) {
                vvbtreeallfree (ihandle, ikeynumber, pskey->pschild);
                pskey->pschild = NULL;
                goto treeload_exit;
            }
            pstree->pskeycurr->psparent = pstree;
            pstree->pskeycurr->pschild->psparent = pstree;
            if (pstree->iistof && pstree->pskeycurr == pstree->pskeyfirst) {
                pstree->pskeycurr->pschild->iistof = 1;
            }
            if (pstree->iiseof && pstree->pskeycurr == pstree->pskeylast) {
                pstree->pskeycurr->pschild->iiseof = 1;
            }
        }
        pstree = pstree->pskeycurr->pschild;
    }
    /*
     * When we get here, iresult is set depending upon whether the located
     * key was:
     * -1   LESS than the desired value
     * 0    EQUAL to the desired value (including a tdupnumber match!)
     * 1    GREATER than the desired value
     * By simply adding one to the value, we're cool for a NON-STANDARD
     * comparison return value.
     */
    psvbptr->pskeycurr[ikeynumber] = pstree->pskeycurr;
    if (!pstree->pskeycurr) {
        vb_rtd->iserrno = EBADFILE;
        return -1;
    }
    vb_rtd->iserrno = 0;
    if (pstree->pskeycurr->iisdummy) {
        vb_rtd->iserrno = EENDFILE;
        if (pstree->pskeycurr->psprev) {
            return 0;   /* EOF */
        } else {
            return 2;   /* Empty file! */
        }
    }
    return(iresult + 1);

    treeload_exit:
    return -1;
}

/* Global functions */

void
vvbmakekey (const struct keydesc *pskeydesc, VB_CHAR *pcrow_buffer,
            VB_UCHAR *pckeyvalue)
{
    VB_CHAR     *pcsource;
    int     ipart;

    /* Wierdly enough, a single keypart *CAN* span multiple instances */
    /* EG: Part number 1 might contain 4 long values */
    for (ipart = 0; ipart < pskeydesc->k_nparts; ipart++) {
        pcsource = pcrow_buffer + pskeydesc->k_part[ipart].kp_start;
        memcpy (pckeyvalue, pcsource, (size_t)pskeydesc->k_part[ipart].kp_leng);
        pckeyvalue += pskeydesc->k_part[ipart].kp_leng;
    }
}

int
ivbkeysearch (const int ihandle, const int imode, const int ikeynumber,
              int ilength, VB_UCHAR *pckeyvalue, off_t tdupnumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct VBKEY    *pskey;
    struct keydesc  *pskeydesc;
    int     iresult;
    VB_UCHAR    ckeyvalue[VB_MAX_KEYLEN];

    pskeydesc = vb_rtd->psvbfile[ihandle]->pskeydesc[ikeynumber];
    if (ilength == 0) {
        ilength = pskeydesc->k_len;
    }
    switch (imode) {
        case ISFIRST:
            vvbkeyvalueset (0, pskeydesc, ckeyvalue);
            tdupnumber = -1;
            return itreeload (ihandle, ikeynumber, ilength, ckeyvalue, tdupnumber);

        case ISLAST:
            vvbkeyvalueset (1, pskeydesc, ckeyvalue);
            tdupnumber = VB_MAX_OFF_T;
            return itreeload (ihandle, ikeynumber, ilength, ckeyvalue, tdupnumber);

        case ISNEXT:
            iresult = ivbkeyload (ihandle, ikeynumber, ISNEXT, 1, &pskey);
            vb_rtd->iserrno = iresult;
            if (iresult == EENDFILE) {
                iresult = 0;
            }
            if (iresult) {
                return -1;
            }
            return 1;   /* "NEXT" can NEVER be an exact match! */

        case ISPREV:
            iresult = ivbkeyload (ihandle, ikeynumber, ISPREV, 1, &pskey);
            vb_rtd->iserrno = iresult;
            if (iresult == EENDFILE) {
                iresult = 0;
            }
            if (iresult) {
                return -1;
            }
            return 1;   /* "PREV" can NEVER be an exact match */

        case ISCURR:
            return itreeload (ihandle, ikeynumber, ilength, pckeyvalue, tdupnumber);

        case ISEQUAL:       /* Falls thru to ISGTEQ */
            tdupnumber = 0;
        case ISGTEQ:
            return itreeload (ihandle, ikeynumber, ilength, pckeyvalue, tdupnumber);

        case ISGREAT:
            tdupnumber = VB_MAX_OFF_T;
            return itreeload (ihandle, ikeynumber, ilength, pckeyvalue, tdupnumber);

        default:
#ifdef	VBDEBUG
            fprintf (stderr, "HUGE ERROR! File %s, Line %d imode %d\n", __FILE__, __LINE__,
                     imode);
            exit (1);
#else
            return -1;
#endif
    }
}

int
ivbkeylocaterow (const int ihandle, const int ikeynumber, off_t trownumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    struct VBKEY    *pskey;
    struct VBTREE   *pstree;
    int     iresult;
    VB_UCHAR    ckeyvalue[VB_MAX_KEYLEN];

    /*
     * Step 1:
     *      The easy way out...
     *      If it is already the current index pointer *AND*
     *      the index file has remained unchanged since then,
     *      we don't need to do anything
     */
    psvbptr = vb_rtd->psvbfile[ihandle];
    iresult = 1;
    pskey = psvbptr->pskeycurr[ikeynumber];
    if (pskey && pskey->trownode == trownumber) {
        pskey->psparent->pskeycurr = pskey;
        /* Position pskeycurr all the way up to the root to point at us */
        pstree = pskey->psparent;
        while (pstree->psparent) {
            for (pstree->psparent->pskeycurr = pstree->psparent->pskeyfirst;
                pstree->psparent->pskeycurr
                && pstree->psparent->pskeycurr->pschild != pstree;
                pstree->psparent->pskeycurr =
                pstree->psparent->pskeycurr->psnext) ;
            if (!pstree->psparent->pskeycurr) {
                iresult = 0;
            }
            pstree = pstree->psparent;
        }
        if (iresult) {
            return 0;
        }
    }

    /*
     * Step 2:
     *      It's a valid and non-deleted row.  Therefore, let's make a
     *      contiguous key from it to search by.
     *      Find the damn key!
     */
    vvbmakekey (psvbptr->pskeydesc[ikeynumber], psvbptr->ppcrowbuffer, ckeyvalue);
    iresult = ivbkeysearch (ihandle, ISGTEQ, ikeynumber, 0, ckeyvalue, (off_t)0);
    if (iresult < 0 || iresult > 1) {
        vb_rtd->iserrno = ENOREC;
        return -1;
    }
    while (psvbptr->pskeycurr[ikeynumber]->trownode != trownumber) {
        vb_rtd->iserrno = ivbkeyload (ihandle, ikeynumber, ISNEXT, 1, &pskey);
        if (vb_rtd->iserrno) {
            if (vb_rtd->iserrno == EENDFILE) {
                vb_rtd->iserrno = ENOREC;
            }
            return -1;
        }
        if (ivbkeycompare (ihandle, ikeynumber, 0, ckeyvalue,
                           psvbptr->pskeycurr[ikeynumber]->ckey)) {
            vb_rtd->iserrno = ENOREC;
            return -1;
        }
    }
    return 0;
}

int
ivbkeyload (const int ihandle, const int ikeynumber, const int imode,
            const int isetcurr, struct VBKEY **ppskey)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    struct VBKEY    *pskey, *pskeyhold;
    struct VBTREE   *pstree;
    int     iresult;

    psvbptr = vb_rtd->psvbfile[ihandle];
    pskey = psvbptr->pskeycurr[ikeynumber];
    if (pskey->psparent->ilevel) {
        return EBADFILE;
    }
    switch (imode) {
        case ISPREV:
            if (pskey->psprev) {
                *ppskey = pskey->psprev;
                if (isetcurr) {
                    psvbptr->pskeycurr[ikeynumber] = pskey->psprev;
                    pskey->psparent->pskeycurr = pskey->psprev;
                }
                return 0;
            }
            pstree = pskey->psparent;
            if (pstree->iistof) {
                return EENDFILE;
            }
            /* Back up the tree until we find a node where there is a < */
            while (pstree->pskeycurr == pstree->pskeyfirst) {
                if (pstree->psparent) {
                    pstree = pstree->psparent;
                } else {
                    break;
                }
            }
            /* Back down the tree selecting the LAST valid key of each */
            pskey = pstree->pskeycurr->psprev;
            if (isetcurr) {
                pstree->pskeycurr = pstree->pskeycurr->psprev;
            }
            while (pstree->ilevel && pskey) {
                if (isetcurr) {
                    pstree->pskeycurr = pskey;
                }
                if (!pskey->pschild || psvbptr->iindexchanged) {
                    if (!pskey->pschild) {
                        pskey->pschild = psvbtreeallocate (ihandle);
                        if (!pskey->pschild) {
                            return errno;
                        }
                        pskey->pschild->psparent = pskey->psparent;
                        if (pskey->psparent->iistof
                            && pskey == pskey->psparent->pskeyfirst) {
                            pskey->pschild->iistof = 1;
                        }
                        if (pskey->psparent->iiseof
                            && pskey == pskey->psparent->pskeylast->psprev) {
                            pskey->pschild->iiseof = 1;
                        }
                    }
                    pskeyhold = pskey;
                    iresult = ivbnodeload (ihandle, ikeynumber, pskey->pschild,
                                           pskey->trownode, (int)pstree->ilevel);
                    if (iresult) {
                        /* Ooops, make sure the tree is not corrupt */
                        vvbtreeallfree (ihandle, ikeynumber,
                                        pskeyhold->pschild);
                        pskeyhold->pschild = NULL;
                        return iresult;
                    }
                }
                pstree = pskey->pschild;
                /* Last key is always the dummy, so backup by one */
                pskey = pstree->pskeylast->psprev;
            }
            if (isetcurr) {
                pstree->pskeycurr = pskey;
                psvbptr->pskeycurr[ikeynumber] = pskey;
            }
            *ppskey = pskey;
            break;

        case ISNEXT:
            if (pskey->psnext && !pskey->psnext->iisdummy) {
                *ppskey = pskey->psnext;
                if (isetcurr) {
                    psvbptr->pskeycurr[ikeynumber] = pskey->psnext;
                    pskey->psparent->pskeycurr = pskey->psnext;
                }
                return 0;
            }
            pstree = pskey->psparent;
            if (pstree->iiseof) {
                return EENDFILE;
            }
            pstree = pstree->psparent;
            /* Back up the tree until we find a node where there is a > */
            while (1) {
                if (pstree->pskeylast->psprev != pstree->pskeycurr) {
                    break;
                }
                pstree = pstree->psparent;
            }
            pskey = pstree->pskeycurr->psnext;
            if (isetcurr) {
                pstree->pskeycurr = pstree->pskeycurr->psnext;
            }
            /* Back down the tree selecting the FIRST valid key of each */
            while (pstree->ilevel) {
                if (isetcurr) {
                    pstree->pskeycurr = pskey;
                }
                if (!pskey->pschild || psvbptr->iindexchanged) {
                    if (!pskey->pschild) {
                        pskey->pschild = psvbtreeallocate (ihandle);
                        if (!pskey->pschild) {
                            return errno;
                        }
                        pskey->pschild->psparent = pskey->psparent;
                        if (pskey->psparent->iistof
                            && pskey == pskey->psparent->pskeyfirst) {
                            pskey->pschild->iistof = 1;
                        }
                        if (pskey->psparent->iiseof
                            && pskey == pskey->psparent->pskeylast->psprev) {
                            pskey->pschild->iiseof = 1;
                        }
                    }
                    pskeyhold = pskey;
                    iresult = ivbnodeload (ihandle, ikeynumber, pskey->pschild,
                                           pskey->trownode, (int)pstree->ilevel);
                    if (iresult) {
                        /* Ooops, make sure the tree is not corrupt */
                        vvbtreeallfree (ihandle, ikeynumber,
                                        pskeyhold->pschild);
                        pskeyhold->pschild = NULL;
                        return iresult;
                    }
                }
                pstree = pskey->pschild;
                pskey = pstree->pskeyfirst;
            }
            if (isetcurr) {
                pstree->pskeycurr = pskey;
                psvbptr->pskeycurr[ikeynumber] = pskey;
            }
            *ppskey = pskey;
            break;

        default:
#ifdef	VBDEBUG
            fprintf (stderr, "HUGE ERROR! File %s, Line %d Mode %d\n", __FILE__, __LINE__,
                     imode);
            exit (1);
#else
            break;
#endif
    }
    return 0;
}

void
vvbkeyvalueset (const int ihigh, struct keydesc *pskeydesc, VB_UCHAR *pckeyvalue)
{
    int ipart, iremainder;
    VB_CHAR cbuffer[QUADSIZE];

    for (ipart = 0; ipart < pskeydesc->k_nparts; ipart++) {
        switch ((pskeydesc->k_part[ipart].kp_type & BYTEMASK) & ~ISDESC) {
            case CHARTYPE:
                memset (pckeyvalue, ihigh ? 0xff : 0, (size_t)pskeydesc->k_part[ipart].kp_leng);
                pckeyvalue += pskeydesc->k_part[ipart].kp_leng;
                break;

            case INTTYPE:
                iremainder = pskeydesc->k_part[ipart].kp_leng;
                while (iremainder > 0) {
                    inl_stint (ihigh ? SHRT_MAX : SHRT_MIN, pckeyvalue);
                    pckeyvalue += INTSIZE;
                    iremainder -= INTSIZE;
                }
                break;

            case LONGTYPE:
                iremainder = pskeydesc->k_part[ipart].kp_leng;
                while (iremainder > 0) {
                    inl_stlong (ihigh ? LONG_MAX : LONG_MIN, pckeyvalue);
                    pckeyvalue += LONGSIZE;
                    iremainder -= LONGSIZE;
                }
                break;

            case QUADTYPE:
                memset (cbuffer, ihigh ? 0xff : 0, QUADSIZE);
                cbuffer[0] = ihigh ? 0x7f : 0x80;
                iremainder = pskeydesc->k_part[ipart].kp_leng;
                while (iremainder > 0) {
                    memcpy (pckeyvalue, cbuffer, QUADSIZE);
                    pckeyvalue += QUADSIZE;
                    iremainder -= QUADSIZE;
                }
                break;

            case FLOATTYPE:
                iremainder = pskeydesc->k_part[ipart].kp_leng;
                while (iremainder > 0) {
                    stfloat (ihigh ? FLT_MAX : FLT_MIN, pckeyvalue);
                    pckeyvalue += FLOATSIZE;
                    iremainder -= FLOATSIZE;
                }
                break;

            case DOUBLETYPE:
                iremainder = pskeydesc->k_part[ipart].kp_leng;
                while (iremainder > 0) {
                    stdbl (ihigh ? DBL_MAX : DBL_MIN, pckeyvalue);
                    pckeyvalue += DOUBLESIZE;
                    iremainder -= DOUBLESIZE;
                }
                break;

            default:
#ifdef	VBDEBUG
                fprintf (stderr, "HUGE ERROR! File %s, Line %d Type %d\n", __FILE__,
                         __LINE__, pskeydesc->k_part[ipart].kp_type);
                exit (1);
#else
                break;
#endif
        }
    }
}

int
ivbkeyinsert (const int ihandle, struct VBTREE *pstree, const int ikeynumber,
              VB_UCHAR *pckeyvalue, off_t trownode, off_t tdupnumber,
              struct VBTREE *pschild)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    struct VBKEY    *pskey, *pstempkey;
    int     iposn = 0, iresult, i, klen;
    unsigned char nullchar;

    psvbptr = vb_rtd->psvbfile[ihandle];
    /* If NULLKEY is asked for, then check if key value is all NULL Character */
    if((psvbptr->pskeydesc[ikeynumber]->k_flags & NULLKEY)) {
    	nullchar = (psvbptr->pskeydesc[ikeynumber]->k_type >> BYTESHFT) & BYTEMASK;
    	klen = psvbptr->pskeydesc[ikeynumber]->k_len;
    	for(i=0; i < klen && pckeyvalue[i] == nullchar; i++);
    	if(i >= klen) {		/* KEY is all 'nullchar' so skip adding to index */
    		return 0;
    	}
    }
    pskey = psvbkeyallocate (ihandle, ikeynumber);
    if (!pskey) {
        return errno;
    }
    if (!psvbptr->pskeycurr[ikeynumber]) {
        return EBADFILE;
    }
    if (!pstree) {
        pstree = psvbptr->pskeycurr[ikeynumber]->psparent;
    }
    pskey->psparent = pstree;
    pskey->pschild = pschild;
    pskey->trownode = trownode;
    pskey->tdupnumber = tdupnumber;
    pskey->iisnew = 1;
    memcpy (pskey->ckey, pckeyvalue, (size_t)psvbptr->pskeydesc[ikeynumber]->k_len);
    pskey->psnext = pstree->pskeycurr;
    pskey->psprev = pstree->pskeycurr->psprev;
    if (pstree->pskeycurr->psprev) {
        pstree->pskeycurr->psprev->psnext = pskey;
    } else {
        pstree->pskeyfirst = pskey;
    }
    pstree->pskeycurr->psprev = pskey;
    pstree->pskeycurr = pskey;
    psvbptr->pskeycurr[ikeynumber] = pskey;
    pstree->ikeysinnode = 0;
    for (pstempkey = pstree->pskeyfirst; pstempkey; pstempkey = pstempkey->psnext) {
        if (pstempkey == pskey) {
            iposn = pstree->ikeysinnode;
        }
        pstree->pskeylist[pstree->ikeysinnode] = pstempkey;
        pstree->ikeysinnode++;
    }
    iresult = ivbnodesave (ihandle, ikeynumber, pskey->psparent,
                           pskey->psparent->tnodenumber, 1, iposn);
    pskey->iisnew = 0;
    return iresult;
}

int
ivbkeydelete (const int ihandle, const int ikeynumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    struct VBKEY    *pskey, *pskeytemp;
    struct VBTREE   *pstree, *pstreeroot;
    int     iforcerewrite = 0, iposn, iresult;

    psvbptr = vb_rtd->psvbfile[ihandle];
    pskey = psvbptr->pskeycurr[ikeynumber];
    /*
     * We're going to *TRY* to keep the index buffer populated!
     * However, since it's technically feasible for the current node to be
     * removed in it's entirety, we can only do this if there is at least 1
     * other key in the node that's not the dummy entry.
     * Since the current key is guaranteed to be at the LEAF node level (0),
     * it's impossible to ever have an iishigh entry in the node.
     */
    if (pskey->psnext && pskey->psnext->iisdummy == 0) {
        psvbptr->pskeycurr[ikeynumber] = pskey->psnext;
        pskey->psparent->pskeycurr = pskey->psnext;
    } else {
        if (pskey->psprev) {
            psvbptr->pskeycurr[ikeynumber] = pskey->psprev;
            pskey->psparent->pskeycurr = pskey->psprev;
        } else {
            psvbptr->pskeycurr[ikeynumber] = NULL;
            pskey->psparent->pskeycurr = NULL;
        }
    }
    while (1) {
        pstree = pskey->psparent;
        if (pskey->iishigh) {
            /*
             * Handle removal of the high key in a node.
             * Since we're modifying a key OTHER than the one we're
             * deleting, we need a FULL node rewrite!
             */
            if (pskey->psprev) {
                pskey->pschild = pskey->psprev->pschild;
                pskey->trownode = pskey->psprev->trownode;
                pskey->tdupnumber = pskey->psprev->tdupnumber;
                pskey = pskey->psprev;
                iforcerewrite = 1;
            } else {
                iresult = ivbnodefree (ihandle, pstree->tnodenumber);   /* BUG - didn't check iresult */
                pstree = pstree->psparent;
                vvbtreeallfree (ihandle, ikeynumber,
                                pstree->pskeycurr->pschild);
                pskey = pstree->pskeycurr;
                pskey->pschild = NULL;
                continue;
            }
        }
        iposn = -1;
        pskey->psparent->ikeysinnode = 0;
        for (pskeytemp = pskey->psparent->pskeyfirst; pskeytemp;
            pskeytemp = pskeytemp->psnext) {
            if (pskey == pskeytemp) {
                iposn = pskey->psparent->ikeysinnode;
            } else {
                pskey->psparent->pskeylist[pskey->psparent->ikeysinnode] = pskeytemp;
                pskey->psparent->ikeysinnode++;
            }
        }
        if (pskey->psprev) {
            pskey->psprev->psnext = pskey->psnext;
        } else {
            pstree->pskeyfirst = pskey->psnext;
        }
        if (pskey->psnext) {
            pskey->psnext->psprev = pskey->psprev;
        }
        pskey->psparent = NULL;
        pskey->pschild = NULL;
        vvbkeyfree (ihandle, ikeynumber, pskey);
        if (pstree->iisroot
            && (pstree->pskeyfirst->iishigh
                || pstree->pskeyfirst->iisdummy)) {
            pskey = pstree->pskeyfirst;
            if (!pskey->pschild) {
                pstreeroot = pstree;
            } else {
                pstreeroot = pskey->pschild;
                pskey->pschild = NULL;
            }
            if (pskey->iisdummy) {  /* EMPTY FILE! */
                return ivbnodesave (ihandle, ikeynumber, pstreeroot,
                                    pstreeroot->tnodenumber, 0, 0);
            }
            iresult = ivbnodeload (ihandle, ikeynumber, pstreeroot, pskey->trownode,
                                   (int)pstree->ilevel);
            if (iresult) {
                return iresult; /* BUG Corrupt! */
            }
            iresult = ivbnodefree (ihandle, pstreeroot->tnodenumber);
            if (iresult) {
                return iresult; /* BUG Corrupt! */
            }
            pstreeroot->tnodenumber =
            psvbptr->pskeydesc[ikeynumber]->k_rootnode;
            pstreeroot->psparent = NULL;
            pstreeroot->iistof = 1;
            pstreeroot->iiseof = 1;
            pstreeroot->iisroot = 1;
            if (pstree != pstreeroot) {
                vvbtreeallfree (ihandle, ikeynumber, pstree);
            }
            psvbptr->pstree[ikeynumber] = pstreeroot;
            return ivbnodesave (ihandle, ikeynumber, pstreeroot,
                                pstreeroot->tnodenumber, 0, 0);
        }
        if (pstree->pskeyfirst->iisdummy) {
            /* Handle removal of the last key in a node */
            iresult = ivbnodefree (ihandle, pstree->tnodenumber);
            if (iresult) {
                return iresult; /* BUG Corrupt! */
            }
            pstree = pstree->psparent;
            vvbtreeallfree (ihandle, ikeynumber, pstree->pskeycurr->pschild);
            pstree->pskeycurr->pschild = NULL;
            pskey = pstree->pskeycurr;
            continue;
        }
        break;
    }
    if (iforcerewrite) {
        return ivbnodesave (ihandle, ikeynumber, pstree, pstree->tnodenumber, 0, 0);
    }
    return ivbnodesave (ihandle, ikeynumber, pstree, pstree->tnodenumber, -1, iposn);
}

int
ivbkeycompare (const int ihandle, const int ikeynumber, int ilength,
               VB_UCHAR *pckey1, VB_UCHAR *pckey2)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct keydesc  *pskeydesc;
    off_t       tvalue1, tvalue2;
    int     idescbias, ipart, ilengthtocompare;
    int     iresult = 0, ivalue1, ivalue2;
    int     n;
    int     lvalue1, lvalue2;
    float       fvalue1, fvalue2;
    double      dvalue1, dvalue2;

    pskeydesc = vb_rtd->psvbfile[ihandle]->pskeydesc[ikeynumber];
    if (ilength == 0) {
        ilength = pskeydesc->k_len;
    }
    for (ipart = 0; ilength > 0 && ipart < pskeydesc->k_nparts; ipart++) {
        if (ilength >= pskeydesc->k_part[ipart].kp_leng) {
            ilengthtocompare = pskeydesc->k_part[ipart].kp_leng;
        } else {
            ilengthtocompare = ilength;
        }
        ilength -= ilengthtocompare;
        if (pskeydesc->k_part[ipart].kp_type & ISDESC) {
            idescbias = -1;
        } else {
            idescbias = 1;
        }
        iresult = 0;
        switch ((pskeydesc->k_part[ipart].kp_type & BYTEMASK) & ~ISDESC) {
            case CHARTYPE:

                n = memcmp (pckey1, pckey2, (size_t)ilengthtocompare);
                if (n < 0) {
                    return -idescbias;
                }
                if (n > 0) {
                    return idescbias;
                }
                pckey1 += ilengthtocompare;
                pckey2 += ilengthtocompare;
/*
            while (ilengthtocompare-- && !iresult) {
                if (*pckey1 < *pckey2) {
                    return -idescbias;
                }
                if (*pckey1++ > *pckey2++) {
                    return idescbias;
                }
            }
*/
                break;

            case INTTYPE:
                while (ilengthtocompare >= INTSIZE && !iresult) {
                    ivalue1 = inl_ldint (pckey1);
                    ivalue2 = inl_ldint (pckey2);
                    if (ivalue1 < ivalue2) {
                        return -idescbias;
                    }
                    if (ivalue1 > ivalue2) {
                        return idescbias;
                    }
                    pckey1 += INTSIZE;
                    pckey2 += INTSIZE;
                    ilengthtocompare -= INTSIZE;
                }
                break;

            case LONGTYPE:
                while (ilengthtocompare >= LONGSIZE && !iresult) {
                    lvalue1 = inl_ldlong (pckey1);
                    lvalue2 = inl_ldlong (pckey2);
                    if (lvalue1 < lvalue2) {
                        return -idescbias;
                    }
                    if (lvalue1 > lvalue2) {
                        return idescbias;
                    }
                    pckey1 += LONGSIZE;
                    pckey2 += LONGSIZE;
                    ilengthtocompare -= LONGSIZE;
                }
                break;

            case QUADTYPE:
                while (ilengthtocompare >= QUADSIZE && !iresult) {
                    tvalue1 = inl_ldquad (pckey1);
                    tvalue2 = inl_ldquad (pckey2);
                    if (tvalue1 < tvalue2) {
                        return -idescbias;
                    }
                    if (tvalue1 > tvalue2) {
                        return idescbias;
                    }
                    pckey1 += QUADSIZE;
                    pckey2 += QUADSIZE;
                    ilengthtocompare -= QUADSIZE;
                }
                break;

            case FLOATTYPE:
                while (ilengthtocompare >= FLOATSIZE && !iresult) {
                    fvalue1 = ldfloat (pckey1);
                    fvalue2 = ldfloat (pckey2);
                    if (fvalue1 < fvalue2) {
                        return -idescbias;
                    }
                    if (fvalue1 > fvalue2) {
                        return idescbias;
                    }
                    pckey1 += FLOATSIZE;
                    pckey2 += FLOATSIZE;
                    ilengthtocompare -= FLOATSIZE;
                }
                break;

            case DOUBLETYPE:
                while (ilengthtocompare >= DOUBLESIZE && !iresult) {
                    dvalue1 = lddbl (pckey1);
                    dvalue2 = lddbl (pckey2);
                    if (dvalue1 < dvalue2) {
                        return -idescbias;
                    }
                    if (dvalue1 > dvalue2) {
                        return idescbias;
                    }
                    pckey1 += DOUBLESIZE;
                    pckey2 += DOUBLESIZE;
                    ilengthtocompare -= DOUBLESIZE;
                }
                break;

            default:
#ifdef	VBDEBUG
                fprintf (stderr, "HUGE ERROR! File %s, Line %d itype %d\n", __FILE__,
                         __LINE__, pskeydesc->k_part[ipart].kp_type);
                exit (1);
#else
                break;
#endif
        }
    }
    return 0;
}

#ifdef	VBDEBUG
static void vdumpkey (struct VBKEY *pskey, struct VBTREE *pstree, int iindent);

static void
vdumptree (struct VBTREE *pstree, int iindent)
{
    struct VBKEY *pskey;

#if	ISAMMODE == 1
    printf
    ("%-*.*sNODE:%lld  \t%08X DAD:%04X LVL:%04X CUR:%04X",
     iindent, iindent, "          ",
     pstree->tnodenumber,
     (int)pstree,
     (int)pstree->psparent & 0xffff,
     pstree->ilevel, (int)pstree->pskeycurr & 0xffff);
#else	/* ISAMMODE == 1 */
    printf
    ("%-*.*sNODE:%d  \t%08X DAD:%04X LVL:%04X CUR:%04X",
     iindent, iindent, "          ",
     (int)pstree->tnodenumber,
     (int)pstree,
     (int)pstree->psparent & 0xffff,
     pstree->ilevel, (int)pstree->pskeycurr & 0xffff);
#endif	/* ISAMMODE == 1 */
    fflush (stdout);
    if (pstree->iisroot) {
        printf (" RT.");
    }
    if (pstree->iistof) {
        printf (" TOF");
    }
    if (pstree->iiseof) {
        printf (" EOF");
    }
    printf ("\n");
    for (pskey = pstree->pskeyfirst; pskey; pskey = pskey->psnext) {
        vdumpkey (pskey, pstree, iindent);
    }
}

static void
vdumpkey (struct VBKEY *pskey, struct VBTREE *pstree, int iindent)
{
    int     ikey;
    VB_UCHAR    cbuffer[QUADSIZE];

    memcpy (cbuffer, pskey->ckey, QUADSIZE);
    ikey = inl_ldint (cbuffer);
#if ISAMMODE == 1
    printf
    ("%-*.*s KEY :%02X%02llX\t%08X DAD:%04X KID:%04X ROW:%04llX",
     iindent, iindent, "          ",
     cbuffer[0],
     pskey->tdupnumber,
     (int)pskey,
     (int)pskey->psparent & 0xffff,
     (int)pskey->pschild & 0xffff, pskey->trownode & 0xffff);
#else	/* ISAMMODE == 1 */
    printf
    ("%-*.*s KEY :%02X%02llX\t%08X DAD:%04X KID:%04X ROW:%04llX",
     iindent, iindent, "          ",
     cbuffer[0],
     (unsigned long long)pskey->tdupnumber,
     (int)pskey,
     (int)pskey->psparent & 0xffff,
     (int)pskey->pschild & 0xffff, (unsigned long long)pskey->trownode & 0xffff);
#endif	/* ISAMMODE == 1 */
    if (pskey == pstree->pskeyfirst) {
        printf (" 1ST");
    }
    if (pskey == pstree->pskeycurr) {
        printf (" CUR");
    }
    if (pskey == pstree->pskeylast) {
        printf (" LST");
    }
    if (pskey->iisnew) {
        printf (" NEW");
    }
    if (pskey->iisdummy) {
        printf (" DMY");
    }
    if (pskey->iishigh) {
        printf (" HI");
    }
    printf ("\n");
    fflush (stdout);
    if (pskey->pschild) {
        vdumptree (pskey->pschild, iindent + 1);
    }
}

int
idumptree (int ihandle, int ikeynumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct VBTREE *pstree = vb_rtd->psvbfile[ihandle]->pstree[ikeynumber];

    fflush (stdout);
    vdumptree (pstree, 0);
    return 0;
}

static int
ichktree2 (struct VBTREE *pstree, int ilevel, struct VBKEY *pscurrkey, int *picurrentintree)
{
    int     iresult;
    struct VBKEY    *pskey;

    for (pskey = pstree->pskeyfirst; pskey; pskey = pskey->psnext) {
        if (pskey->psparent != pstree) {
            return 1;
        }
        if (pskey == pscurrkey) {
            *picurrentintree = *picurrentintree + 1;
        }
        if (!pskey->pschild) {
            continue;
        }
        if (pskey->pschild->psparent != pstree) {
            return 2;
        }
        if (pskey->pschild->ilevel != ilevel - 1) {
            return 3;
        }
        iresult = ichktree2 (pskey->pschild, pskey->pschild->ilevel,
                             pscurrkey, picurrentintree);
        if (iresult) {
            return iresult;
        }
    }
    return 0;
}

int
ichktree (int ihandle, int ikeynumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    int     icurrintree;
    struct VBTREE   *pstree;
    struct VBKEY    *pskey;
    VB_UCHAR    cbuffer[QUADSIZE];

    psvbptr = vb_rtd->psvbfile[ihandle];
    pstree = psvbptr->pstree[ikeynumber];
    if (!pstree) {
        return 0;
    }
    pskey = psvbptr->pskeycurr[ikeynumber];
    if (pskey) {
        icurrintree = 0;
        cbuffer[0] = pskey->ckey[0];
    } else {
        icurrintree = 1;
    }
    if (ichktree2 (pstree, pstree->ilevel, pskey, &icurrintree)) {
        printf ("Tree is invalid!\n");
    } else if (pskey && pskey->trownode == -1) {
        printf ("CurrentKey is invalid!\n");
    } else if (icurrintree != 1) {
        printf ("CurrentKey is in the tree %d times!\n", icurrintree);
    } else if (pskey != pskey->psparent->pskeycurr) {
        printf ("CurrentKey is NOT the current key of its parent!\n");
    }
    printf
    ("CURR KEY :%02X%02llX\t%08X DAD:%04X KID:%04X ROW:%04llX",
     cbuffer[0],
     (unsigned long long)pskey->tdupnumber,
     (int)pskey,
     (int)pskey->psparent & 0xffff,
     (int)pskey->pschild & 0xffff, (unsigned long long)pskey->trownode & 0xffff);
    if (pskey->iisnew) {
        printf (" NEW");
    }
    if (pskey->iisdummy) {
        printf (" DMY");
    }
    if (pskey->iishigh) {
        printf (" HI");
    }
    printf ("\n");
    idumptree (ihandle, ikeynumber);
    return 0;
}
#endif	/* VBDEBUG */
