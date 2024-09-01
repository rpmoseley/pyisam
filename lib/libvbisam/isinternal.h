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

#ifndef VB_LIBVBISAM_H
#define VB_LIBVBISAM_H

/* Note : following are pulled in from config.h : */
/* ISAMMODE   - ISAM Mode */
/* HAVE_LFS64   - 64 bit file I/O */
/* VBDEBUG    - Debugging mode */

#ifdef WITH_LFS64
    #define _LFS64_LARGEFILE  1
    #define _LFS64_STDIO  1
    #define _FILE_OFFSET_BITS 64
    #define _LARGEFILE64_SOURCE 1
    #define __USE_LARGEFILE64 1
    #define __USE_FILE_OFFSET64 1
    #define _LARGE_FILES  1

    #if (defined(__hpux__) || defined(__hpux)) && !defined(__LP64__)
        #define _APP32_64BIT_OFF_T  1
    #endif
    #if (defined(__hpux__) || defined(__hpux))
        #define VB_SHORT_LOCK_RANGE     1
    #endif

    /* ??? #define  VB_MAX_OFF_T    (off_t)2147483647*/
    #define VB_MAX_OFF_T    (off_t)9223372036854775807LL
#else
    #define VB_MAX_OFF_T    (off_t)2147483647
    /* #define  VB_MAX_OFF_T    (off_t)9223372036854775807*/
#endif

#ifdef  HAVE_UNISTD_H
    #include  <unistd.h>
#endif
#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <ctype.h>
#ifdef  HAVE_FCNTL_H
    #include  <fcntl.h>
#endif
#include  <stdlib.h>
#include  <string.h>
#include  <errno.h>
#include  <time.h>
#include  <limits.h>
#include  <float.h>

#ifdef _WIN32
    #define WINDOWS_LEAN_AND_MEAN
    #include  <windows.h>
    #include  <io.h>
    #include  <sys/locking.h>
    #define open(x, y, z) _open(x, y, z)
    #define read(x, y, z) _read(x, y, z)
    #define write(x, y, z)  _write(x, y, z)
    #if (!defined(_FILE_OFFSET_BITS) || (_FILE_OFFSET_BITS != 64))
        #ifdef WITH_LFS64
            #define lseek(x, y, z)  _lseeki64(x, y, z)
        #else
            #define lseek(x, y, z)  _lseek(x, y, z)
        #endif
    #endif
    #define close(x)  _close(x)
    #define unlink(x) _unlink(x)
    #define fsync   _commit
typedef int     uid_t;
    #ifndef _PID_T_
typedef int     pid_t;
    #endif
    #ifndef _MODE_T_
typedef unsigned short  mode_t;
    #endif
    #if (!defined(_SSIZE_T_DEFINED) && !defined(_SIZE_T_))
        typedef int     ssize_t;
    #endif
#endif

#ifdef _MSC_VER

    #define _CRT_SECURE_NO_DEPRECATE 1
    #define VB_INLINE _inline
    #include  <malloc.h>
    #pragma warning(disable: 4996)
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp
    #define __attribute__(x)
    #define __i386__
    /*#define VB_THREAD __declspec( thread ) */

#elif  __370__
    #define VB_INLINE __inline
#elif defined(HAS_INLINE)
    #define VB_INLINE inline
#else
    #define VB_INLINE
#endif


#if ISAMMODE == 1
    #define QUADSIZE  8
    #if     VB_SHORT_LOCK_RANGE == 1
        #define VB_OFFLEN_7F  0x7FFFFFFF
        #define VB_OFFLEN_3F  0x3FFFFFFF
        #define VB_OFFLEN_40  0x40000000
    #else
        #define VB_OFFLEN_7F  0x7fffffffffffffffLL
        #define VB_OFFLEN_3F  0x3fffffffffffffffLL
        #define VB_OFFLEN_40  0x4000000000000000LL
    #endif
#else
    #define QUADSIZE  4
    #define VB_OFFLEN_7F  0x7FFFFFFF
    #define VB_OFFLEN_3F  0x3FFFFFFF
    #define VB_OFFLEN_40  0x40000000
#endif

/*#define VBISAM_LIB only define for a dll*/
#include  "vbisam.h"
#include  "byteswap.h"

