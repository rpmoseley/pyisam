/*
-----------------------------------------------------------------------------
standard interface header
-----------------------------------------------------------------------------
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *             Worldwide Copyright (c) Byte Designs Ltd (2024)             *
 *          Version 7.2 (build 2024/11/30) (expire cobol shared)           *
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


#include <isconfig.h>                   /* library configuration */
#include <isintstd.h>			/* see read/stdint.ref */

/* note ----------------------------------------------------------- *
   the following is pulled from a common source in the disam master,
   and is *identical* from here through #endif ISAUTOLOCK below, in
   both isbase.h and iswrap.h
   ---------------------------------------------------------------- */

#ifndef ISAUTOLOCK			/* avoid multiple inclusions */

/* structures ----------------------------------------------------- */

struct keypart                          /* key component description */
  {
  short kp_start;                       /* offset within data record */
  short kp_leng;                        /* physical length */
  short kp_type;                        /* isam key type */
  };

struct keydesc                          /* full key description */
  {
  short k_flags;                        /* key characteristics */
  short k_nparts;                       /* number of parts in key */
  struct keypart k_part[ISMAXPARTS];    /* description of each part */
  short k_len;                          /* complete key length */
  uint32_t k_rootnode;                  /* root node record number */
  };
  
struct dictinfo                         /* information about file */
  {
  short di_nkeys;                       /* number of indexes */
  short di_recsize;                     /* data record (max) length */
  short di_idxsize;                     /* index record block size */
  uint32_t di_nrecords;                 /* current record count */
  };

struct audhead                          /* audit record header format */
  {
  char au_type[2];                      /* record type (aa/dd/rr/ww) */
  char au_time[4];                      /* date and time */
#if( ISLONGID == 1 )
  char au_procid[4];                    /* process id */
  char au_userid[4];                    /* user id */
#else
  char au_procid[2];                    /* process id */
  char au_userid[2];                    /* user id */
#endif
  char au_recnum[4];                    /* record number */
  char au_reclen[2];                    /* record length if variable */
  };

#define AUDHEADSIZE     14
#define VAUDHEADSIZE    16

/* defines ------------------------------------------------------- */

/* file handling modes */

#define ISBUILDMODE( B, D, M ) ( ( ( (B) & 0xFF ) << 24 ) + ( ( (D) & 0xFF ) << 16 ) + ( (M) & 0xFF ) )


/* file handling modes */

#define ISINPUT         0x00            /* input only */
#define ISOUTPUT        0x01            /* output only */
#define ISINOUT         0x02            /* input and output */
#define ISTRANS         0x04            /* transaction processing */
#define ISNOLOG         0x08            /* turn off logging */
#define ISFIXLEN        0x00            /* dummy */
#define ISVARLEN        0x10            /* variable length data */
#define ISVARCMP        0x30            /* compressed varlen */
#define ISSYNCWR        0x40            /* synchronous writes */
#define ISMASKED        0x80            /* masking active */
#define ISNOCARE      0x8000            /* set mode to match file */

/* locking methods */

#define ISRDONLY        0x100           /* read only - no locking */
#define ISAUTOLOCK      0x200           /* automatic lock on read */
#define ISMANULOCK      0x400           /* manual locking */
#define ISEXCLLOCK      0x800           /* exclusive access to file */
#if( SEMILOCK == 1 )
#define ISSEMILOCK     0x1000		/* C-ISAM compatible semi-lock */
#endif

/* key types */
 
#define CHARTYPE        0               /* array of bytes/characters */
#define DECIMALTYPE     0               /* handled as per char type */
#define INTTYPE         1               /* two byte (short) integer */
#define LONGTYPE        2               /* four byte (long) integer */
#define DOUBLETYPE      3               /* ieee double floating point */
#define FLOATTYPE       4               /* ieee single floating point */
#define MINTTYPE        5               /* machine (native) short */
#define MLONGTYPE       6               /* machine (native) long */
#define STRINGTYPE      7               /* null terminated byte string */

#define ISDESC          0x80            /* add for descending order */

