/*
 * Title:	vbIndexIO.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	21Nov2003
 * Description:
 *	This module handles ALL the low level index file I/O operations for the
 *	VBISAM library.
 * Version:
 *	$Id: vbIndexIO.c,v 1.7 2004/06/11 22:16:16 trev_vb Exp $
 * Modification History:
 *	$Log: vbIndexIO.c,v $
 *	Revision 1.7  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.6  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.5  2004/01/06 14:31:59  trev_vb
 *	TvB 06Jan2004 Added in VARLEN processing (In a fairly unstable sorta way)
 *	
 *	Revision 1.4  2004/01/05 07:36:17  trev_vb
 *	TvB 05Feb2002 Added licensing et al as Johann v. N. noted I'd overlooked it
 *	
 *	Revision 1.3  2004/01/03 02:28:48  trev_vb
 *	TvB 02Jan2004 WAY too many changes to enumerate!
 *	TvB 02Jan2003 Transaction processing done (excluding iscluster)
 *	
 *	Revision 1.2  2003/12/22 04:40:22  trev_vb
 *	TvB 21Dec2003 Fixups to make freelists compatible with a commercial competitor
 *	TvB 21Dec2003 Also, changed the cvs 'ID' header to 'Id' (Duh!)
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:17  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"

/*
 * Prototypes
 */
int	iVBUniqueIDSet (int, off_t);
off_t	tVBUniqueIDGetNext (int);
off_t	tVBNodeCountGetNext (int);
off_t	tVBDataCountGetNext (int);
int	iVBNodeFree (int, off_t);
int	iVBDataFree (int, off_t);
off_t	tVBNodeAllocate (int);
off_t	tVBDataAllocate (int);
int	iVBForceDataAllocate (int iHandle, off_t);

/*
 * Name:
 *	int	iVBUniqueIDSet (int iHandle, off_t tUniqueID);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	off_t	tUniqueID
 *		The new starting unique ID
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	ENOTOPEN
 *	EBADARG
 * Problems:
 *	NONE known
 * Comments:
 *	This routine is used to set the current unique id to a new starting
 *	number.  It REFUSES to set it to a lower number than is current!
 */
int
iVBUniqueIDSet (int iHandle, off_t tUniqueID)
{
	off_t	tValue;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);

	iserrno = 0;
	tValue = ldquad (psVBFile [iHandle]->sDictNode.cUniqueID);
	if (tUniqueID > tValue)
	{
		stquad (tUniqueID, psVBFile [iHandle]->sDictNode.cUniqueID);
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	}
	return (0);
}

/*
 * Name:
 *	off_t	tVBUniqueIDGetNext (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Failure (iserrno has more info)
 *	OTHER	The new unique id
 * Problems:
 *	NONE known
 */
off_t
tVBUniqueIDGetNext (int iHandle)
{
	off_t	tValue;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);
	iserrno = 0;

	tValue = ldquad (psVBFile [iHandle]->sDictNode.cUniqueID);
	stquad (tValue + 1, psVBFile [iHandle]->sDictNode.cUniqueID);
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	return (tValue);
}

/*
 * Name:
 *	off_t	tVBNodeCountGetNext (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Failure (iserrno has more info)
 *	OTHER	The new index node number
 * Problems:
 *	NONE known
 */
off_t
tVBNodeCountGetNext (int iHandle)
{
	off_t	tValue;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);
	iserrno = 0;

	tValue = ldquad (psVBFile [iHandle]->sDictNode.cNodeCount) + 1;
	stquad (tValue, psVBFile [iHandle]->sDictNode.cNodeCount);
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	return (tValue);
}

/*
 * Name:
 *	off_t	tVBDataCountGetNext (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Failure (iserrno has more info)
 *	OTHER	The new (data) row number
 * Problems:
 *	NONE known
 */
off_t
tVBDataCountGetNext (int iHandle)
{
	off_t	tValue;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);
	iserrno = 0;

	tValue = ldquad (psVBFile [iHandle]->sDictNode.cDataCount) + 1;
	stquad (tValue, psVBFile [iHandle]->sDictNode.cDataCount);
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	return (tValue);
}

/*
 * Name:
 *	int	iVBNodeFree (int iHandle, off_t tNodeNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	off_t	tNodeNumber
 *		The node number to append to the index node free list
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	ENOTOPEN
 *	EBADFILE
 *	EBADARG
 * Problems:
 *	NONE known
 * Comments:
 *	By the time this function is called, tNodeNumber should be COMPLETELY
 *	unreferenced by any other node.
 */