#ifdef _MSC_VER
    #define COB_THREAD __declspec( thread )
    #define HAVE__THEAD_ATTR 1
    extern COB_THREAD vb_rtd_t vb_rtd_data;
#else

#ifdef HAVE__PTHREAD_ATTR
  extern __pthread vb_rtd_t vb_rtd_data;
  #undef VB_GET_RTD
  #define VB_GET_RTD &vb_rtd_data

#elif defined (HAVE_PTHREAD_H)
#else
  extern vb_rtd_t vb_rtd_data;
  #undef VB_GET_RTD
  #define VB_GET_RTD &vb_rtd_data

#endif
#endif


/*
#if defined(__GNUC__) && defined(linux) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
    #define VB_HIDDEN __attribute__ ((visibility("hidden")))
#else
    #define VB_HIDDEN
#endif
*/
#define VB_HIDDEN

#ifndef O_BINARY
    #define O_BINARY  0
#endif

#ifdef  WITH_LFS64
    #ifdef  _WIN32
        #define off_t __int64
    #endif
#endif

#if defined(__GNUC__) && (__GNUC__ >= 3)
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x) (x)
    #define unlikely(x) (x)
#endif

#if !defined(__i386__) && !defined(__x86_64__) && !defined(__powerpc__) && !defined(__powerpc64__) && !defined(__ppc__) && !defined(__amd64__)
    #if defined(_MSC_VER)
        #define ALLOW_MISALIGNED
        #define MISALIGNED __unaligned
    #else
        #define MISALIGNED
    #endif
#else
    #define ALLOW_MISALIGNED
    #define MISALIGNED
#endif

typedef VB_UCHAR * ucharptr;

/* Inline versions of load/store routines */
#if defined(NEED_VBINLINE_INT_LOAD) && NEED_VBINLINE_INT_LOAD
static VB_INLINE int
inl_ldint (void *pclocation)
{
#ifndef WORDS_BIGENDIAN
    #ifdef ALLOW_MISALIGNED
    return(int)VB_BSWAP_16(*(unsigned short *)pclocation);
    #else
    short   ivalue = 0;
    VB_UCHAR   *pctemp = (VB_UCHAR *) &ivalue;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;

    *(pctemp + 0) = *(pctemp2 + 1);
    *(pctemp + 1) = *(pctemp2 + 0);
    return(int)ivalue;
    #endif
#else
    short   ivalue = 0;
    VB_UCHAR   *pctemp = (VB_UCHAR *) &ivalue;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;

    *(pctemp + 0) = *(pctemp2 + 0);
    *(pctemp + 1) = *(pctemp2 + 1);
    return(int)ivalue;
#endif
}
#endif

#if defined(NEED_VBINLINE_INT_STORE) && NEED_VBINLINE_INT_STORE
static VB_INLINE void
inl_stint (int ivalue, void *pclocation)
{
#ifndef WORDS_BIGENDIAN
    #ifdef ALLOW_MISALIGNED
    *(unsigned short *)pclocation = VB_BSWAP_16((unsigned short)ivalue);
    #else
    short           s =(short)(ivalue & 0xFFFF);
    VB_UCHAR   *pctemp = (VB_UCHAR *) &s;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;

    *(pctemp2 + 0) = *(pctemp + 1 );
    *(pctemp2 + 1) = *(pctemp + 0 );
    #endif
#else
    short           s =(short)(ivalue & 0xFFFF);
    VB_UCHAR   *pctemp = (VB_UCHAR *) &s;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;

    *(pctemp2 + 0) = *(pctemp + 0 );
    *(pctemp2 + 1) = *(pctemp + 1 );
#endif
    return;
}
#endif

#if defined(NEED_VBINLINE_LONG_LOAD) && NEED_VBINLINE_LONG_LOAD
static VB_INLINE int
inl_ldlong (void *pclocation)
{
#ifndef WORDS_BIGENDIAN
    #ifdef ALLOW_MISALIGNED
    return VB_BSWAP_32(*(unsigned int *)pclocation);
    #else
    int   ivalue = 0;
    VB_UCHAR   *pctemp = (VB_UCHAR *) &ivalue;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;

    *(pctemp + 0) = *(pctemp2 + 3);
    *(pctemp + 1) = *(pctemp2 + 2);
    *(pctemp + 2) = *(pctemp2 + 1);
    *(pctemp + 3) = *(pctemp2 + 0);
    return(int)ivalue;
    #endif
#else
    int lvalue;

    memcpy((VB_UCHAR *)&lvalue, (VB_UCHAR *)pclocation, 4);
    return lvalue;
#endif
}
#endif

