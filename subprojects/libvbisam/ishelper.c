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

#define NEED_VBINLINE_QUAD_LOAD 1
#define NEED_VBINLINE_QUAD_STORE 1
#include	"isinternal.h"

/* Global functions */

int
iscluster (int ihandle, struct keydesc *pskeydesc)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	/* BUG Write iscluster() and don't forget to call ivbtranscluster */
	if (ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle) {
		vb_rtd->iserrno = EBADARG;
		return -1;
	}
	return 0;
}

int
iserase (VB_CHAR *pcfilename)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	int	ihandle;
	VB_CHAR	cbuffer[1024];

	for (ihandle = 0; ihandle <= vb_rtd->ivbmaxusedhandle; ihandle++) {
		if (vb_rtd->psvbfile[ihandle] != NULL) {
			if (!strcmp ((char*)vb_rtd->psvbfile[ihandle]->cfilename, (char*)pcfilename)) {
				isclose (ihandle);
				ivbclose3 (ihandle);
				break;
			}
		}
	}
	sprintf ((char*)cbuffer, "%s.idx", pcfilename);
	unlink ((char*)cbuffer);
	sprintf ((char*)cbuffer, "%s.dat", pcfilename);
	unlink ((char*)cbuffer);
	return ivbtranserase (pcfilename);
}

int
isflush (int ihandle)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;

	if (ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle) {
		vb_rtd->iserrno = EBADARG;
		return -1;
	}
	psvbptr = vb_rtd->psvbfile[ihandle];
	if (!psvbptr || psvbptr->iisopen) {
		vb_rtd->iserrno = ENOTOPEN;
		return -1;
	}
	if (psvbptr->iindexhandle >= 0) {
		fsync (psvbptr->iindexhandle);
	}
	if (psvbptr->idatahandle >= 0) {
		fsync (psvbptr->idatahandle);
	}
	return 0;
}

int
islock (int ihandle)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;

	if (ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle) {
		vb_rtd->iserrno = EBADARG;
		return -1;
	}
	psvbptr = vb_rtd->psvbfile[ihandle];
	if (!psvbptr || psvbptr->iisopen) {
		vb_rtd->iserrno = ENOTOPEN;
		return -1;
	}
	return ivbdatalock (ihandle, VBWRLOCK, (off_t)0);
}

int
isrelcurr (int ihandle)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;

	if (ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle) {
		vb_rtd->iserrno = EBADARG;
		return -1;
	}
	psvbptr = vb_rtd->psvbfile[ihandle];
	if (!psvbptr || psvbptr->iisopen) {
		vb_rtd->iserrno = ENOTOPEN;
		return -1;
	}
	if (vb_rtd->ivbintrans != VBNOTRANS) {
		return 0;
	}
	if (!psvbptr->trownumber) {
		vb_rtd->iserrno = ENOREC;
		return -1;
	}
	vb_rtd->iserrno = ivbdatalock (ihandle, VBUNLOCK, psvbptr->trownumber);
	if (vb_rtd->iserrno) {
		return -1;
	}
	return 0;
}

int
isrelease (int ihandle)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;

	if (ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle) {
		vb_rtd->iserrno = EBADARG;
		return -1;
	}
	psvbptr = vb_rtd->psvbfile[ihandle];
	if (!psvbptr || psvbptr->iisopen) {
		vb_rtd->iserrno = ENOTOPEN;
		return -1;
	}
	if (vb_rtd->ivbintrans != VBNOTRANS) {
		return 0;
	}
	ivbdatalock (ihandle, VBUNLOCK, (off_t)0);	/* Ignore the return */
	return 0;
}

int
isrelrec (int ihandle, vbisam_off_t trownumber)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;

	if (ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle) {
		vb_rtd->iserrno = EBADARG;
		return -1;
	}
	psvbptr = vb_rtd->psvbfile[ihandle];
	if (!psvbptr || psvbptr->iisopen) {
		vb_rtd->iserrno = ENOTOPEN;
		return -1;
	}
	vb_rtd->iserrno = ivbdatalock (ihandle, VBUNLOCK, trownumber);
	if (vb_rtd->iserrno) {
		return -1;
	}
	return 0;
}