/* key atomic sizes */

#define CHARSIZE        1
#define INTSIZE         2
#define LONGSIZE        4
#define DOUBLESIZE      8
#define FLOATSIZE       4
#define MINTSIZE        2
#define MLONGSIZE       4
#define STRINGSIZE      1

/* shortcuts for single part keys */

#define k_start   k_part[0].kp_start
#define k_leng    k_part[0].kp_leng
#define k_type    k_part[0].kp_type

#define NPARTS    ISMAXPARTS

/* index management flags */

#define ISNODUPS  0x00                  /* no duplicates permitted */
#define ISDUPS    0x01                  /* duplicates permitted */
#define DCOMPRESS 0x02                  /* compress duplicates */
#define LCOMPRESS 0x04                  /* leading compression */
#define TCOMPRESS 0x08                  /* trailing compression */
#define COMPRESS  0x0E                  /* full compression */
#define TNULL     0x10			/* compress trailing nulls */
#define NULLKEY   0x20                  /* null key masking */

#define ISMASK(I) ( 1L << ((I)-1) )     /* index masking bit flags */

/* access control */

#define ISFIRST         0               /* first record */
#define ISLAST          1               /* last record */
#define ISNEXT          2               /* next record */
#define ISPREV          3               /* previous record */
#define ISCURR          4               /* current record */
#define ISEQUAL         5               /* find match */
#define ISGREAT         6               /* greater than */
#define ISGTEQ          7               /* greater than or equal */

/* record locking */

#define ISLOCK          0x100           /* lock record or fail */
#if( ISCISAM != 0 )
# define ISSKIPLOCK     0x200           /* skip record if locked */
#endif
#define ISWAIT          0x400           /* wait until free */
#define ISLCKW          0x500           /* wait and lock */
#define ISKEEPLOCK	0x800		/* isstart keep lock */

/* audit trace control */

#define AUDSETNAME      0               /* set audit file name */
#define AUDGETNAME      1               /* get audit file name */
#define AUDSTART        2               /* begin audit logging */
#define AUDSTOP         3               /* stop audit logging */
#define AUDINFO         4               /* logging status */

#define USERINFOSIZE	10		/* userinfo pad size */

/* isam specific error codes (iserrno) */

