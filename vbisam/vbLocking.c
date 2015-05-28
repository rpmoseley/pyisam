/*
 * Title:	vbLocking.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	21Nov2003
 * Description:
 *	This module handles the locking on both the index and data files for the
 *	VBISAM library.
 * Version:
 *	$Id: vbLocking.c,v 1.9 2004/06/16 10:53:56 trev_vb Exp $
 * Modification History:
 *	$Log: vbLocking.c,v $
 *	Revision 1.9  2004/06/16 10:53:56  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
 *	
 *	Revision 1.8  2004/06/13 07:52:17  trev_vb
 *	TvB 13June2004
 *	Implemented sharing of open files.
 *	Changed the locking strategy slightly to allow table-level locking granularity
 *	(i.e. A process opening the same table more than once can now lock itself!)
 *	
 *	Revision 1.7  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.6  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.3  2004/01/03 02:28:48  trev_vb
 *	TvB 02Jan2004 WAY too many changes to enumerate!
 *	TvB 02Jan2003 Transaction processing done (excluding iscluster)
 *	
 *	Revision 1.2  2003/12/22 04:42:14  trev_vb
 *	TvB 21Dec2003 Changed name of environment var (Prep for future)
 *	TvB 21Dec2003 Also, changed 'ID' cvs header to 'Id' (Duh on me!)
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:19  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"

static	int	iVBBufferLevel = -1;

/*
 * Prototypes
 */
int	iVBEnter (int, int);
int	iVBExit (int);
int	iVBForceExit (int);
int	iVBFileOpenLock (int, int);
int	iVBDataLock (int, int, off_t);
static	int	iLockInsert (int, off_t);
static	int	iLockDelete (int, off_t);

/*
 *	Overview
 *	========
 *	Ideally, I'd prefer using making use of the high bit for the file open
 *	locks and the row locks.  However, this is NOT allowed by Linux.
 *
 *	After a good deal of deliberation (followed by some snooping with good
 *	old strace), I've decided to make the locking strategy 100% compatible
 *	with Informix(R) CISAM 7.24UC2 as it exists for Linux.
 *	When used in 64-bit mode, it simply extends the values accordingly.
 *
 *	Index file locks (ALL locks are on the index file)
 *	==================================================
 *	Non exclusive file open lock
 *		Off:0x7fffffff	Len:0x00000001	Typ:RDLOCK
 *	Exclusive file open lock (i.e. ISEXCLLOCK)
 *		Off:0x7fffffff	Len:0x00000001	Typ:WRLOCK
 *	Enter a primary function - Non modifying
 *		Off:0x00000000	Len:0x3fffffff	Typ:RDLCKW
 *	Enter a primary function - Modifying
 *		Off:0x00000000	Len:0x3fffffff	Typ:WRLCKW
 *	Lock a data row (Add the row number to the offset)
 *		Off:0x40000000	Len:0x00000001	Typ:WRLOCK
 *	Lock *ALL* the data rows (i.e. islock is called)
 *		Off:0x40000000	Len:0x3fffffff	Typ:WRLOCK
 */

/*
 * Name:
 *	int	iVBEnter (int iHandle, int iModifying);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	int	iModifying
 *		0 - Acquire a read lock to inhibit others acquiring a write lock
 *		OTHER - Acquire a write lock
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	EBADARG	Failure
 * Problems:
 *	NONE known
 * Comments:
 *	This function is called upon entry to any of the functions that require
 *	the index file to be in a 'stable' state throughout their life.
 *	If the calling function is going to MODIFY the index file, then it
 *	needs an exclusive lock on the index file.
 *	As a bonus, it also loads the dictionary node of the file into memory
 *	if the file is not opened EXCLLOCK.
 */
