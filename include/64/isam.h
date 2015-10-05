/***************************************************************************
 * Licensed Material - Property Of IBM
 *
 *  "Restricted Materials of IBM"
 *
 *  (c) Copyright IBM Corporation 2008 All rights reserved.
 *
 *
 *			   INFORMIX SOFTWARE, INC.
 *
 *			      PROPRIETARY DATA
 *
 *	THIS DOCUMENT CONTAINS TRADE SECRET DATA WHICH IS THE PROPERTY OF 
 *	INFORMIX SOFTWARE, INC.  THIS DOCUMENT IS SUBMITTED TO RECIPIENT IN
 *	CONFIDENCE.  INFORMATION CONTAINED HEREIN MAY NOT BE USED, COPIED OR 
 *	DISCLOSED IN WHOLE OR IN PART EXCEPT AS PERMITTED BY WRITTEN AGREEMENT 
 *	SIGNED BY AN OFFICER OF INFORMIX SOFTWARE, INC.
 *
 *	THIS MATERIAL IS ALSO COPYRIGHTED AS AN UNPUBLISHED WORK UNDER
 *	SECTIONS 104 AND 408 OF TITLE 17 OF THE UNITED STATES CODE. 
 *	UNAUTHORIZED USE, COPYING OR OTHER REPRODUCTION IS PROHIBITED BY LAW.
 *
 *
 *  Title:	isam.h
 *  Sccsid:	@(#)isam.h	9.11	2/23/94  14:31:41
 *  Description:
 *		Header file for programs using C-ISAM.
 *		MLS changes
 *			added defines for ISALL, ISNORECUR, should not
 *			be used for SE, compilation of sql needs these
 *			defines
 *
 ***************************************************************************
 */

/*
 *       C-ISAM version 4.10
 *  Indexed Sequential Access Method
 *  Relational Database Systems, Inc.
 */

#ifndef ISAM_INCL		/* avoid multiple include problems */
#define ISAM_INCL

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__
#include "decimal.h"
#endif


#define CHARTYPE	0
#define DECIMALTYPE	0
#define CHARSIZE	1

#define INTTYPE		1
#define INTSIZE		2

#define LONGTYPE	2
#define LONGSIZE	4

#define DOUBLETYPE	3
#ifndef NOFLOAT
#define DOUBLESIZE	(sizeof(double))
#endif /* NOFLOAT */

#ifndef NOFLOAT
#define FLOATTYPE	4
#define FLOATSIZE	(sizeof(float))
#endif /* NOFLOAT */

#define USERCOLL(x)	((x))

#define COLLATE1	0x10
#define COLLATE2	0x20
#define COLLATE3	0x30
#define COLLATE4	0x40
#define COLLATE5	0x50
#define COLLATE6	0x60
#define COLLATE7	0x70

#define NCHARTYPE       7	/* CHARacter TYPE with localized collation */

#define MAXTYPE		5
#define ISDESC		0x80	/* add to make descending type	*/
#define TYPEMASK	0x7F	/* type mask			*/

#define BYTEMASK  0xFF		/* mask for one byte		*/
#define BYTESHFT  8		/* shift for one byte		*/

#ifndef	ldint
#define ldint(p)	((short)(((p)[0]<<BYTESHFT)+((p)[1]&BYTEMASK)))
#define stint(i,p)	((p)[0]=(char)((i)>>BYTESHFT),(p)[1]=(char)(i))
#endif	

#define ISFIRST		0	/* position to first record	*/
#define ISLAST		1	/* position to last record	*/
#define ISNEXT		2	/* position to next record	*/
#define ISPREV		3	/* position to previous record	*/
#define ISCURR		4	/* position to current record	*/
#define ISEQUAL		5	/* position to equal value	*/
#define ISGREAT		6	/* position to greater value	*/
#define ISGTEQ		7	/* position to >= value		*/


/* isread lock modes */
#define ISLOCK     	0x100	/* record lock			*/
#define ISSKIPLOCK	0x200	/* skip record even if locked	*/
#define ISWAIT		0x400	/* wait for record lock		*/
#define ISLCKW		0x500   /* ISLOCK + ISWAIT              */

/* isstart lock modes */
#define ISKEEPLOCK	0x800	/* keep rec lock in autolk mode	*/