#define EDUPL     ( ISERRBASE +  0 )    /* illegal duplicate */
#define ENOTOPEN  ( ISERRBASE +  1 )    /* file not open */
#define EBADARG   ( ISERRBASE +  2 )    /* illegal argument */
#define EBADKEY   ( ISERRBASE +  3 )    /* illegal key description */
#define ETOOMANY  ( ISERRBASE +  4 )    /* out of isam file handles */
#define EBADFILE  ( ISERRBASE +  5 )    /* isam file is corrupt */
#define ENOTEXCL  ( ISERRBASE +  6 )    /* can't get exclusive access */
#define ELOCKED   ( ISERRBASE +  7 )    /* record is locked */
#define EKEXISTS  ( ISERRBASE +  8 )    /* index already defined */
#define EPRIMKEY  ( ISERRBASE +  9 )    /* illegal primary key operation */
#define EENDFILE  ( ISERRBASE + 10 )    /* start or end file reached */
#define ENOREC    ( ISERRBASE + 11 )    /* record not found */
#define ENOCURR   ( ISERRBASE + 12 )    /* no current record */
#define EFLOCKED  ( ISERRBASE + 13 )    /* file is locked */
#define EFNAME    ( ISERRBASE + 14 )    /* file name is too long */
#define ENOLOK    ( ISERRBASE + 15 )    /* not used */
#define EBADMEM   ( ISERRBASE + 16 )    /* can't allocate memory */
#define EBADCOLL  ( ISERRBASE + 17 )    /* not used, see next */
#define EBADIO    ( ISERRBASE + 17 )    /* trouble in IO */
#define ELOGREAD  ( ISERRBASE + 18 )    /* error reading log file */
#define EBADLOG   ( ISERRBASE + 19 )    /* log file is corrupt */
#define ELOGOPEN  ( ISERRBASE + 20 )    /* unable to open log */
#define ELOGWRIT  ( ISERRBASE + 21 )    /* unable to write log */
#define ENOTRANS  ( ISERRBASE + 22 )    /* transaction not found */
#define ENOSHMEM  ( ISERRBASE + 23 )    /* out of shared memory */
#define ENOBEGIN  ( ISERRBASE + 24 )    /* no current transaction */
#define ENONFS    ( ISERRBASE + 25 )    /* not used */
#define EBADROWID ( ISERRBASE + 26 )    /* not used */
#define ENOPRIM   ( ISERRBASE + 27 )    /* no primary key */
#define ENOLOG    ( ISERRBASE + 28 )    /* logging not allowed */
#define EUSER     ( ISERRBASE + 29 )    /* too many users */
#define ENODBS    ( ISERRBASE + 30 )    /* not used */
#define ENOFREE   ( ISERRBASE + 31 )    /* not used */
#define EROWSIZE  ( ISERRBASE + 32 )    /* varlen record too big */
#define EAUDIT    ( ISERRBASE + 33 )    /* existing audit trail */
#define ENOLOCKS  ( ISERRBASE + 34 )    /* out of room in lock table */
#define EEXPIRED  ( ISERRBASE + 35 )    /* evaluation library has expired */
#define ENETORDER ( ISERRBASE + 37 )    /* server has wrong endian order */
#define EREPAIR   ( ISERRBASE + 38 )    /* auto repair in progress */
#define EBADADDR  ( ISERRBASE + 39 )    /* bad network address */
#define ENOSCHEMA ( ISERRBASE + 40 )	/* no schema defined */
#define EEXSCHEMA ( ISERRBASE + 41 )	/* schema already defined */
#define ENOTAVAIL ( ISERRBASE + 42 )	/* function not available */

/* on system error - iserrio = IO_call + IO_file */

#define IO_OPEN   0x10                  /* open */
#define IO_CREA   0x20                  /* create */
#define IO_SEEK   0x30                  /* seek */
#define IO_READ   0x40                  /* read */
#define IO_WRIT   0x50                  /* write */
#define IO_LOCK   0x60                  /* lock */
#define IO_OTHER  0x70                  /* other */

#define IO_IDX    0x01                  /* index file */
#define IO_DAT    0x02                  /* data file */
#define IO_AUD    0x03                  /* audit trace */
#define IO_RAW    0x04			/* any other file */
#define IO_REP    0x05			/* auto repair file */
#define IO_LOG    0x06                  /* transaction log */
#define IO_LCK    0x07                  /* offline lock file */

#define IO_LOK    0x04                  /* cisam, not used */
#define IO_SEM    0x05                  /* cisam, not used */

/* cobol isstat4 values --------------------------------------------*/

#define STATNULL       '/'              /* empty isstat4 value     */
#define STAT1          '1'
#define STAT2          '2'
#define STAT3          '3'
#define STAT4          '4'
#define STAT5          '5'
#define STAT6          '6'
#define STAT7          '7'
#define STAT8          '8'
#define STAT9          '9'
#define STAT0          '0'

/* self check facility ------------------------------------------- */

#define IC_DATREAD      0               /* data file read error */
#define IC_BADDATA      1               /* data record corrupt */
#define IC_DATFREE      2               /* data free list is corrupt */

#define IC_IDXREAD      3               /* index file read error */
#define IC_ORDER        4               /* key out of order */
#define IC_COUNT        5               /* index count mismatch */
#define IC_BADMAGIC     6               /* bad index node magic number */
#define IC_MATCH        7               /* index/data mismatch */
#define IC_SANITY	8               /* compiler sanity check */
#define IC_DUPSEQ	9		/* bad duplicate sequence */

#if( ISVARIABLE )

#define IC_VLHASHZERO   11              /* hash table zero loaded */
#define IC_VLMISFILE    12              /* node misfiled in hash table */
#define IC_VLHASHLINK   13              /* mangled link in hash list */
#define IC_VLMISSING    14              /* nodes missing or unaccounted */
#define IC_VLBADDATA    15              /* data length mismatch */
  
