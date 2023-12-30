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

#define NEED_VBINLINE_FUNCS 1
#include        "isinternal.h"

/* Local functions */

static int
istartrownumber (const int ihandle, const int imode, const int iisread)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbfptr;
    int ibias = 1, ideleted = 1, ilockresult = 0, iresult = 0;

    psvbfptr = vb_rtd->psvbfile[ihandle];
    switch (imode) {
    case ISFIRST:
        psvbfptr->iisdisjoint = 1;
        vb_rtd->isrecnum = 1;
        break;

    case ISLAST:
        vb_rtd->isrecnum = inl_ldquad (psvbfptr->sdictnode.cdatacount);
        ibias = -1;
        break;

    case ISNEXT:            /* Falls thru to next case! */
        if (unlikely(!iisread)) {
            vb_rtd->iserrno = EBADARG;
            return -1;
        }
        vb_rtd->isrecnum = psvbfptr->trownumber;
        if (psvbfptr->iisdisjoint) {
            ibias = 0;
            break;
        }
    case ISGREAT:           /* Falls thru to next case! */
        vb_rtd->isrecnum++;
    case ISGTEQ:
        break;

    case ISCURR:            /* Falls thru to next case! */
        if (unlikely(!iisread)) {
            vb_rtd->iserrno = EBADARG;
            return -1;
        }
    case ISEQUAL:
        ibias = 0;
        break;

    case ISPREV:
        if (unlikely(!iisread)) {
            vb_rtd->iserrno = EBADARG;
            return -1;
        }
        vb_rtd->isrecnum = psvbfptr->trownumber;
        vb_rtd->isrecnum--;
        ibias = -1;
        break;

    default:
        vb_rtd->iserrno = EBADARG;
        return -1;
    }

    vb_rtd->iserrno = ENOREC;
    while (ideleted) {
        if (vb_rtd->isrecnum > 0
            && vb_rtd->isrecnum <= inl_ldquad (psvbfptr->sdictnode.cdatacount)) {
            if (psvbfptr->iopenmode & ISAUTOLOCK || imode & ISLOCK) {
                if (ivbdatalock (ihandle, imode & ISWAIT ? VBWRLCKW : VBWRLOCK, (off_t)vb_rtd->isrecnum)) {
                    ilockresult = ELOCKED;
                    iresult = -1;
                }
            }
            if (!ilockresult) {
                iresult = ivbdataread (ihandle,
                                       (void *)psvbfptr->ppcrowbuffer,
                                       &ideleted, (off_t)vb_rtd->isrecnum);
            }
            if (iresult) {
                vb_rtd->isrecnum = 0;
                vb_rtd->iserrno = EBADFILE;
                return -1;
            }
        }
        if (!ideleted) {
            psvbfptr->trownumber = vb_rtd->isrecnum;
            vb_rtd->iserrno = 0;
            return 0;
        }
        if (!ibias) {
            vb_rtd->isrecnum = 0;
            return -1;
        }
        vb_rtd->isrecnum += ibias;
        if (vb_rtd->isrecnum < 1
            || vb_rtd->isrecnum > inl_ldquad (psvbfptr->sdictnode.cdatacount)) {
            vb_rtd->isrecnum = 0;
            return -1;
        }
    }
    return 0;
}

/* Global functions */

