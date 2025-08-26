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
#define NEED_VBINLINE_QUAD_LOAD 1
#define NEED_VBINLINE_QUAD_STORE 1
#include	"isinternal.h"

/* Local functions */

static off_t
tvbdatacountgetnext (const int ihandle)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *tvbptr;
    off_t       tvalue;

    tvbptr = vb_rtd->psvbfile[ihandle];
    vb_rtd->iserrno = EBADARG;
    if (!tvbptr->iisdictlocked) {
        return -1;
    }
    vb_rtd->iserrno = 0;

    tvalue = inl_ldquad (tvbptr->sdictnode.cdatacount) + 1;
    inl_stquad (tvalue, tvbptr->sdictnode.cdatacount);
    tvbptr->iisdictlocked |= 0x02;
    return tvalue;
}

/* Global functions */

off_t
tvbnodecountgetnext (const int ihandle)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *tvbptr;
    off_t       tvalue;

    tvbptr = vb_rtd->psvbfile[ihandle];
    vb_rtd->iserrno = EBADARG;
    if (!tvbptr->iisdictlocked) {
        return -1;
    }
    vb_rtd->iserrno = 0;

    tvalue = inl_ldquad (tvbptr->sdictnode.cnodecount) + 1;
    inl_stquad (tvalue, tvbptr->sdictnode.cnodecount);
    tvbptr->iisdictlocked |= 0x02;
    return tvalue;
}

int
ivbnodefree (const int ihandle, off_t tnodenumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *tvbptr;
    off_t           theadnode;
    int             ilengthused, iresult;
    VB_CHAR	        cvbnodetmp[MAX_NODE_LENGTH];    
    VB_CHAR         cvbnodetmp2[MAX_NODE_LENGTH];

    /* Sanity check - Is ihandle a currently open table? */
    vb_rtd->iserrno = ENOTOPEN;
    tvbptr = vb_rtd->psvbfile[ihandle];
    if (!tvbptr) {
        return -1;
    }
    vb_rtd->iserrno = EBADARG;
    if (!tvbptr->iisdictlocked) {
        return -1;
    }
    vb_rtd->iserrno = 0;

    memset (cvbnodetmp2, 0, (size_t) tvbptr->inodesize);
    inl_stint (INTSIZE, cvbnodetmp2);

    theadnode = inl_ldquad (tvbptr->sdictnode.cnodefree);
    /* If the list is empty, node tnodenumber becomes the whole list */
    if (theadnode == 0) {
        inl_stint (INTSIZE + QUADSIZE, cvbnodetmp2);
        inl_stquad ((off_t)0, cvbnodetmp2 + INTSIZE);
        cvbnodetmp2[tvbptr->inodesize - 2] = 0x7f;
        cvbnodetmp2[tvbptr->inodesize - 3] = -2;
        iresult = ivbblockwrite (ihandle, 1, tnodenumber, cvbnodetmp2);
        if (iresult) {
            return iresult;
        }
        inl_stquad (tnodenumber, tvbptr->sdictnode.cnodefree);
        tvbptr->iisdictlocked |= 0x02;
        return 0;
    }

    /* Read in the head of the current free list */
    iresult = ivbblockread (ihandle, 1, theadnode, cvbnodetmp);
    if (iresult) {
        return iresult;
    }
/* Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO) */
#if	ISAMMODE == 1
    if (cvbnodetmp[tvbptr->inodesize - 2] != 0x7f) {
        return EBADFILE;
    }
#endif	/* ISAMMODE == 1 */
    if (cvbnodetmp[tvbptr->inodesize - 3] != -2) {
        return EBADFILE;
    }
    ilengthused = inl_ldint (cvbnodetmp);
    if (ilengthused >= tvbptr->inodesize - (QUADSIZE + 3)) {
        /* If there was no space left, tnodenumber becomes the head */
        cvbnodetmp2[tvbptr->inodesize - 2] = 0x7f;
        cvbnodetmp2[tvbptr->inodesize - 3] = -2;
        inl_stint (INTSIZE + QUADSIZE, cvbnodetmp2);
        inl_stquad (theadnode, &cvbnodetmp2[INTSIZE]);
        iresult = ivbblockwrite (ihandle, 1, tnodenumber, cvbnodetmp2);
        if (!iresult) {
            inl_stquad (tnodenumber, tvbptr->sdictnode.cnodefree);
            tvbptr->iisdictlocked |= 0x02;
        }
        return iresult;
    }

    /* If we got here, there's space left in the theadnode to store it */
    cvbnodetmp2[tvbptr->inodesize - 2] = 0x7f;
    cvbnodetmp2[tvbptr->inodesize - 3] = -2;
    iresult = ivbblockwrite (ihandle, 1, tnodenumber, cvbnodetmp2);
    if (iresult) {
        return iresult;
    }
    inl_stquad (tnodenumber, &cvbnodetmp[ilengthused]);
    ilengthused += QUADSIZE;
    inl_stint (ilengthused, cvbnodetmp);
    iresult = ivbblockwrite (ihandle, 1, theadnode, cvbnodetmp);

    return iresult;
}

