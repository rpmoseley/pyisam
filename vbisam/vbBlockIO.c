/*
 * Title:	vbBlockIO.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	18Nov2003
 * Description:
 *	This module handles ALL the low level file-block I/O operations for the
 *	VBISAM library.
 *	There are two primary 'entry points': iVBBlockRead & iVBBlockWrite
 *	Secondary entry points are: iVBBlockFlush and iVBBlockInvalidate
 *	If VB_CACHE is VB_CACHE_OFF then direct I/O is performed
 *	If VB_CACHE is VB_CACHE_ON then cached I/O is performed
 *	Otherwise, a compile error results
 * Version:
 *	$Id: vbBlockIO.c,v 1.3 2004/06/13 06:32:33 trev_vb Exp $
 * Modification History:
 *	$Log: vbBlockIO.c,v $
 *	Revision 1.3  2004/06/13 06:32:33  trev_vb
 *	TvB 12June2004 See CHANGELOG 1.03 (Too lazy to enumerate)
 *	
 *	Revision 1.2  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.1  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 */
#include	"isinternal.h"
#include	<assert.h>

/*
 * Prototypes
 */
int	iVBBlockRead (int iHandle, int iIsIndex, off_t tBlockNumber, char *cBuffer);
int	iVBBlockWrite (int iHandle, int iIsIndex, off_t tBlockNumber, char *cBuffer);
int	iVBBlockFlush (int iHandle);
void	vVBBlockInvalidate (int iHandle);
#if	VB_CACHE == VB_CACHE_ON
void	vVBBlockDeinit (void);
static	void	vBlockInit (void);

static	int	iInitialized = FALSE,
		iBlockCount = 0;

struct	VBBLOCK
{
	struct	VBBLOCK
		*psNext,
		*psPrev;
	int	iFileHandle,
		iHandle,
		iIsDirty,
		iIsIndex;
	off_t	tBlockNumber;
	char	cBuffer [MAX_NODE_LENGTH];
};
#define	VBBLOCK_NULL	((struct VBBLOCK *) 0)
static	struct	VBBLOCK
	*psBlockHead = VBBLOCK_NULL,
	*psBlockTail = VBBLOCK_NULL;

#endif	// VB_CACHE == VB_CACHE_ON

