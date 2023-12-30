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

#include	"isinternal.h"


/* Local functions */

static int
irowdelete (const int ihandle, off_t trownumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    int             ikeynumber, iresult;
    struct DICTINFO *psvbfptr;
    off_t           tdupnumber[MAXSUBS];

    psvbfptr = vb_rtd->psvbfile[ihandle];
    /*
     * Step 1:
     *      Check each index for existance of trownumber
     */
    for (ikeynumber = 0; ikeynumber < psvbfptr->inkeys; ikeynumber++) {
        if (psvbfptr->pskeydesc[ikeynumber]->k_nparts == 0) {
            continue;
        }
        iresult = ivbkeylocaterow (ihandle, ikeynumber, trownumber);
		if (iresult		/* May not be present if key was SUPPRESSED */
		&& psvbfptr->pskeydesc[ikeynumber]->k_flags & NULLKEY)
			continue;
        vb_rtd->iserrno = EBADFILE;
        if (iresult) {
            return -1;
        }
        tdupnumber[ikeynumber] = psvbfptr->pskeycurr[ikeynumber]->tdupnumber;
    }

    /*
     * Step 2:
     *      Perform the actual deletion from each index
     */
    for (ikeynumber = 0; ikeynumber < psvbfptr->inkeys; ikeynumber++) {
        if (psvbfptr->pskeydesc[ikeynumber]->k_nparts == 0) {
            continue;
        }
        iresult = ivbkeydelete (ihandle, ikeynumber);
        if (iresult) {
            vb_rtd->iserrno = iresult;
            return -1;
        }
    }

    return 0;
}

static int
iprocessdelete (const int ihandle, off_t trownumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    int     ideleted;
    struct DICTINFO *psvbfptr;

    psvbfptr = vb_rtd->psvbfile[ihandle];
    if (psvbfptr->iopenmode & ISTRANS) {
        vb_rtd->iserrno = ivbdatalock (ihandle, VBWRLOCK, trownumber);
        if (vb_rtd->iserrno) {
            return -1;
        }
    }
    vb_rtd->iserrno = ivbdataread (ihandle, (void *)psvbfptr->ppcrowbuffer,
                                   &ideleted, trownumber);
    if (!vb_rtd->iserrno && ideleted) {
        vb_rtd->iserrno = ENOREC;
    }
    if (vb_rtd->iserrno) {
        return -1;
    }
    if (irowdelete (ihandle, trownumber)) {
        return -1;
    }
    if (!vb_rtd->pcwritebuffer) {
        vb_rtd->pcwritebuffer = pvvbmalloc (MAX_RESERVED_LENGTH);
        if (!vb_rtd->pcwritebuffer) {
            vb_rtd->iserrno = EBADMEM;
            return -1;
        }
    }
    vb_rtd->iserrno = ivbdatawrite (ihandle, (void *)vb_rtd->pcwritebuffer, 1, trownumber);
    if (vb_rtd->iserrno) {
        return -1;
    }
    if (!(psvbfptr->iopenmode & ISTRANS) || vb_rtd->ivbintrans == VBNOTRANS
        || vb_rtd->ivbintrans == VBCOMMIT || vb_rtd->ivbintrans == VBROLLBACK) {
        vb_rtd->iserrno = ivbdatafree (ihandle, trownumber);
        if (vb_rtd->iserrno) {
            return -1;
        }
    }
    vb_rtd->isrecnum = trownumber;
    if (trownumber == psvbfptr->trownumber) {
        psvbfptr->trownumber = 0;
    }
    ivbtransdelete (ihandle, trownumber, vb_rtd->isreclen); /* BUG - retval */
    return 0;
}

/* Global functions */

int
isdelete (int ihandle, VB_CHAR *pcrow)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    int             iresult = 0;
    struct DICTINFO *psvbfptr;
    VB_UCHAR        ckeyvalue[VB_MAX_KEYLEN];

    if (ivbenter (ihandle, 1)) {
        return -1;
    }

    psvbfptr = vb_rtd->psvbfile[ihandle];
    if (psvbfptr->pskeydesc[0]->k_flags & ISDUPS) {
        vb_rtd->iserrno = ENOPRIM;
        iresult = -1;
    } else {
        vvbmakekey (psvbfptr->pskeydesc[0], pcrow, ckeyvalue);
        iresult = ivbkeysearch (ihandle, ISEQUAL, 0, 0, ckeyvalue, (off_t)0);
        switch (iresult) {
        case 1: /* Exact match */
            iresult =
            iprocessdelete (ihandle, psvbfptr->pskeycurr[0]->trownode);
            break;

        case 0: /* LESS than */
        case 2: /* EMPTY file */
            vb_rtd->iserrno = ENOREC;
            iresult = -1;
            break;

        default:
            vb_rtd->iserrno = EBADFILE;
            iresult = -1;
            break;
        }
    }

    if (iresult == 0) {
        psvbfptr->iisdictlocked |= 0x02;
    }
    iresult |= ivbexit (ihandle);
    return iresult;
}

int
isdelcurr (int ihandle)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    int             iresult = 0;

    if (ivbenter (ihandle, 1)) {
        return -1;
    }
    psvbptr = vb_rtd->psvbfile[ihandle];

    if (psvbptr->trownumber > 0) {
        iresult = iprocessdelete (ihandle, psvbptr->trownumber);
    } else {
        vb_rtd->iserrno = ENOREC;
        iresult = -1;
    }

    if (iresult == 0) {
        psvbptr->iisdictlocked |= 0x02;
    }
    iresult |= ivbexit (ihandle);
    return iresult;
}

int
isdelrec (int ihandle, vbisam_off_t trownumber)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    int             iresult = 0;

    if (ivbenter (ihandle, 1)) {
        return -1;
    }
    psvbptr = vb_rtd->psvbfile[ihandle];

    if (trownumber > 0) {
        iresult = iprocessdelete (ihandle, trownumber);
    } else {
        vb_rtd->iserrno = ENOREC;
        iresult = -1;
    }

    if (iresult == 0) {
        psvbptr->iisdictlocked |= 0x02;
    }
    iresult |= ivbexit (ihandle);
    return iresult;
}
