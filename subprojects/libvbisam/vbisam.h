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

#ifndef VBISAM_INCL     /* avoid multiple include problems */
    #define     VBISAM_INCL

    #define     VBISAM_USE_LONG_LONG    /* Note hack for now */

    #ifdef      VBISAM_USE_LONG_LONG
        typedef long long vbisam_off_t;
    #else
        typedef signed int vbisam_off_t;
    #endif

    typedef signed char VB_CHAR;
    typedef unsigned char VB_UCHAR;

    #include    "vbdecimal.h"

    #ifdef      _MSC_VER
        #ifdef VBISAM_BUILD_DLL
            #ifdef VBISAM_LIB
                #define VBISAM_DLL_EXPIMP __declspec(dllexport)
            #else
                #define VBISAM_DLL_EXPIMP __declspec(dllimport)
            #endif
        #else
            #define VBISAM_DLL_EXPIMP
        #endif
    #else
        #define VBISAM_DLL_EXPIMP
    #endif

    #define     CHARTYPE        0
    #define     INTTYPE         1
    #define     LONGTYPE        2
    #define     DOUBLETYPE      3
    #define     FLOATTYPE       4
    #define     QUADTYPE        5

    #define     CHARSIZE        1
    #define     INTSIZE         2
    #define     LONGSIZE        4
    #define     FLOATSIZE       (sizeof (float))
    #define     DOUBLESIZE      (sizeof (double))

    #define     ISDESC          0x80

    #define     BYTEMASK        0xFF    /* Mask for one byte */
    #define     BYTESHFT        8       /* Shift for one byte */

    #define     ISFIRST         0       /* Position to first row */
    #define     ISLAST          1       /* Position to last row */
    #define     ISNEXT          2       /* Position to next row */
    #define     ISPREV          3       /* Position to previous row */
    #define     ISCURR          4       /* Position to current row */
    #define     ISEQUAL         5       /* Position to equal value */
    #define     ISGREAT         6       /* Position to greater value */
    #define     ISGTEQ          7       /* Position to >= value */

/* isread () locking modes */
    #define     ISLOCK          0x100   /* Row lock */
    #define     ISSKIPLOCK      0x200   /* Skip row even if locked */
    #define     ISWAIT          0x400   /* Wait for row lock */
    #define     ISLCKW          ISLOCK | ISWAIT

/* isstart () lock modes */
    #define     ISKEEPLOCK      0x800   /* Keep rec lock in autolk mode */

/* isopen (), isbuild () lock modes */
    #define     ISAUTOLOCK      0x200   /* Automatic row lock */
    #define     ISMANULOCK      0x400   /* Manual row lock */
    #define     ISEXCLLOCK      0x800   /* Exclusive isam file lock */

/* isopen (), isbuild () file types */
    #define     ISINPUT         0       /* Open for input only */
    #define     ISOUTPUT        1       /* Open for output only */
    #define     ISINOUT         2       /* Open for input and output */
    #define     ISTRANS         4       /* Open for transaction proc */
    #define     ISNOLOG         8       /* No loggin for this file */
    #define     ISVARLEN        0x10    /* Variable length rows */
    #define     ISFIXLEN        0x0     /* (Non-flag) Fixed length rows only */
    #define     ISREBUILD       0x20    /* Open for Rebuild (forget some check)*/

/* audit trail mode parameters */
    #define     AUDSETNAME      0       /* Set new audit trail name */
    #define     AUDGETNAME      1       /* Get audit trail name */
    #define     AUDSTART        2       /* Start audit trail */
    #define     AUDSTOP         3       /* Stop audit trail */
    #define     AUDINFO         4       /* Audit trail running */

    #define     VB_MAX_KEYLEN   511     /* BUG - FIXME! Maximum number of bytes in a key */
    #define     NPARTS          8       /* Maximum number of key parts */

struct  keypart {
    short   kp_start;   /* Starting byte of key part */
    short   kp_leng;    /* Length in bytes */
    short   kp_type;    /* Type of key part (include ISDESC as needed) */
};

typedef struct keydesc {
    short           k_flags;            /* Flags (Compression) */
    short           k_nparts;           /* Number of parts in this key */
    struct keypart  k_part[NPARTS];     /* Each key part */
    short           k_len;              /* Length of entire uncompressed key */
    vbisam_off_t    k_rootnode;         /* Pointer to rootnode */
} keydesc_t;

    #define     k_start k_part[0].kp_start
    #define     k_leng  k_part[0].kp_leng
    #define     k_type  k_part[0].kp_type

/* Possible values for iFlags */
    #define     ISNODUPS        0x00    /* No duplicates allowed */
    #define     ISDUPS          0x01    /* Duplicates allowed */
    #define     DCOMPRESS       0x02    /* Duplicate compression */
    #define     LCOMPRESS       0x04    /* Leading compression */
    #define     TCOMPRESS       0x08    /* Trailing compression */
    #define     NULLKEY         0x20    /* null key masking */
    #define     COMPRESS        0x0e    /* All compression */
    #define     ISCLUSTER       0x00