int
ivbcheckkey (const int ihandle, struct keydesc *pskey, const int imode,
             int irowlength, const int iisbuild)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    struct keydesc  *pslocalkey;
    int             iloop, ipart, itype, ilocalkeylength;

    psvbptr = vb_rtd->psvbfile[ihandle];
    if (imode) {
        irowlength = psvbptr->iminrowlength;
    }
    if (imode < 2) {
        /* Basic key validity test */
        pskey->k_len = 0;
	if (pskey->k_flags < 0 || pskey->k_flags > NULLKEY + ISDUPS) {
		goto vbcheckkey_exit;
	}
        if (pskey->k_nparts >= NPARTS || pskey->k_nparts < 0) {
            goto vbcheckkey_exit;
        }
        if (pskey->k_nparts == 0 && !iisbuild) {
            goto vbcheckkey_exit;
        }
        for (ipart = 0; ipart < pskey->k_nparts; ipart++) {
            /* Wierdly enough, a single keypart CAN span multiple instances */
            /* EG: Part number 1 might contain 4 long values */
            pskey->k_len += pskey->k_part[ipart].kp_leng;
            if (pskey->k_len > VB_MAX_KEYLEN) {
                goto vbcheckkey_exit;
            }
            itype = (pskey->k_part[ipart].kp_type & BYTEMASK) & ~ISDESC;
            switch (itype) {
            case CHARTYPE:
                break;

            case INTTYPE:
                if (pskey->k_part[ipart].kp_leng % INTSIZE) {
                    goto vbcheckkey_exit;
                }
                break;

            case LONGTYPE:
                if (pskey->k_part[ipart].kp_leng % LONGSIZE) {
                    goto vbcheckkey_exit;
                }
                break;

            case QUADTYPE:
                if (pskey->k_part[ipart].kp_leng % QUADSIZE) {
                    goto vbcheckkey_exit;
                }
                break;

            case FLOATTYPE:
                if (pskey->k_part[ipart].kp_leng % FLOATSIZE) {
                    goto vbcheckkey_exit;
                }
                break;

            case DOUBLETYPE:
                if (pskey->k_part[ipart].kp_leng % DOUBLESIZE) {
                    goto vbcheckkey_exit;
                }
                break;

            default:
                goto vbcheckkey_exit;
            }
            if (pskey->k_part[ipart].kp_start + pskey->k_part[ipart].kp_leng >
                irowlength) {
                goto vbcheckkey_exit;
            }
            if (pskey->k_part[ipart].kp_start < 0) {
                goto vbcheckkey_exit;
            }
        }
        if (!imode) {
            return 0;
        }
    }

    /* Check whether the key already exists */
    for (iloop = 0; iloop < psvbptr->inkeys; iloop++) {
        pslocalkey = psvbptr->pskeydesc[iloop];
        if (pslocalkey->k_nparts != pskey->k_nparts) {
            continue;
        }
        ilocalkeylength = 0;
        for (ipart = 0; ipart < pslocalkey->k_nparts; ipart++) {
            if (pslocalkey->k_part[ipart].kp_start != pskey->k_part[ipart].kp_start) {
                break;
            }
            if (pslocalkey->k_part[ipart].kp_leng != pskey->k_part[ipart].kp_leng) {
                break;
            }
            if (pslocalkey->k_part[ipart].kp_type != pskey->k_part[ipart].kp_type) {
                break;
            }
            ilocalkeylength += pskey->k_part[ipart].kp_leng;
        }
        if (ipart == pslocalkey->k_nparts) {
            pskey->k_len = ilocalkeylength;
            break;  /* found */
        }
    }
    if (iloop == psvbptr->inkeys) {
        if (imode == 2) {
            goto vbcheckkey_exit;
        }
        return iloop;
    }
    if (imode == 1) {
        goto vbcheckkey_exit;
    }
    return iloop;

    vbcheckkey_exit:
    vb_rtd->iserrno = EBADKEY;
    return -1;
}