int
iVBNodeFree (int iHandle, off_t tNodeNumber)
{
	int	iLengthUsed,
		iResult;
	off_t	tHeadNode;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);
	iserrno = 0;

	memset (cVBNode [1], 0, (size_t) psVBFile [iHandle]->iNodeSize);
	stint (INTSIZE, cVBNode [1]);

	tHeadNode = ldquad (psVBFile [iHandle]->sDictNode.cNodeFree);
	// If the list is empty, node tNodeNumber becomes the whole list
	if (tHeadNode == (off_t) 0)
	{
		stint (INTSIZE + QUADSIZE, cVBNode [1]);
		stquad (0, cVBNode [1] + INTSIZE);
		cVBNode [1] [psVBFile [iHandle]->iNodeSize - 2] = 0x7f;
		cVBNode [1] [psVBFile [iHandle]->iNodeSize - 3] = -2;
		iResult = iVBBlockWrite (iHandle, TRUE, tNodeNumber, cVBNode [1]);
		if (iResult)
			return (iResult);
		stquad (tNodeNumber, psVBFile [iHandle]->sDictNode.cNodeFree);
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
		return (0);
	}

	// Read in the head of the current free list
	iResult = iVBBlockRead (iHandle, TRUE, tHeadNode, cVBNode [0]);
	if (iResult)
		return (iResult);
// Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO)
#if	ISAMMODE == 1
	if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 2] != 0x7f)
		return (EBADFILE);
#endif	// ISAMMODE == 1
	if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 3] != -2)
		return (EBADFILE);
	iLengthUsed = ldint (cVBNode [0]);
	if (iLengthUsed >= psVBFile [iHandle]->iNodeSize - (QUADSIZE + 3))
	{
		// If there was no space left, tNodeNumber becomes the head
		cVBNode [1] [psVBFile [iHandle]->iNodeSize - 2] = 0x7f;
		cVBNode [1] [psVBFile [iHandle]->iNodeSize - 3] = -2;
		stint (INTSIZE + QUADSIZE, cVBNode [1]);
		stquad (tHeadNode, &cVBNode [1] [INTSIZE]);
		iResult = iVBBlockWrite (iHandle, TRUE, tNodeNumber, cVBNode [1]);
		if (!iResult)
		{
			stquad (tNodeNumber, psVBFile [iHandle]->sDictNode.cNodeFree);
			psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
		}
		return (iResult);
	}

	// If we got here, there's space left in the tHeadNode to store it
	cVBNode [1] [psVBFile [iHandle]->iNodeSize - 2] = 0x7f;
	cVBNode [1] [psVBFile [iHandle]->iNodeSize - 3] = -2;
	iResult = iVBBlockWrite (iHandle, TRUE, tNodeNumber, cVBNode [1]);
	if (iResult)
		return (iResult);
	stquad (tNodeNumber, &cVBNode [0] [iLengthUsed]);
	iLengthUsed += QUADSIZE;
	stint (iLengthUsed, cVBNode [0]);
	iResult = iVBBlockWrite (iHandle, TRUE, tHeadNode, cVBNode [0]);

	return (iResult);
}

/*
 * Name:
 *	int	iVBDataFree (int iHandle, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	off_t	tRowNumber
 *		The absolute row number to be added onto the data row free list
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	ENOTOPEN?
 *	EBADFILE?
 *	EBADARG?
 * Problems:
 *	NONE known
 */
int
iVBDataFree (int iHandle, off_t tRowNumber)
{
	int	iLengthUsed,
		iResult;
	off_t	tHeadNode,
		tNodeNumber;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);
	iserrno = 0;

	if (ldquad (psVBFile [iHandle]->sDictNode.cDataCount) == tRowNumber)
	{
		stquad (tRowNumber - 1, psVBFile [iHandle]->sDictNode.cDataCount);
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
		return (0);
	}

	tHeadNode = ldquad (psVBFile [iHandle]->sDictNode.cDataFree);
	if (tHeadNode != (off_t) 0)
	{
		iResult = iVBBlockRead (iHandle, TRUE, tHeadNode, cVBNode [0]);
		if (iResult)
			return (iResult);
// Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO)
#if	ISAMMODE == 1
		if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 2] != 0x7f)
			return (EBADFILE);