struct  dictinfo {
    short           di_nkeys;   /* Number of keys defined (msb set if VARLEN) */
    short           di_recsize; /* Maximum data row length */
    short           di_idxsize; /* Number of bytes in an index node */
    vbisam_off_t        di_nrecords;/* Number of rows in data file */
};

/* Possible error return values */
    #define     EDUPL           100     /* Duplicate row */
    #define     ENOTOPEN        101     /* File not open */
    #define     EBADARG         102     /* Illegal argument */
    #define     EBADKEY         103     /* Illegal key desc */
    #define     ETOOMANY        104     /* Too many files open */
    #define     EBADFILE        105     /* Bad isam file format */
    #define     ENOTEXCL        106     /* Non-exclusive access */
    #define     ELOCKED         107     /* Row locked */
    #define     EKEXISTS        108     /* Key already exists */
    #define     EPRIMKEY        109     /* Is primary key */
    #define     EENDFILE        110     /* End/begin of file */
    #define     ENOREC          111     /* No row found */
    #define     ENOCURR         112     /* No current row */
    #define     EFLOCKED        113     /* File locked */
    #define     EFNAME          114     /* File name too long */
    #define     EBADMEM         116     /* Can't alloc memory */
    #define     ELOGREAD        118     /* Cannot read log rec */
    #define     EBADLOG         119     /* Bad log row */
    #define     ELOGOPEN        120     /* Cannot open log file */
    #define     ELOGWRIT        121     /* Cannot write log rec */
    #define     ENOTRANS        122     /* No transaction */
    #define     ENOBEGIN        124     /* No begin work yet */
    #define     ENOPRIM         127     /* No primary key */
    #define     ENOLOG          128     /* No logging */
    #define     ENOFREE         131     /* No free disk space */
    #define     EROWSIZE        132     /* Row size too short / long */
    #define     EAUDIT          133     /* Audit trail exists */
    #define     ENOLOCKS        134     /* No more locks */
    #define     EDEADLOK        143     /* Deadlock avoidance */
    #define     ENOMANU         153     /* Must be in ISMANULOCK mode */
    #define     EINTERUPT       157     /* Interrupted isam call */
    #define     EBADFORMAT      171     /* Locking or NODESIZE change */


struct DICTINFO;
struct VBLOCK;
struct VBTREE;
struct SLOGHDR;
#define	MAXSUBS		        32  /* Maximum number of indexes per table */
#define	VB_MAX_FILES	    128	/* Maximum number of open VBISAM files */
#define MAX_BUFFER_LENGTH	65536

struct  VBFILE {
    struct VBLOCK   *pslockhead;    /* Ordered linked list of locked row numbers */
    struct VBLOCK   *pslocktail;
    int             ihandle;
    int             irefcount;      /* How many times we are 'open' */
    /*dev_t*/ long  tdevice;
    /*ino_t*/ long  tinode;
#ifdef	_WIN32
    void*          whandle;
    VB_CHAR        *cfilename;
#endif
};
/* Implementation limits */
/* 64-bit versions have a maximum node length of 4096 bytes */
/* 32-bit versions have a maximum node length of 1024 bytes */

#if	ISAMMODE == 1
    #define	MAX_NODE_LENGTH	4096
    #define	MAX_KEYS_PER_NODE	((MAX_NODE_LENGTH - (INTSIZE + QUADSIZE + 2)) / (QUADSIZE + 1))
#else	/* ISAMMODE == 1 */
    #define	MAX_NODE_LENGTH	1024
    #define	MAX_KEYS_PER_NODE	((MAX_NODE_LENGTH - (INTSIZE + 2)) / (QUADSIZE + 1))
#endif	/* ISAMMODE == 1 */

typedef struct vb_rtd_s {
    int            iserrno;    /* Isam error return code */
    int            iserrio;    /* NOT used with VBISAM */
    int            isreclen;   /* Used for varlen tables */
    vbisam_off_t   isrecnum;   /* Current row number */

    /* Private Storages */
    VB_CHAR	        *ds;
    void	        *pcwritebuffer;
    int             ivbintrans;
    int             ivblogfilehandle;
    int             ivbmaxusedhandle;
    struct DICTINFO *psvbfile[VB_MAX_FILES + 1];
    struct VBFILE   svbfile[VB_MAX_FILES * 3];
    struct SLOGHDR	*psvblogheader;
    long int	    tvbpid;
    long int	    tvbuid;
    VB_CHAR		    cvbtransbuffer[MAX_BUFFER_LENGTH]; /* Buffer for holding transaction */
    int             iinitialized;
    vbisam_off_t    toffset;
    int             iprevlen;
    /*VB_CHAR	        cvbnodetmp[MAX_NODE_LENGTH];*/
    int             lowiinitialized;
    struct VBLOCK	*pslockfree;
    struct VBTREE	*pstreefree;
    int             vb_isinit;
#ifdef	VBDEBUG
    int		        icurrhandle;
    size_t		    tmallocused;
    size_t		    tmallocmax;
#endif
} vb_rtd_t;

#define VB_RTD vb_rtd_t* vb_rtd
extern vb_rtd_t *vb_get_rtd (void);
#define VB_GET_RTD vb_get_rtd()