#if defined(NEED_VBINLINE_LONG_STORE) && NEED_VBINLINE_LONG_STORE
static VB_INLINE void
inl_stlong (int lvalue, void *pclocation)
{
#ifndef WORDS_BIGENDIAN
    #ifdef ALLOW_MISALIGNED
    *(unsigned int *)pclocation = VB_BSWAP_32((unsigned int)lvalue);
    #else
    VB_UCHAR   *pctemp = (VB_UCHAR *) &lvalue;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;

    *(pctemp2 + 0) = *(pctemp + 3 );
    *(pctemp2 + 1) = *(pctemp + 2 );
    *(pctemp2 + 2) = *(pctemp + 1 );
    *(pctemp2 + 3) = *(pctemp + 0 );
    #endif
#else
    memcpy((VB_UCHAR *)pclocation, (VB_UCHAR *)&lvalue, 4);
#endif
    return;
}
#endif

#if defined(NEED_VBINLINE_QUAD_LOAD) && NEED_VBINLINE_QUAD_LOAD
static VB_INLINE off_t
inl_ldquad (void *pclocation)
{

#ifndef WORDS_BIGENDIAN
#if ISAMMODE == 1
    #ifdef ALLOW_MISALIGNED
    return VB_BSWAP_64(*(unsigned long long *)pclocation);
    #else
    unsigned long long   ivalue = 0;
    VB_UCHAR   *pctemp = (VB_UCHAR *) &ivalue;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;

    *(pctemp + 0) = *(pctemp2 + 7);
    *(pctemp + 1) = *(pctemp2 + 6);
    *(pctemp + 2) = *(pctemp2 + 5);
    *(pctemp + 3) = *(pctemp2 + 4);
    *(pctemp + 4) = *(pctemp2 + 3);
    *(pctemp + 5) = *(pctemp2 + 2);
    *(pctemp + 6) = *(pctemp2 + 1);
    *(pctemp + 7) = *(pctemp2 + 0);
    return (off_t)ivalue;
    #endif
#else /* ISAMMODE == 1 */

    #ifdef ALLOW_MISALIGNED
    return VB_BSWAP_32(*(unsigned int *)pclocation);
    #else
    off_t   ivalue = 0;
    VB_UCHAR   *pctemp = (VB_UCHAR *) &ivalue;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;

    *(pctemp + 0) = *(pctemp2 + 3);
    *(pctemp + 1) = *(pctemp2 + 2);
    *(pctemp + 2) = *(pctemp2 + 1);
    *(pctemp + 3) = *(pctemp2 + 0);
    return (off_t)ivalue;
    #endif
#endif  /* ISAMMODE == 1 */
#else
    off_t   tvalue = 0;

#if ISAMMODE == 1
    memcpy((VB_UCHAR *)&tvalue, (VB_UCHAR *)pclocation, 8);
#else /* ISAMMODE == 1 */
    memcpy((VB_UCHAR *)&tvalue, (VB_UCHAR *)pclocation, 4);
#endif  /* ISAMMODE == 1 */
    return tvalue;
#endif
}
#endif

