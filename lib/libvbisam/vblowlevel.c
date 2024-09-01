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

#ifdef	VBDEBUG
    #include	<assert.h>
#endif

/* HP UX need use of F_SETLK64*/
#if HAVE_STRUCT_FLOCK64
    #define VB_F_SETLK      F_SETLK64
    #define VB_F_GETLK      F_GETLK64
    #define VB_F_SETLKW     F_SETLKW64
    #define VB_flock        flock64
#else
    #define VB_F_SETLK      F_SETLK
    #define VB_F_GETLK      F_GETLK
    #define VB_F_SETLKW     F_SETLK
    #define VB_flock        flock
#endif

#if defined(O_LARGEFILE)
    #define VB_OPEN_FLAGS | O_LARGEFILE
#else
    #define VB_OPEN_FLAGS
#endif

/* Activate to define LockFileEx locking */
#define	USE_LOCKFILE_EX

int
ivbopen (VB_CHAR *pcfilename, const int iflags, const mode_t tmode)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    int     iloop;
    struct stat sstat;

    if ( !vb_rtd->lowiinitialized ) {
        memset ((void *)&vb_rtd->svbfile[0], 0, sizeof (vb_rtd->svbfile));
        vb_rtd->lowiinitialized = 1;
    }
    if ( stat ((char*)pcfilename, &sstat) ) {
        if ( !(iflags & O_CREAT) ) {
            return -1;
        }
    } else {
        for ( iloop = 0; iloop < VB_MAX_FILES * 3; iloop++ ) {
            if ( vb_rtd->svbfile[iloop].irefcount
                 && vb_rtd->svbfile[iloop].tdevice == (long)sstat.st_dev
#ifdef	_WIN32
                 && !strcmp(vb_rtd->svbfile[iloop].cfilename, pcfilename) ) {
#else
                 && vb_rtd->svbfile[iloop].tinode == (long)sstat.st_ino ) {
#endif
                vb_rtd->svbfile[iloop].irefcount++;
                return iloop;
            }
        }
    }
    for ( iloop = 0; iloop < VB_MAX_FILES * 3; iloop++ ) {
        if ( vb_rtd->svbfile[iloop].irefcount == 0 ) {
            vb_rtd->svbfile[iloop].ihandle = open ((char*)pcfilename, iflags | O_BINARY VB_OPEN_FLAGS, tmode);
            if ( vb_rtd->svbfile[iloop].ihandle == -1 ) {
                break;
            }
            if ( (iflags & O_CREAT) && stat ((char*)pcfilename, &sstat) ) {
                close (vb_rtd->svbfile[iloop].ihandle);
                return -1;
            }
#ifdef	_WIN32
            vb_rtd->svbfile[iloop].tinode = 0;
            if ( vb_rtd->svbfile[iloop].cfilename ) {
                free (vb_rtd->svbfile[iloop].cfilename);
            }
            vb_rtd->svbfile[iloop].whandle = (HANDLE)_get_osfhandle (vb_rtd->svbfile[iloop].ihandle);
            if ( vb_rtd->svbfile[iloop].whandle == INVALID_HANDLE_VALUE ) {
                close (vb_rtd->svbfile[iloop].ihandle);
                return -1;
            }
            vb_rtd->svbfile[iloop].cfilename = strdup (pcfilename);
#else
            vb_rtd->svbfile[iloop].tinode = (long)sstat.st_ino;
#endif
            vb_rtd->svbfile[iloop].tdevice = (long)sstat.st_dev;
            vb_rtd->svbfile[iloop].irefcount++;
            return iloop;
        }
    }
    errno = ENOENT;
    return -1;
}

int
ivbclose (const int ihandle)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    if ( !vb_rtd->svbfile[ihandle].irefcount ) {
        errno = ENOENT;
        return -1;
    }
    vb_rtd->svbfile[ihandle].irefcount--;
    if ( !vb_rtd->svbfile[ihandle].irefcount ) {
        return close (vb_rtd->svbfile[ihandle].ihandle);
    }
    return 0;
}

off_t
tvblseek (const int ihandle, off_t toffset, const int iwhence)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    if ( unlikely(!vb_rtd->svbfile[ihandle].irefcount) ) {
        errno = ENOENT;
        return -1;
    }
    return lseek (vb_rtd->svbfile[ihandle].ihandle, toffset, iwhence);
}