struct  audhead {
    VB_CHAR au_type[2]; /* Audit row type aa,dd,rr,ww */
    VB_CHAR au_time[4]; /* Audit date-time */
    VB_CHAR au_procid[2];   /* Process id number */
    VB_CHAR au_userid[2];   /* User id number */
    VB_CHAR au_recnum[4];   /* Row number - RXW Set to 4 */
    VB_CHAR au_reclen[2];   /* audit row length beyond header */
};

/* Number of bytes in audit header */
    #define     AUDHEADSIZE     14
/* VARLEN num of bytes in audit header */
    #define     VAUDHEADSIZE    16

/* Prototypes for file manipulation functions */
extern void vb_init_rtd (void);
extern int  isaddindex (int ihandle, struct keydesc *pskeydesc);
extern int  isaudit (int ihandle, VB_CHAR *pcfilename, int imode);
extern int  isbegin (void);
extern int  isbuild (const VB_CHAR *pcfilename, int imaxrowlength,
                     struct keydesc *pskey, int imode);
extern int  ischeck (const VB_CHAR *pcfile);
extern int  iscleanup (void);
extern int  isclose (int ihandle);
extern int  iscluster (int ihandle, struct keydesc *pskeydesc);
extern int  iscommit (void);
extern int  isdelcurr (int ihandle);
extern int  isdelete (int ihandle, VB_CHAR *pcrow);
extern int  isdelindex (int ihandle, struct keydesc *pskeydesc);
extern int  isdelindex (int ihandle, struct keydesc *pskeydesc);
extern int  isdelrec (int ihandle, vbisam_off_t trownumber);
extern int  iserase (VB_CHAR *pcfilename);
extern int  isflush (int ihandle);
extern int  isfullclose (int ihandle);
extern int  isindexinfo (int ihandle, void *pskeydesc, int ikeynumber);
extern int  islock (int ihandle);
extern int  islogclose (void);
extern int  islogopen (VB_CHAR *pcfilename);
extern int  isopen (const VB_CHAR *pcfilename, int imode);
extern int  isread (int ihandle, VB_CHAR *pcrow, int imode);
extern int  isrecover (void);
extern int  isrelcurr (int ihandle);
extern int  isrelease (int ihandle);
extern int  isrelrec (int ihandle, vbisam_off_t trownumber);
extern int  isrename (VB_CHAR *pcoldname, VB_CHAR *pcnewname);
extern int  isrewcurr (int ihandle, VB_CHAR *pcrow);
extern int  isrewrec (int ihandle, vbisam_off_t trownumber, VB_CHAR *pcrow);
extern int  isrewrite (int ihandle, VB_CHAR *pcrow);
extern int  isrollback (void);
extern int  issetcollate (int ihandle, VB_UCHAR *collating_sequence);
extern int  issetunique (int ihandle, vbisam_off_t tuniqueid);
extern int  isstart (int ihandle, struct keydesc *pskeydesc,
                     int ilength, VB_CHAR *pcrow, int imode);
extern int  isuniqueid (int ihandle, vbisam_off_t *ptuniqueid);
extern int  isunlock (int ihandle);
extern int  iswrcurr (int ihandle, VB_CHAR *pcrow);
extern int  iswrite (int ihandle, VB_CHAR *pcrow);

extern void ldchar (VB_CHAR *pcsource, int ilength, VB_CHAR *pcdestination);
extern void stchar (VB_CHAR *pcsource, VB_CHAR *pcdestination, int ilength);
extern int  ldint (void *pclocation);
extern void stint (int ivalue, void *pclocation);
extern int  ldlong (void *pclocation);
extern void stlong (int lvalue, void *pclocation);
/* RXW
extern vbisam_off_t     ldquad (void *pclocation);
extern void     stquad (vbisam_off_t tvalue, void *pclocation);
*/
extern double   ldfloat (void *pclocation);
extern void     stfloat (double dsource, void *pcdestination);
extern double   ldfltnull (void *pclocation, short *pinullflag);
extern void     stfltnull (double dsource, void *pcdestination, int inullflag);
extern double   lddbl (void *pclocation);
extern void     stdbl (double dsource, void *pcdestination);
extern double   lddblnull (void *pclocation, short *pinullflag);
extern void     stdblnull (double dsource, void *pcdestination, int inullflag);
extern int      lddecimal (VB_UCHAR *cp, int len, dec_t *dp);
extern void     stdecimal (dec_t *dp, VB_UCHAR *cp, int len);

extern int          isdictinfo (int ihandle, struct dictinfo *psdictinfo);
extern int          iskeyinfo (int ihandle, struct keydesc *pskeydesc, int ikeynumber);
extern int          iserrno (void);
extern int          iserrio (void);
extern vbisam_off_t isrecnum (void);
extern int          set_isrecnum (vbisam_off_t irecnum);
extern int          isreclen (void);
extern int          set_isreclen (int ireclen);
extern const char  *is_strerror (int errcode);
extern const char  *isversnumber (void);
extern const char  *iscopyright (void);
extern const char  *isserial (void);
extern const int    issingleuser (void);
extern const int    is_nerr (void);
#endif  /* VBISAM_INCL */