#if defined(NEED_VBINLINE_QUAD_STORE) && NEED_VBINLINE_QUAD_STORE
static VB_INLINE void
inl_stquad (off_t tvalue, void *pclocation)
{
#ifndef WORDS_BIGENDIAN
#if ISAMMODE == 1
    #ifdef ALLOW_MISALIGNED
    *(unsigned long long *)pclocation = VB_BSWAP_64((unsigned long long)tvalue);
    #else
    VB_UCHAR   *pctemp = (VB_UCHAR *) &tvalue;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;
    *(pctemp2 + 0) = *(pctemp + 7 );
    *(pctemp2 + 1) = *(pctemp + 6 );
    *(pctemp2 + 2) = *(pctemp + 5 );
    *(pctemp2 + 3) = *(pctemp + 4 );
    *(pctemp2 + 4) = *(pctemp + 3 );
    *(pctemp2 + 5) = *(pctemp + 2 );
    *(pctemp2 + 6) = *(pctemp + 1 );
    *(pctemp2 + 7) = *(pctemp + 0 );
    #endif

#else /* ISAMMODE == 1 */
    #ifdef ALLOW_MISALIGNED
    *(unsigned int *)pclocation = VB_BSWAP_32((unsigned int)tvalue);
    #else
    VB_UCHAR   *pctemp = (VB_UCHAR *) &tvalue;
    VB_UCHAR   *pctemp2 = (VB_UCHAR *) pclocation;
    *(pctemp2 + 0) = *(pctemp + 3 );
    *(pctemp2 + 1) = *(pctemp + 2 );
    *(pctemp2 + 2) = *(pctemp + 1 );
    *(pctemp2 + 3) = *(pctemp + 0 );
    #endif
#endif  /* ISAMMODE == 1 */
#else
#if ISAMMODE == 1
    memcpy((VB_UCHAR *)pclocation, (VB_UCHAR *)&tvalue, 8);
#else /* ISAMMODE == 1 */
    memcpy((VB_UCHAR *)pclocation, (VB_UCHAR *)&tvalue, 4);
#endif  /* ISAMMODE == 1 */
#endif
    return;
}
#endif


#define SLOTS_PER_NODE  ((MAX_NODE_LENGTH >> 2) - 1)  /* Used in vbvarlenio.c */

#define MAX_PATH_LENGTH   512
#define MAX_OPEN_TRANS    1024  /* Currently, only used in isrecover () */
#define MAX_ISREC_LENGTH  32511
#define MAX_RESERVED_LENGTH 32768 /* Greater then MAX_ISREC_LENGTH */

/* Arguments to ivblock */
#define VBUNLOCK  0 /* Unlock */
#define VBRDLOCK  1 /* A simple read lock, non-blocking */
#define VBRDLCKW  2 /* A simple read lock, blocking */
#define VBWRLOCK  3 /* An exclusive write lock, non-blocking */
#define VBWRLCKW  4 /* An exclusive write lock, blocking */

/* Values for ivbrcvmode (used to control how isrecover works) */
#define RECOV_C   0x00  /* Boring old buggy C-ISAM mode */
#define RECOV_VB  0x01  /* New, improved error detection */
#define RECOV_LK  0x02  /* Use locks in isrecover (See documentation) */

/* Values for vb_rtd->ivbintrans */
#define VBNOTRANS 0 /* NOT in a transaction at all */
#define VBBEGIN   1 /* An isbegin has been issued but no more */
#define VBNEEDFLUSH 2 /* Something BEYOND just isbegin has been issued */
#define VBCOMMIT  3 /* We're in 'iscommit' mode */
#define VBROLLBACK  4 /* We're in 'isrollback' mode */
#define VBRECOVER 5 /* We're in 'isrecover' mode */

/*
VB_HIDDEN extern  VB_CHAR vb_rtd->cvbtransbuffer[MAX_BUFFER_LENGTH];
*/

struct  VBLOCK {
    struct  VBLOCK *psnext;
    int     ihandle;    /* The handle that 'applied' this lock */
    off_t       trownumber;
};

struct  VBKEY {
    struct  VBKEY   *psnext;    /* Next key in this node */
    struct  VBKEY   *psprev;    /* Previous key in this node */
    struct  VBTREE  *psparent;  /* Pointer towards ROOT */
    struct  VBTREE  *pschild;   /* Pointer towards LEAF */
    off_t       trownode;   /* The row / node number */
    off_t       tdupnumber; /* The duplicate number (1st = 0) */
    VB_UCHAR   iisnew;     /* If this is a new entry (split use) */
    VB_UCHAR   iishigh;    /* Is this a GREATER THAN key? */
    VB_UCHAR   iisdummy;   /* A simple end of node marker */
    VB_UCHAR   pspare[5];  /* Spare */
    VB_UCHAR   ckey[1];    /* Placeholder for the key itself */
};