/* ifdef MLS */
	/* isread flag, used for B1 case only, but bits should be reserved */
#define ISNORECUR       0x0000  /* no recursion for catalog read */
/* endif MLS */

/* isopen, isbuild lock modes */
#define ISAUTOLOCK	0x200	/* automatic record lock	*/
#define ISMANULOCK	0x400	/* manual record lock		*/
#define ISEXCLLOCK	0x800	/* exclusive isam file lock	*/


/* isopen, isbuild file types */
#define ISINPUT		0	/* open for input only		*/
#define ISOUTPUT	1	/* open for output only		*/
#define ISINOUT		2	/* open for input and output	*/
#define ISTRANS		4	/* open for transaction proc	*/
#define ISNOLOG		8	/* no loggin for this file	*/
#define ISVARLEN	0x10	/* variable length records	*/
#define ISSPECAUTH	0x20	/* special authority to break rules */
#define ISFIXLEN	0x0	/* (non-flag) fixed length records only	*/

#define ISNOCKLANG 0x80    /* this is a mode flag for isopen and isenter
                            * telling them not to verify the spec. language
                            */

/* ifdef MLS */
#define ISALL       0x0000  /* Open the table at datahi, only
				allowed when PASSMAC is set */
/* endif MLS */

/* audit trail mode parameters */
#define AUDSETNAME	0	/* set new audit trail name	*/
#define AUDGETNAME	1	/* get audit trail name		*/
#define AUDSTART	2	/* start audit trail 		*/
#define AUDSTOP		3	/* stop audit trail 		*/
#define AUDINFO		4	/* audit trail running ?	*/
#define AUDRECVR	5	/* recovering table             */

/*
 * Define MAXKEYSIZE 240 and NPARTS 16 for AF251
 */
#define MAXKEYSIZE	120	/* max number of bytes in key	*/
#define NPARTS		8	/* max number of key parts	*/

struct keypart
    {
    short kp_start;		/* starting byte of key part	*/
    short kp_leng;		/* length in bytes		*/
    short kp_type;		/* type of key part		*/
    };

typedef struct keydesc
    {
    short k_flags;		/* flags			*/
    short k_nparts;		/* number of parts in key	*/
    struct keypart
	k_part[NPARTS];		/* each key part		*/
		    /* the following is for internal use only	*/
    short k_len;		/* length of whole key		*/
    int  k_rootnode;		/* pointer to rootnode		*/
    } keydesc_t;
#define k_start   k_part[0].kp_start
#define k_leng    k_part[0].kp_leng
#define k_type    k_part[0].kp_type

#define ISNODUPS  000		/* no duplicates allowed	*/
#define ISDUPS	  001		/* duplicates allowed		*/
#define DCOMPRESS 002		/* duplicate compression	*/
#define LCOMPRESS 004		/* leading compression		*/
#define TCOMPRESS 010		/* trailing compression		*/
#define COMPRESS  016		/* all compression		*/
#define ISCLUSTER 020		/* index is a cluster one       */

struct dictinfo
    {
    short di_nkeys;		/* number of keys defined (msb set for VARLEN)*/
    short di_recsize;		/* (maximum) data record size	*/
    short di_idxsize;		/* index record size		*/
    int  di_nrecords;		/* number of records in file	*/
    };