int
ivbdatafree (const int ihandle, off_t trownumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *tvbptr;
    off_t           theadnode, tnodenumber;
    int             ilengthused, iresult;
    VB_CHAR	        cvbnodetmp[MAX_NODE_LENGTH];
    /* Sanity check - Is ihandle a currently open table? */
    vb_rtd->iserrno = ENOTOPEN;
    tvbptr = vb_rtd->psvbfile[ihandle];
    if (!tvbptr) {
        return -1;
    }
    vb_rtd->iserrno = EBADARG;
    if (!tvbptr->iisdictlocked) {
        return -1;
    }
    vb_rtd->iserrno = 0;

    if (inl_ldquad (tvbptr->sdictnode.cdatacount) == trownumber) {
        inl_stquad (trownumber - 1, tvbptr->sdictnode.cdatacount);
        tvbptr->iisdictlocked |= 0x02;
        return 0;
    }

    theadnode = inl_ldquad (tvbptr->sdictnode.cdatafree);
    if (theadnode != 0) {
        iresult = ivbblockread (ihandle, 1, theadnode, cvbnodetmp);
        if (iresult) {
            return iresult;
        }
/* Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO) */
#if	ISAMMODE == 1
        if (cvbnodetmp[tvbptr->inodesize - 2] != 0x7f) {
            return EBADFILE;
        }
#endif	/* ISAMMODE == 1 */
        if (cvbnodetmp[tvbptr->inodesize - 3] != -1) {
            return EBADFILE;
        }
        ilengthused = inl_ldint (cvbnodetmp);
        if (ilengthused < tvbptr->inodesize - (QUADSIZE + 3)) {
            /* We need to add trownumber to the current node */
            inl_stquad ((off_t) trownumber, cvbnodetmp + ilengthused);
            ilengthused += QUADSIZE;
            inl_stint (ilengthused, cvbnodetmp);
            iresult = ivbblockwrite (ihandle, 1, theadnode, cvbnodetmp);
            return iresult;
        }
    }
    /* We need to allocate a new row-free node! */
    /* We append any existing nodes using the next pointer from the new node */
    tnodenumber = tvbnodeallocate (ihandle);
    if (tnodenumber == (off_t)-1) {
        return vb_rtd->iserrno;
    }
    memset (cvbnodetmp, 0, MAX_NODE_LENGTH);
    cvbnodetmp[tvbptr->inodesize - 2] = 0x7f;
    cvbnodetmp[tvbptr->inodesize - 3] = -1;
    inl_stint (INTSIZE + (2 * QUADSIZE), cvbnodetmp);
    inl_stquad (theadnode, &cvbnodetmp[INTSIZE]);
    inl_stquad (trownumber, &cvbnodetmp[INTSIZE + QUADSIZE]);
    iresult = ivbblockwrite (ihandle, 1, tnodenumber, cvbnodetmp);
    if (iresult) {
        return iresult;
    }
    inl_stquad (tnodenumber, tvbptr->sdictnode.cdatafree);
    tvbptr->iisdictlocked |= 0x02;
    return 0;
}