struct IsVarStat                        /* variable length statistics */
  {
  uint32_t hashcount;
  uint32_t hashspace;
  uint32_t fullcount;
  uint32_t fullspace;
  uint32_t filecount;
  uint32_t filespace;
  };
  
#endif 

#endif /*ISAUTOLOCK*/

/* ---------------------------------------------------------------- *
   above is pulled from common source, see #ifdef ISAUTOLOCK above.
   ---------------------------------------------------------------- */


/* useful defines ------------------------------------------------ */

#define SUCCESS     0
#define ISCLOSED   -1
#define NPARTS     ISMAXPARTS

/* custom key types ---------------------------------------------- */

#if( ISCUSTOM == 1 )
# include <iscustom.h>                   /* custom keys */
#endif

/* function prototypes ------------------------------------------- */

#if( ISCISAM != 0 )
#define isvclose( A ) { return( 0 ); }
#endif

#if( ISDECLARE )
# define ISD3(s) s
#else
# define ISD3(s) ()
#endif

#if( ISDATAVOID )
# define ISDD void
#else
# define ISDD char
#endif

#if( ISCONSTCHAR )
# define ISCC const char
#else
# define ISCC char
#endif

#ifdef ISEXTERN
#undef ISEXTERN
#endif

#if( ISDYNAMIC == 0 || ISDYNAMIC == 1 || ISDYNAMIC == 2 )
# define ISEXTERN extern
# if( defined ISINTERNAL )
#  define ISD1 
#  define ISD2
# else
#  define ISD1 extern
#  define ISD2
# endif
#endif

#if( ISDYNAMIC == 3 )
# define ISEXTERN extern
# if( defined ISINTERNAL )
#  define ISD2 __far __pascal __export
# else
#  define ISD2 __far __pascal
# endif
# define ISD1 extern
#endif

#if( ISDYNAMIC == 4 )
# if( defined ISINTERNAL )
#  define ISD1 __declspec( dllexport )
# else
#  define ISD1 extern __declspec( dllimport )
# endif
# define ISD2
# define ISEXTERN ISD1
#endif

