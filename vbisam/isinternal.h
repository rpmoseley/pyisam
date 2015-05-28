/*
 * Title:	isinternal.h
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	17Nov2003
 * Description:
 *	This is the header that defines the internally used structures for the
 *	VBISAM library.
 * Version:
 *	$Id: isinternal.h,v 1.12 2004/06/16 10:53:55 trev_vb Exp $
 * Modification History:
 *	$Log: isinternal.h,v $
 *	Revision 1.12  2004/06/16 10:53:55  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
 *	
 *	Revision 1.11  2004/06/13 07:52:17  trev_vb
 *	TvB 13June2004
 *	Implemented sharing of open files.
 *	Changed the locking strategy slightly to allow table-level locking granularity
 *	(i.e. A process opening the same table more than once can now lock itself!)
 *	
 *	Revision 1.10  2004/06/13 06:32:33  trev_vb
 *	TvB 12June2004 See CHANGELOG 1.03 (Too lazy to enumerate)
 *	
 *	Revision 1.9  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.8  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.7  2004/03/23 21:55:56  trev_vb
 *	TvB 23Mar2004 Endian on SPARC (Phase I).  Makefile changes for SPARC.
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
 *	Revision 1.2  2003/12/22 04:46:55  trev_vb
 *	TvB 21Dec2003 Changes to add iserrio and fix isreclen type
 *	TvB 21Dec2003 Also, modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:19  trev_vb
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
#define	VBISAM_LIB
#include	"vbisam.h"

// If your operating system is POOR on caching data, I can do it for you.
// This is also useful if the data is remotely located (e.g. NFS) since it
//  suppresses re-reading and writing wherever possible
#define	VB_CACHE_ON	1
#define	VB_CACHE_OFF	2
#define	VB_CACHE	VB_CACHE_ON

#ifdef	VB_ENDIAN
#undef	VB_ENDIAN
#endif
#ifdef	hpux			// Thanks to Mikhail
#define	VB_ENDIAN	4321
#endif
#ifdef	aix			// Thanks to Mikhail
#define	VB_ENDIAN	4321
#endif
#ifdef	sparc			// Thanks to Hemant
#define	VB_ENDIAN	4321
#endif
#ifdef	i386
#define	VB_ENDIAN	1234
#endif
#ifndef	VB_ENDIAN
#error I do not know whether the CPU is big or little endian! HELP me!
#endif

// Implementation limits
// 64-bit versions have a maximum node length of 4096 bytes
// 32-bit versions have a maximum node length of 1024 bytes
#if	ISAMMODE == 1
# define	MAX_NODE_LENGTH	4096
# define	MAX_KEYS_PER_NODE	((MAX_NODE_LENGTH - (INTSIZE + QUADSIZE + 2)) / (QUADSIZE + 1))
# if	_FILE_OFFSET_BITS != 64
#error	It is REDUNDANT to use ISAMMODE = 1 if you do not ALSO use 64-bit I/O
# endif	// _FILE_OFFSET_BITS != 64
#else	// ISAMMODE == 1
# define	MAX_NODE_LENGTH	1024
# define	MAX_KEYS_PER_NODE	((MAX_NODE_LENGTH - (INTSIZE + 2)) / (QUADSIZE + 1))
#endif	// ISAMMODE == 1
#define	SLOTS_PER_NODE	((MAX_NODE_LENGTH >> 2) - 1)	// Used in vbVarlenIO.c

#define	MAX_PATH_LENGTH	128
#define	MAX_OPEN_TRANS	1024	// Currently, only used in isrecover ()

#ifdef	FALSE
#undef	FALSE
#endif
#define	FALSE	(0)

#ifdef	TRUE
#undef	TRUE
#endif
#define	TRUE	((!FALSE))

// Arguments to iVBLock
#define	VBUNLOCK	0	// Take a WILD guess!
#define	VBRDLOCK	1	// A simple read lock, non-blocking
#define	VBRDLCKW	2	// A simple read lock, blocking
#define	VBWRLOCK	3	// An exclusive write lock, non-blocking
#define	VBWRLCKW	4	// An exclusive write lock, blocking

// Values for iVBInTrans
#define	VBNOTRANS	0	// NOT in a transaction at all
#define	VBBEGIN		1	// An isbegin has been issued but no more
#define	VBNEEDFLUSH	2	// Something BEYOND just isbegin has been issued
#define	VBCOMMIT	3	// We're in 'iscommit' mode
#define	VBROLLBACK	4	// We're in 'isrollback' mode
#define	VBRECOVER	5	// We're in 'isrecover' mode

#ifdef	VBISAMMAIN
#define	EXTERN
	char	*pcRowBuffer = (char *) 0,
		*pcWriteBuffer,		// Common areas for a data row
		cVBNode [2][MAX_NODE_LENGTH],	// Scratch nodes
		cVBTransBuffer [65536];	// Buffer for holding a transaction
	int	iVBMaxUsedHandle = -1,	// The highest opened file handle
		iVBLogfileHandle = -1,	// The handle to the current logfile
		iVBInTrans = VBNOTRANS,	// If not zero, we're in a transaction
		iVBRowBufferLength = 0;
	off_t	tLogfilePosition = 0;	// The logfile size at islogopen()
#else	// VBISAMMAIN
#define	EXTERN	extern
extern	char	*pcRowBuffer,
		*pcWriteBuffer,		// Common areas for a data row
		cVBNode [2][MAX_NODE_LENGTH],	// Scratch nodes
		cVBTransBuffer [65536];	// Buffer for holding a transaction
extern	int	iVBMaxUsedHandle,
		iVBLogfileHandle,
		iVBInTrans,
		iVBRowBufferLength;
extern	off_t	tLogfilePosition;
#endif	// VBISAMMAIN

EXTERN	pid_t	tVBPID;
EXTERN	uid_t	tVBUID;
EXTERN	int	iserrno,	// Value of error is returned here
		iserrio,	// Contains value of last function called
		isreclen;	// Used for varlen tables
EXTERN	off_t	isrecnum;	// Current row number

struct	VBLOCK
{
	struct	VBLOCK
		*psNext;
	int	iHandle;	// The handle that 'applied' this lock
	off_t	tRowNumber;
};
#define	VBLOCK_NULL	((struct VBLOCK *) 0)

struct	VBKEY
{
	struct	VBKEY
		*psNext,		// Next key in this node
		*psPrev;		// Previous key in this node
	struct	VBTREE
		*psParent,		// Pointer towards ROOT
		*psChild;		// Pointer towards LEAF
	off_t	tRowNode,		// The row / node number
		tDupNumber;		// The duplicate number (1st = 0)
	struct
	{
		 unsigned int
			iIsNew:1,	// If this is a new entry (split use)
			iIsHigh:1,	// Is this a GREATER THAN key?
			iIsDummy:1,	// A simple end of node marker
			iRFU:29;	// Reserved
	} sFlags;
	char	cKey [1];		// Placeholder for the key itself
};
#define	VBKEY_NULL	((struct VBKEY *) 0)

struct	VBTREE
{
	struct	VBTREE
		*psNext,		// Used for the free list only!
		*psParent;		// The next level up from this node
	struct	VBKEY
#if	ISAMMODE == 1			// Highly non-portable.. kind of
		*psKeyList [512],
#else	// ISAMMODE == 1
		*psKeyList [256],
#endif	// ISAMMODE == 1
		*psKeyFirst,		// Pointer to the FIRST key in this node
		*psKeyLast,		// Pointer to the LAST key in this node
		*psKeyCurr;		// Pointer to the CURRENT key
	off_t	tNodeNumber,		// The actual node
		tTransNumber;		// Transaction number stamp
	struct
	{
		unsigned int
			iLevel:16,	// The level number (0 = LEAF)
			iIsRoot:1,	// 1 = This is the ROOT node
			iIsTOF:1,	// 1 = First entry in index
			iIsEOF:1,	// 1 = Last entry in index
			iKeysInNode:10,	// # keys in psKeyList
			iRFU:3;		// Reserved for Future Use
	} sFlags;
};
#define	VBTREE_NULL	((struct VBTREE *) 0)

struct	DICTNODE			// Offset	32Val	64Val
{					// 32IO	64IO
	char	cValidation [2],	// 0x00	0x00	0xfe53	0x5642
		cHeaderRsvd,		// 0x02 0x02	0x02	Same
		cFooterRsvd,		// 0x03	0x03	0x02	Same
		cRsvdPerKey,		// 0x04	0x04	0x04	0x08
		cRFU1,			// 0x05 0x05	0x04	Same
		cNodeSize [INTSIZE],	// 0x06	0x06	0x03ff	0x0fff
		cIndexCount [INTSIZE],	// 0x08	0x08	Varies	Same
		cRFU2 [2],		// 0x0a	0x0a	0x0704	Same
		cFileVersion,		// 0x0c	0x0c	0x00	Same
		cMinRowLength [INTSIZE],// 0x0d	0x0d	Varies	Same
		cNodeKeydesc [QUADSIZE],// 0x0f	0x0f	Normally 2
		cLocalIndex,		// 0x13	0x17	0x00	Same
		cRFU3 [5],		// 0x14	0x18	0x00...	Same
		cDataFree [QUADSIZE],	// 0x19	0x1d	Varies	Same
		cNodeFree [QUADSIZE],	// 0x1d	0x25	Varies	Same
		cDataCount [QUADSIZE],	// 0x21	0x2d	Varies	Same
		cNodeCount [QUADSIZE],	// 0x25	0x35	Varies	Same
		cTransNumber [QUADSIZE],// 0x29	0x3d	Varies	Same
		cUniqueID [QUADSIZE],	// 0x2d	0x45	Varies	Same
		cNodeAudit [QUADSIZE],	// 0x31	0x4d	Varies	Same
		cLockMethod [INTSIZE],	// 0x35	0x55	0x0008	Same
		cRFU4 [QUADSIZE],	// 0x37	0x57	0x00...	Same
		cMaxRowLength [INTSIZE],// 0x3b	0x5f	Varies	Same
		cVarlenG0 [QUADSIZE],	// 0x3d	0x61	Varies	Same
		cVarlenG1 [QUADSIZE],	// 0x41	0x69	Varies	Same
		cVarlenG2 [QUADSIZE],	// 0x45	0x71	Varies	Same
		cVarlenG3 [QUADSIZE],	// 0x49	0x79	Varies	Same
		cVarlenG4 [QUADSIZE],	// 0x4d	0x81	Varies	Same
#if	ISAMMODE == 1
		cVarlenG5 [QUADSIZE],	//	0x89	Varies	Same
		cVarlenG6 [QUADSIZE],	//	0x91	Varies	Same
		cVarlenG7 [QUADSIZE],	//	0x99	Varies	Same
		cVarlenG8 [QUADSIZE],	//	0xa1	Varies	Same
#endif	// ISAMMODE == 1
		cRFULocalIndex [36];	// 0x51	0xa9	0x00...	Same
			//		   ---- ----
			// Length Total	   0x75	0xcd
};

struct	DICTINFO
{
	short	iNKeys,		// Number of keys
		iActiveKey,	// Which key is the active key
		iNodeSize,	// Number of bytes in an index block
		iMinRowLength,	// Minimum data row length
		iMaxRowLength;	// Maximum data row length
	int	iDataHandle,	// file descriptor of the .dat file
		iIndexHandle,	// file descriptor of the .idx file
		iIsOpen,	// 0: Table open, Files open, Buffers OK
				// 1: Table closed, Files open, Buffers OK
				//	Used to retain locks
				// 2: Table closed, Files closed, Buffers OK
				//	Basically, just caching
		iOpenMode,	// The type of open which was used
		iVarlenLength,	// Length of varlen component
		iVarlenSlot;	// The slot number within tVarlenNode
	off_t	tRowNumber,	// Which data row is "CURRENT" 0 if none
		tDupNumber,	// Which duplicate number is "CURRENT" (0=First)
		tRowStart,	// ONLY set to nonzero by isstart()
		tTransLast,	// Used to see whether to set iIndexChanged
		tNRows,		// Number of rows (0 IF EMPTY, 1 IF NOT)
		tVarlenNode;	// Node containing 1st varlen data
	char	cFilename [MAX_PATH_LENGTH],
		**ppcRowBuffer;	// tMinRowLength buffer for key (re)construction
	struct	DICTNODE
		sDictNode;	// Holds the dictionary node data
	struct	keydesc		// Array of key description information
		*psKeydesc [MAXSUBS];
	struct	VBTREE
		*psTree [MAXSUBS]; // Linked list of index nodes
	struct	VBKEY
		*psKeyFree [MAXSUBS], // An array of linked lists of free VBKEYs
		*psKeyCurr [MAXSUBS]; // An array of 'current' VBKEY pointers
	struct
	{	
	unsigned int
		iIsDisjoint:1,	// If set, CURR & NEXT give same result
		iIsDataLocked:1,// If set, islock() is active
		iIsDictLocked:3,// Relates to sDictNode below
				// 	0x00: Content on file MIGHT be different
				// 	0x01: Dictionary node is LOCKED
				// 	0x02: sDictNode needs to be rewritten
				//	0x04: do NOT increment Dict.Trans
				//		isrollback () is in progress
				//	(Thus, suppress some iVBEnter/iVBExit)
		iIndexChanged:2,// Various
				//	0: Index has NOT changed since last time
				//	1: Index has changed, blocks invalid
				//	2: Index has changed, blocks are valid
		iTransYet:2,	// Relates to isbegin () et al
				//	0: FO Trans not yet written
				//	1: FO Trans written outside isbegin
				//	2: FO Trans written within isbegin
		iRFU:23;
	} sFlags;
};
EXTERN	struct	DICTINFO
	*psVBFile [VB_MAX_FILES + 1];
// If the corresponding handle is not open, psVBFile [iHandle] == NULL

struct	VBFILE
{
	int	iHandle,
		iRefCount;	// How many times we are 'open'
	dev_t	tDevice;
	ino_t	tInode;
	off_t	tPosn;
	struct	VBLOCK		// Ordered linked list of locked row numbers
		*psLockHead,
		*psLockTail;
};
EXTERN	struct	VBFILE
	sVBFile [VB_MAX_FILES * 3];	// Enough? Hehe

#define	VBL_BUILD	("BU")
#define	VBL_BEGIN	("BW")
#define	VBL_CREINDEX	("CI")
#define	VBL_CLUSTER	("CL")
#define	VBL_COMMIT	("CW")
#define	VBL_DELETE	("DE")
#define	VBL_DELINDEX	("DI")
#define	VBL_FILEERASE	("ER")
#define	VBL_FILECLOSE	("FC")
#define	VBL_FILEOPEN	("FO")
#define	VBL_INSERT	("IN")
#define	VBL_RENAME	("RE")
#define	VBL_ROLLBACK	("RW")
#define	VBL_SETUNIQUE	("SU")
#define	VBL_UNIQUEID	("UN")
#define	VBL_UPDATE	("UP")

struct	SLOGHDR
{
	char	cLength [INTSIZE],
		cOperation [2],
		cPID [INTSIZE],
		cUID [INTSIZE],
		cTime [LONGSIZE],
		cRFU1 [INTSIZE],
		cLastPosn [INTSIZE],
		cLastLength [INTSIZE];
} *psVBLogHeader;

// Globally defined function prototypes
// isHelper.c
int	iscluster (int, struct keydesc *);
int	iserase (char *);
int	isflush (int);
int	islock (int);
int	isrelease (int);
int	isrename (char *, char *);
int	issetunique (int, off_t);
int	isuniqueid (int, off_t *);
int	isunlock (int);
void	ldchar (char *, int, char *);
void	stchar (char *, char *, int);
int	ldint (char *);
void	stint (int, char *);
long	ldlong (char *);
void	stlong (long, char *);
off_t	ldquad (char *);
void	stquad (off_t, char *);
double	ldfloat (char *);
void	stfloat (double, char *);
double	ldfltnull (char *, short *);
void	stfltnull (double, char *, short);
double	lddbl (char *);
void	stdbl (double, char *);
double	lddblnull (char *, short *);
void	stdblnull (double, char *, short);

// isaudit.c
int	isaudit (int, char *, int);

// isbuild.c
int	isbuild (char *, int, struct keydesc *, int);
int	isaddindex (int, struct keydesc *);
int	isdelindex (int, struct keydesc *);

// isdelete.c
int	isdelete (int, char *);
int	isdelcurr (int);
int	isdelrec (int, off_t);

// isopen.c
int	iscleanup (void);
int	isclose (int);
int	iVBClose2 (int);
int	isindexinfo (int, struct keydesc *, int);
int	isopen (char *, int);

// isread.c
int	isread (int, char *, int);
int	isstart (int, struct keydesc *, int, char *, int);
int	iVBCheckKey (int, struct keydesc *, int, int, int);

// isrecover.c
int	isrecover (void);

// isrewrite.c
int	isrewrite (int, char *);
int	isrewcurr (int, char *);
int	isrewrec (int, off_t, char *);

// istrans.c
int	isbegin (void);
int	iscommit (void);
int	islogclose (void);
int	islogopen (char *);
int	isrollback (void);
int	iVBRollMeBack (off_t, int, int);
int	iVBRollMeForward (off_t, int);
int	iVBTransBuild (char *, int, int, struct keydesc *, int);
int	iVBTransCreateIndex (int, struct keydesc *);
int	iVBTransCluster (void);	// BUG - Unknown args
int	iVBTransDelete (int, off_t, int);
int	iVBTransDeleteIndex (int, struct keydesc *);
int	iVBTransErase (char *);
int	iVBTransClose (int, char *);
int	iVBTransOpen (int, char *);
int	iVBTransInsert (int, off_t, int, char *);
int	iVBTransRename (char *, char *);
int	iVBTransSetUnique (int, off_t);
int	iVBTransUniqueID (int, off_t);
int	iVBTransUpdate (int, off_t, int, int, char *);

// iswrite.c
int	iswrcurr (int, char *);
int	iswrite (int, char *);
int	iVBWriteRow (int, char *, off_t);

// vbBlockIO.c
int	iVBBlockRead (int, int, off_t, char *);
int	iVBBlockWrite (int, int, off_t, char *);
int	iVBBlockFlush (int);
void	vVBBlockInvalidate (int);
void	vVBBlockDeinit (void);

// vbDataIO.c
int	iVBDataRead (int, char *, int *, off_t, int);
int	iVBDataWrite (int, char *, int, off_t, int);

// vbIndexIO.c
int	iVBUniqueIDSet (int, off_t);
off_t	tVBUniqueIDGetNext (int);
off_t	tVBNodeCountGetNext (int);
off_t	tVBDataCountGetNext (int);
int	iVBNodeFree (int, off_t);
int	iVBDataFree (int, off_t);
off_t	tVBNodeAllocate (int);
off_t	tVBDataAllocate (int iHandle);
int	iVBForceDataAllocate (int iHandle, off_t tRowNumber);

// vbKeysIO.c
void	vVBMakeKey (int, int, char *, char *);
int	iVBKeySearch (int, int, int, int, char *, off_t);
int	iVBKeyLocateRow (int, int, off_t);
int	iVBKeyLoad (int, int, int, int, struct VBKEY **);
void	vVBKeyValueSet (int, struct keydesc *, char *);
int	iVBKeyInsert (int, struct VBTREE *, int, char *, off_t, off_t, struct VBTREE *);
int	iVBKeyDelete (int, int);
int	iVBKeyCompare (int, int, int, unsigned char *, unsigned char *);
#ifdef	DEBUG
int	iDumpTree (int, int);
int	iChkTree (int, int);
#endif	// DEBUG

// vbLocking.c
int	iVBEnter (int, int);
int	iVBExit (int);
int	iVBForceExit (int);
int	iVBFileOpenLock (int, int);
int	iVBDataLock (int, int, off_t);

// vbLowLovel.c
int	iVBOpen (char *, int, mode_t);
int	iVBClose (int);
off_t	tVBLseek (int, off_t, int);
ssize_t	tVBRead (int, void *, size_t);
ssize_t	tVBWrite (int, void *, size_t);
int	iVBLock (int, off_t, off_t, int);
int	iVBLink (char *, char *);
int	iVBUnlink (char *);
int	iVBStat (char *, struct stat *);

// vbMemIO.c
struct	VBLOCK *psVBLockAllocate (int);
void	vVBLockFree (struct VBLOCK *);
struct	VBTREE *psVBTreeAllocate (int);
void	vVBTreeAllFree (int, int, struct VBTREE *);
struct	VBKEY *psVBKeyAllocate (int, int);
void	vVBKeyAllFree (int, int, struct VBTREE *);
void	vVBKeyFree (int, int, struct VBKEY *);
void	vVBKeyUnMalloc (int, int);
void	*pvVBMalloc (size_t);
void	vVBFree (void *, size_t);
void	vVBUnMalloc (void);
#ifdef	DEBUG
void	vVBMallocReport (void);
#endif	// DEBUG

// vbNodeMemIO.c
int	iVBNodeLoad (int, int, struct VBTREE *, off_t, int);
int	iVBNodeSave (int, int, struct VBTREE *, off_t, int, int);

// vbVarLenIO.c
int	iVBVarlenRead (int, char *, off_t, int, int);
int	iVBVarlenWrite (int, char *, int);
int	iVBVarlenDelete (int, off_t, int, int);