off_t
tvbnodeallocate (const int ihandle)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *tvbptr;
    off_t           theadnode, tvalue;
    int             ilengthused;
    VB_CHAR	        cvbnodetmp[MAX_NODE_LENGTH];
    /* Sanity check - Is ihandle a currently open table? */
    vb_rtd->iserrno = ENOTOPEN;
    tvbptr = vb_rtd->psvbfile[ihandle];
    if (!tvbptr) {
        return -1;
    }
    vb_rtd->iserrno = EBADARG;
    if (!tvbptr->iisdictlocked) {
        return -1;
    }
    vb_rtd->iserrno = 0;

    /* If there's *ANY* nodes in the free list, use them first! */
    theadnode = inl_ldquad (tvbptr->sdictnode.cnodefree);
    if (theadnode != 0) {
        vb_rtd->iserrno = ivbblockread (ihandle, 1, theadnode, cvbnodetmp);
        if (vb_rtd->iserrno) {
            return -1;
        }
        vb_rtd->iserrno = EBADFILE;
/* Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO) */
#if	ISAMMODE == 1
        if (cvbnodetmp[tvbptr->inodesize - 2] != 0x7f) {
            return -1;
        }
#endif	/* ISAMMODE == 1 */
        if (cvbnodetmp[tvbptr->inodesize - 3] != -2) {
            return -1;
        }
        ilengthused = inl_ldint (cvbnodetmp);
        if (ilengthused > (INTSIZE + QUADSIZE)) {
            tvalue = inl_ldquad (cvbnodetmp + INTSIZE + QUADSIZE);
            memcpy (cvbnodetmp + INTSIZE + QUADSIZE,
                    cvbnodetmp + INTSIZE + QUADSIZE + QUADSIZE,
                    (size_t)(ilengthused - (INTSIZE + QUADSIZE + QUADSIZE)));
            ilengthused -= QUADSIZE;
            memset (cvbnodetmp + ilengthused, 0, QUADSIZE);
            inl_stint (ilengthused, cvbnodetmp);
            vb_rtd->iserrno = ivbblockwrite (ihandle, 1, theadnode, cvbnodetmp);
            if (vb_rtd->iserrno) {
                return -1;
            }
            return tvalue;
        }
        /* If it's last entry in the node, use the node itself! */
        tvalue = inl_ldquad (cvbnodetmp + INTSIZE);
        inl_stquad (tvalue, tvbptr->sdictnode.cnodefree);
        tvbptr->iisdictlocked |= 0x02;
        return theadnode;
    }
    /* If we get here, we need to allocate a NEW node. */
    /* Since we already hold a dictionary lock, we don't need another */
    tvalue = tvbnodecountgetnext (ihandle);
    return tvalue;
}

off_t
tvbdataallocate (const int ihandle)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *tvbptr;
    off_t           theadnode, tnextnode, tvalue;
    int             ilengthused, iresult;
    VB_CHAR	        cvbnodetmp[MAX_NODE_LENGTH];

    tvbptr = vb_rtd->psvbfile[ihandle];
    vb_rtd->iserrno = EBADARG;
    if (!tvbptr->iisdictlocked) {
        return -1;
    }
    vb_rtd->iserrno = 0;

    /* If there's *ANY* rows in the free list, use them first! */
    theadnode = inl_ldquad (tvbptr->sdictnode.cdatafree);
    while (theadnode != 0) {
        vb_rtd->iserrno = ivbblockread (ihandle, 1, theadnode, cvbnodetmp);
        if (vb_rtd->iserrno) {
            return -1;
        }
        vb_rtd->iserrno = EBADFILE;
/* Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO) */
#if	ISAMMODE == 1
        if (cvbnodetmp[tvbptr->inodesize - 2] != 0x7f) {
            return -1;
        }
#endif	/* ISAMMODE == 1 */
        if (cvbnodetmp[tvbptr->inodesize - 3] != -1) {
            return -1;
        }
        ilengthused = inl_ldint (cvbnodetmp);
        if (ilengthused > INTSIZE + QUADSIZE) {
            tvbptr->iisdictlocked |= 0x02;
            ilengthused -= QUADSIZE;
            inl_stint (ilengthused, cvbnodetmp);
            tvalue = inl_ldquad (&cvbnodetmp[ilengthused]);
            inl_stquad ((off_t)0, &cvbnodetmp[ilengthused]);
            if (ilengthused > INTSIZE + QUADSIZE) {
                vb_rtd->iserrno = ivbblockwrite (ihandle, 1, theadnode, cvbnodetmp);
                if (vb_rtd->iserrno) {
                    return -1;
                }
                return tvalue;
            }
            /* If we're using the last entry in the node, advance */
            tnextnode = inl_ldquad (&cvbnodetmp[INTSIZE]);
            iresult = ivbnodefree (ihandle, theadnode);
            if (iresult) {
                return -1;
            }
            inl_stquad (tnextnode, tvbptr->sdictnode.cdatafree);
            return tvalue;
        }
        /* Ummmm, this is an INTEGRITY ERROR of sorts! */
        /* However, let's fix it anyway! */
        tnextnode = inl_ldquad (&cvbnodetmp[INTSIZE]);
        iresult = ivbnodefree (ihandle, theadnode);
        if (iresult) {
            return -1;
        }
        inl_stquad (tnextnode, tvbptr->sdictnode.cdatafree);
        tvbptr->iisdictlocked |= 0x02;
        theadnode = tnextnode;
    }
    /* If we get here, we need to allocate a NEW row number. */
    /* Since we already hold a dictionary lock, we don't need another */
    tvalue = tvbdatacountgetnext (ihandle);
    return tvalue;
}

