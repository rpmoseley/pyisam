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
#include	"isinternal.h"

struct RCV_HDL {
    struct RCV_HDL  *psnext;
    struct RCV_HDL  *psprev;
    int     ipid;
    int     ihandle;
    int     itranslock;
};

struct STRANS {
    struct STRANS   *psnext;
    struct STRANS   *psprev;
    int     ipid;
    int     iignoreme;  /* If 1, isrecover will NOT process this one */
};

static int              ivbrecvmode = RECOV_C;      /* Sets isrecover mode */
static struct   STRANS  *pstranshead = NULL;
static struct   SLOGHDR *psvblogheader;
static struct   RCV_HDL *psrecoverhandle[VB_MAX_FILES + 1];
static VB_CHAR          *clclbuffer = NULL;
static VB_CHAR          *cvbrtransbuffer = NULL;

/* Local functions */

static int
ircvrollback (void)
{
    struct STRANS   *pstrans = pstranshead;
    int     ipid;

    ipid = inl_ldint (psvblogheader->cpid);
    while (pstrans && pstrans->ipid != ipid) {
        pstrans = pstrans->psnext;
    }
    if (!pstrans) {
        return EBADLOG;
    }
    if (pstrans->psnext) {
        pstrans->psnext->psprev = pstrans->psprev;
    }
    if (pstrans->psprev) {
        pstrans->psprev->psnext = pstrans->psnext;
    } else {
        pstranshead = pstrans->psnext;
    }
    vvbfree (pstrans, sizeof (struct STRANS));
    return 0;
}

static int
irollbackall (void)
{
    while (pstranshead) {
        inl_stint (pstranshead->ipid, psvblogheader->cpid);
        ircvrollback ();
    }
    return 0;
}

static void
vcloseall (void)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbptr;
    int     iloop;

    for (iloop = 0; iloop <= VB_MAX_FILES; iloop++) {
        psvbptr = vb_rtd->psvbfile[iloop];
        if (psvbptr && psvbptr->iisopen == 0) {
            isclose (iloop);
        }
    }
}

static int
igetrcvhandle (const int ihandle, const int ipid)
{
    struct RCV_HDL *psrcvhdl = psrecoverhandle[ihandle];

    while (psrcvhdl && psrcvhdl->ipid != ipid) {
        psrcvhdl = psrcvhdl->psnext;
    }
    if (psrcvhdl && psrcvhdl->ipid == ipid) {
        return psrcvhdl->ihandle;
    }
    return -1;
}

static int
ircvchecktrans (const int ilength, const int ipid)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    off_t   tlength;
    off_t   tlength2;
    off_t   trcvsaveoffset;
    int ifound = 0, iresult = 0;

    tlength = ilength;
    trcvsaveoffset = tvblseek (vb_rtd->ivblogfilehandle, (off_t)0, SEEK_CUR);
    if (!clclbuffer) {
        clclbuffer = pvvbmalloc (MAX_BUFFER_LENGTH);
    }
    psvblogheader = (struct SLOGHDR *)(clclbuffer - INTSIZE);
    while (!ifound) {
        tlength2 = tvbread (vb_rtd->ivblogfilehandle, clclbuffer, (size_t) tlength);
        if (tlength2 != tlength && tlength2 != tlength - INTSIZE) {
            break;
        }
        if (inl_ldint (psvblogheader->cpid) == ipid) {
            if (!memcmp (psvblogheader->coperation, VBL_COMMIT, 2)) {
                ifound = 1;
            } else if (!memcmp (psvblogheader->coperation, VBL_BEGIN, 2)) {
                ifound = 1;
                if (ivbrecvmode & RECOV_VB) {
                    iresult = 1;
                }
            } else if (!memcmp (psvblogheader->coperation, VBL_ROLLBACK, 2)) {
                ifound = 1;
                iresult = 1;
            }
        }
        if (tlength2 == tlength - INTSIZE) {
            break;
        }
        tlength = inl_ldint (clclbuffer + tlength - INTSIZE);
    }

    psvblogheader = (struct SLOGHDR *)(cvbrtransbuffer - INTSIZE);
    tvblseek (vb_rtd->ivblogfilehandle, trcvsaveoffset, SEEK_SET);
    return iresult;
}