#define EDUPL	  100		/* duplicate record	*/
#define ENOTOPEN  101		/* file not open	*/
#define EBADARG   102		/* illegal argument	*/
#define EBADKEY   103		/* illegal key desc	*/
#define ETOOMANY  104		/* too many files open	*/
#define EBADFILE  105		/* bad isam file format	*/
#define ENOTEXCL  106		/* non-exclusive access	*/
#define ELOCKED   107		/* record locked	*/
#define EKEXISTS  108		/* key already exists	*/
#define EPRIMKEY  109		/* is primary key	*/
#define EENDFILE  110		/* end/begin of file	*/
#define ENOREC    111		/* no record found	*/
#define ENOCURR   112		/* no current record	*/
#define EFLOCKED  113		/* file locked		*/
#define EFNAME    114		/* file name too long	*/
#define ENOLOK    115		/* can't create lock file */
#define EBADMEM   116		/* can't alloc memory	*/
#define EBADCOLL  117		/* bad custom collating	*/
#define ELOGREAD  118		/* cannot read log rec  */
#define EBADLOG   119		/* bad log record	*/
#define ELOGOPEN  120		/* cannot open log file	*/
#define ELOGWRIT  121		/* cannot write log rec */
#define ENOTRANS  122		/* no transaction	*/
#define ENOSHMEM  123		/* no shared memory	*/
#define ENOBEGIN  124		/* no begin work yet	*/
#define ENONFS    125		/* can't use nfs 	*/
#define EBADROWID 126		/* reserved for future use */
#define ENOPRIM   127		/* no primary key	*/
#define ENOLOG    128		/* no logging		*/
#define EUSER     129		/* reserved for future use */
#define ENODBS    130		/* reserved for future use */
#define ENOFREE   131		/* no free disk space	*/
#define EROWSIZE  132		/* row size too big	*/
#define EAUDIT	  133		/* audit trail exists   */
#define ENOLOCKS  134		/* no more locks	*/
#define ENOPARTN  135		/* reserved for future use */
#define ENOEXTN   136		/* reserved for future use */
#define EOVCHUNK  137		/* reserved for future use */
#define EOVDBS    138		/* reserved for future use */
#define EOVLOG    139		/* reserved for future use */
#define EGBLSECT  140		/* global section disallowing access - VMS */
#define EOVPARTN  141		/* reserved for future use */
#define EOVPPAGE  142		/* reserved for future use */
#define EDEADLOK  143		/* reserved for future use */
#define EKLOCKED  144		/* reserved for future use */
#define ENOMIRROR 145           /* reserved for future use */
#define EDISKMODE 146           /* reserved for future use */
#define EARCHBUSY 147		/* reserved for future use */
#define ENEMPTY	  148		/* reserved for future use */
#define EDEADDEM  149		/* reserved for future use */
#define EDEMO	  150		/* demo limits have been exceeded */
#define EBADVCLEN 151		/* reserved for future use */
#define EBADRMSG  152		/* reserved for future use */
#define ENOMANU   153		/* must be in ISMANULOCK mode */
#define EDEADTIME 154           /* lock timeout expired */
#define EPMCHKBAD 155           /* primary and mirror chunk bad */
#define EBADSHMEM 156           /* can't attach to shared memory*/
#define EINTERUPT 157           /* interrupted isam call */
#define ENOSMI    158           /* operation disallowed on SMI pseudo table */
#define ECOL_SPEC 159		/* invalid collation specifier */
#define ENLS_LANG ECOL_SPEC	/* retained for backward compatibility */
#define EB_BUSY	  160		/* reserved for future use */
#define EB_NOOPEN 161		/* reserved for future use */
#define EB_NOBS	  162		/* reserved for future use */
#define EB_PAGE	  163		/* reserved for future use */
#define EB_STAMP  164		/* reserved for future use */
#define EB_NOCOL  165		/* reserved for future use */
#define EB_FULL   166		/* reserved for future use */
#define EB_PSIZE  167		/* reserved for future use */
#define EB_ARCH   168		/* reserved for future use */
#define EB_CHKNLOG 169		/* reserved for future use */
#define EB_IUBS	  170		/* reserved for future use */
#define EBADFORMAT 171		/* locking or NODESIZE change */
#define ERECFULL      21501     /* Record overflow  */
#define ELOGINCOMPAT  21502     /* Incompatiable log file */
#define ENODEFULL     26061     /* NODE overflow */
#define ETREEFULL     26062     /* BTREE limit exceeded */


/* Dismountable media blobs errors */
#define EB_SFULL  180		/* reserved for future use */
#define EB_NOSUBSYS  181	/* reserved for future use */
#define EB_DUPBS  182		/* reserved for future use */
/* Shared Memory errors */
#define ES_PROCDEFS	21584	/* can't open config file */
#define ES_IILLVAL	21586	/* illegal config file value */
#define ES_ICONFIG	21595	/* bad config parameter */
#define ES_ILLUSRS	21596	/* illegal number of users */
#define ES_ILLLCKS	21597	/* illegal number of locks */
#define ES_ILLFILE	21598	/* illegal number of files */
#define ES_ILLBUFF	21599	/* illegal number of buffs */
#define ES_SHMGET	25501	/* shmget error */
#define ES_SHMCTL	25502	/* shmctl error */
#define ES_SEMGET	25503	/* semget error */
#define ES_SEMCTL	25504	/* semctl error */