#endif	//ISAMMODE == 1
		if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 3] != -1)
			return (EBADFILE);
		iLengthUsed = ldint (cVBNode [0]);
		if (iLengthUsed < psVBFile [iHandle]->iNodeSize - (QUADSIZE + 3))
		{
			// We need to add tRowNumber to the current node
			stquad ((off_t) tRowNumber, cVBNode [0] + iLengthUsed);
			iLengthUsed += QUADSIZE;
			stint (iLengthUsed, cVBNode [0]);
			iResult = iVBBlockWrite (iHandle, TRUE, tHeadNode, cVBNode [0]);
			return (iResult);
		}
	}
	// We need to allocate a new row-free node!
	// We append any existing nodes using the next pointer from the new node
	tNodeNumber = tVBNodeAllocate (iHandle);
	if (tNodeNumber == (off_t) -1)
		return (iserrno);
	memset (cVBNode [0], 0, MAX_NODE_LENGTH);
	cVBNode [0] [psVBFile [iHandle]->iNodeSize - 2] = 0x7f;
	cVBNode [0] [psVBFile [iHandle]->iNodeSize - 3] = -1;
	stint (INTSIZE + (2 * QUADSIZE), cVBNode [0]);
	stquad (tHeadNode, &cVBNode [0] [INTSIZE]);
	stquad (tRowNumber, &cVBNode [0] [INTSIZE + QUADSIZE]);
	iResult = iVBBlockWrite (iHandle, TRUE, tNodeNumber, cVBNode [0]);
	if (iResult)
		return (iResult);
	stquad (tNodeNumber, psVBFile [iHandle]->sDictNode.cDataFree);
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	return (0);
}

/*
 * Name:
 *	off_t	tVBNodeAllocate (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno has more info)
 *	OTHER	The newly allocated node number
 * Problems:
 *	NONE known
 */
off_t
tVBNodeAllocate (int iHandle)
{
	int	iLengthUsed;
	off_t	tHeadNode,
		tValue;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);
	iserrno = 0;

	// If there's *ANY* nodes in the free list, use them first!
	tHeadNode = ldquad (psVBFile [iHandle]->sDictNode.cNodeFree);
	if (tHeadNode != (off_t) 0)
	{
		iserrno = iVBBlockRead (iHandle, TRUE, tHeadNode, cVBNode [0]);
		if (iserrno)
			return (-1);
		iserrno = EBADFILE;
// Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO)
#if	ISAMMODE == 1
		if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 2] != 0x7f)
			return (-1);
#endif	// ISAMMODE == 1
		if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 3] != -2)
			return (-1);
		iLengthUsed = ldint (cVBNode [0]);
		if (iLengthUsed > (INTSIZE + QUADSIZE))
		{
			tValue = ldquad (cVBNode [0] + INTSIZE + QUADSIZE);
			memcpy (cVBNode [0] + INTSIZE + QUADSIZE, cVBNode [0] + INTSIZE + QUADSIZE + QUADSIZE, iLengthUsed - (INTSIZE + QUADSIZE + QUADSIZE));
			iLengthUsed -= QUADSIZE;
			memset (cVBNode [0] + iLengthUsed, 0, QUADSIZE);
			stint (iLengthUsed, cVBNode [0]);
			iserrno = iVBBlockWrite (iHandle, TRUE, tHeadNode, cVBNode [0]);
			if (iserrno)
				return (-1);
			return (tValue);
		}
		// If it's last entry in the node, use the node itself!
		tValue = ldquad (cVBNode [0] + INTSIZE);
		stquad (tValue, psVBFile [iHandle]->sDictNode.cNodeFree);
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
		return (tHeadNode);
	}
	// If we get here, we need to allocate a NEW node.
	// Since we already hold a dictionary lock, we don't need another
	tValue = tVBNodeCountGetNext (iHandle);
	return (tValue);
}

/*
 * Name:
 *	off_t	tVBDataAllocate (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	ENOTOPEN?
 *	EBADFILE?
 *	EBADARG?
 * Problems:
 *	NONE known
 */
off_t
tVBDataAllocate (int iHandle)
{
	int	iLengthUsed,
		iResult;
	off_t	tHeadNode,
		tNextNode,
		tValue;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);
	iserrno = 0;

	// If there's *ANY* rows in the free list, use them first!
	tHeadNode = ldquad (psVBFile [iHandle]->sDictNode.cDataFree);
	while (tHeadNode != (off_t) 0)
	{
		iserrno = iVBBlockRead (iHandle, TRUE, tHeadNode, cVBNode [0]);
		if (iserrno)
			return (-1);
		iserrno = EBADFILE;
// Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO)
#if	ISAMMODE == 1
		if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 2] != 0x7f)
			return (-1);