static int
iignore (const int ipid)
{
    struct STRANS *pstrans = pstranshead;

    while (pstrans && pstrans->ipid != ipid) {
        pstrans = pstrans->psnext;
    }
    if (pstrans && pstrans->ipid == ipid) {
        return pstrans->iignoreme;
    }
    return 0;
}

static int
ircvbuild (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    VB_CHAR     *pcfilename;
    int     ihandle, iloop, imode, imaxrowlen, ipid;
    struct keydesc  skeydesc;

    imode = inl_ldint (pcbuffer);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    vb_rtd->isreclen = inl_ldint (pcbuffer + INTSIZE);
    imaxrowlen = inl_ldint (pcbuffer + (INTSIZE * 2));
    skeydesc.k_flags = inl_ldint (pcbuffer + (INTSIZE * 3));
    skeydesc.k_nparts = inl_ldint (pcbuffer + (INTSIZE * 4));
    pcbuffer += (INTSIZE * 6);
    for (iloop = 0; iloop < skeydesc.k_nparts; iloop++) {
        skeydesc.k_part[iloop].kp_start = inl_ldint (pcbuffer + (iloop * 3 * INTSIZE));
        skeydesc.k_part[iloop].kp_leng =
        inl_ldint (pcbuffer + INTSIZE + (iloop * 3 * INTSIZE));
        skeydesc.k_part[iloop].kp_type =
        inl_ldint (pcbuffer + (INTSIZE * 2) + +(iloop * 3 * INTSIZE));
    }
    pcfilename = pcbuffer + (skeydesc.k_nparts * 3 * INTSIZE);
    ihandle = isbuild (pcfilename, imaxrowlen, &skeydesc, imode);
    imode = vb_rtd->iserrno;    /* Save any error result */
    if (ihandle != -1) {
        isclose (ihandle);
    }
    return imode;
}

static int
ircvbegin (VB_CHAR *pcbuffer, const int ilength)
{
    struct STRANS   *pstrans = pstranshead;
    int     ipid;

    ipid = inl_ldint (psvblogheader->cpid);
    while (pstrans && pstrans->ipid != ipid) {
        pstrans = pstrans->psnext;
    }
    if (pstrans) {
        ircvrollback ();
    }
    pstrans = pvvbmalloc (sizeof (struct STRANS));
    if (!pstrans) {
        return ENOMEM;
    }
    pstrans->psprev = NULL;
    pstrans->psnext = pstranshead;
    if (pstranshead) {
        pstranshead->psprev = pstrans;
    }
    pstranshead = pstrans;
    pstrans->ipid = ipid;
    pstrans->iignoreme = ircvchecktrans (ilength, ipid);
    return 0;
}

static int
ircvcreateindex (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    int     ihandle, iloop, ipid, isaveerror = 0;
    struct keydesc  skeydesc;

    ihandle = inl_ldint (pcbuffer);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    ihandle = igetrcvhandle (ihandle, ipid);
    if (ihandle == -1) {
        return ENOTOPEN;
    }
    skeydesc.k_flags = inl_ldint (pcbuffer + INTSIZE);
    skeydesc.k_nparts = inl_ldint (pcbuffer + (INTSIZE * 2));
    pcbuffer += (INTSIZE * 4);
    for (iloop = 0; iloop < skeydesc.k_nparts; iloop++) {
        skeydesc.k_part[iloop].kp_start = inl_ldint (pcbuffer + (iloop * 3 * INTSIZE));
        skeydesc.k_part[iloop].kp_leng =
        inl_ldint (pcbuffer + INTSIZE + (iloop * 3 * INTSIZE));
        skeydesc.k_part[iloop].kp_type =
        inl_ldint (pcbuffer + (INTSIZE * 2) + +(iloop * 3 * INTSIZE));
    }
    /* Promote the file open lock to EXCLUSIVE */
    vb_rtd->iserrno = ivbfileopenlock (ihandle, 2);
    if (vb_rtd->iserrno) {
        return vb_rtd->iserrno;
    }
    vb_rtd->psvbfile[ihandle]->iopenmode |= ISEXCLLOCK;
    if (isaddindex (ihandle, &skeydesc)) {
        isaveerror = vb_rtd->iserrno;
    }
    /* Demote the file open lock back to SHARED */
    vb_rtd->psvbfile[ihandle]->iopenmode &= ~ISEXCLLOCK;
    ivbfileopenlock (ihandle, 0);
    ivbfileopenlock (ihandle, 1);
    return isaveerror;
}