int
ivbforcedataallocate (const int ihandle, off_t trownumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *tvbptr;
    off_t           theadnode, tprevnode, tnextnode;
    int             iloop, ilengthused;
    VB_CHAR	        cvbnodetmp[MAX_NODE_LENGTH];

    /* Sanity check - Is ihandle a currently open table? */
    vb_rtd->iserrno = ENOTOPEN;
    tvbptr = vb_rtd->psvbfile[ihandle];
    if (!tvbptr) {
        return -1;
    }
    vb_rtd->iserrno = EBADARG;
    if (!tvbptr->iisdictlocked) {
        return -1;
    }
    vb_rtd->iserrno = 0;

    /* Test 1: Is it already beyond EOF (the SIMPLE test) */
    theadnode = inl_ldquad (tvbptr->sdictnode.cdatacount);
    if (theadnode < trownumber) {
        tvbptr->iisdictlocked |= 0x02;
        inl_stquad (trownumber, tvbptr->sdictnode.cdatacount);
        theadnode++;
        while (theadnode < trownumber) {
            if (theadnode != 0) {
                ivbdatafree (ihandle, theadnode);
            }
            theadnode++;
        }
        return 0;
    }
    /* <SIGH> It SHOULD be *SOMEWHERE* in the data free list! */
    tprevnode = 0;
    theadnode = inl_ldquad (tvbptr->sdictnode.cdatafree);
    while (theadnode != 0) {
        vb_rtd->iserrno = ivbblockread (ihandle, 1, theadnode, cvbnodetmp);
        if (vb_rtd->iserrno) {
            return -1;
        }
        vb_rtd->iserrno = EBADFILE;
/* Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO) */
#if	ISAMMODE == 1
        if (cvbnodetmp[tvbptr->inodesize - 2] != 0x7f) {
            return -1;
        }
#endif	/* ISAMMODE == 1 */
        if (cvbnodetmp[tvbptr->inodesize - 3] != -1) {
            return -1;
        }
        ilengthused = inl_ldint (cvbnodetmp);
        for (iloop = INTSIZE + QUADSIZE; iloop < ilengthused; iloop += QUADSIZE) {
            if (inl_ldquad (&cvbnodetmp[iloop]) == trownumber) {    /* Extract it */
                memcpy (&(cvbnodetmp[iloop]), &(cvbnodetmp[iloop + QUADSIZE]),
                        (size_t)(ilengthused - iloop));
                ilengthused -= QUADSIZE;
                if (ilengthused > INTSIZE + QUADSIZE) {
                    inl_stquad ((off_t)0, &cvbnodetmp[ilengthused]);
                    inl_stint (ilengthused, cvbnodetmp);
                    return ivbblockwrite (ihandle, 1, theadnode, cvbnodetmp);
                } else {    /* It was the last one in the node! */
                    tnextnode = inl_ldquad (&cvbnodetmp[INTSIZE]);
                    if (tprevnode) {
                        vb_rtd->iserrno =
                        ivbblockread (ihandle, 1, tprevnode,
                                      cvbnodetmp);
                        if (vb_rtd->iserrno) {
                            return -1;
                        }
                        inl_stquad (tnextnode, &cvbnodetmp[INTSIZE]);
                        return ivbblockwrite
                        (ihandle, 1, tprevnode, cvbnodetmp);
                    } else {
                        tvbptr->iisdictlocked |= 0x02;
                        inl_stquad (tnextnode,
                                    tvbptr->sdictnode.cdatafree);
                    }
                    return ivbnodefree (ihandle, theadnode);
                }
            }
        }
        tprevnode = theadnode;
        theadnode = inl_ldquad (&cvbnodetmp[INTSIZE]);
    }
    /* If we get here, we've got a MAJOR integrity error in that the */
    /* nominated row number was simply *NOT FREE* */
    vb_rtd->iserrno = EBADFILE;
    return -1;
}