/*
 * For system call errors
 *   iserrno = errno (system error code 1-99)
 *   iserrio = IO_call + IO_file
 *		IO_call  = what system call
 *		IO_file  = which file caused error
 */

#define IO_OPEN	  0x10		/* open()	*/
#define IO_CREA	  0x20		/* creat()	*/
#define IO_SEEK	  0x30		/* lseek()	*/
#define IO_READ	  0x40		/* read()	*/
#define IO_WRIT	  0x50		/* write()	*/
#define IO_LOCK	  0x60		/* locking()	*/
#define IO_IOCTL  0x70		/* ioctl()	*/

#define IO_IDX	  0x01		/* index file	*/
#define IO_DAT	  0x02		/* data file	*/
#define IO_AUD	  0x03		/* audit file	*/
#define IO_LOK	  0x04		/* lock file	*/
#define IO_SEM	  0x05		/* semaphore file */

/* 
 * NOSHARE was needed as an attribute for global variables on VMS systems
 * It has been left here to make sure that it is defined for the
 * plethera of scattered references.
 */
#define NOSHARE

extern  int iserrno;		/* isam error return code	*/
extern  int iserrio;		/* system call error code	*/
extern  int  isrecnum;		/* record number of last call	*/
extern  int isreclen;		/* actual record length, or	*/
				/* minimum (isbuild, isindexinfo) */
				/* or maximum (isopen )		*/
extern  char isstat1;		/* cobol status characters	*/
extern  char isstat2;
extern  char isstat3;
extern  char isstat4;
extern  char *isversnumber;	/* C-ISAM version number	*/
extern  char *iscopyright;	/* RDS copyright		*/
extern  char *isserial;		/* C-ISAM software serial number */
extern  int  issingleuser;	/* set for single user access	*/
extern  int  is_nerr;		/* highest C-ISAM error code	*/
extern  char *is_errlist[];	/* C-ISAM error messages	*/
/*  error message usage:
 *	if (iserrno >= 100 && iserrno < is_nerr)
 *	    printf("ISAM error %d: %s\n", iserrno, is_errlist[iserrno-100]);
 */

struct audhead
    {
    char au_type[2];		/* audit record type aa,dd,rr,ww*/
    char au_time[4];		/* audit date-time		*/
    char au_procid[2];		/* process id number		*/
    char au_userid[2];		/* user id number		*/
    char au_recnum[4];		/* record number		*/
    char au_reclen[2];		/* audit record length beyond header */
    };
#define AUDHEADSIZE   14	/* num of bytes in audit header	*/
#define VAUDHEADSIZE  16	/* VARLEN num of bytes in audit header	*/

#ifdef __STDC__
/* 
** prototypes for file manipulation functions 
*/
int    isaddindex(int isfd, struct keydesc *keydesc);
int    isaudit(int isfd, char *filename, int mode);
int    isbegin();
int    isbuild(char *filename, int reclen, struct keydesc *keydesc, int mode);
int    iscleanup();
int    isclose(int isfd);
int    iscluster(int isfd, struct keydesc *keydesc);
int    iscommit();
int    isdelcurr(int isfd);
int    isdelete(int isfd, char *record);
int    isdelindex(int isfd, struct keydesc *keydesc);
int    isdelrec(int isfd, int  recnum);
int    iserase(char *filename);
int    isflush(int isfd);
int    isindexinfo(int isfd, void *buffer, int number);
int    isdictinfo(int isfd, struct dictinfo *buffer);
int    iskeyinfo(int isfd, struct keydesc *buffer, int number);
void   islangchk();
char  *islanginfo(char *filename);
int    islock(int isfd);
int    islogclose();
int    islogopen(char *logname);
int    isnlsversion(char *filename);
int    isglsversion(char *filename);
void   isnolangchk();
int    isopen(char *filename, int mode);
int    isread(int isfd, char *record, int mode);
int    isrecover();
int    isrelease(int isfd);
int    isrename(char *oldname, char *newname);
int    isrewcurr(int isfd, char *record);
int    isrewrec(int isfd, int  recnum, char *record);
int    isrewrite(int isfd, char *record);
int    isrollback();
int    issetunique(int isfd, int  uniqueid);
int    isstart(int isfd, struct keydesc *keydesc, 
	       int length, char *record, int mode);