static int
ircvcluster (VB_CHAR *pcbuffer)
{
    int ipid;

    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
/* RXW
    fprintf (stderr, "ircvcluster not yet implemented!\n");
*/
    return 0;
}

static int
ircvcommit (VB_CHAR *pcbuffer)
{
    struct RCV_HDL  *psrcv;
    struct STRANS   *pstrans = pstranshead;
    int     iloop, ipid;

    ipid = inl_ldint (psvblogheader->cpid);
    while (pstrans && pstrans->ipid != ipid) {
        pstrans = pstrans->psnext;
    }
    if (!pstrans) {
        return EBADLOG;
    }
    if (pstrans->psnext) {
        pstrans->psnext->psprev = pstrans->psprev;
    }
    if (pstrans->psprev) {
        pstrans->psprev->psnext = pstrans->psnext;
    } else {
        pstranshead = pstrans->psnext;
    }
    vvbfree (pstrans, sizeof (struct STRANS));
    for (iloop = 0; iloop <= VB_MAX_FILES; iloop++) {
        if (!psrecoverhandle[iloop]) {
            continue;
        }
        for (psrcv = psrecoverhandle[iloop]; psrcv; psrcv = psrcv->psnext) {
            if (psrcv->itranslock) {
                isrelease (psrcv->ihandle);
            }
        }
    }
    return 0;
}

static int
ircvdelete (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    VB_CHAR *pcrow;
    off_t   trownumber;
    int ihandle, ipid;

    ihandle = inl_ldint (pcbuffer);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    ihandle = igetrcvhandle (ihandle, ipid);
    if (ihandle == -1) {
        return ENOTOPEN;
    }
    trownumber = inl_ldquad (pcbuffer + INTSIZE);
    pcrow = pcbuffer + INTSIZE + QUADSIZE + INTSIZE;
    isdelrec (ihandle, trownumber);
    return vb_rtd->iserrno;
}

static int
ircvdeleteindex (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    int     ihandle, iloop, ipid, isaveerror = 0;
    struct keydesc  skeydesc;

    ihandle = inl_ldint (pcbuffer);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    ihandle = igetrcvhandle (ihandle, ipid);
    if (ihandle == -1) {
        return ENOTOPEN;
    }
    skeydesc.k_flags = inl_ldint (pcbuffer + INTSIZE);
    skeydesc.k_nparts = inl_ldint (pcbuffer + (INTSIZE * 2));
    pcbuffer += (INTSIZE * 4);
    for (iloop = 0; iloop < skeydesc.k_nparts; iloop++) {
        skeydesc.k_part[iloop].kp_start = inl_ldint (pcbuffer + (iloop * 3 * INTSIZE));
        skeydesc.k_part[iloop].kp_leng =
        inl_ldint (pcbuffer + INTSIZE + (iloop * 3 * INTSIZE));
        skeydesc.k_part[iloop].kp_type =
        inl_ldint (pcbuffer + (INTSIZE * 2) + +(iloop * 3 * INTSIZE));
    }
    /* Promote the file open lock to EXCLUSIVE */
    vb_rtd->iserrno = ivbfileopenlock (ihandle, 2);
    if (vb_rtd->iserrno) {
        return vb_rtd->iserrno;
    }
    vb_rtd->psvbfile[ihandle]->iopenmode |= ISEXCLLOCK;
    if (isdelindex (ihandle, &skeydesc)) {
        isaveerror = vb_rtd->iserrno;
    }
    /* Demote the file open lock back to SHARED */
    vb_rtd->psvbfile[ihandle]->iopenmode &= ~ISEXCLLOCK;
    ivbfileopenlock (ihandle, 0);
    ivbfileopenlock (ihandle, 1);
    return isaveerror;
}