int
isrename (VB_CHAR *pcoldname, VB_CHAR *pcnewname)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	int	iresult;
	VB_CHAR	cbuffer[2][1024];

	sprintf ((char*)cbuffer[0], "%s.idx", pcoldname);
	sprintf ((char*)cbuffer[1], "%s.idx", pcnewname);
	iresult = rename ((char*)cbuffer[0], (char*)cbuffer[1]);
	if (iresult == -1) {
		goto renameexit;
	}
	sprintf ((char*)cbuffer[0], "%s.dat", pcoldname);
	sprintf ((char*)cbuffer[1], "%s.dat", pcnewname);
	iresult = rename ((char*)cbuffer[0], (char*)cbuffer[1]);
	if (iresult == -1) {
		sprintf ((char*)cbuffer[0], "%s.idx", pcoldname);
		sprintf ((char*)cbuffer[1], "%s.idx", pcnewname);
		rename ((char*)cbuffer[1], (char*)cbuffer[0]);
		goto renameexit;
	}
	return ivbtransrename (pcoldname, pcnewname);
renameexit:
	vb_rtd->iserrno = errno;
	return -1;
}

int
issetunique (int ihandle, vbisam_off_t tuniqueid)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO *tvbptr;
	off_t		tvalue;
	int		iresult, iresult2;

	if (ivbenter (ihandle, 1)) {
		return -1;
	}
	tvbptr = vb_rtd->psvbfile[ihandle];
	vb_rtd->iserrno = EBADARG;
	if (!tvbptr->iisdictlocked) {
		/*CIT*/ivbexit (ihandle);
		return -1;
	}

	vb_rtd->iserrno = 0;
	tvalue = inl_ldquad (tvbptr->sdictnode.cuniqueid);
	if (tuniqueid > tvalue) {
		inl_stquad (tuniqueid, tvbptr->sdictnode.cuniqueid);
		tvbptr->iisdictlocked |= 0x02;
	}

	iresult = ivbtranssetunique (ihandle, tuniqueid);
	tvbptr->iisdictlocked |= 0x02;
	iresult2 = ivbexit (ihandle);
	if (iresult) {
		return -1;
	}
	return iresult2;
}

int
isuniqueid (int ihandle, vbisam_off_t *ptuniqueid)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO *tvbptr;
	off_t		tvalue;
	int		iresult = 0;
	int		iresult2;

	if (ivbenter (ihandle, 1)) {
		return -1;
	}

	tvbptr = vb_rtd->psvbfile[ihandle];
	vb_rtd->iserrno = EBADARG;
	if (!tvbptr->iisdictlocked) {
		/*CIT*/ivbexit (ihandle);
		return -1;
	}
	vb_rtd->iserrno = 0;

	tvalue = inl_ldquad (tvbptr->sdictnode.cuniqueid);
	inl_stquad (tvalue + 1, tvbptr->sdictnode.cuniqueid);
	tvbptr->iisdictlocked |= 0x02;
	iresult = ivbtransuniqueid (ihandle, tvalue);
	iresult2 = ivbexit (ihandle);
	if (iresult) {
		return -1;
	}
	*ptuniqueid = tvalue;
	return iresult2;
}

int
isunlock (int ihandle)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;

	if (unlikely(ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle)) {
		vb_rtd->iserrno = EBADARG;
		return -1;
	}
	psvbptr = vb_rtd->psvbfile[ihandle];
	if (!psvbptr || psvbptr->iisopen) {
		vb_rtd->iserrno = ENOTOPEN;
		return -1;
	}
	return ivbdatalock (ihandle, VBUNLOCK, (off_t)0);
}

void
ldchar (VB_CHAR *pcsource, int ilength, VB_CHAR *pcdestination)
{
	VB_CHAR *pcdst;

	memcpy ((void *)pcdestination, (void *)pcsource, (size_t)ilength);
	for (pcdst = pcdestination + ilength - 1; pcdst >= (VB_CHAR *)pcdestination; pcdst--) {
		if (*pcdst != ' ') {
			pcdst++;
			*pcdst = 0;
			return;
		}
	}
	*(++pcdst) = 0;
}

void
stchar (VB_CHAR *pcsource, VB_CHAR *pcdestination, int ilength)
{
	VB_CHAR *pcsrc, *pcdst;
	int icount;

	pcsrc = pcsource;
	pcdst = pcdestination;
	for (icount = ilength; icount && *pcsrc; icount--, pcsrc++, pcdst++) {
		*pcdst = *pcsrc;
	}
	for (; icount; icount--, pcdst++) {
		*pcdst = ' ';
	}
}

