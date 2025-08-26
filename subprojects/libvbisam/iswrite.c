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

static int
irowinsert (const int ihandle, VB_CHAR *pcrow_buffer, off_t trownumber)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct VBKEY	*pskey;
	struct DICTINFO	*psvbfptr;
	struct keydesc	*pskptr;
	off_t		tdupnumber[MAXSUBS];
	int		ikeynumber, iresult;
	VB_UCHAR	ckeyvalue[VB_MAX_KEYLEN];

	psvbfptr = vb_rtd->psvbfile[ihandle];
	/*
	 * Step 1:
	 *      Check each index for a potential ISNODUPS error (EDUPL)
	 *      Also, calculate the duplicate number as needed
	 */
	for (ikeynumber = 0; ikeynumber < psvbfptr->inkeys; ikeynumber++) {
		pskptr = psvbfptr->pskeydesc[ikeynumber];
		if (pskptr->k_nparts == 0) {
			continue;
		}
		vvbmakekey (psvbfptr->pskeydesc[ikeynumber], pcrow_buffer, ckeyvalue);
		iresult = ivbkeysearch (ihandle, ISGREAT, ikeynumber, 0, ckeyvalue, (off_t)0);
		tdupnumber[ikeynumber] = 0;
		if (iresult >= 0 && !ivbkeyload (ihandle, ikeynumber, ISPREV, 0, &pskey)
		    && !memcmp (pskey->ckey, ckeyvalue, (size_t)pskptr->k_len)) {
			vb_rtd->iserrno = EDUPL;
			if (pskptr->k_flags & ISDUPS) {
				tdupnumber[ikeynumber] = pskey->tdupnumber + 1;
			} else {
				return -1;
			}
		}
		iresult = ivbkeysearch (ihandle, ISGTEQ, ikeynumber, 0, ckeyvalue,
				  tdupnumber[ikeynumber]);
	}

	/* Step 2: Perform the actual insertion into each index */
	for (ikeynumber = 0; ikeynumber < psvbfptr->inkeys; ikeynumber++) {
		if (psvbfptr->pskeydesc[ikeynumber]->k_nparts == 0) {
			continue;
		}
		vvbmakekey (psvbfptr->pskeydesc[ikeynumber], pcrow_buffer, ckeyvalue);
		iresult = ivbkeyinsert (ihandle, NULL, ikeynumber, ckeyvalue,
				trownumber, tdupnumber[ikeynumber], NULL);
		if (iresult) {
/* BUG - do something SANE here */
			/* Eeek, an error occured.  Let's remove what we added */
			/* while (ikeynumber >= 0) */
			/* { */
			/* ivbkeydelete (ihandle, ikeynumber); */
			/* ikeynumber--; */
			/* } */
			return iresult;
		}
	}

	return 0;
}

/* Global functions */

int
ivbwriterow (const int ihandle, VB_CHAR *pcrow, off_t trownumber)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbfptr;
	int		iresult = 0;

	psvbfptr = vb_rtd->psvbfile[ihandle];
	vb_rtd->isrecnum = trownumber;
	if (psvbfptr->iopenmode & ISTRANS) {
		vb_rtd->iserrno = ivbdatalock (ihandle, VBWRLOCK, trownumber);
		if (vb_rtd->iserrno) {
			return -1;
		}
	}
	iresult = irowinsert (ihandle, pcrow, trownumber);
	if (!iresult) {
		vb_rtd->iserrno = 0;
		psvbfptr->tvarlennode = 0;	/* Stop it from removing */
		iresult = ivbdatawrite (ihandle, (void *)pcrow, 0, trownumber);
		if (iresult) {
			vb_rtd->iserrno = iresult;
			if (psvbfptr->iopenmode & ISTRANS) {
				ivbdatalock (ihandle, VBUNLOCK, trownumber);
			}
			return -1;
		}
		if (psvbfptr->iopenmode & ISVARLEN) {
			iresult = ivbtransinsert (ihandle, trownumber,
					vb_rtd->isreclen, pcrow);
		} else {
			iresult = ivbtransinsert (ihandle, trownumber,
					psvbfptr->iminrowlength, pcrow);
		}
	}
	return iresult;
}

int
iswrcurr (int ihandle, VB_CHAR *pcrow)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;
	off_t	trownumber;
	int	iresult;

	if (ivbenter (ihandle, 1)) {
		return -1;
	}
	psvbptr = vb_rtd->psvbfile[ihandle];

	if ((psvbptr->iopenmode & ISVARLEN) && (vb_rtd->isreclen > psvbptr->imaxrowlength
		|| vb_rtd->isreclen < psvbptr->iminrowlength)) {
		ivbexit (ihandle);
		vb_rtd->iserrno = EBADARG;
		return -1;
	}

	trownumber = tvbdataallocate (ihandle);
	if (trownumber == -1) {
		ivbexit (ihandle);
		return -1;
	}

	iresult = ivbwriterow (ihandle, pcrow, trownumber);
	if (!iresult) {
		psvbptr->trownumber = trownumber;
	} else {
		ivbdatafree (ihandle, trownumber);
	}

	ivbexit (ihandle);
	return iresult;
}

int
iswrite (int ihandle, VB_CHAR *pcrow)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;
	off_t trownumber;
	int iresult, isaveerror;

	if (ivbenter (ihandle, 1)) {
		return -1;
	}
	psvbptr = vb_rtd->psvbfile[ihandle];

	if ((psvbptr->iopenmode & ISVARLEN) && (vb_rtd->isreclen > psvbptr->imaxrowlength
		|| vb_rtd->isreclen < psvbptr->iminrowlength)) {
		ivbexit (ihandle);
		vb_rtd->iserrno = EBADARG;
		return -1;
	}

	trownumber = tvbdataallocate (ihandle);
	if (trownumber == -1) {
		ivbexit (ihandle);
		return -1;
	}

	iresult = ivbwriterow (ihandle, pcrow, trownumber);
	if (iresult) {
		isaveerror = vb_rtd->iserrno;
		ivbdatafree (ihandle, trownumber);
		vb_rtd->iserrno = isaveerror;
	}

	ivbexit (ihandle);
	return iresult;
}
