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
irowupdate (const int ihandle, VB_CHAR *pcrow, off_t trownumber)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct VBKEY	*pskey;
	struct DICTINFO	*psvbfptr;
	struct keydesc	*pskeyptr;
	off_t		tdupnumber = 0;
	int		ikeynumber, iresult;
	char		keypresent[64];
	VB_UCHAR	ckeyvalue[VB_MAX_KEYLEN];

	psvbfptr = vb_rtd->psvbfile[ihandle];
	/*
	 * Step 1:
	 *      For each index that's changing, confirm that the NEW value
	 *      doesn't conflict with an existing ISNODUPS flag.
	 */
	for (ikeynumber = 0; ikeynumber < psvbfptr->inkeys; ikeynumber++) {
		pskeyptr = psvbfptr->pskeydesc[ikeynumber];
		if (pskeyptr->k_nparts == 0) {
			continue;
		}
		if (pskeyptr->k_flags & ISDUPS) {
			continue;
		}
		vvbmakekey (pskeyptr, pcrow, ckeyvalue);
		iresult = ivbkeysearch (ihandle, ISGTEQ, ikeynumber, 0, ckeyvalue, (off_t)0);
		if (iresult != 1
		    || trownumber == psvbfptr->pskeycurr[ikeynumber]->trownode
		    || psvbfptr->pskeycurr[ikeynumber]->iisdummy) {
			continue;
		}
		vb_rtd->iserrno = EDUPL;
		return -1;
	}

	/*
	 * Step 2:
	 *      Check each index for existance of trownumber
	 *      This 'preload' additionally helps determine which indexes change
	 */
	for (ikeynumber = 0; ikeynumber < psvbfptr->inkeys; ikeynumber++) {
		pskeyptr = psvbfptr->pskeydesc[ikeynumber];
		if (pskeyptr->k_nparts == 0) {
			continue;
		}
		iresult = ivbkeylocaterow (ihandle, ikeynumber, trownumber);
		if (iresult) {
			keypresent[ikeynumber] = 0;
			if ((pskeyptr->k_flags & NULLKEY))
				continue;
			vb_rtd->iserrno = EBADFILE;
			return -1;
		}
		keypresent[ikeynumber] = 1;
	}

	/*
	 * Step 3:
	 *      Perform the actual deletion / insertion with each index
	 *      But *ONLY* for those indexes that have actually CHANGED!
	 */
	for (ikeynumber = 0; ikeynumber < psvbfptr->inkeys; ikeynumber++) {
		pskeyptr = psvbfptr->pskeydesc[ikeynumber];
		if (pskeyptr->k_nparts == 0) {
			continue;
		}
		/* pcrow is the UPDATED key! */
		vvbmakekey (pskeyptr, pcrow, ckeyvalue);
		if (keypresent[ikeynumber] == 0) {
			iresult = 0;
		} else {
			iresult = memcmp (ckeyvalue, psvbfptr->pskeycurr[ikeynumber]->ckey,
					(size_t)pskeyptr->k_len);
			/* If NEW key is DIFFERENT than CURRENT, remove CURRENT */
			if (iresult) {
				iresult = ivbkeydelete (ihandle, ikeynumber);
			} else {
				continue;
			}
			if (iresult) {
				/* Eeek, an error occured.  Let's put back what we removed! */
				while (ikeynumber >= 0) {
	/* BUG - We need to do SOMETHING sane here? Dunno WHAT */
					ivbkeyinsert (ihandle, NULL, ikeynumber, ckeyvalue,
							  trownumber, tdupnumber, NULL);
					ikeynumber--;
					vvbmakekey (pskeyptr,
							psvbfptr->ppcrowbuffer, ckeyvalue);
				}
				vb_rtd->iserrno = EBADFILE;
				return -1;
			}
			iresult = ivbkeysearch (ihandle, ISGREAT, ikeynumber, 0, ckeyvalue, (off_t)0);
		}
		tdupnumber = 0;
		if (iresult >= 0) {
			iresult = ivbkeyload (ihandle, ikeynumber, ISPREV, 1, &pskey);
			if (!iresult) {
				if (pskeyptr->k_flags & ISDUPS
				    && !memcmp (pskey->ckey, ckeyvalue,
						(size_t)pskeyptr->k_len)) {
					tdupnumber = pskey->tdupnumber + 1;
				}
				psvbfptr->pskeycurr[ikeynumber] =
				    psvbfptr->pskeycurr[ikeynumber]->psnext;
				psvbfptr->pskeycurr[ikeynumber]->psparent->pskeycurr =
				    psvbfptr->pskeycurr[ikeynumber];
			}
			iresult = ivbkeysearch (ihandle, ISGTEQ, ikeynumber, 0, ckeyvalue,
					  tdupnumber);
			iresult = ivbkeyinsert (ihandle, NULL, ikeynumber, ckeyvalue,
					  trownumber, tdupnumber, NULL);
		}
		if (iresult) {
			/* Eeek, an error occured.  Let's remove what we added */
			while (ikeynumber >= 0) {
/* BUG - This is WRONG, we should re-establish what we had before! */
				/* ivbkeydelete (ihandle, ikeynumber); */
				ikeynumber--;
			}
			return iresult;
		}
	}

	vb_rtd->iserrno = 0;
	return 0;
}