#ifdef	VBDEBUG
ssize_t
tvbread (const int ihandle, void *pvbuffer, const size_t tcount)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    if ( unlikely(!vb_rtd->svbfile[ihandle].irefcount) ) {
        errno = ENOENT;
        return -1;
    }
    return read (vb_rtd->svbfile[ihandle].ihandle, pvbuffer, tcount);
}

ssize_t
tvbwrite (const int ihandle, void *pvbuffer, const size_t tcount)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    if ( unlikely(!vb_rtd->svbfile[ihandle].irefcount) ) {
        errno = ENOENT;
        return -1;
    }
    return write (vb_rtd->svbfile[ihandle].ihandle, pvbuffer, tcount);
}
#endif

int
ivbblockread (const int ihandle, const int iisindex, off_t tblocknumber, VB_CHAR *cbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbfptr;
    off_t       tresult, toffset;
    int     thandle;

    psvbfptr = vb_rtd->psvbfile[ihandle];
    toffset = (off_t) ((tblocknumber - 1) * psvbfptr->inodesize);
    if ( iisindex ) {
        thandle = psvbfptr->iindexhandle;
    } else {
        thandle = psvbfptr->idatahandle;
    }
    tresult = tvblseek (thandle, toffset, SEEK_SET);
    if ( tresult != toffset ) {
#ifdef	VBDEBUG
        fprintf (stderr,
#if	ISAMMODE == 1
                 "Failed to position to block %lld of the %s file of table %s!\n",
#else
                 "Failed to position to block %ld of the %s file of table %s!\n",
#endif
                 tblocknumber, iisindex ? "Index" : "Data",
                 psvbfptr->cfilename);
        assert (0);
#else
        return EIO;
#endif
    }

    tresult = (off_t) tvbread (thandle, cbuffer, (size_t) psvbfptr->inodesize);
    if ( !iisindex && tresult == 0 ) {
        tresult = (ssize_t) psvbfptr->inodesize;
        memset (cbuffer, 0, (size_t)psvbfptr->inodesize);
    }
    if ( (int)tresult != psvbfptr->inodesize ) {
#ifdef	VBDEBUG
#if	ISAMMODE == 1
        fprintf (stderr, "Failed to read in block %lld from the %s file of table %s!\n",
#else
        fprintf (stderr, "Failed to read in block %ld from the %s file of table %s!\n",
#endif
                 tblocknumber, iisindex ? "Index" : "Data",
                 psvbfptr->cfilename);
        assert (0);
#else
        return EIO;
#endif
    }
    return 0;
}

int
ivbblockwrite (const int ihandle, const int iisindex, off_t tblocknumber, VB_CHAR *cbuffer)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
    struct DICTINFO *psvbfptr;
    off_t       tresult, toffset;
    int     thandle;

    psvbfptr = vb_rtd->psvbfile[ihandle];
    toffset = (off_t) ((tblocknumber - 1) * psvbfptr->inodesize);
    if ( iisindex ) {
        thandle = psvbfptr->iindexhandle;
    } else {
        thandle = psvbfptr->idatahandle;
    }
    tresult = tvblseek (thandle, toffset, SEEK_SET);
    if ( tresult == (off_t) -1 ) {
#ifdef	VBDEBUG
        fprintf (stderr,
#if	ISAMMODE == 1
                 "Failed to position to block %lld of the %s file of table %s!\n",
#else
                 "Failed to position to block %ld of the %s file of table %s!\n",
#endif
                 tblocknumber, iisindex ? "Index" : "Data",
                 psvbfptr->cfilename);
        assert (0);
#else
        return EIO;
#endif
    }

    tresult = (off_t) tvbwrite (thandle, cbuffer, (size_t) psvbfptr->inodesize);
    if ( (int)tresult != psvbfptr->inodesize ) {
#ifdef	VBDEBUG
#if	ISAMMODE == 1
        fprintf (stderr, "Failed to write out block %lld to the %s file of table %s!\n",
#else
        fprintf (stderr, "Failed to write out block %ld to the %s file of table %s!\n",
#endif
                 tblocknumber, iisindex ? "Index" : "Data",
                 psvbfptr->cfilename);
        assert (0);
#else
        return EIO;
#endif
    }
    return 0;
}

int
ivblock (const int ihandle, off_t toffset, off_t tlength, const int imode)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
#ifdef	_WIN32
#ifdef	USE_LOCKFILE_EX

    OVERLAPPED  toverlapped;
    DWORD       tflags = 0;
    int     bunlock = 0;

    if ( !vb_rtd->svbfile[ihandle].irefcount ) {
        errno = ENOENT;
        return -1;
    }

    switch ( imode ) {
    case    VBUNLOCK:
        bunlock = 1;
        break;

    case    VBRDLOCK:
        tflags = LOCKFILE_FAIL_IMMEDIATELY;
        break;

    case    VBRDLCKW:
        tflags = 0;
        break;

    case    VBWRLOCK:
        tflags = LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY;
        break;

    case    VBWRLCKW:
        tflags = LOCKFILE_EXCLUSIVE_LOCK;
        break;

    default:
        errno = EBADARG;
        return -1;
    }

    memset (&toverlapped, 0, sizeof(OVERLAPPED));
    toverlapped.Offset = (DWORD)(toffset & 0xffffffff);
    toverlapped.OffsetHigh = (DWORD)(toffset >> 32);
    errno = 0;
    if ( !bunlock ) {
        if ( LockFileEx (vb_rtd->svbfile[ihandle].whandle, tflags, 0, (int)(tlength & 0xffffffff),
                         (int)(tlength >> 32), &toverlapped) ) {
            return 0;
        }
    } else {
        if ( UnlockFileEx (vb_rtd->svbfile[ihandle].whandle, 0, (int)(tlength & 0xffffffff),
                           (int)(tlength >> 32), &toverlapped) ) {
            return 0;
        }
    }

    errno = EBADARG;
    return -1;
#else
/* This will probably not work correctly - 64 bit */
    int     itype, iresult = -1;
    off_t       soffset;
    off_t       tempoffset;

    if ( !vb_rtd->svbfile[ihandle].irefcount ) {
        errno = ENOENT;
        return -1;
    }
    switch ( imode ) {
    case VBUNLOCK:
        itype = _LK_UNLCK;
        break;

    case VBRDLOCK:
        itype = _LK_NBRLCK;
        break;

    case VBRDLCKW:
        itype = _LK_RLCK;
        break;

    case VBWRLOCK:
        itype = _LK_NBLCK;
        break;

    case VBWRLCKW:
        itype = _LK_LOCK;
        break;

    default:
        errno = EBADARG;
        return -1;
    }
    soffset = lseek (vb_rtd->svbfile[ihandle].ihandle, 0, SEEK_CUR);
    if ( soffset == -1 ) {
        errno = 172;
        return -1;
    }
    tempoffset = lseek (vb_rtd->svbfile[ihandle].ihandle, toffset, SEEK_SET);
    if ( tempoffset == -1 ) {
        errno = 172;
        return -1;
    }
    errno = 0;
    iresult = _locking (vb_rtd->svbfile[ihandle].ihandle, itype, (long)tlength);
    /* Something weird going on here */
    if ( errno == 22 ) {
        errno = 0;
        iresult = 0;
    }
    lseek (vb_rtd->svbfile[ihandle].ihandle, soffset, SEEK_SET);
    return iresult;
#endif
#else
    int     icommand, itype, iresult;
    struct VB_flock sflock;

    if ( !vb_rtd->svbfile[ihandle].irefcount ) {
        errno = ENOENT;
        return -1;
    }
    switch ( imode ) {
    case VBUNLOCK:
        icommand = VB_F_SETLK;
        itype = F_UNLCK;
        break;

    case VBRDLOCK:
        icommand = VB_F_SETLK;
        itype = F_RDLCK;
        break;

    case VBRDLCKW:
        icommand = VB_F_SETLKW;
        itype = F_RDLCK;
        break;

    case VBWRLOCK:
        icommand = VB_F_SETLK;
        itype = F_WRLCK;
        break;

    case VBWRLCKW:
        icommand = VB_F_SETLKW;
        itype = F_WRLCK;
        break;

    default:
        errno = EBADARG;
        return -1;
    }
    sflock.l_type = itype;
    sflock.l_whence = SEEK_SET;
    sflock.l_start = toffset;
    sflock.l_len = tlength;
    sflock.l_pid = 0;
/*
    iresult = -1;
    errno = EINTR;
    while (iresult && errno == EINTR) {
        sflock.l_type = itype;
        sflock.l_whence = SEEK_SET;
        sflock.l_start = toffset;
        sflock.l_len = tlength;
        sflock.l_pid = 0;
        iresult = fcntl (vb_rtd->svbfile[ihandle].ihandle, icommand, &sflock);
    }
*/
    do {
        iresult = fcntl (vb_rtd->svbfile[ihandle].ihandle, icommand, &sflock);
    } while ( iresult && errno == EINTR );

    return iresult;
#endif
}