#endif	// ISAMMODE == 1
		if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 3] != -1)
			return (-1);
		iLengthUsed = ldint (cVBNode [0]);
		if (iLengthUsed > INTSIZE + QUADSIZE)
		{
			iLengthUsed -= QUADSIZE;
			stint (iLengthUsed, cVBNode [0]);
			tValue = ldquad (&cVBNode [0] [iLengthUsed]);
			stquad ((off_t) 0, &cVBNode [0] [iLengthUsed]);
			if (iLengthUsed > INTSIZE + QUADSIZE)
			{
				iserrno = iVBBlockWrite (iHandle, TRUE, tHeadNode, cVBNode [0]);
				if (iserrno)
					return (-1);
				return (tValue);
			}
			// If we're using the last entry in the node, advance
			tNextNode = ldquad (&cVBNode [0] [INTSIZE]);
			iResult = iVBNodeFree (iHandle, tHeadNode);
			if (iResult)
				return (-1);
			stquad (tNextNode, psVBFile [iHandle]->sDictNode.cDataFree);
			psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
			return (tValue);
		}
		// Ummmm, this is an INTEGRITY ERROR of sorts!
		// However, let's fix it anyway!
		tNextNode = ldquad (&cVBNode [0] [INTSIZE]);
		iResult = iVBNodeFree (iHandle, tHeadNode);
		if (iResult)
			return (-1);
		stquad (tNextNode, psVBFile [iHandle]->sDictNode.cDataFree);
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
		tHeadNode = tNextNode;
	}
	// If we get here, we need to allocate a NEW row number.
	// Since we already hold a dictionary lock, we don't need another
	tValue = tVBDataCountGetNext (iHandle);
	return (tValue);
}

/*
 * Name:
 *	int	iVBForceDataAllocate (int iHandle, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the idx file)
 *	off_t	tRowNumber
 *		The data row number that we *MUST* allocate
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	Other	Failed
 * Problems:
 *	NONE known
 */
int
iVBForceDataAllocate (int iHandle, off_t tRowNumber)
{
	int	iLoop,
		iLengthUsed;
	off_t	tHeadNode,
		tPrevNode,
		tNextNode;

	// Sanity check - Is iHandle a currently open table?
	iserrno = ENOTOPEN;
	if (!psVBFile [iHandle])
		return (-1);
	iserrno = EBADARG;
	if (!psVBFile [iHandle]->sFlags.iIsDictLocked)
		return (-1);
	iserrno = 0;

	// Test 1: Is it already beyond EOF (the SIMPLE test)
	tHeadNode = ldquad (psVBFile [iHandle]->sDictNode.cDataCount);
	if (tHeadNode < tRowNumber)
	{
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
		stquad (tRowNumber, psVBFile [iHandle]->sDictNode.cDataCount);
		tHeadNode++;
		while (tHeadNode < tRowNumber)
		{
			if (tHeadNode != 0)
				iVBDataFree (iHandle, tHeadNode);
			tHeadNode++;
		}
		return (0);
	}
	// <SIGH> It SHOULD be *SOMEWHERE* in the data free list!
	tPrevNode = 0;
	tHeadNode = ldquad (psVBFile [iHandle]->sDictNode.cDataFree);
	while (tHeadNode != (off_t) 0)
	{
		iserrno = iVBBlockRead (iHandle, TRUE, tHeadNode, cVBNode [0]);
		if (iserrno)
			return (-1);
		iserrno = EBADFILE;
// Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO)
#if	ISAMMODE == 1
		if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 2] != 0x7f)
			return (-1);
#endif	// ISAMMODE == 1
		if (cVBNode [0] [psVBFile [iHandle]->iNodeSize - 3] != -1)
			return (-1);
		iLengthUsed = ldint (cVBNode [0]);
		for (iLoop = INTSIZE + QUADSIZE; iLoop < iLengthUsed; iLoop += QUADSIZE)
		{
			if (ldquad (&cVBNode [0] [iLoop]) == tRowNumber)
			{	// Found the bitch! Now to extract her
				memcpy (&(cVBNode [0] [iLoop]), &(cVBNode [0] [iLoop + QUADSIZE]), iLengthUsed - iLoop);
				iLengthUsed -= QUADSIZE;
				if (iLengthUsed > INTSIZE + QUADSIZE)
				{
					stquad (0, &cVBNode [0] [iLengthUsed]);
					stint (iLengthUsed, cVBNode [0]);
					return (iVBBlockWrite (iHandle, TRUE, tHeadNode, cVBNode [0]));
				}
				else
				{	// CRAP It was the last one in the node!
					tNextNode = ldquad (&cVBNode [0] [INTSIZE]);
					if (tPrevNode)
					{
						iserrno = iVBBlockRead (iHandle, TRUE, tPrevNode, cVBNode [0]);
						if (iserrno)
							return (-1);
						stquad (tNextNode, &cVBNode [0] [INTSIZE]);
						return (iVBBlockWrite (iHandle, TRUE, tPrevNode, cVBNode [0]));
					}
					else
					{
						psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
						stquad (tNextNode, psVBFile [iHandle]->sDictNode.cDataFree);
					}
					return (iVBNodeFree (iHandle, tHeadNode));
				}
			}
		}
		tPrevNode = tHeadNode;
		tHeadNode = ldquad (&cVBNode [0] [INTSIZE]);
	}
	// If we get here, we've got a MAJOR integrity error in that the
	// nominated row number was simply *NOT FREE*
	iserrno = EBADFILE;
	return (-1);
}