static int
ircvfileerase (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    int ipid;

    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    iserase (pcbuffer);
    return vb_rtd->iserrno;
}

static int
ircvfileclose (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct RCV_HDL  *psrcv;
    int     ihandle, ivarlenflag, ipid;

    ihandle = inl_ldint (pcbuffer);
    ivarlenflag = inl_ldint (pcbuffer + INTSIZE);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    psrcv = psrecoverhandle[ihandle];
    while (psrcv && psrcv->ipid != ipid) {
        psrcv = psrcv->psnext;
    }
    if (!psrcv || psrcv->ipid != ipid) {
        return ENOTOPEN;    /* It wasn't open! */
    }
    vb_rtd->iserrno = 0;
    isclose (psrcv->ihandle);
    if (psrcv->psprev) {
        psrcv->psprev->psnext = psrcv->psnext;
    } else {
        psrecoverhandle[ihandle] = psrcv->psnext;
    }
    if (psrcv->psnext) {
        psrcv->psnext->psprev = psrcv->psprev;
    }

    return vb_rtd->iserrno;
}

static int
ircvfileopen (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct RCV_HDL  *psrcv;
    int     ihandle, ivarlenflag, ipid;

    ihandle = inl_ldint (pcbuffer);
    ivarlenflag = inl_ldint (pcbuffer + INTSIZE);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    if (igetrcvhandle (ihandle, ipid) != -1) {
        return ENOTOPEN;    /* It was already open! */
    }
    psrcv = pvvbmalloc (sizeof (struct RCV_HDL));
    if (psrcv == NULL) {
        return ENOMEM;  /* Oops */
    }
    psrcv->ihandle = isopen (pcbuffer + INTSIZE + INTSIZE,
                             ISINOUT | ISMANULOCK | (ivarlenflag ? ISVARLEN : ISFIXLEN));
    psrcv->ipid = ipid;
    if (psrcv->ihandle < 0) {
        vvbfree (psrcv, sizeof (struct RCV_HDL));
        return vb_rtd->iserrno;
    }
    psrcv->psprev = NULL;
    psrcv->psnext = psrecoverhandle[ihandle];
    if (psrecoverhandle[ihandle]) {
        psrecoverhandle[ihandle]->psprev = psrcv;
    }
    psrecoverhandle[ihandle] = psrcv;
    return 0;
}

static int
ircvinsert (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    VB_CHAR *pcrow;
    off_t   trownumber;
    int ihandle, ipid;
    int res = 0;

    ihandle = inl_ldint (pcbuffer);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    ihandle = igetrcvhandle (ihandle, ipid);
    if (ihandle == -1) {
        return ENOTOPEN;
    }
    trownumber = inl_ldquad (pcbuffer + INTSIZE);
    vb_rtd->isreclen = inl_ldint (pcbuffer + INTSIZE + QUADSIZE);
    pcrow = pcbuffer + INTSIZE + QUADSIZE + INTSIZE;
    if (ivbenter (ihandle, 1)) {
        return vb_rtd->iserrno;
    }
    vb_rtd->psvbfile[ihandle]->iisdictlocked |= 0x02;
    if (ivbforcedataallocate (ihandle, trownumber)) {
        res= EDUPL;
        goto ircvinsert_exit;
    }
    if (ivbwriterow (ihandle, pcrow, trownumber)) {
        res = vb_rtd->iserrno;
        goto ircvinsert_exit;
    }
    ivbexit (ihandle);
    return vb_rtd->iserrno;

ircvinsert_exit:
    ivbexit (ihandle);
    return res;
}