int    isuniqueid(int isfd, int  *uniqueid);
int    isunlock(int isfd);
int    iswrcurr(int isfd, char *record);
int    iswrite(int isfd, char *record);

/*
** prototypes for format-conversion and manipulation fuctions 
*/
void   ldchar(char *source, int length, char *destination);
double lddbl(char *location);
double lddblnull(char *location, short *nullflag);
int    lddecimal(char *location, int length, dec_t *destination);
double ldfloat(char *location);
float ldfltnull(char *location, short *nullflag);
int    ldlong(char *location);
void   stchar(char *source, char *destination, int length);
/* nullflag is type int to preserve old param passing behavior */
void   stdblnull(double source, char *destination, int nullflag);
void   stdecimal(dec_t *source, char *destination, int length);
/* nullflag is type int to preserve old param passing behavior */
void   stfltnull(double source, char *destination, int nullflag);
#ifndef stdbl
void   stdbl(double source, char *destination);
#endif
#ifndef stfloat
void   stfloat(double source, char *destination);
#endif
#ifndef stlong
void   stlong(int  source, char *destination);
#endif

#else

extern char *islanginfo();        /* locale used for collation   */

#ifndef ldlong
int  ldlong();
#endif
 
#ifndef NOFLOAT
#ifndef ldfloat
double  ldfloat();
#endif
#ifndef lddbl
double  lddbl();
#endif
float ldfltnull();
double lddblnull();
#endif

#endif /*__STDC__*/

/* 
 * Each tuple in an exchange buffer or an RSAM set oriented
 * read buffer is prepended with a header that includes a
 * row ID and fragment ID.
 */
typedef struct tuple_hdr
    {
    short	 th_status;          /* status flags for the tuple          */
    int          th_rowid;           /* ROW ID for a buffered tuple         */
    int          th_fragid;          /* FRAGMENT ID for a buffered tuple    */
    int          th_keyhash;         /* hash value for a given key          */
    int          th_tuphash;         /* hash value for the entire tuple     */
    void        *th_vptr;            /* pointer to operator specific info   */
	struct tuple_hdr *th_left_ptr;   /* pointer to the left child; used for
                                      * AVL implementation; th_vptr is used
									  * as the right ptr                    */
    struct tuple_hdr *th_next_tuple; /* next tuple link                     */
    } tuple_hdr_t;

/* bit positions for th_status */
#define BUFT_LOCKED	1 

/*
 * Common header for all exchange buffers.
 *
 * Various flavorsof record buffers can be accomodated via 
 * subcomponent specific headers that can be interspersed 
 * between this common header and the actual tuple space.
 * That is, since buft_hdr_t contains a pointer to the 
 * buffer's data space, it can point "around" any other
 * intervening headers, i.e. can point to a data space 
 * that is noncontiguous with this header.
 */
typedef struct buft_hdr
    {
    char        *bh_data_space;       /* pointer to the tuple data space    */
    int          bh_data_length;      /* total bytes in the data space      */
    int          bh_tuple_length;     /* including tuple_hdr_t & pad        */
    int          bh_next_read;        /* next tuple to read (FIFO policy)   */
    int          bh_tuple_count;      /* # of tuples now in data space      */
    int          bh_max_tuple_count;  /* max # of tuples for the data space */
    int          bh_packet_num;       /* global number of this packet       */
    int          bh_xchg_num;         /* exchange number of this packet     */
    int          bh_prodby;           /* producer of this packet            */
    int          bh_sentto;           /* destination                        */
    int          bh_status;           /* error codes for this packet        */
    void        *bh_vptr;             /* pointer to operator defined info   */
    tuple_hdr_t *bh_next_tuple;       /* user defined pointer               */
    struct buft_hdr *bh_next_buffer;  /* link to next buffer                */
    } buft_hdr_t; 


#ifdef __cplusplus
}
#endif

#endif /* ISAM_INCL */