struct  VBTREE {
    struct  VBTREE  *psnext;    /* Used for the free list only! */
    struct  VBTREE  *psparent;  /* The next level up from this node */
    struct  VBKEY   *pskeyfirst;    /* Pointer to the FIRST key in this node */
    struct  VBKEY   *pskeylast; /* Pointer to the LAST key in this node */
    struct  VBKEY   *pskeycurr; /* Pointer to the CURRENT key */
    off_t       tnodenumber;    /* The actual node */
    off_t       ttransnumber;   /* Transaction number stamp */
    unsigned int    ilevel;     /* The level number (0 = LEAF) */
    unsigned int    ikeysinnode;    /* # keys in pskeylist */
    VB_UCHAR   iisroot;    /* 1 = This is the ROOT node */
    VB_UCHAR   iistof;     /* 1 = First entry in index */
    VB_UCHAR   iiseof;     /* 1 = Last entry in index */
    VB_UCHAR   pspare[5];  /* Spare */
#if ISAMMODE == 1     /* Highly non-portable.. kind of */
    struct  VBKEY   *pskeylist[512];
#else /* ISAMMODE == 1 */
    struct  VBKEY   *pskeylist[256];
#endif  /* ISAMMODE == 1 */
};

struct  DICTNODE {                     /* Offset      32Val   64Val */
    /* 32IO 64IO */
    VB_CHAR    cvalidation[2];         /* 0x00  0x00  0xfe53  Same */
    VB_CHAR    cheaderrsvd;            /* 0x02  0x02  0x02    Same */
    VB_CHAR    cfooterrsvd;            /* 0x03  0x03  0x02    Same */
    VB_CHAR    crsvdperkey;            /* 0x04  0x04  0x04    0x08 */
    VB_CHAR    crfu1;                  /* 0x05  0x05  0x04    Same */
    VB_CHAR    cnodesize[INTSIZE];     /* 0x06  0x06  0x03ff  Same */
    VB_CHAR    cindexcount[INTSIZE];   /* 0x08  0x08  Varies  Same */
    VB_CHAR    crfu2[2];               /* 0x0a  0x0a  0x0704  Same */
    VB_CHAR    cfileversion;           /* 0x0c  0x0c  0x00    Same */
    VB_CHAR    cminrowlength[INTSIZE]; /* 0x0d  0x0d  Varies  Same */
    VB_CHAR    cnodekeydesc[QUADSIZE]; /* 0x0f  0x0f  Normally 2 */
    VB_CHAR    clocalindex;            /* 0x13  0x17  0x00    Same */
    VB_CHAR    crfu3[5];               /* 0x14  0x18  0x00... Same */
    VB_CHAR    cdatafree[QUADSIZE];    /* 0x19  0x1d  Varies  Same */
    VB_CHAR    cnodefree[QUADSIZE];    /* 0x1d  0x25  Varies  Same */
    VB_CHAR    cdatacount[QUADSIZE];   /* 0x21  0x2d  Varies  Same */
    VB_CHAR    cnodecount[QUADSIZE];   /* 0x25  0x35  Varies  Same */
    VB_CHAR    ctransnumber[QUADSIZE]; /* 0x29  0x3d  Varies  Same */
    VB_CHAR    cuniqueid[QUADSIZE];    /* 0x2d  0x45  Varies  Same */
    VB_CHAR    cnodeaudit[QUADSIZE];   /* 0x31  0x4d  Varies  Same */
    VB_CHAR    clockmethod[INTSIZE];   /* 0x35  0x55  0x0008  Same */
    VB_CHAR    crfu4[QUADSIZE];        /* 0x37  0x57  0x00... Same */
    VB_CHAR    cmaxrowlength[INTSIZE]; /* 0x3b  0x5f  Varies  Same */
    VB_CHAR    cvarleng0[QUADSIZE];    /* 0x3d  0x61  Varies  Same */
    VB_CHAR    cvarleng1[QUADSIZE];    /* 0x41  0x69  Varies  Same */
    VB_CHAR    cvarleng2[QUADSIZE];    /* 0x45  0x71  Varies  Same */
    VB_CHAR    cvarleng3[QUADSIZE];    /* 0x49  0x79  Varies  Same */
    VB_CHAR    cvarleng4[QUADSIZE];    /* 0x4d  0x81  Varies  Same */
#if ISAMMODE == 1
    VB_CHAR    cvarleng5[QUADSIZE];    /* 0x89  Varies  Same */
    VB_CHAR    cvarleng6[QUADSIZE];    /* 0x91  Varies  Same */
    VB_CHAR    cvarleng7[QUADSIZE];    /* 0x99  Varies  Same */
    VB_CHAR    cvarleng8[QUADSIZE];    /* 0xa1  Varies  Same */
#endif  /* ISAMMODE == 1 */
    VB_CHAR    crfulocalindex[36]; /* 0x51  0xa9  0x00... Same */
    /*       ---- ---- */
    /* Length Total    0x75 0xcd */
};