/*
 * Name:
 *	int	iVBBlockRead (int iHandle, int iIsIndex, off_t tBlockNumber, char *cBuffer)
 * Arguments:
 *	int	iHandle
 *		The VBISAM file handle
 *	int	iIsIndex
 *		TRUE
 *			We're reading the index file
 *		FALSE
 *			We're reading the data file
 *	off_t	tBlockNumber
 *		The absolute blocknumber to read (1 = TOF)
 *	char	*cBuffer
 *		The place to put the data read in
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
#if	VB_CACHE == VB_CACHE_ON
int
iVBBlockRead (int iHandle, int iIsIndex, off_t tBlockNumber, char *cBuffer)
{
	int	iFileHandle;
	struct	VBBLOCK
		*psBlock;
	ssize_t	tLength;
	off_t	tOffset,
		tResult;

	if (!iInitialized)
		vBlockInit ();
	/*
	 * We *CANNOT* rely on buffering index node #1 since it *IS* the node
	 * that we *MUST* read in to determine whether the buffers are dirty.
	 * An obvious exception to this is when the table is opened in
	 * ISEXCLLOCK mode.
	 */
	if (iIsIndex && tBlockNumber == 1 && !(psVBFile [iHandle]->iOpenMode & ISEXCLLOCK))
	{
		tResult = tVBLseek (psVBFile [iHandle]->iIndexHandle, 0, SEEK_SET);
		if (tResult != 0)
		{
			iserrno = errno;
			return (-1);
		}

		if (tVBRead (psVBFile [iHandle]->iIndexHandle, (void *) cBuffer, (size_t) sizeof (struct DICTNODE)) != (ssize_t) sizeof (struct DICTNODE))
		{
			iserrno = EBADFILE;
			return (-1);
		}
		return (0);
	}
	if (iIsIndex)
		iFileHandle = psVBFile [iHandle]->iIndexHandle;
	else
		iFileHandle = psVBFile [iHandle]->iDataHandle;
	if (psVBFile [iHandle]->sFlags.iIndexChanged == 1)
	{
		if (iVBBlockFlush (iHandle))
			return (-1);
		vVBBlockInvalidate (iHandle);
	}
	for (psBlock = psBlockHead; psBlock; psBlock = psBlock->psNext)
		if (psBlock->iFileHandle == iFileHandle && psBlock->tBlockNumber == tBlockNumber)
			break;
	if (psBlock)
	{
		memcpy (cBuffer, psBlock->cBuffer, psVBFile [iHandle]->iNodeSize);
		if (!psBlock->psPrev) // Already at head of list?
			return (0);
		// Remove psBlock from the list
		psBlock->psPrev->psNext = psBlock->psNext;
		if (psBlock->psNext)
			psBlock->psNext->psPrev = psBlock->psPrev;
		else
			psBlockTail = psBlock->psPrev;
		// Re-insert it at the head
		psBlock->psPrev = VBBLOCK_NULL;
		psBlock->psNext = psBlockHead;
		psBlockHead->psPrev = psBlock;
		psBlockHead = psBlock;
		return (0);
	}
	psBlock = psBlockTail;
	if (psBlock->iIsDirty)
	{
		iserrno = iVBBlockFlush (psBlock->iHandle);
		if (iserrno)
			return (-1);
	}
	tOffset = (tBlockNumber - 1) * psVBFile [iHandle]->iNodeSize;
	if (iIsIndex)
		tResult = tVBLseek (iFileHandle, tOffset, SEEK_SET);
	else
		tResult = tVBLseek (iFileHandle, tOffset, SEEK_SET);
	if (tResult == tOffset)
	{
		tLength = tVBRead (iFileHandle, (void *) psBlock->cBuffer, (size_t) psVBFile [iHandle]->iNodeSize);
		if (!iIsIndex && tLength == 0)
		{
			tLength = (ssize_t) psVBFile [iHandle]->iNodeSize;
			memset (psBlock->cBuffer, 0, psVBFile [iHandle]->iNodeSize);
		}
		if (tLength == (ssize_t) psVBFile [iHandle]->iNodeSize)
		{
			memcpy (cBuffer, psBlock->cBuffer, psVBFile [iHandle]->iNodeSize);
			psBlock->iFileHandle = iFileHandle;
			psBlock->iHandle = iHandle;
			psBlock->iIsIndex = iIsIndex;
			psBlock->tBlockNumber = tBlockNumber;
			// Remove psBlock from the list
			psBlock->psPrev->psNext = psBlock->psNext;
			psBlockTail = psBlock->psPrev;
			// Re-insert it at the head
			psBlock->psPrev = VBBLOCK_NULL;
			psBlock->psNext = psBlockHead;
			psBlockHead->psPrev = psBlock;
			psBlockHead = psBlock;
			return (0);
		}
		else
		{
			fprintf (stderr, "Failed to read in block %lld from the %s file of table %s!\n", tBlockNumber, iIsIndex ? "Index" : "Data", psVBFile [iHandle]->cFilename);
			assert (FALSE);
		}
	}
	fprintf (stderr, "Failed to position to block %lld of the %s file of table %s!\n", tBlockNumber, iIsIndex ? "Index" : "Data", psVBFile [iHandle]->cFilename);
	assert (FALSE);
	return (-1);
}
#else	// VB_CACHE == VB_CACHE_ON
# if VB_CACHE == VB_CACHE_OFF
int
iVBBlockRead (int iHandle, int iIsIndex, off_t tBlockNumber, char *cBuffer)
{
	off_t	tResult,
		tOffset;

	tOffset = (off_t) ((tBlockNumber - 1) * psVBFile [iHandle]->iNodeSize);
	if (iIsIndex)
		tResult = tVBLseek (psVBFile [iHandle]->iIndexHandle, tOffset, SEEK_SET);
	else
		tResult = tVBLseek (psVBFile [iHandle]->iDataHandle, tOffset, SEEK_SET);
	if (tResult != tOffset)
	{
		fprintf (stderr, "Failed to position to block %lld of the %s file of table %s!\n", tBlockNumber, iIsIndex ? "Index" : "Data", psVBFile [iHandle]->cFilename);
		assert (FALSE);
	}

	if (iIsIndex)
		tResult = (off_t) tVBRead (psVBFile [iHandle]->iIndexHandle, cBuffer, (size_t) psVBFile [iHandle]->iNodeSize);
	else
		tResult = (off_t) tVBRead (psVBFile [iHandle]->iDataHandle, cBuffer, (size_t) psVBFile [iHandle]->iNodeSize);
	if (!iIsIndex && tResult == 0)
	{
		tResult = (ssize_t) psVBFile [iHandle]->iNodeSize;
		memset (cBuffer, 0, psVBFile [iHandle]->iNodeSize);
	}
	if ((int) tResult != psVBFile [iHandle]->iNodeSize)
	{
		fprintf (stderr, "Failed to read in block %lld from the %s file of table %s!\n", tBlockNumber, iIsIndex ? "Index" : "Data", psVBFile [iHandle]->cFilename);
		assert (FALSE);
	}
	return (0);
}
# else	// VB_CACHE == FALSE
#ERROR: VB_CACHE must be VB_CACHE_ON or VB_CACHE_OFF
# endif	// VB_CACHE == VB_CACHE_OFF
#endif	// VB_CACHE == VB_CACHE_ON