#ifdef __cplusplus
extern "C" {
#endif




# ifdef ISEXPIRE
ISEXTERN ISCC * ISD2 isevaluation;
# endif
ISEXTERN ISCC * ISD2 iscopyright;
ISEXTERN ISCC * ISD2 isversnumber;
ISEXTERN ISCC * ISD2 isverstring;
ISEXTERN ISCC * ISD2 is_CheckMsg[16];
ISEXTERN ISCC * ISD2 is_errlist[44];
ISEXTERN int ISD2 is_nerr;

#if( ! defined STDGLOBALS )
 
#if( ISCISAM > 0 )
ISEXTERN long ISD2 isrecnum;
#else
ISEXTERN uint32_t ISD2 isrecnum;
#endif
ISEXTERN int ISD2 isreclen;
ISEXTERN int ISD2 iserrno;
ISEXTERN int ISD2 iserrio;
ISEXTERN int32_t ISD2 ispid;

#if( ISCOBOL & ISCOBOL_STATS )
ISEXTERN char ISD2 isstat1;
ISEXTERN char ISD2 isstat2;
ISEXTERN char ISD2 isstat3;
ISEXTERN char ISD2 isstat4;
#endif

#endif

/* stdbuild.c */
ISD1 int ISD2 isbuild ISD3((char *name, int len, struct keydesc *kdsc, int mode));
ISD1 int ISD2 isvbuild ISD3((char *name, int len, int max, struct keydesc *kdsc, int mode));
ISD1 int ISD2 isaddindex ISD3((int isfd, struct keydesc *kdsc));
ISD1 int ISD2 isdelindex ISD3((int isfd, struct keydesc *kdsc));
ISD1 int ISD2 iscluster ISD3((int isfd, struct keydesc *kdsc));
ISD1 int ISD2 isclone ISD3((int isfd, char *name));
ISD1 int ISD2 iscopy ISD3((int dfd, int sfd, struct keydesc *kdsc));

/* stdextra.c */
ISD1 int ISD2 iserase ISD3((char *name));
ISD1 int ISD2 isrename ISD3((char *oname, char *nname));
ISD1 int ISD2 isflush ISD3((int isfd));
ISD1 int ISD2 issetunique ISD3((int isfd, uint32_t value));
ISD1 int ISD2 isuniqueid ISD3((int isfd, uint32_t *dest));
ISD1 int ISD2 isprecious ISD3((int isfd, int flag));
ISD1 int ISD2 islockcheck ISD3((int isfd, int flag));
ISD1 char * ISD2 isseekey ISD3((int isfd));
ISD1 void ISD2 isfatal ISD3((void(*call)(char *)));
ISD1 int ISD2 isthreaded ISD3((void));

#if( ISDEBUG > 0 )
ISD1 void ISD2 isdebug ISD3(( int isfd, int level, char *fmt, ... ));
#endif

#if( ISSCHEMA > 0 )
ISD1 int ISD2 isldschema ISD3((int isfd, char *path));
ISD1 int ISD2 isdpschema ISD3((int isfd));
ISD1 int ISD2 isstschema ISD3((int isfd, char *loadfile));
ISD1 int ISD2 isrmschema ISD3((int isfd));
#endif

#if( ISREPAIR == 1 )
ISD1 int ISD2 isstartup ISD3((void));
#endif

/* stdinfo.c */
ISD1 int ISD2 isindexinfo ISD3((int isfd, struct keydesc *dest, int idx));
ISD1 int ISD2 isisaminfo ISD3((int isfd, struct dictinfo *dest));
ISD1 int ISD2 isuserinfo ISD3((int isfd, int mode, char *pad));
ISD1 char * ISD2 isdi_name ISD3((int isfd));
ISD1 int ISD2 isdi_datlen ISD3((int isfd));
ISD1 int ISD2 isdi_curidx ISD3((int isfd));
ISD1 int ISD2 isdi_idxfd ISD3((int isfd));
ISD1 int ISD2 isdi_datfd ISD3((int isfd));
ISD1 struct keydesc * ISD2 isdi_kdsc ISD3((int isfd));

/* stdlast.c */
ISD1 int ISD2 islastrec ISD3((int isfd, uint32_t *last));
ISD1 int ISD2 isgetlastrec ISD3((int isfd, uint32_t *last));
ISD1 int ISD2 issetlastrec ISD3((int isfd, uint32_t last));

/* stdlock.c */
ISD1 int ISD2 islock ISD3((int isfd));
ISD1 int ISD2 isunlock ISD3((int isfd));
ISD1 int ISD2 isrelease ISD3((int isfd));
ISD1 int ISD2 isrelrec ISD3((int isfd, uint32_t recnum));
ISD1 int ISD2 isrelcurr ISD3((int isfd));

/* stdread.c */
ISD1 int ISD2 isopen ISD3((char *name, int mode));
ISD1 int ISD2 isgetmode ISD3((int isfd, int *mode));
ISD1 int ISD2 issetmode ISD3((int isfd, int mode));
ISD1 int ISD2 isclose ISD3((int isfd));
ISD1 int ISD2 iscleanup ISD3((void));
ISD1 int ISD2 isstart ISD3((int isfd, struct keydesc *key, int len, ISDD *data, int mode));
ISD1 int ISD2 isread ISD3((int isfd, ISDD *data, int mode));
#if( ISCONFIG && ISCONFIG_ISINDEX_LOCATE )
ISD1 int ISD2 isindex ISD3((int isfd, int idx, int locate));
#else
ISD1 int ISD2 isindex ISD3((int isfd, int idx ));
#endif
ISD1 int ISD2 isgoto ISD3((int isfd, uint32_t recnum));
ISD1 int ISD2 ispush ISD3((int isfd, int *idx, uint32_t *rec));
ISD1 int ISD2 ispop ISD3((int isfd, int idx, uint32_t rec));
ISD1 int ISD2 isdata ISD3((int isfd, ISDD *pad, uint32_t recnum));
#if( ISCISAM > 0 )
ISD1 long * ISD2 is_recnum ISD3((int isfd));
#else
ISD1 uint32_t * ISD2 is_recnum ISD3((int isfd));
#endif
ISD1 int * ISD2 is_reclen ISD3((int isfd));
ISD1 char * ISD2 is_error ISD3((int isfd));
ISD1 int * ISD2 is_errno ISD3((int isfd));
ISD1 int * ISD2 is_errio ISD3((int isfd));

/* stdtrans.c */
ISD1 int ISD2 islogopen ISD3((char *name));
ISD1 int ISD2 islogclose ISD3((void));
ISD1 int ISD2 isbegin ISD3((void));
ISD1 int ISD2 iscommit ISD3((void));
ISD1 int ISD2 isrollback ISD3((void));
ISD1 int ISD2 isrecover ISD3((void));
ISD1 int ISD2 issusplog ISD3((void));
ISD1 int ISD2 isresumlog ISD3((void));
ISD1 int ISD2 istxnid ISD3((int value));
ISD1 int ISD2 isaudit ISD3((int isfd, char *pad, int mode ));

/* stdwrite.c */
ISD1 int ISD2 iswrite ISD3((int isfd, ISDD *data));
ISD1 int ISD2 iswrlock ISD3((int isfd, ISDD *data));
ISD1 int ISD2 iswrcurr ISD3((int isfd, ISDD *data));
ISD1 int ISD2 isdelete ISD3((int isfd, ISDD *data));
ISD1 int ISD2 isdelrec ISD3((int isfd, uint32_t recnum));
ISD1 int ISD2 isdelcurr ISD3((int isfd));
ISD1 int ISD2 isrewrite ISD3((int isfd, ISDD *data));
ISD1 int ISD2 isrewrec ISD3((int isfd, uint32_t recnum, ISDD *data));
ISD1 int ISD2 isrewcurr ISD3((int isfd, ISDD *data));
ISD1 int ISD2 isrewnxt ISD3((int isfd, ISDD *data));
ISD1 uint32_t ISD2 isgetmask ISD3((int isfd));
ISD1 int ISD2 issetmask ISD3((int isfd, uint32_t mask));

/* stdcheck.c */
ISD1 int ISD2 ischeckdata ISD3((int fd));
ISD1 int ISD2 ischeckindex ISD3((int fd, int idx));
#if( ISVARIABLE )
ISD1 int ISD2 ischeckvarlen ISD3((int fd, struct IsVarStat *stats));
#endif

/* stdmif.c */
ISD1 int ISD2 ldint ISD3((char *pad));
ISD1 int ISD2 stint ISD3((int value, char *pad));
ISD1 int32_t ISD2 ldlong ISD3((char *pad));
ISD1 int ISD2 stlong ISD3((int32_t value, char *pad));
ISD1 float ISD2 ldfloat ISD3((char *pad));
ISD1 int ISD2 stfloat ISD3((float value, char *pad));
ISD1 double ISD2 lddbl ISD3((char *pad));
ISD1 int ISD2 stdbl ISD3((double value, char *pad));

ISD1 int ISD2 stchar ISD3((char *pad, char *str, int len));
ISD1 int ISD2 ldchar ISD3((char *pad, int len, char *str));
ISD1 int ISD2 stfltnull ISD3((float value, char *pad, short null));
ISD1 float ISD2 ldfltnull ISD3((char *pad, short *null));
ISD1 int ISD2 stdblnull ISD3((double value, char *pad, short null));
ISD1 double ISD2 lddblnull ISD3((char *pad, short *null));

#ifdef __cplusplus
};
#endif

#if( defined ISINTERNAL )
#else
# undef ISD1
# undef ISD2
# undef ISD3
# undef ISDD
#endif

/* end header ---------------------------------------------------- */
