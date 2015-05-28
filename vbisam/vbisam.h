/*
 * Title:	vbisam.h
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	17Nov2003
 * Description:
 *	This is the header that defines all the various structures et al for
 *	the VBISAM library.
 * Version:
 *	$Id: vbisam.h,v 1.7 2004/06/11 22:16:16 trev_vb Exp $
 * Modification History:
 *	$Log: vbisam.h,v $
 *	Revision 1.7  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.6  2004/01/06 14:31:59  trev_vb
 *	TvB 06Jan2004 Added in VARLEN processing (In a fairly unstable sorta way)
 *	
 *	Revision 1.5  2004/01/05 07:36:17  trev_vb
 *	TvB 05Feb2002 Added licensing et al as Johann v. N. noted I'd overlooked it
 *	
 *	Revision 1.4  2004/01/03 02:28:48  trev_vb
 *	TvB 02Jan2004 WAY too many changes to enumerate!
 *	TvB 02Jan2003 Transaction processing done (excluding iscluster)
 *	
 *	Revision 1.3  2003/12/23 03:08:56  trev_vb
 *	TvB 22Dec2003 Minor compilation glitch 'fixes'
 *	
 *	Revision 1.2  2003/12/22 04:50:15  trev_vb
 *	TvB 21Dec2003 Changes to support iserrio and fix type on isreclen
 *	TvB 21Dec2003 Also, modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:25  trev_vb
 *	Init import
 *	
 */
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<limits.h>
#include	<float.h>

#ifndef	VBISAM_INCL	// avoid multiple include problems
#define	VBISAM_INCL

#define	CHARTYPE	0
#define	INTTYPE		1
#define	LONGTYPE	2
#define	DOUBLETYPE	3
#define	FLOATTYPE	4
#define	QUADTYPE	5
#define	CHARSIZE	1
#define	INTSIZE		2
#define	LONGSIZE	4
#define	DOUBLESIZE	(sizeof (double))
#define	FLOATSIZE	(sizeof (float))
#if	ISAMMODE == 1
#define	QUADSIZE	8
#else	// ISAMMODE == 1
#define	QUADSIZE	4
#endif	// ISAMMODE == 1
#define	ISDESC		0x80

#define	BYTEMASK	0xFF	// Mask for one byte
#define	BYTESHFT	8	// Shift for one byte

#define	ISFIRST		0	// Position to first row
#define	ISLAST		1	// Position to last row
#define	ISNEXT		2	// Position to next row
#define	ISPREV		3	// Position to previous row
#define	ISCURR		4	// Position to current row
#define	ISEQUAL		5	// Position to equal value
#define	ISGREAT		6	// Position to greater value
#define	ISGTEQ		7	// Position to >= value

// isread () locking modes
#define	ISLOCK		0x100	// Row lock
#define	ISSKIPLOCK	0x200	// Skip row even if locked
#define	ISWAIT		0x400	// Wait for row lock
#define	ISLCKW		ISLOCK | ISWAIT

// isstart () lock modes
#define	ISKEEPLOCK	0x800	// Keep rec lock in autolk mode

// isopen (), isbuild () lock modes
#define	ISAUTOLOCK	0x200	// Automatic row lock
#define	ISMANULOCK	0x400	// Manual row lock
#define	ISEXCLLOCK	0x800	// Exclusive isam file lock

// isopen (), isbuild () file types
#define	ISINPUT		0	// Open for input only
#define	ISOUTPUT	1	// Open for output only
#define	ISINOUT		2	// Open for input and output
#define	ISTRANS		4	// Open for transaction proc
#define	ISNOLOG		8	// No loggin for this file
#define	ISVARLEN	0x10	// Variable length rows
#define	ISFIXLEN	0x0	// (Non-flag) Fixed length rows only

// audit trail mode parameters
#define	AUDSETNAME	0	// Set new audit trail name
#define	AUDGETNAME	1	// Get audit trail name
#define	AUDSTART	2	// Start audit trail
#define	AUDSTOP		3	// Stop audit trail
#define	AUDINFO		4	// Audit trail running

#define	VB_MAX_KEYLEN	120	// BUG - FIXME! Maximum number of bytes in a key
#define	NPARTS		8	// Maximum number of key parts
#define	MAXSUBS		32	// Maximum number of indexes per table
#define	VB_MAX_FILES	128	// Maximum number of open VBISAM files
/*
 * In order to keep consistent variable names in VBISAM, the following #define
 * lines are helpful
 */