struct  DICTINFO {
    int inkeys;     /* Number of keys */
    int iactivekey; /* Which key is the active key */
    int inodesize;  /* Number of bytes in an index block */
    int iminrowlength;  /* Minimum data row length */
    int imaxrowlength;  /* Maximum data row length */
    int idatahandle;    /* file descriptor of the .dat file */
    int iindexhandle;   /* file descriptor of the .idx file */
    int iisopen;    /* 0: Table open, Files open, Buffers OK */
    /* 1: Table closed, Files open, Buffers OK */
    /*  Used to retain locks */
    /* 2: Table closed, Files closed, Buffers OK */
    /*  Basically, just caching */
    int iopenmode;  /* The type of open which was used */
    int ivarlenlength;  /* Length of varlen component */
    int ivarlenslot;    /* The slot number within tvarlennode */
    int dspare1;
    off_t   trownumber; /* Which data row is "CURRENT" 0 if none */
    off_t   tdupnumber; /* Which duplicate number is "CURRENT" (0=First) */
    off_t   trowstart;  /* ONLY set to nonzero by isstart() */
    off_t   ttranslast; /* Used to see whether to set iindexchanged */
    off_t   tnrows;     /* Number of rows (0 IF EMPTY, 1 IF NOT) */
    off_t   tvarlennode;    /* Node containing 1st varlen data */
    VB_CHAR    *cfilename;
    VB_CHAR    *ppcrowbuffer;  /* tminrowlength buffer for key (re)construction */
    VB_UCHAR       *collating_sequence; /* Collating sequence */
    VB_UCHAR       iisdisjoint;    /* If set, CURR & NEXT give same result */
    VB_UCHAR       iisdatalocked;  /* If set, islock() is active */
    VB_UCHAR       iisdictlocked;  /* Relates to sdictnode below */
    /* 0x00: Content on file MIGHT be different */
    /* 0x01: Dictionary node is LOCKED */
    /* 0x02: sdictnode needs to be rewritten */
    /* 0x04: do NOT increment Dict.Trans */
    /*       isrollback () is in progress */
    /*      (Thus, suppress some ivbenter/ivbexit) */
    VB_UCHAR       iindexchanged;  /* Various */
    /* 0: Index has NOT changed since last time */
    /* 1: Index has changed, blocks invalid */
    /* 2: Index has changed, blocks are valid */
    VB_UCHAR       itransyet;  /* Relates to isbegin () et al */
    /* 0: FO Trans not yet written */
    /* 1: FO Trans written outside isbegin */
    /* 2: FO Trans written within isbegin */
    VB_UCHAR       dspare2[3]; /* spare */
    struct  DICTNODE    sdictnode;  /* Holds dictionary node data */
    struct  keydesc     *pskeydesc[MAXSUBS];    /* Array of key description info */
    struct  VBTREE      *pstree[MAXSUBS]; /* Linked list of index nodes */
    struct  VBKEY       *pskeyfree[MAXSUBS]; /* An array of linked lists of free VBKEYs */
    struct  VBKEY       *pskeycurr[MAXSUBS]; /* An array of 'current' VBKEY pointers */
};

#define VBL_BUILD ("BU")
#define VBL_BEGIN ("BW")
#define VBL_CREINDEX  ("CI")
#define VBL_CLUSTER ("CL")
#define VBL_COMMIT  ("CW")
#define VBL_DELETE  ("DE")
#define VBL_DELINDEX  ("DI")
#define VBL_FILEERASE ("ER")
#define VBL_FILECLOSE ("FC")
#define VBL_FILEOPEN  ("FO")
#define VBL_INSERT  ("IN")
#define VBL_RENAME  ("RE")
#define VBL_ROLLBACK  ("RW")
#define VBL_SETUNIQUE ("SU")
#define VBL_UNIQUEID  ("UN")
#define VBL_UPDATE  ("UP")

struct  SLOGHDR {
    VB_CHAR    clength[INTSIZE];
    VB_CHAR    coperation[2];
    VB_CHAR    cpid[INTSIZE];
    VB_CHAR    cuid[INTSIZE];
    VB_CHAR    ctime[LONGSIZE];
    VB_CHAR    crfu1[INTSIZE];
    VB_CHAR    clastposn[INTSIZE];
    VB_CHAR    clastlength[INTSIZE];
};