int
iVBEnter (int iHandle, int iModifying)
{
	int	iLockMode,
		iLoop,
		iResult;
	off_t	tLength;

	for (iLoop = 0; iLoop < MAXSUBS; iLoop++)
		if (psVBFile [iHandle]->psKeyCurr [iLoop] && psVBFile [iHandle]->psKeyCurr [iLoop]->tRowNode == -1)
			psVBFile [iHandle]->psKeyCurr [iLoop] = VBKEY_NULL;
	iserrno = 0;
	if (!psVBFile [iHandle] || (psVBFile [iHandle]->iIsOpen && iVBInTrans != VBCOMMIT && iVBInTrans != VBROLLBACK))
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	if (psVBFile [iHandle]->iOpenMode & ISTRANS && iVBInTrans == VBNOTRANS)
	{
		iserrno = ENOTRANS;
		return (-1);
	}
	psVBFile [iHandle]->sFlags.iIndexChanged = 0;
	if (psVBFile [iHandle]->iOpenMode & ISEXCLLOCK)
		return (0);
	if (psVBFile [iHandle]->sFlags.iIsDictLocked & 0x03)
	{
		iserrno = EBADARG;
		return (-1);
	}
	if (iModifying)
		iLockMode = VBWRLCKW;
	else
		iLockMode = VBRDLCKW;
	memset (cVBNode [0], 0xff, QUADSIZE);
	cVBNode [0] [0] = 0x3f;
	tLength = ldquad (cVBNode [0]);
	if (!(psVBFile [iHandle]->iOpenMode & ISEXCLLOCK))
	{
		iResult = iVBLock (psVBFile [iHandle]->iIndexHandle, 0, tLength, iLockMode);
		if (iResult)
			return (-1);
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x01;
		iResult = iVBBlockRead (iHandle, TRUE, (off_t) 1, cVBNode [0]);
		if (iResult)
		{
			psVBFile [iHandle]->sFlags.iIsDictLocked = 0;
			iVBExit (iHandle);
			iserrno = EBADFILE;
			return (-1);
		}
		memcpy ((void *) &psVBFile [iHandle]->sDictNode, (void *) cVBNode [0], sizeof (struct DICTNODE));
	}
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x01;
	if (psVBFile [iHandle]->tTransLast == ldquad (psVBFile [iHandle]->sDictNode.cTransNumber))
		psVBFile [iHandle]->sFlags.iIndexChanged = 0;
	else
	{
		psVBFile [iHandle]->sFlags.iIndexChanged = 1;
		vVBBlockInvalidate (iHandle);
	}
	return (0);
}

/*
 * Name:
 *	int	iVBExit (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	int	iUpdateTrans
 *		0 - Leave the dictionary node transaction number alone
 *		OTHER - Increment the dictionary node transaction number
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	EBADARG	Failure
 * Problems:
 *	NONE known
 */