/* NEEDS TESTING! */
static int
ircvfilerename (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    VB_CHAR *pcoldname, *pcnewname;
    int ioldnamelength, ipid;

    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    ioldnamelength = inl_ldint (pcbuffer);
    pcoldname = pcbuffer + INTSIZE + INTSIZE;
    pcnewname = pcbuffer + INTSIZE + INTSIZE + ioldnamelength;
    isrename (pcoldname, pcnewname);
    return vb_rtd->iserrno;
}

static int
ircvsetunique (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    off_t   tuniqueid;
    int ihandle, ipid;

    ihandle = inl_ldint (pcbuffer);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    ihandle = igetrcvhandle (ihandle, ipid);
    if (ihandle == -1) {
        return ENOTOPEN;
    }
    tuniqueid = inl_ldquad (pcbuffer + INTSIZE);
    issetunique (ihandle, tuniqueid);
    return vb_rtd->iserrno;
}

static int
ircvuniqueid (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    off_t   tuniqueid;
    int ihandle, ipid;

    ihandle = inl_ldint (pcbuffer);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    ihandle = igetrcvhandle (ihandle, ipid);
    if (ihandle == -1) {
        return ENOTOPEN;
    }
    tuniqueid = inl_ldquad (pcbuffer + INTSIZE);
    if (tuniqueid != inl_ldquad (vb_rtd->psvbfile[ihandle]->sdictnode.cuniqueid)) {
        return EBADFILE;
    }
    isuniqueid (ihandle, &tuniqueid);
    return 0;
}

static int
ircvupdate (VB_CHAR *pcbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    VB_CHAR *pcrow;
    off_t   trownumber;
    int ihandle, ipid;

    ihandle = inl_ldint (pcbuffer);
    ipid = inl_ldint (psvblogheader->cpid);
    if (iignore (ipid)) {
        return 0;
    }
    ihandle = igetrcvhandle (ihandle, ipid);
    if (ihandle == -1) {
        return ENOTOPEN;
    }
    trownumber = inl_ldquad (pcbuffer + INTSIZE);
    vb_rtd->isreclen = inl_ldint (pcbuffer + INTSIZE + QUADSIZE);
    pcrow = pcbuffer + INTSIZE + QUADSIZE + INTSIZE + INTSIZE + vb_rtd->isreclen;
    vb_rtd->isreclen = inl_ldint (pcbuffer + INTSIZE + QUADSIZE + INTSIZE);
    isrewrec (ihandle, trownumber, pcrow);
    return vb_rtd->iserrno;
}

/* Global functions */