/*
 * Name:
 *	int	iVBBlockWrite (int iHandle, int iIsIndex, off_t tBlockNumber, char *cBuffer)
 * Arguments:
 *	int	iHandle
 *		The VBISAM file handle
 *	int	iIsIndex
 *		TRUE
 *			We're writing the index file
 *		FALSE
 *			We're writing the data file
 *	off_t	tBlockNumber
 *		The absolute blocknumber to read (1 = TOF)
 *	char	*cBuffer
 *		The data to write to the file
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
#if	VB_CACHE == VB_CACHE_ON
int
iVBBlockWrite (int iHandle, int iIsIndex, off_t tBlockNumber, char *cBuffer)
{
	struct	VBBLOCK
		*psBlock;

	if (!iInitialized)
		vBlockInit ();
	/*
	 * We *CANNOT* rely on buffering index node #1 since it *IS* the node
	 * that we *MUST* read in to determine whether the buffers are dirty.
	 * An obvious exception to this is when the table is opened in
	 * ISEXCLLOCK mode.
	 */
	if (tBlockNumber < 1)
	{
		fprintf (stderr, "Failed to write out block 0 to the %s file of table %s!\n", iIsIndex ? "Index" : "Data", psVBFile [iHandle]->cFilename);
		assert (FALSE);
	}
	if (iIsIndex && tBlockNumber == 1 && !(psVBFile [iHandle]->iOpenMode & ISEXCLLOCK))
	{
		if (tVBLseek (psVBFile [iHandle]->iIndexHandle, 0, SEEK_SET) != 0)
		{
			iserrno = errno;
			return (-1);
		}

		if (tVBWrite (psVBFile [iHandle]->iIndexHandle, (void *) cBuffer, (size_t) sizeof (struct DICTNODE)) != (ssize_t) sizeof (struct DICTNODE))
		{
			iserrno = EBADFILE;
			return (-1);
		}
		return (0);
	}
	if (psVBFile [iHandle]->sFlags.iIndexChanged == 1)
	{
		if (iVBBlockFlush (iHandle))
			return (-1);
		vVBBlockInvalidate (iHandle);
	}
	for (psBlock = psBlockHead; psBlock; psBlock = psBlock->psNext)
		if (psBlock->iHandle == iHandle && psBlock->iIsIndex == iIsIndex && psBlock->tBlockNumber == tBlockNumber)
			break;
	if (!psBlock)
		psBlock = psBlockTail;
	// If we're about to reuse a *DIFFERENT* block that's dirty, FLUSH all
	if (psBlock->iIsDirty && (psBlock->iHandle != iHandle || psBlock->iIsIndex != iIsIndex || psBlock->tBlockNumber != tBlockNumber))
	{
		iserrno = iVBBlockFlush (psBlock->iHandle);
		if (iserrno)
			return (-1);
	}
	if (iIsIndex)
		psBlock->iFileHandle = psVBFile [iHandle]->iIndexHandle;
	else
		psBlock->iFileHandle = psVBFile [iHandle]->iDataHandle;
	psBlock->iHandle = iHandle;
	psBlock->iIsDirty = TRUE;
	psBlock->iIsIndex = iIsIndex;
	psBlock->tBlockNumber = tBlockNumber;
	memcpy (psBlock->cBuffer, cBuffer, psVBFile [iHandle]->iNodeSize);
	// Already at head of list?
	if (!psBlock->psPrev)
		return (0);
	// Remove psBlock from the list
	psBlock->psPrev->psNext = psBlock->psNext;
	if (psBlock->psNext)
		psBlock->psNext->psPrev = psBlock->psPrev;
	else
		psBlockTail = psBlock->psPrev;
	// Re-insert it at the head
	psBlock->psPrev = VBBLOCK_NULL;
	psBlock->psNext = psBlockHead;
	psBlockHead->psPrev = psBlock;
	psBlockHead = psBlock;
	return (0);
}
#else	// VB_CACHE == VB_CACHE_ON
# if VB_CACHE == VB_CACHE_OFF
int
iVBBlockWrite (int iHandle, int iIsIndex, off_t tBlockNumber, char *cBuffer)
{
	off_t	tResult,
		tOffset;

	tOffset = (off_t) ((tBlockNumber - 1) * psVBFile [iHandle]->iNodeSize);
	if (iIsIndex)
		tResult = tVBLseek (psVBFile [iHandle]->iIndexHandle, tOffset, SEEK_SET);
	else
		tResult = tVBLseek (psVBFile [iHandle]->iDataHandle, tOffset, SEEK_SET);
	if (tResult == (off_t) -1)
	{
		fprintf (stderr, "Failed to position to block %lld of the %s file of table %s!\n", tBlockNumber, iIsIndex ? "Index" : "Data", psVBFile [iHandle]->cFilename);
		assert (FALSE);
	}

	if (iIsIndex)
		tResult = (off_t) tVBWrite (psVBFile [iHandle]->iIndexHandle, cBuffer, (size_t) psVBFile [iHandle]->iNodeSize);
	else
		tResult = (off_t) tVBWrite (psVBFile [iHandle]->iDataHandle, cBuffer, (size_t) psVBFile [iHandle]->iNodeSize);
	if ((int) tResult != psVBFile [iHandle]->iNodeSize)
	{
		fprintf (stderr, "Failed to write out block %lld to the %s file of table %s!\n", tBlockNumber, iIsIndex ? "Index" : "Data", psVBFile [iHandle]->cFilename);
		assert (FALSE);
	}
	return (0);
}
# else	// VB_CACHE == VB_CACHE_OFF
#ERROR: VB_CACHE must be VB_CACHE_ON or VB_CACHE_OFF
# endif	// VB_CACHE == VB_CACHE_OFF
#endif	// VB_CACHE == VB_CACHE_ON