int
isread (int ihandle, VB_CHAR *pcrow, int imode)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct VBKEY    *pskey;
    struct DICTINFO *psvbfptr;
    int             ideleted = 0, ikeynumber, ilockresult = 0;
    int             ireadmode, iresult = -1;
    VB_UCHAR        ckeyvalue[VB_MAX_KEYLEN];

    if (ivbenter (ihandle, 0)) {
        return -1;
    }

    vb_rtd->iserrno = EBADKEY;
    psvbfptr = vb_rtd->psvbfile[ihandle];
    ikeynumber = psvbfptr->iactivekey;

    if (psvbfptr->iopenmode & ISAUTOLOCK) {
        isrelease (ihandle);
    }

    ireadmode = imode & BYTEMASK;

    if (ikeynumber == -1 || !psvbfptr->pskeydesc[ikeynumber]->k_nparts) {
        /*
         * This code relies on the fact that istartrownumber will
         * populate the global VBISAM pcrowbuffer with the fixed-length
         * portion of the row on success.
         */
        iresult = istartrownumber (ihandle, ireadmode, 1);
        if (!iresult) {
            memcpy (pcrow, psvbfptr->ppcrowbuffer, (size_t)psvbfptr->iminrowlength);
            psvbfptr->iisdisjoint = 0;
        }
        goto read_exit;
    }
    vb_rtd->iserrno = 0;
    vb_rtd->isrecnum = 0;
    switch (ireadmode) {
    case ISFIRST:
        /* ckeyvalue is just a placeholder for ISFIRST */
        iresult = ivbkeysearch (ihandle, ISFIRST, ikeynumber, 0, ckeyvalue, (off_t)0);
        if (iresult < 0) {
            break;
        }
        if (iresult == 2) {
            vb_rtd->iserrno = EENDFILE;
        } else {
            iresult = 0;
        }
        break;

    case ISLAST:
        /*
         * ckeyvalue is just a placeholder for ISLAST
         * Note that the KeySearch (ISLAST) will position the pointer onto the
         * LAST key of the LAST tree which, by definition, is a DUMMY key
         */
        iresult = ivbkeysearch (ihandle, ISLAST, ikeynumber, 0, ckeyvalue, (off_t)0);
        if (iresult < 0) {
            break;
        }
        if (iresult == 2) {
            vb_rtd->iserrno = EENDFILE;
        } else {
            iresult = 0;
            vb_rtd->iserrno = ivbkeyload (ihandle, ikeynumber, ISPREV, 1, &pskey);
            if (vb_rtd->iserrno) {
                iresult = -1;
            }
        }
        break;

    case ISEQUAL:
        vvbmakekey (psvbfptr->pskeydesc[ikeynumber], pcrow, ckeyvalue);
        iresult = ivbkeysearch (ihandle, ISGTEQ, ikeynumber, 0, ckeyvalue, (off_t)0);
        if (iresult == -1) {    /* Error */
            break;
        }
        if (iresult == 1) {     /* Found it! */
            iresult = 0;
        } else {
            if (psvbfptr->pskeycurr[ikeynumber]->iisdummy) {
                iresult = ivbkeyload (ihandle, ikeynumber, ISNEXT, 1, &pskey);
                if (iresult == EENDFILE) {
                    iresult = -1;
                    vb_rtd->iserrno = ENOREC;
                    break;
                }
                vb_rtd->iserrno = 0;
            }
            if (memcmp (ckeyvalue, psvbfptr->pskeycurr[ikeynumber]->ckey,
                        (size_t)psvbfptr->pskeydesc[ikeynumber]->k_len)) {
                iresult = -1;
                vb_rtd->iserrno = ENOREC;
            } else {
                iresult = 0;
            }
        }
        break;

    case ISGREAT:
    case ISGTEQ:
        vvbmakekey (psvbfptr->pskeydesc[ikeynumber], pcrow, ckeyvalue);
        iresult = ivbkeysearch (ihandle, ireadmode, ikeynumber, 0, ckeyvalue, (off_t)0);
        if (iresult < 0) {      /* Error is always error */
            break;
        }
        if (iresult == 2) {
            vb_rtd->iserrno = EENDFILE;
            break;
        }
        if (iresult == 1) {
            iresult = 0;
        } else {
            iresult = 0;
            if (psvbfptr->pskeycurr[ikeynumber]->iisdummy) {
                vb_rtd->iserrno = EENDFILE;
                iresult = -1;
            }
        }
        break;

    case ISPREV:
        if (psvbfptr->trowstart) {
            iresult = ivbkeylocaterow (ihandle, ikeynumber,
                                       psvbfptr->trowstart);
        } else if (psvbfptr->trownumber) {
            iresult = ivbkeylocaterow (ihandle, ikeynumber,
                                       psvbfptr->trownumber);
        } else {
            vb_rtd->iserrno = ENOCURR;
            iresult = -1;
            break;
        }
        if (iresult) {
            vb_rtd->iserrno = ENOCURR;
        } else {
            iresult = 0;
            vb_rtd->iserrno = ivbkeyload (ihandle, ikeynumber, ISPREV, 1, &pskey);
            if (vb_rtd->iserrno) {
                iresult = -1;
            }
        }
        break;

    case ISNEXT:            /* Might fall thru to ISCURR */
        if (!psvbfptr->iisdisjoint) {
            if (psvbfptr->trowstart) {
                iresult = ivbkeylocaterow (ihandle, ikeynumber,
                                           psvbfptr->trowstart);
            } else if (psvbfptr->trownumber) {
                iresult = ivbkeylocaterow (ihandle, ikeynumber,
                                           psvbfptr->trownumber);
            } else {
                vb_rtd->iserrno = ENOCURR;
                iresult = -1;
                break;
            }
            if (iresult) {
                vb_rtd->iserrno = EENDFILE;
            } else {
                iresult = 0;
                vb_rtd->iserrno = ivbkeyload (ihandle, ikeynumber, ISNEXT, 1, &pskey);
                if (vb_rtd->iserrno) {
                    iresult = -1;
                }
            }
            break;  /* Exit the switch case */
        }
    case ISCURR:
        if (psvbfptr->trowstart) {
            iresult = ivbkeylocaterow (ihandle, ikeynumber, psvbfptr->trowstart);
        } else if (psvbfptr->trownumber) {
            iresult = ivbkeylocaterow (ihandle, ikeynumber,
                                       psvbfptr->trownumber);
        } else {
            vb_rtd->iserrno = ENOCURR;
            iresult = -1;
            break;
        }
        if (iresult) {
            vb_rtd->iserrno = ENOCURR;
        }
        break;

    default:
        vb_rtd->iserrno = EBADARG;
        iresult = -1;
    }
    /* By the time we get here, we're done with index positioning... */
    /* If iresult == 0 then we have a valid row to read in */
    if (!iresult) {
        psvbfptr->trowstart = 0;
        if ((imode & ISLOCK) 
	 || (imode & ISSKIPLOCK)
	 || (psvbfptr->iopenmode & ISAUTOLOCK)) {
            if (ivbdatalock (ihandle, imode & ISWAIT ? VBWRLCKW : VBWRLOCK,
                             psvbfptr->pskeycurr[ikeynumber]->trownode)) {
                iresult = -1;
                vb_rtd->iserrno = ilockresult = ELOCKED;
		if (imode & ISSKIPLOCK) {
		    vb_rtd->isrecnum = psvbfptr->pskeycurr[ikeynumber]->trownode;
		    psvbfptr->trownumber = vb_rtd->isrecnum;
		    psvbfptr->iisdisjoint = 0;
		    ilockresult = 0;
		}
            }
        }
        if (!ilockresult) {
            iresult = ivbdataread (ihandle, pcrow, &ideleted,
                                   psvbfptr->pskeycurr[ikeynumber]->trownode);
        }
        if (!iresult && (!ilockresult || (imode & ISSKIPLOCK && vb_rtd->iserrno == ELOCKED))) {
            vb_rtd->isrecnum = psvbfptr->pskeycurr[ikeynumber]->trownode;
            psvbfptr->trownumber = vb_rtd->isrecnum;
            psvbfptr->iisdisjoint = 0;
        }
    }

    read_exit:
    psvbfptr->iisdictlocked |= 0x04;
    ivbexit (ihandle);
    if(vb_rtd->iserrno == ELOCKED)
	    return 1;
    return iresult;
}