int
iVBExit (int iHandle)
{
	char	*pcEnviron;
	int	iResult = 0,
		iLoop,
		iLoop2,
		iSaveError;
	off_t	tLength,
		tTransNumber;

	iSaveError = iserrno;
	if (!psVBFile [iHandle] || (!psVBFile [iHandle]->sFlags.iIsDictLocked && !(psVBFile [iHandle]->iOpenMode & ISEXCLLOCK)))
	{
		iserrno = EBADARG;
		return (-1);
	}
	tTransNumber = ldquad (psVBFile [iHandle]->sDictNode.cTransNumber);
	psVBFile [iHandle]->tTransLast = tTransNumber;
	if (psVBFile [iHandle]->iOpenMode & ISEXCLLOCK)
		return (0);
	if (psVBFile [iHandle]->sFlags.iIsDictLocked & 0x02)
	{
		if (!(psVBFile [iHandle]->sFlags.iIsDictLocked & 0x04))
		{
			psVBFile [iHandle]->tTransLast = tTransNumber + 1;
			stquad (tTransNumber + 1, psVBFile [iHandle]->sDictNode.cTransNumber);
		}
		memset (cVBNode [0], 0, MAX_NODE_LENGTH);
		memcpy ((void *) cVBNode [0], (void *) &psVBFile [iHandle]->sDictNode, sizeof (struct DICTNODE));
		iResult = iVBBlockWrite (iHandle, TRUE, (off_t) 1, cVBNode [0]);
		if (iResult)
			iserrno = EBADFILE;
		else
			iserrno = 0;
	}
	iResult |= iVBBlockFlush (iHandle);
	memset (cVBNode [0], 0xff, QUADSIZE);
	cVBNode [0] [0] = 0x3f;
	tLength = ldquad (cVBNode [0]);
	if (iVBLock (psVBFile [iHandle]->iIndexHandle, 0, tLength, VBUNLOCK))
	{
		iserrno = errno;
		return (-1);
	}
	psVBFile [iHandle]->sFlags.iIsDictLocked = 0;
	// Free up any key/tree no longer wanted
	if (iVBBufferLevel == -1)
	{
		pcEnviron = getenv ("VB_TREE_LEVEL");
		if (pcEnviron)
			iVBBufferLevel = atoi (pcEnviron);
		else
			iVBBufferLevel = 4;
	}
	for (iLoop2 = 0; iLoop2 < psVBFile [iHandle]->iNKeys; iLoop2++)
	{
		struct	VBKEY
			*psKey,
			*psKeyCurr;

		psKey = psVBFile [iHandle]->psKeyCurr [iLoop2];
		/*
		 * This is a REAL fudge factor...
		 * We simply free up all the dynamically allocated memory
		 * associated with non-current nodes above a certain level.
		 * In other words, we wish to keep the CURRENT key and all data
		 * in memory above it for iVBBufferLevel levels.
		 * Anything *ABOVE* that in the tree is to be purged with
		 * EXTREME prejudice except for the 'current' tree up to the
		 * root.
		 */
		for (iLoop = 0; psKey && iLoop < iVBBufferLevel; iLoop++)
		{
			if (psKey->psParent->psParent)
				psKey = psKey->psParent->psParent->psKeyCurr;
			else
				psKey = VBKEY_NULL;
		}
		if (!psKey)
		{
			iserrno = iSaveError;
			return (0);
		}
		while (psKey)
		{
			for (psKeyCurr = psKey->psParent->psKeyFirst; psKeyCurr; psKeyCurr = psKeyCurr->psNext)
			{
				if (psKeyCurr != psKey && psKeyCurr->psChild)
				{
					vVBTreeAllFree (iHandle, iLoop2, psKeyCurr->psChild);
					psKeyCurr->psChild = VBTREE_NULL;
				}
			}
			if (psKey->psParent->psParent)
				psKey = psKey->psParent->psParent->psKeyCurr;
			else
				psKey = VBKEY_NULL;
		}
	}
	if (iResult)
		return (-1);
	iserrno = iSaveError;
	return (0);
}

/*
 * Name:
 *	int	iVBForceExit (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	EBADARG	Failure
 * Problems:
 *	NONE known
 */