#ifdef	VBISAM_LIB
# define	keypart		SKEYPART
# define	kp_start	iStart
# define	kp_leng		iLength
# define	kp_type		iType
# define	keydesc		SKEYDESC
# define	keydesc_t	TKEYDESC
# define	k_flags		iFlags
# define	k_nparts	iNParts
# define	k_part		sPart
# define	k_len		iKeyLength
# define	k_rootnode	tRootNode
#endif	// VBISAM_LIB
struct	keypart
{
	short	kp_start,	// Starting byte of key part
		kp_leng,	// Length in bytes
		kp_type;	// Type of key part (include ISDESC as needed)
};
typedef	struct	keydesc
{
	short	k_flags,	// Flags (Compression)
		k_nparts;	// Number of parts in this key
	struct	keypart
		k_part [NPARTS];// Each key part
	short	k_len;		// Length of entire uncompressed key
	off_t	k_rootnode;	// Pointer to rootnode
} keydesc_t;
#define	k_start	k_part [0].kp_start
#define	k_leng	k_part [0].kp_leng
#define	k_type	k_part [0].kp_type
// Possible values for iFlags
#define	ISNODUPS	0x00	// No duplicates allowed
#define	ISDUPS		0x01	// Duplicates allowed
#define	DCOMPRESS	0x02	// Duplicate compression
#define	LCOMPRESS	0x04	// Leading compression
#define	TCOMPRESS	0x08	// Trailing compression
#define	COMPRESS	0x0e	// All compression
#define	ISCLUSTER	0x00
/*
 * In order to keep consistent variable names in VBISAM, the following #define
 * lines are helpful
 */
#ifdef	VBISAM_LIB
# define	di_nkeys	iNKeys
# define	di_recsize	iMaxRowLength
# define	di_idxsize	iIndexLength
# define	di_nrecords	tNRows
#endif	// VBISAM_LIB
struct	dictinfo
{
	short	di_nkeys,	// Number of keys defined (msb set if VARLEN)
		di_recsize,	// Maximum data row length
		di_idxsize;	// Number of bytes in an index node
	off_t	di_nrecords;	// Number of rows in data file
};
// Possible error return values
#define	EDUPL		100	// Duplicate row
#define	ENOTOPEN	101	// File not open
#define	EBADARG		102	// Illegal argument
#define	EBADKEY		103	// Illegal key desc
#define	ETOOMANY	104	// Too many files open
#define	EBADFILE	105	// Bad isam file format
#define	ENOTEXCL	106	// Non-exclusive access
#define	ELOCKED		107	// Row locked
#define	EKEXISTS	108	// Key already exists
#define	EPRIMKEY	109	// Is primary key
#define	EENDFILE	110	// End/begin of file
#define	ENOREC		111	// No row found
#define	ENOCURR		112	// No current row
#define	EFLOCKED	113	// File locked
#define	EFNAME		114	// File name too long
#define	EBADMEM		116	// Can't alloc memory
#define	ELOGREAD	118	// Cannot read log rec
#define	EBADLOG		119	// Bad log row
#define	ELOGOPEN	120	// Cannot open log file
#define	ELOGWRIT	121	// Cannot write log rec
#define	ENOTRANS	122	// No transaction
#define	ENOBEGIN	124	// No begin work yet
#define	ENOPRIM		127	// No primary key
#define	ENOLOG		128	// No logging
#define	ENOFREE		131	// No free disk space
#define	EROWSIZE	132	// Row size too short / long
#define	EAUDIT		133	// Audit trail exists
#define	ENOLOCKS	134	// No more locks
#define	EDEADLOK	143	// Deadlock avoidance
#define	ENOMANU		153	// Must be in ISMANULOCK mode
#define	EINTERUPT	157	// Interrupted isam call
#define	EBADFORMAT	171	// Locking or NODESIZE change

extern	int	iserrno,	// Isam error return code
		iserrio,	// NOT used with VBISAM
		isreclen;	// Used for varlen tables