/*
 * Name:
 *	int	iVBBlockFlush (int iHandle);
 * Arguments:
 *	int	iHandle
 *		-1
 *			Flush *EVERY* handle
 *		Other
 *			The VBISAM handle to be flushed
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 * Comments:
 *	This routine flushes out the nominated dirty blocks to disk
 *	If iHandle == -1 the table to which they are associated is ignored
 */
#if	VB_CACHE == VB_CACHE_ON
int
iVBBlockFlush (int iHandle)
{
	int	iFileHandle;
	struct	VBBLOCK
		*psBlock;
	ssize_t	tLength;
	off_t	tOffset,
		tResult;

	for (psBlock = psBlockHead; psBlock; psBlock = psBlock->psNext)
	{
		if (!psBlock->iIsDirty)
			continue;
		if (iHandle != -1 && iHandle != psBlock->iHandle)
			continue;
		iFileHandle = psBlock->iFileHandle;
		tOffset = (psBlock->tBlockNumber - 1) * psVBFile [psBlock->iHandle]->iNodeSize;
		if (psBlock->iIsIndex)
			tResult = tVBLseek (iFileHandle, tOffset, SEEK_SET);
		else
			tResult = tVBLseek (iFileHandle, tOffset, SEEK_SET);
		if (tResult == tOffset)
		{
			tLength = tVBWrite (iFileHandle, (void *) psBlock->cBuffer, (size_t) psVBFile [psBlock->iHandle]->iNodeSize);
			if (tLength == (ssize_t) psVBFile [psBlock->iHandle]->iNodeSize)
				psBlock->iIsDirty = FALSE;
			else
			{
				fprintf (stderr, "Failed to write out block %lld to the %s file of table %s!\n", psBlock->tBlockNumber, psBlock->iIsIndex ? "Index" : "Data", psVBFile [psBlock->iHandle]->cFilename);
				assert (FALSE);
			}
		}
		else
		{
			fprintf (stderr, "Failed to position to block %lld of the %s file of table %s!\n", psBlock->tBlockNumber, psBlock->iIsIndex ? "Index" : "Data", psVBFile [psBlock->iHandle]->cFilename);
			assert (FALSE);
		}
	}
	return (0);
}
#else	// VB_CACHE == VB_CACHE_ON
# if VB_CACHE == VB_CACHE_OFF
int
iVBBlockFlush (int iHandle)
{
	return (0);
}
# else	// VB_CACHE == VB_CACHE_OFF
#ERROR: VB_CACHE must be VB_CACHE_ON or VB_CACHE_OFF
# endif	// VB_CACHE == VB_CACHE_OFF
#endif	// VB_CACHE == VB_CACHE_ON