int
iVBForceExit (int iHandle)
{
	int	iResult = 0;

	if (psVBFile [iHandle]->sFlags.iIsDictLocked & 0x02)
	{
		memset (cVBNode [0], 0, MAX_NODE_LENGTH);
		memcpy ((void *) cVBNode [0], (void *) &psVBFile [iHandle]->sDictNode, sizeof (struct DICTNODE));
		iResult = iVBBlockWrite (iHandle, TRUE, (off_t) 1, cVBNode [0]);
		if (iResult)
			iserrno = EBADFILE;
		else
			iserrno = 0;
	}
	iResult |= iVBBlockFlush (iHandle);
	psVBFile [iHandle]->sFlags.iIsDictLocked = 0;
	if (iResult)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBFileOpenLock (int iHandle, int iMode);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	int	iMode
 *		0 - A 'per-process' file open lock is removed
 *		1 - A 'per-process' file open lock is desired
 *		2 - An 'exclusive' file open lock is desired
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	ENOTOPEN
 *	EBADARG
 *	EFLOCKED
 * Problems:
 *	NONE known
 * Comments:
 *	This routine is used to establish a 'file-open' lock on the index file
 */
int
iVBFileOpenLock (int iHandle, int iMode)
{
	int	iLockType,
		iResult;
	off_t	tOffset;

	// Sanity check - Is iHandle a currently open table?
	if (iHandle < 0 || iHandle > iVBMaxUsedHandle)
		return (ENOTOPEN);
	if (!psVBFile [iHandle])
		return (ENOTOPEN);

	memset (cVBNode [0], 0xff, QUADSIZE);
	cVBNode [0] [0] = 0x7f;
	tOffset = ldquad (cVBNode [0]);

	switch (iMode)
	{
	case	0:
		iLockType = VBUNLOCK;
		break;

	case	1:
		iLockType = VBRDLOCK;
		break;

	case	2:
		iLockType = VBWRLOCK;
		iResult = iVBBlockRead (iHandle, TRUE, (off_t) 1, cVBNode [0]);
		memcpy ((void *) &psVBFile [iHandle]->sDictNode, (void *) cVBNode [0], sizeof (struct DICTNODE));
		break;

	default:
		return (EBADARG);
	}

	iResult = iVBLock (psVBFile [iHandle]->iIndexHandle, tOffset, 1, iLockType);
	if (iResult != 0)
		return (EFLOCKED);

	return (0);
}

/*
 * Name:
 *	int	iVBDataLock (int iHandle, int iMode, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	int	iMode
 *		VBUNLOCK
 *		VBWRLOCK
 *		VBWRLCKW
 *	off_t	tRowNumber
 *		The row number to be (un)locked
 *		If zero, then this is a file-wide (un)lock
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	ENOTOPEN
 *	EBADFILE
 *	ELOCKED
 * Problems:
 *	NONE known
 */
int
iVBDataLock (int iHandle, int iMode, off_t tRowNumber)
{
	int	iResult = 0;
	struct	VBLOCK
		*psLock,
		*psLockNext;
	off_t	tLength = 1,
		tOffset;

	// Sanity check - Is iHandle a currently open table?
	if (iHandle < 0 || iHandle > iVBMaxUsedHandle)
		return (ENOTOPEN);
	if (!psVBFile [iHandle])
		return (ENOTOPEN);
	if (psVBFile [iHandle]->iOpenMode & ISEXCLLOCK)
		return (0);
	/*
	 * If this is a FILE (un)lock (row = 0), then we may as well free ALL
	 * locks. Even if CISAMLOCKS is set, we do this!
	 */
	if (tRowNumber == 0)
	{
		for (psLock = sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead; psLock; psLock = psLockNext)
		{
			psLockNext = psLock->psNext;
			iVBDataLock (iHandle, VBUNLOCK, psLock->tRowNumber);
		}
		memset (cVBNode [0], 0xff, QUADSIZE);
		cVBNode [0] [0] = 0x3f;
		tLength = ldquad (cVBNode [0]);
		if (iMode == VBUNLOCK)
			psVBFile [iHandle]->sFlags.iIsDataLocked = FALSE;
		else
			psVBFile [iHandle]->sFlags.iIsDataLocked = TRUE;
	}
	else
		if (iMode == VBUNLOCK)
			iResult = iLockDelete (iHandle, tRowNumber);
	if (!iResult)
	{
		memset (cVBNode [0], 0x00, QUADSIZE);
		cVBNode [0] [0] = 0x40;
		tOffset = ldquad (cVBNode [0]);
		iResult = iVBLock (psVBFile [iHandle]->iIndexHandle, tOffset + tRowNumber, tLength, iMode);
		if (iResult != 0)
			return (ELOCKED);
	}
	if (iMode != VBUNLOCK && tRowNumber)
		return (iLockInsert (iHandle, tRowNumber));
	return (iResult);
}

/*
 * Name:
 *	static	int	iLockInsert (int iHandle, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	off_t	tRowNumber
 *		The row number to be added to the files lock list
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	Failure
 * Problems:
 *	NONE known
 */
static	int
iLockInsert (int iHandle, off_t tRowNumber)
{
	struct	VBLOCK
		*psNewLock = VBLOCK_NULL,
		*psLock;

	psLock = sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead;
	// Insertion at head of list
	if (psLock == VBLOCK_NULL || tRowNumber < psLock->tRowNumber)
	{
		psNewLock = psVBLockAllocate (iHandle);
		if (psNewLock == VBLOCK_NULL)
			return (errno);
		psNewLock->iHandle = iHandle;
		psNewLock->tRowNumber = tRowNumber;
		psNewLock->psNext = psLock;
		sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead = psNewLock;
		if (sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail == VBLOCK_NULL)
			sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail = psNewLock;
		return (0);
	}
	// Insertion at tail of list
	if (tRowNumber > sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail->tRowNumber)
	{
		psNewLock = psVBLockAllocate (iHandle);
		if (psNewLock == VBLOCK_NULL)
			return (errno);
		psNewLock->iHandle = iHandle;
		psNewLock->tRowNumber = tRowNumber;
		sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail->psNext = psNewLock;
		sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail = psNewLock;
		return (0);
	}
	// Position psLock to insertion point (Keep in mind, we insert AFTER)
	while (psLock->psNext && tRowNumber > psLock->psNext->tRowNumber)
		psLock = psLock->psNext;
	// Already locked?
	if (psLock->tRowNumber == tRowNumber)
	{
		if (psLock->iHandle == iHandle)
			return (0);
		else
#ifdef	CISAMLOCKS
			return (0);
#else	// CISAMLOCKS
			return (ELOCKED);
#endif	// CISAMLOCKS
	}
	psNewLock = psVBLockAllocate (iHandle);
	if (psNewLock == VBLOCK_NULL)
		return (errno);
	psNewLock->iHandle = iHandle;
	psNewLock->tRowNumber = tRowNumber;
	// Insert psNewLock AFTER psLock
	psNewLock->psNext = psLock->psNext;
	psLock->psNext = psNewLock;

	return (0);
}

/*
 * Name:
 *	static	int	iLockDelete (int iHandle, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	off_t	tRowNumber
 *		The row number to be removed from the files lock list
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	EBADARG	Failure
 * Problems:
 *	NONE known
 */
static	int
iLockDelete (int iHandle, off_t tRowNumber)
{
	struct	VBLOCK
		*psLockToDelete,
		*psLock = sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead;

	// Sanity check #1.  If it wasn't locked, ignore it!
	if (!psLock || psLock->tRowNumber > tRowNumber)
		return (0);
	// Check if deleting first entry in list
	if (psLock->tRowNumber == tRowNumber)
	{
#ifndef	CISAMLOCKS
		if (psLock->iHandle != iHandle)
			return (ELOCKED);
#endif	// CISAMLOCKS
		sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead = psLock->psNext;
		if (!sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead)
			sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail = VBLOCK_NULL;
		vVBLockFree (psLock);
		return (0);
	}
	// Position psLock pointer to previous
	while (psLock->psNext && psLock->psNext->tRowNumber < tRowNumber)
		psLock = psLock->psNext;
	// Sanity check #2
	if (!psLock->psNext || psLock->psNext->tRowNumber != tRowNumber)
		return (0);
	psLockToDelete = psLock->psNext;
#ifndef	CISAMLOCKS
	if (psLockToDelete->iHandle != iHandle)
		return (ELOCKED);
#endif	// CISAMLOCKS
	psLock->psNext = psLockToDelete->psNext;
	if (psLockToDelete == sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail)
		sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail = psLock;
	vVBLockFree (psLockToDelete);

	return (0);
}