extern	off_t	isrecnum;	// Current row number
struct	audhead
{
	char	au_type [2];		// Audit row type aa,dd,rr,ww
	char	au_time [LONGSIZE];	// Audit date-time
	char	au_procid [INTSIZE];	// Process id number
	char	au_userid [INTSIZE];	// User id number
	char	au_recnum [QUADSIZE];	// Row number
	char	au_reclen [INTSIZE];	// audit row length beyond header
};
// Number of bytes in audit header
#define	AUDHEADSIZE	(2+LONGSIZE+INTSIZE+INTSIZE+QUADSIZE)
// VARLEN num of bytes in audit header
#define	VAUDHEADSIZE	(2+LONGSIZE+INTSIZE+INTSIZE+QUADSIZE+INTSIZE)
#ifdef	__STDC__
// Prototypes for file manipulation functions
int	isaddindex (int iHandle, struct keydesc *psKeyDescription);
int	isaudit (int iHandle, char *pcFilename, int iMode);
int	isbegin (void);
int	isbuild (char *pcFilename, int iRowLength, struct keydesc *psKeyDescription, int iMode);
int	iscleanup (void);
int	isclose (int iHandle);
int	iscluster (int iHandle, struct keydesc *psKeyDescription);
int	iscommit (void);
int	isdelcurr (int iHandle);
int	isdelete (int iHandle, char *pcRow);
int	isdelindex (int iHandle, struct keydesc *psKeyDescription);
int	isdelrec (int iHandle, off_t tRowNumber);
int	iserase (char *pcFilename);
int	isflush (int iHandle);
int	isindexinfo (int iHandle, struct keydesc *psKeyDescription, int iIndexNumber);
int	islock (int iHandle);
int	islogclose (void);
int	islogopen (char *pcLogFilename);
int	isopen (char *pcFilename, int iMode);
int	isread (int iHandle, char *pcRow, int iMode);
int	isrecover (void);
int	isrelcurr (int iHandle);
int	isrelease (int iHandle);
int	isrelrec (int iHandle, off_t tRowNumber);
int	isrename (char *pcOldFilename, char *pcNewFilename);
int	isrewcurr (int iHandle, char *pcRow);
int	isrewrec (int iHandle, off_t tRowNumber, char *pcRow);
int	isrewrite (int iHandle, char *pcRow);
int	isrollback (void);
int	issetunique (int iHandle, off_t tUniqueID);
int	isstart (int iHandle, struct keydesc *psKeyDescription, int iLength, char *pcRow, int iMode);
int	isuniqueid (int iHandle, off_t *llUniqueID);
int	isunlock (int iHandle);
int	iswrcurr (int iHandle, char *pcRow);
int	iswrite (int iHandle, char *pcRow);
// prototypes for format-conversion and manipulation fuctions
void	ldchar (char *pcSource, int iLength, char *pcDestination);
void	stchar (char *pcSource, char *pcDestination, int iLength);
int	ldint (char *pcLocation);
void	stint (int iSource, char *pcDestination);
long	ldlong (char *pcLocation);
void	stlong (long lSource, char *pcDestination);
off_t	ldquad (char *pcLocation);
void	stquad (off_t tSource, char *pcDestination);
double	ldfloat (char *pcLocation);
void	stfloat (double dSource, char *pcDestination);
double	ldfltnull (char *pcLocation, short *piNullflag);
void	stfltnull (double dSource, char *pcDestination, short iNullflag);
double	lddbl (char *pcLocation);
void	stdbl (double dSource, char *pcDestination);
double	lddblnull (char *pcLocation, short *piNullflag);
void	stdblnull (double dSource, char *pcDestination, short iNullflag);
// int	lddecimal (char *pcLocation, int lLength, dec_t *psDestination);
// void	stdecimal (dec_t *psSource, char *pcDestination, int iLength);
// Decimaltype Functions
/*
 * int	deccvasc (char *pcSource, int iLength, dec_t *psDestination);
 * int	dectoasc (dec_t *psSource, char *pcDestination, int iLength, int iRight);
 * int	deccvint (int iSource, dec_t *psDestination);
 * int	dectoint (dec_t *psSource, int *piDestination);
 * int	deccvlong (long lSource, dec_t *psDestination);
 * int	dectolong (dec_t *psSource, long *plDestination);
 * int	deccvflt (float fSource, dec_t *psDestination);
 * int	dectoflt (dec_t *psSource, float *pfDestination);
 * int	deccvdbl (double dSource, dec_t *psDestination);
 * int	dectodbl (dec_t *psSource, double *pdDestination);
 * int	decadd (dec_t *psN1, dec_t *psN2, dec_t *psResult);
 * int	decsub (dec_t *psN1, dec_t *psN2, dec_t *psResult);
 * int	decmul (dec_t *psN1, dec_t *psN2, dec_t *psResult);
 * int	decdiv (dec_t *psN1, dec_t *psN2, dec_t *psResult);
 * int	deccmp (dec_t *psN1, dec_t *psN2);
 * void	deccopy (dec_t *psSource, dec_t *psDestination);
 * char	*dececvt (dec_t *psSource, int iNDigit, int *piDecPt, int *piSign);
 * char	*decfcvt (dec_t *psSource, int iNDigit, int *piDecPt, int *piSign);
 */
#ifdef	DEBUG
void	vVBMallocReport (void);
#endif	// DEBUG
#endif	// __STDC__
#endif	// VBISAM_INCL