int
ldint (void *pclocation)
{
#ifndef	WORDS_BIGENDIAN
	return (int)VB_BSWAP_16 (*(unsigned short *)pclocation);
#else
	short ivalue = 0;
	VB_UCHAR *pctemp = (VB_UCHAR *)&ivalue;
	VB_UCHAR *pctemp2 = (VB_UCHAR *)pclocation;

	*(pctemp + 0) = *(pctemp2 + 0);
	*(pctemp + 1) = *(pctemp2 + 1);
	return (int)ivalue;
#endif
}

void
stint (int ivalue, void *pclocation)
{
#ifndef	WORDS_BIGENDIAN
	*(unsigned short *)pclocation = VB_BSWAP_16 ((unsigned short)ivalue);
#else
	VB_UCHAR *pctemp = (VB_UCHAR *)&ivalue;

	*((VB_UCHAR *)pclocation + 0) = *(pctemp + 0 + INTSIZE);
	*((VB_UCHAR *)pclocation + 1) = *(pctemp + 1 + INTSIZE);
#endif
}

int
ldlong (void *pclocation)
{
#ifndef	WORDS_BIGENDIAN
	return VB_BSWAP_32 (*(unsigned int *)pclocation);
#else
	int lvalue;

	memcpy ((VB_UCHAR *)&lvalue, (VB_UCHAR *)pclocation, 4);
	return lvalue;
#endif
}

void
stlong (int lvalue, void *pclocation)
{
#ifndef	WORDS_BIGENDIAN
	*(unsigned int *)pclocation = VB_BSWAP_32 ((unsigned int)lvalue);
#else
	memcpy ((VB_UCHAR *)pclocation, (VB_UCHAR *)&lvalue, 4);
#endif
}

/* RXW
off_t
ldquad (void *pclocation)
{

#ifndef	WORDS_BIGENDIAN
#if	ISAMMODE == 1
	return VB_BSWAP_64 (*(unsigned long long *)pclocation);
#else
	return VB_BSWAP_32 (*(unsigned int *)pclocation);
#endif
#else
	off_t tvalue;
#if	ISAMMODE == 1
	memcpy ((VB_UCHAR *)&tvalue, (VB_UCHAR *)pclocation, 8);
#else
	memcpy ((VB_UCHAR *)&tvalue, (VB_UCHAR *)pclocation, 4);
#endif
	return tvalue;
#endif
}

void
stquad (off_t tvalue, void *pclocation)
{

#ifndef	WORDS_BIGENDIAN
#if	ISAMMODE == 1
	*(unsigned long long *)pclocation = VB_BSWAP_64 ((unsigned long long)tvalue);
#else
	*(unsigned int *)pclocation = VB_BSWAP_32 ((unsigned int)tvalue);
#endif
#else
#if	ISAMMODE == 1
	memcpy ((VB_UCHAR *)pclocation, (VB_UCHAR *)&tvalue, 8);
#else
	memcpy ((VB_UCHAR *)pclocation, (VB_UCHAR *)&tvalue, 4);
#endif
#endif
}
*/

double
ldfloat (void *pclocation)
{
	float ffloat;
	double ddouble;

	memcpy (&ffloat, pclocation, FLOATSIZE);
	ddouble = ffloat;
	return (double)ddouble;
}

void
stfloat (double dsource, void *pcdestination)
{
	float ffloat;

	ffloat = dsource;
	memcpy (pcdestination, &ffloat, FLOATSIZE);
}

double
ldfltnull (void *pclocation, short *pinullflag)
{
	double dvalue;

	*pinullflag = 0;
	dvalue = ldfloat (pclocation);
	return (double)dvalue;
}

void
stfltnull (double dsource, void *pcdestination, int inullflag)
{
	if (inullflag) {
		dsource = 0;
	}
	stfloat (dsource, pcdestination);
}

double
lddbl (void *pclocation)
{
	double ddouble;

	memcpy (&ddouble, pclocation, DOUBLESIZE);
	return ddouble;
}

void
stdbl (double dsource, void *pcdestination)
{
	memcpy (pcdestination, &dsource, DOUBLESIZE);
}

double
lddblnull (void *pclocation, short *pinullflag)
{
	*pinullflag = 0;
	return (lddbl (pclocation));
}

void
stdblnull (double dsource, void *pcdestination, int inullflag)
{
	if (inullflag) {
		dsource = 0;
	}
	stdbl (dsource, pcdestination);
}