/* Global functions */

int
isrewrite (int ihandle, VB_CHAR *pcrow)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbfptr;
	off_t		trownumber;
	int		ideleted, inewreclen, ioldreclen = 0, iresult = 0;
	VB_UCHAR	ckeyvalue[VB_MAX_KEYLEN];

	if (ivbenter (ihandle, 1)) {
		return -1;
	}
	psvbfptr = vb_rtd->psvbfile[ihandle];
	if ((psvbfptr->iopenmode & ISVARLEN) && (vb_rtd->isreclen > psvbfptr->imaxrowlength
		|| vb_rtd->isreclen < psvbfptr->iminrowlength)) {
		ivbexit (ihandle);
		vb_rtd->iserrno = EBADARG;
		return -1;
	}

	inewreclen = vb_rtd->isreclen;
	if (psvbfptr->pskeydesc[0]->k_flags & ISDUPS) {
		iresult = -1;
		vb_rtd->iserrno = ENOREC;
	} else {
		vvbmakekey (psvbfptr->pskeydesc[0], pcrow, ckeyvalue);
		iresult = ivbkeysearch (ihandle, ISEQUAL, 0, 0, ckeyvalue, (off_t)0);
		switch (iresult) {
		case 1:	/* Exact match */
			iresult = 0;
			psvbfptr->iisdictlocked |= 0x02;
			trownumber = psvbfptr->pskeycurr[0]->trownode;
			if (psvbfptr->iopenmode & ISTRANS) {
				vb_rtd->iserrno = ivbdatalock (ihandle, VBWRLOCK, trownumber);
				if (vb_rtd->iserrno) {
					iresult = -1;
					goto isrewrite_exit;
				}
			}
			vb_rtd->iserrno = ivbdataread (ihandle, (void *)psvbfptr->ppcrowbuffer,
					 &ideleted, trownumber);
			if (!vb_rtd->iserrno && ideleted) {
				vb_rtd->iserrno = ENOREC;
			}
			if (vb_rtd->iserrno) {
				iresult = -1;
			} else {
				ioldreclen = vb_rtd->isreclen;
			}

			if (!iresult) {
				iresult = irowupdate (ihandle, pcrow, trownumber);
			}
			if (!iresult) {
				vb_rtd->isrecnum = trownumber;
				vb_rtd->isreclen = inewreclen;
				iresult =
				    ivbdatawrite (ihandle, (void *)pcrow, 0, (off_t)vb_rtd->isrecnum);
			}
			if (!iresult) {
				if (psvbfptr->iopenmode & ISVARLEN) {
					iresult =
					    ivbtransupdate (ihandle, trownumber, ioldreclen,
							    inewreclen, pcrow);
				} else {
					iresult =
					    ivbtransupdate (ihandle, trownumber,
							    psvbfptr->iminrowlength,
							    psvbfptr->iminrowlength,
							    pcrow);
				}
			}
			break;

		case 0:	/* LESS than */
		case 2:	/* GREATER than */
		case 3:	/* EMPTY file */
			vb_rtd->iserrno = ENOREC;
			iresult = -1;
			break;

		default:
			vb_rtd->iserrno = EBADFILE;
			iresult = -1;
			break;
		}
	}

isrewrite_exit:
	iresult |= ivbexit (ihandle);
	return iresult;
}