int
isstart (int ihandle, struct keydesc *pskeydesc, int ilength, VB_CHAR *pcrow, int imode)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct VBKEY    *pskey;
    struct DICTINFO *psvbfptr;
    int             ikeynumber, iresult;
    VB_UCHAR        ckeyvalue[VB_MAX_KEYLEN];
    VB_UCHAR        ckeyvalue2[VB_MAX_KEYLEN];

    if (ivbenter (ihandle, 0)) {
        return -1;
    }

    psvbfptr = vb_rtd->psvbfile[ihandle];
    ikeynumber = ivbcheckkey (ihandle, pskeydesc, 2, 0, 0);
    iresult = -1;
    if (ikeynumber == -1 && pskeydesc->k_nparts) {
        goto startexit;
    }
    if (ilength < 1 || ilength > psvbfptr->pskeydesc[ikeynumber]->k_len) {
        ilength = pskeydesc->k_len;
    }
    psvbfptr->iactivekey = ikeynumber;
    if (!(imode & ISKEEPLOCK)) {
        isrelease (ihandle);
    }
    imode &= BYTEMASK;
    if (!pskeydesc->k_nparts) {
        iresult = istartrownumber (ihandle, imode, 0);
        if (iresult && vb_rtd->iserrno == ENOREC && imode <= ISLAST) {
            iresult = 0;
            vb_rtd->iserrno = 0;
        }
        goto startexit;
    }
    vb_rtd->iserrno = 0;
    switch (imode) {
    case ISFIRST:           /* ckeyvalue is just a placeholder for 1st/last */
        psvbfptr->iisdisjoint = 1;
        iresult = ivbkeysearch (ihandle, ISFIRST, ikeynumber, 0, ckeyvalue, (off_t)0);
        if (iresult < 0) {
            break;
        }
        iresult = 0;
        break;

    case ISLAST:            /* ckeyvalue is just a placeholder for 1st/last */
        psvbfptr->iisdisjoint = 0;
        iresult = ivbkeysearch (ihandle, ISLAST, ikeynumber, 0, ckeyvalue, (off_t)0);
        if (iresult < 0 || iresult > 2) {
            iresult = -1;
            break;
        }
        vb_rtd->iserrno = ivbkeyload (ihandle, ikeynumber, ISPREV, 1, &pskey);
        if (vb_rtd->iserrno) {
            iresult = -1;
        }
        break;

    case ISEQUAL:
        psvbfptr->iisdisjoint = 1;
        vvbmakekey (psvbfptr->pskeydesc[ikeynumber], pcrow, ckeyvalue2);
        memset (ckeyvalue, 0, sizeof (ckeyvalue));
        memcpy (ckeyvalue, ckeyvalue2, (size_t)ilength);
        if (ilength < pskeydesc->k_len) {
            iresult = ivbkeysearch (ihandle, ISGTEQ, ikeynumber, 0, ckeyvalue, (off_t)0);
        } else {
            iresult = ivbkeysearch (ihandle, ISEQUAL, ikeynumber, ilength, ckeyvalue, (off_t)0);
        }
        vb_rtd->iserrno = EBADFILE;
        if (iresult == -1) {    /* Error */
            break;
        }
        /* Map EQUAL onto OK and LESS THAN onto OK if the basekey is == */
        if (iresult == 1) {
            iresult = 0;
        } else if (iresult == 0
                   && memcmp (ckeyvalue,
                              psvbfptr->pskeycurr[psvbfptr->iactivekey]->ckey,
                              (size_t)ilength)) {
            vb_rtd->iserrno = ENOREC;
            iresult = -1;
        }
        break;

    case ISGREAT:
    case ISGTEQ:
        psvbfptr->iisdisjoint = 1;
        vvbmakekey (psvbfptr->pskeydesc[ikeynumber], pcrow, ckeyvalue2);
        if (ilength < pskeydesc->k_len && imode == ISGREAT) {
            memset (ckeyvalue, 255, sizeof (ckeyvalue));
        } else {
            memset (ckeyvalue, 0, sizeof (ckeyvalue));
        }
        memcpy (ckeyvalue, ckeyvalue2, (size_t)ilength);
        iresult = ivbkeysearch (ihandle, imode, ikeynumber, 0, ckeyvalue, (off_t)0);
        if (vb_rtd->iserrno == EENDFILE) {
            vb_rtd->iserrno = ENOREC;
            iresult = -1;
            break;
        }
        if (iresult < 0) {      /* Error is always error */
            break;
        }
        if (iresult < 2) {
            iresult = 0;
            break;
        }
        vb_rtd->iserrno = EENDFILE;
        iresult = -1;
        break;

    default:
        vb_rtd->iserrno = EBADARG;
        iresult = -1;
    }
    if (!iresult) {
        vb_rtd->iserrno = 0;
        vb_rtd->isrecnum = psvbfptr->pskeycurr[ikeynumber]->trownode;
        psvbfptr->trowstart = vb_rtd->isrecnum;
    } else {
        psvbfptr->trowstart = 0;
        iresult = -1;
    }
    startexit:
    psvbfptr->iisdictlocked |= 0x04;
    ivbexit (ihandle);
    return iresult;
}
