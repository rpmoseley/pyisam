/*
-----------------------------------------------------------------------------
configuration options
-----------------------------------------------------------------------------
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *             Worldwide Copyright (c) Byte Designs Ltd (2024)             *
 *          Version 7.2 (build 2024/11/30) (expire cisam shared)           *
 *                                                                         *
 *                            Byte Designs Ltd.                            *
 *                            20568 - 32 Avenue                            *
 *                               LANGLEY, BC                               *
 *                             V2Z 2C8 CANADA                              *
 *                                                                         *
 *                       Sales: sales@bytedesigns.com                      *
 *                     Support: support@bytedesigns.com                    *
 *              Phone: (604) 534 0722     Fax: (604) 534 2601              *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#define ISEXPIRE 1767157200 /* Wed Dec 31 00:00:00 2025 */

/* features */
#define ISADMIN     1                   /* global administration */
#define ISAUDIT     1                   /* 0:off 1:full audit */
                                        /* ISAUDIT 2 thru 7 see audit.ref */
#define ISLOGGING   1                   /* 0:off 1:standard */
#define ISVARIABLE  3                   /* 0:off 1:cisam 2:disam 3:huge */
#define ISLOCKING   2                   /* 0:off 1:old 2:std 3:lck */
#define ISDUPLOCKS  1                   /* 1 - honour locks in dup opens */
#define ISDECLARE   1                   /* declare parameters */
#define ISDATAVOID  1                   /* pointer is 0:char 1: void */
#define ISCUSTOM    0                   /* include custom extensions */
#define ISBERKELY   0                   /* 0:memcpy() 1: bcopy() */
#define ISHUGE      1                   /* 0:automatic 1:large file support */
#define ISNOWAIT    0			/* optional ISAUTOLOCK nowait */
#define ISLONGID    0                   /* see read/install.ref */
#define ISTXNID     2                   /* 0: short, 1: 32bit, 2: 64bit */
#define ISDEBUG     1                   /* see read/debug.ref */
#define ISREPAIR    0 			/* 0: off 1: auto repair enabled */
#define ISPURE      0                   /* pure idx isrecnum 0: user assigned */
                                        /* see pure.ref      1: d96 assigned */
#define ISKCOMP     0                   /* very wide keys, see keycomp.ref */
#define ISEXTENDED  0			/* see read/extended.ref */
#define ISZERODATA  0			/* 1: wipe (write zeros) on delete */

#define ISNETPORT   5004                /* your default port number here */
#define ISNETENV    "DISAMNET"          /* environment key - default server */
#define ISNETSSH    "DISAMSSH"          /* environment key - user@host shell call */ 
#define ISMAPENV    "DISAMMAP"          /* environment key - url://path/to/maps */


/* values */
#define ISMAXPATH   1600                /* maxlen of complete path name */
#define ISIDXBLK    1024                /* default index block size */
#define ISDUPLEN    4                   /* default duplicate width */
#define ISMAXIDX    20                  /* indexes per file */
#define ISMAXPARTS  20                  /* parts per key */
#define ISMAXKEY    256                 /* bytes per key */
#define ISMAXBUF    40                  /* buffers per index */
#define ISERRBASE   500                 /* start error codes at */

/* earlier D-ISAM compatible ------------------------------------------*/
#define D3XAUTOLOCK 0                   /* 0: default next read release */
                                        /* 1: D-ISAM v3.x behaviour */

/* set for C-ISAM compatible behaviours -------------------------------*/
#define ISCISAM     2                   /* 0: pure disam               */
                                        /* 1: cisam 5.0 > 7.1          */
                                        /* 2: cisam 7.2 and over       */

#define SEMILOCK  0			/* 0: default                  */
 					/* 1: c-isam ISSEMILOCK        */

/* set for C-ISAM 5.0 -> 7.1 locking concurrency ----------------------*/
#define C7LOCKING 2                     /* 0: disam/cisam pre 5.0     */
                                        /* 1: cisam 5.0 > 7.1          */
                                        /* 2: cisam 7.2x               */

/* GENERAL configuration flags ----------------------------------------*/
#define ISCONFIG                0

#if( ISCONFIG > 0 )
#define ISCONFIG_REWCURR_LOCATE 1       /* rewcurr follows updated key */
#define ISCONFIG_NOCURR_ON_FAIL 2       /* ISNOCURR after read fails */
#define ISCONFIG_NOCURR_ON_SEEK 4       /* ENOCURR on read no CURR */
#define ISCONFIG_ISINDEX_LOCATE 8       /* non-standard islocate() call */
#endif

/* COBOL support flags ------------------------------------------------*/
#define ISCOBOL     0                   /* COBOL support */

#if( ISCOBOL > 0 )                      /* the sum of these options:   */
#define ISCOBOL_STATS           1       /* support for isstat codes    */
#define ISCOBOL_NO_NEW_EXT      2       /* create data files no .dat   */
#define ISCOBOL_NO_DAT_EXT      4       /* open expects no .dat ext    */
#define ISCOBOL_ANY_EXT         8       /* open either data form       */
#define ISCOBOL_NEW_DUP_ONE    16       /* new dups start at one       */
#endif

/* TIP/ix support flags -----------------------------------------------*/

/* leave undefined unless needed, or set name of environment string */

#define ISAMDATEXT "ISAMDATEXT" /* Y or N to control .dat ext */


/* threadsafe configuration -------------------------------------- */

/* 0 - disabled
   1 - SunOS
   2 - Solaris
   3 - Windows
   4 - OS/2
   5 - pthreads */

/* please note - if threaded transactions needed, set ISLONGID = 2 */
   
#define ISTHREADED   0

/* the rest are internals ---------------------------------------- */

/* bit flag old features back on, in case things get a bit too fixed */
/* =please contact support@bytedesigns.com if you need any of these= */
/* at this time we know that 16 - natural order EOF fix - is in use */

#define ISBITFLAGS   0                  /* 1: compsquash prune logic */
                                        /* 2: islock lkstate changes */
					/* 4: read CURR closest match */
					/* 8: isgrow duplicates v2 */
				       /* 16: natural order EOF fix */
                                       /* 32: sep, 2006 - history.ref */
                                       /* 64: old isWrCurr by isGoto */
                                      /* 128: allow corrupt records */
                                      /* 256: old pthreads extentions */

#define ISMODE      0666                /* full access to created files */

/* conflicting options checks */

#if( ISLOGGING > 0 && ISADMIN == 0 )
#pragma error /* logging requires the administration facility */
#endif