/*
 * Name:
 *	void	vVBBlockInvalidate (int iHandle)
 * Arguments:
 *	int	iHandle
 *		The VBISAM file handle
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 * Comments:
 *	Flags all blocks associated with iHandle as invalid and moves them all
 *	to the tail
 */
#if	VB_CACHE == VB_CACHE_ON
void
vVBBlockInvalidate (int iHandle)
{
	struct	VBBLOCK
		*psBlock = psBlockHead,
		*psBlockNext;

	for (; psBlock && psBlock->iFileHandle != -1; psBlock = psBlockNext)
	{
		psBlockNext = psBlock->psNext;
		if (psBlock->iFileHandle == psVBFile [iHandle]->iDataHandle || psBlock->iFileHandle == psVBFile [iHandle]->iIndexHandle)
		{
			// Set the filehandle to -1 and move it to the END
			psBlock->iFileHandle = -1;
			if (psBlock->psNext)	// Move to END if not already
			{
				psBlock->psNext->psPrev = psBlock->psPrev;
				psBlockTail->psNext = psBlock;
				if (psBlock == psBlockHead)
					psBlockHead = psBlock->psNext;
				else
					psBlock->psPrev->psNext = psBlock->psNext;
				psBlock->psPrev = psBlockTail;
				psBlock->psNext = VBBLOCK_NULL;
				psBlockTail = psBlock;
			}
		}
	}
	psVBFile [iHandle]->sFlags.iIndexChanged = 2;
}
#else	// VB_CACHE == VB_CACHE_ON
# if VB_CACHE == VB_CACHE_OFF
void
vVBBlockInvalidate (int iHandle)
{
	return;
}
# else	// VB_CACHE == VB_CACHE_OFF
#ERROR: VB_CACHE must be VB_CACHE_ON or VB_CACHE_OFF
# endif	// VB_CACHE == VB_CACHE_OFF
#endif	// VB_CACHE == VB_CACHE_ON

#if	VB_CACHE == VB_CACHE_OFF
/*
 * Name:
 *	void	vVBBlockDeinit (void)
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 * Comments:
 *	Releases all the allocated blocks back to the system
 */
void
vVBBlockDeinit (void)
{
	return;
}
#else	// VB_CACHE == VB_CACHE_OFF
/*
 * Name:
 *	void	vVBBlockDeinit (void)
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 * Comments:
 *	Releases all the allocated blocks back to the system
 */
void
vVBBlockDeinit (void)
{
	struct	VBBLOCK
		*psBlock = psBlockHead;

	for (; psBlock; psBlock = psBlockHead)
	{
		psBlockHead = psBlock->psNext;
		vVBFree (psBlock, sizeof (struct VBBLOCK));
	}
	psBlockHead = VBBLOCK_NULL;
	iInitialized = FALSE;
}

/*
 * Name:
 *	static	void	vBlockInit (void)
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 * Comments:
 *	Creates a linked list of blocks
 */
static	void
vBlockInit (void)
{
	char	*pcEnviron;
	struct	VBBLOCK
		*psBlock = VBBLOCK_NULL,
		*psBlockLast = VBBLOCK_NULL;

	iInitialized = TRUE;
	pcEnviron = getenv ("VB_BLOCK_BUFFERS");
	if (pcEnviron)
		iBlockCount = atoi (pcEnviron);
	if (iBlockCount < 4 || iBlockCount > 128)
		iBlockCount = 16;
	while (iBlockCount--)
	{
		psBlock = (struct VBBLOCK *) pvVBMalloc (sizeof (struct VBBLOCK));
		if (psBlock)
		{
			if (psBlockLast)
				psBlockLast->psNext = psBlock;
			else
				psBlockHead = psBlock;
			psBlock->psPrev = psBlockLast;
			psBlock->iFileHandle = -1;
			psBlock->iIsDirty = FALSE;
			psBlockLast = psBlock;
		}
		else
		{
			fprintf (stderr, "Insufficient memory!\n");
			exit (-1);
		}
	}
	psBlock->psNext = VBBLOCK_NULL;
	psBlockTail = psBlock;
}
#endif	// VB_CACHE == VB_CACHE_ON