int
isrewcurr (int ihandle, VB_CHAR *pcrow)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbfptr;
	int		ideleted, inewreclen, ioldreclen = 0, iresult = 0;

	if (ivbenter (ihandle, 1)) {
		return -1;
	}

	psvbfptr = vb_rtd->psvbfile[ihandle];
	if ((psvbfptr->iopenmode & ISVARLEN) && (vb_rtd->isreclen > psvbfptr->imaxrowlength
		|| vb_rtd->isreclen < psvbfptr->iminrowlength)) {
		ivbexit (ihandle);
		vb_rtd->iserrno = EBADARG;
		return -1;
	}

	inewreclen = vb_rtd->isreclen;
	if (psvbfptr->trownumber > 0) {
		if (psvbfptr->iopenmode & ISTRANS) {
			vb_rtd->iserrno = ivbdatalock (ihandle, VBWRLOCK, psvbfptr->trownumber);
			if (vb_rtd->iserrno) {
				iresult = -1;
				goto isrewcurr_exit;
			}
		}
		vb_rtd->iserrno = ivbdataread (ihandle, (void *)psvbfptr->ppcrowbuffer, &ideleted,
				 psvbfptr->trownumber);
		if (!vb_rtd->iserrno && ideleted) {
			vb_rtd->iserrno = ENOREC;
		} else {
			ioldreclen = vb_rtd->isreclen;
		}
		if (vb_rtd->iserrno) {
			iresult = -1;
		}
		if (!iresult) {
			iresult = irowupdate (ihandle, pcrow, psvbfptr->trownumber);
		}
		if (!iresult) {
			vb_rtd->isrecnum = psvbfptr->trownumber;
			vb_rtd->isreclen = inewreclen;
			iresult = ivbdatawrite (ihandle, (void *)pcrow, 0, (off_t)vb_rtd->isrecnum);
		}
		if (!iresult) {
			if (psvbfptr->iopenmode & ISVARLEN) {
				iresult =
				    ivbtransupdate (ihandle, psvbfptr->trownumber,
						    ioldreclen, inewreclen, pcrow);
			} else {
				iresult =
				    ivbtransupdate (ihandle, psvbfptr->trownumber,
						    psvbfptr->iminrowlength,
						    psvbfptr->iminrowlength, pcrow);
			}
		}
	}
isrewcurr_exit:
	psvbfptr->iisdictlocked |= 0x02;
	iresult |= ivbexit (ihandle);
	return iresult;
}

int
isrewrec (int ihandle, vbisam_off_t trownumber, VB_CHAR *pcrow)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbfptr;
	int		ideleted, inewreclen, ioldreclen = 0, iresult = 0;

	if (ivbenter (ihandle, 1)) {
		return -1;
	}

	psvbfptr = vb_rtd->psvbfile[ihandle];
	if ((psvbfptr->iopenmode & ISVARLEN) && (vb_rtd->isreclen > psvbfptr->imaxrowlength
		|| vb_rtd->isreclen < psvbfptr->iminrowlength)) {
		ivbexit (ihandle);
		vb_rtd->iserrno = EBADARG;
		return -1;
	}

	inewreclen = vb_rtd->isreclen;
	if (trownumber < 1) {
		iresult = -1;
		vb_rtd->iserrno = ENOREC;
	} else {
		if (psvbfptr->iopenmode & ISTRANS) {
			vb_rtd->iserrno = ivbdatalock (ihandle, VBWRLOCK, trownumber);
			if (vb_rtd->iserrno) {
				iresult = -1;
				goto isrewrec_exit;
			}
		}
		vb_rtd->iserrno = ivbdataread (ihandle, (void *)psvbfptr->ppcrowbuffer, &ideleted,
				 trownumber);
		if (!vb_rtd->iserrno && ideleted) {
			vb_rtd->iserrno = ENOREC;
		}
		if (vb_rtd->iserrno) {
			iresult = -1;
		} else {
			ioldreclen = vb_rtd->isreclen;
		}
		if (!iresult) {
			iresult = irowupdate (ihandle, pcrow, trownumber);
		}
		if (!iresult) {
			vb_rtd->isrecnum = trownumber;
			vb_rtd->isreclen = inewreclen;
			iresult = ivbdatawrite (ihandle, (void *)pcrow, 0, (off_t)vb_rtd->isrecnum);
		}
		if (!iresult) {
			if (psvbfptr->iopenmode & ISVARLEN) {
				iresult = ivbtransupdate (ihandle, trownumber,
						ioldreclen, inewreclen,
						pcrow);
			} else {
				iresult = ivbtransupdate (ihandle, trownumber,
						psvbfptr->iminrowlength,
						psvbfptr->iminrowlength, pcrow);
			}
		}
	}
isrewrec_exit:
	if (!iresult) {
		psvbfptr->iisdictlocked |= 0x02;
	}
	iresult |= ivbexit (ihandle);
	return iresult;
}