/* isbuild.c */
VB_HIDDEN extern int    VBiaddkeydescriptor (const int ihandle, struct keydesc *pskeydesc);

/* isopen.c */
VB_HIDDEN extern int    ivbclose2 (const int ihandle);
VB_HIDDEN extern void   ivbclose3 (const int ihandle);

/* isread.c */
VB_HIDDEN extern int    ivbcheckkey (const int ihandle, struct keydesc *pskey,
                                     const int imode, int irowlength, const int iisbuild);

/* istrans.c */
VB_HIDDEN extern int    ivbtransbuild (const VB_CHAR *pcfilename, const int iminrowlen, const int imaxrowlen,
                                       struct keydesc *pskeydesc, const int imode);
VB_HIDDEN extern int    ivbtranscreateindex (int ihandle, struct keydesc *pskeydesc);
VB_HIDDEN extern int    ivbtranscluster (void);
VB_HIDDEN extern int    ivbtransdelete (int ihandle, off_t trownumber, int irowlength);
VB_HIDDEN extern int    ivbtransdeleteindex (int ihandle, struct keydesc *pskeydesc);
VB_HIDDEN extern int    ivbtranserase (VB_CHAR *pcfilename);
VB_HIDDEN extern int    ivbtransclose (int ihandle, VB_CHAR *pcfilename);
VB_HIDDEN extern int    ivbtransopen (int ihandle, const VB_CHAR *pcfilename);
VB_HIDDEN extern int    ivbtransinsert (int ihandle, off_t trownumber, int irowlength,
                                        VB_CHAR *pcrow);
VB_HIDDEN extern int    ivbtransrename (VB_CHAR *pcoldname, VB_CHAR *pcnewname);
VB_HIDDEN extern int    ivbtranssetunique (int ihandle, off_t tuniqueid);
VB_HIDDEN extern int    ivbtransuniqueid (int ihandle, off_t tuniqueid);
VB_HIDDEN extern int    ivbtransupdate (int ihandle, off_t trownumber, int ioldrowlen,
                                        int inewrowlen, VB_CHAR *pcrow);

/* iswrite.c */
VB_HIDDEN extern int    ivbwriterow (const int ihandle, VB_CHAR *pcrow, off_t trownumber);

/* vbdataio.c */
VB_HIDDEN extern int    ivbdataread (const int ihandle, VB_CHAR *pcbuffer, int *pideletedrow,
                                     off_t trownumber);
VB_HIDDEN extern int    ivbdatawrite (const int ihandle, VB_CHAR *pcbuffer, int ideletedrow,
                                      off_t trownumber);

/* vbindexio.c */
VB_HIDDEN extern off_t  tvbnodecountgetnext (const int ihandle);
VB_HIDDEN extern int    ivbnodefree (const int ihandle, off_t tnodenumber);
VB_HIDDEN extern int    ivbdatafree (const int ihandle, off_t trownumber);
VB_HIDDEN extern off_t  tvbnodeallocate (const int ihandle);
VB_HIDDEN extern off_t  tvbdataallocate (const int ihandle);
VB_HIDDEN extern int    ivbforcedataallocate (const int ihandle, off_t trownumber);

/* vbkeysio.c */
VB_HIDDEN extern void   vvbmakekey (const struct keydesc *pskeydesc,
                                    VB_CHAR *pcrow_buffer, VB_UCHAR *pckeyvalue);
VB_HIDDEN extern int    ivbkeysearch (const int ihandle, const int imode,
                                      const int ikeynumber, int ilength,
                                      VB_UCHAR *pckeyvalue, off_t tdupnumber);
VB_HIDDEN extern int    ivbkeylocaterow (const int ihandle, const int ikeynumber, off_t trownumber);
VB_HIDDEN extern int    ivbkeyload (const int ihandle, const int ikeynumber, const int imode,
                                    const int isetcurr, struct VBKEY **ppskey);
VB_HIDDEN extern void   vvbkeyvalueset (const int ihigh, struct keydesc *pskeydesc,
                                        VB_UCHAR *pckeyvalue);