int
isrecover ()
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    VB_CHAR *pcbuffer;
    off_t   tlength, tlength2, toffset;
    int iloop, isaveerror;

    /* Initialize by stating that *ALL* tables must be closed! */
    for (iloop = 0; iloop <= VB_MAX_FILES; iloop++) {
        if (vb_rtd->psvbfile[iloop]) {
            vb_rtd->iserrno = ETOOMANY;
            return -1;
        }
    }
    vb_rtd->ivbintrans = VBRECOVER;
    for (iloop = 0; iloop <= VB_MAX_FILES; iloop++) {
        psrecoverhandle[iloop] = NULL;
    }
    /* Begin by reading the header of the first transaction */
    vb_rtd->iserrno = EBADFILE;
    if (tvblseek (vb_rtd->ivblogfilehandle, (off_t)0, SEEK_SET) != 0) {
        return -1;
    }
    cvbrtransbuffer = pvvbmalloc (MAX_BUFFER_LENGTH);
    if (!cvbrtransbuffer) {
        vb_rtd->iserrno = EBADMEM;
        return -1;
    }
    psvblogheader = (struct SLOGHDR *)(cvbrtransbuffer - INTSIZE);
    if (tvbread (vb_rtd->ivblogfilehandle, cvbrtransbuffer, INTSIZE) != INTSIZE) {
        return 0;   /* Nothing to do if the file is empty */
    }
    toffset = 0;
    tlength = inl_ldint (cvbrtransbuffer);
    /* Now, recurse forwards */
    while (1) {
        tlength2 = tvbread (vb_rtd->ivblogfilehandle, cvbrtransbuffer, tlength);
        vb_rtd->iserrno = EBADFILE;
        if (tlength2 != tlength && tlength2 != tlength - INTSIZE) {
            break;
        }
        pcbuffer = cvbrtransbuffer - INTSIZE + sizeof (struct SLOGHDR);
#ifdef	VBDEBUG
#if	ISAMMODE == 1
        printf ("Offset:%08llx PID:%d Type: %-2.2s Row:%08llx\n", toffset,
                inl_ldint (psvblogheader->cpid), psvblogheader->coperation,
                inl_ldquad (pcbuffer + INTSIZE));
#else
        printf ("Offset:%08x PID:%d Type: %-2.2s Row:%08x\n", (unsigned int)toffset,
                (unsigned int)inl_ldint (psvblogheader->cpid), psvblogheader->coperation,
                (unsigned int)inl_ldquad (pcbuffer + INTSIZE));
#endif
        fflush (stdout);
#endif	/* VBDEBUG */
        if (!memcmp (psvblogheader->coperation, VBL_BUILD, 2)) {
            vb_rtd->iserrno = ircvbuild (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_BEGIN, 2)) {
            vb_rtd->iserrno = ircvbegin (pcbuffer,
                                         inl_ldint (cvbrtransbuffer + tlength - INTSIZE));
        } else if (!memcmp (psvblogheader->coperation, VBL_CREINDEX, 2)) {
            vb_rtd->iserrno = ircvcreateindex (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_CLUSTER, 2)) {
            vb_rtd->iserrno = ircvcluster (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_COMMIT, 2)) {
            vb_rtd->iserrno = ircvcommit (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_DELETE, 2)) {
            vb_rtd->iserrno = ircvdelete (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_DELINDEX, 2)) {
            vb_rtd->iserrno = ircvdeleteindex (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_FILEERASE, 2)) {
            vb_rtd->iserrno = ircvfileerase (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_FILECLOSE, 2)) {
            vb_rtd->iserrno = ircvfileclose (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_FILEOPEN, 2)) {
            vb_rtd->iserrno = ircvfileopen (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_INSERT, 2)) {
            vb_rtd->iserrno = ircvinsert (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_RENAME, 2)) {
            vb_rtd->iserrno = ircvfilerename (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_ROLLBACK, 2)) {
            vb_rtd->iserrno = ircvrollback ();
        } else if (!memcmp (psvblogheader->coperation, VBL_SETUNIQUE, 2)) {
            vb_rtd->iserrno = ircvsetunique (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_UNIQUEID, 2)) {
            vb_rtd->iserrno = ircvuniqueid (pcbuffer);
        } else if (!memcmp (psvblogheader->coperation, VBL_UPDATE, 2)) {
            vb_rtd->iserrno = ircvupdate (pcbuffer);
        }
        if (vb_rtd->iserrno) {
            break;
        }
        toffset += tlength2;
        if (tlength2 == tlength - INTSIZE) {
            break;
        }
        tlength = inl_ldint (cvbrtransbuffer + tlength - INTSIZE);
    }
    isaveerror = vb_rtd->iserrno;
    /* We now rollback any transactions that were never 'closed' */
    vb_rtd->iserrno = irollbackall ();
    if (!isaveerror) {
        isaveerror = vb_rtd->iserrno;
    }
    vcloseall ();
    vb_rtd->iserrno = isaveerror;
    vvbfree (cvbrtransbuffer, MAX_BUFFER_LENGTH);
    if (vb_rtd->iserrno) {
        return -1;
    }
    return 0;
}