VB_HIDDEN extern int    ivbkeyinsert (const int ihandle, struct VBTREE *pstree,
                                      const int ikeynumber, VB_UCHAR *pckeyvalue,
                                      off_t trownode, off_t tdupnumber,
                                      struct VBTREE *pschild);
VB_HIDDEN extern int    ivbkeydelete (const int ihandle, const int ikeynumber);
VB_HIDDEN extern int    ivbkeycompare (const int ihandle, const int ikeynumber, int ilength,
                                       VB_UCHAR *pckey1, VB_UCHAR *pckey2);
#ifdef  VBDEBUG
VB_HIDDEN extern int    idumptree (int ihandle, int ikeynumber);
VB_HIDDEN extern int    ichktree (int ihandle, int ikeynumber);
#endif  /* VBDEBUG */

/* vblocking.c */
VB_HIDDEN extern int    ivbenter (const int ihandle, const int imodifying);
VB_HIDDEN extern int    ivbexit (const int ihandle);
VB_HIDDEN extern int    ivbfileopenlock (const int ihandle, const int imode);
VB_HIDDEN extern int    ivbdatalock (const int ihandle, const int imode, off_t trownumber);

/* vblowlovel.c */
extern int    ivbopen (VB_CHAR *pcfilename, const int iflags, const mode_t tmode);
VB_HIDDEN extern int    ivbclose (const int ihandle);
VB_HIDDEN extern off_t  tvblseek (const int ihandle, off_t toffset, const int iwhence);

#ifdef  VBDEBUG
VB_HIDDEN extern ssize_t    tvbread (const int ihandle, void *pvbuffer, const size_t tcount);
VB_HIDDEN extern ssize_t    tvbwrite (const int ihandle, void *pvbuffer, const size_t tcount);
#else
    #define tvbread(x,y,z)  read(vb_rtd->svbfile[(x)].ihandle,(void *)(y),(size_t)(z))
    #define tvbwrite(x,y,z) write(vb_rtd->svbfile[(x)].ihandle,(void *)(y),(size_t)(z))
#endif

VB_HIDDEN extern int    ivbblockread (const int ihandle, const int iisindex,
                                      off_t tblocknumber, VB_CHAR *cbuffer);
VB_HIDDEN extern int    ivbblockwrite (const int ihandle, const int iisindex,
                                       off_t tblocknumber, VB_CHAR *cbuffer);
VB_HIDDEN extern int    ivblock (const int ihandle, off_t toffset, off_t tlength, const int imode);

/* vbmemio.c */
VB_HIDDEN extern struct VBLOCK  *psvblockallocate (const int ihandle);
VB_HIDDEN extern void           vvblockfree (struct VBLOCK *pslock);
VB_HIDDEN extern struct VBTREE  *psvbtreeallocate (const int ihandle);
VB_HIDDEN extern void           vvbtreeallfree (const int ihandle, const int ikeynumber,
                                                struct VBTREE *pstree);
VB_HIDDEN extern struct VBKEY   *psvbkeyallocate (const int ihandle, const int ikeynumber);
VB_HIDDEN extern void           vvbkeyallfree (const int ihandle, const int ikeynumber,
                                               struct VBTREE *pstree);
VB_HIDDEN extern void           vvbkeyfree (const int ihandle, const int ikeynumber,
                                            struct VBKEY *pskey);
VB_HIDDEN extern void           vvbkeyunmalloc (const int ihandle, const int ikeynumber);
#ifdef  VBDEBUG
VB_HIDDEN extern void           *pvvbmalloc (const size_t tlength);
VB_HIDDEN extern void           vvbfree (void *pvpointer, size_t tlength);
VB_HIDDEN extern void           vvbunmalloc (void);
#else
    #define vvbfree(x,y)  free((void *)(x))
    #define pvvbmalloc(x) calloc(1, (size_t)(x))
#endif  /* VBDEBUG */

/* vbnodememio.c */
VB_HIDDEN extern int    ivbnodeload (const int ihandle, const int ikeynumber,
                                     struct VBTREE *pstree, off_t tnodenumber,
                                     int iprevlvl);
VB_HIDDEN extern int    ivbnodesave (const int ihandle, const int ikeynumber,
                                     struct VBTREE *pstree, off_t tnodenumber , int imode,
                                     int iposn);

#endif  /* VB_LIBVBISAM_H */
