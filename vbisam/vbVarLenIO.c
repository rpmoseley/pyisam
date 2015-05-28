/*
 * Title:	vbVarLenIO.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	05Jan2004
 * Description:
 *	This is the 'behind-the-scenes' module that deals with any variable
 *	length row processing in the VBISAM library.
 * Version:
 *	$Id: vbVarLenIO.c,v 1.3 2004/06/11 22:16:16 trev_vb Exp $
 * Modification History:
 *	$Log: vbVarLenIO.c,v $
 *	Revision 1.3  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.2  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.1  2004/01/06 14:31:59  trev_vb
 *	TvB 06Jan2004 Added in VARLEN processing (In a fairly unstable sorta way)
 *	
 */
#include	"isinternal.h"

static	off_t	tTailNode (int, char *, int, int *);

struct	SVARLEN
{
	char	cRFU [INTSIZE],		// Always 0x0000
		cConst [INTSIZE],	// Always 0x7e26
		cFreeNext [QUADSIZE],	// Pointer to next in group with space
		cFreePrev [QUADSIZE],	// Pointer to prev in group with space
		cFreeThis [INTSIZE],	// Free space in THIS node
		cFreeOffset [INTSIZE],	// Position in node of free space
		cFreeCont [QUADSIZE],	// Continuation node (Only on FULL node)
		cFlag,			// Unknown, set to 0x00
#if	ISAMMODE == 1
		cUsedCount [INTSIZE],	// Number of slots in use
#else	// ISAMMODE == 1
		cUsedCount,		// Number of slots in use
#endif	// ISAMMODE == 1
		cGroup;			// Used as a reference in dictionary
};
static	char
	cNode [MAX_NODE_LENGTH];
static	struct	SVARLEN
	*psVarlenHeader = (struct SVARLEN *) cNode;
#if	ISAMMODE == 1
static	int
	iGroupSize [] =
	{
		QUADSIZE, 16, 32, 64, 128, 256, 512, 1024, 2048, MAX_NODE_LENGTH
	};
#else	// ISAMMODE == 1
static	int
	iGroupSize [] =
	{
		QUADSIZE, 8, 32, 128, 512, MAX_NODE_LENGTH
	};
#endif	// ISAMMODE == 1

int
iVBVarlenRead (int iHandle, char *pcBuffer, off_t tNodeNumber, int iSlotNumber, int iLength)
{
	char	*pcNodePtr;
	int	iResult,
		iSlotLength,
		iSlotOffset;

	while (1)
	{
		iResult = iVBBlockRead (iHandle, TRUE, tNodeNumber, cNode);
		if (iResult)
			return (-1);
		if (ldint (psVarlenHeader->cConst) != 0x7e26)
		{
			iserrno = EBADFILE;
			return (-1);
		}
		pcNodePtr = cNode + psVBFile [iHandle]->iNodeSize - 3;
		if (*pcNodePtr != 0x7c)
		{
			iserrno = EBADFILE;
			return (-1);
		}
		pcNodePtr -= ((iSlotNumber + 1) * INTSIZE * 2);
		iSlotLength = ldint (pcNodePtr);
		iSlotOffset = ldint (pcNodePtr + INTSIZE);
		if (iSlotLength <= iLength)
		{
			if (ldquad (psVarlenHeader->cFreeCont) != 0)
			{
				iserrno = EBADFILE;
				return (-1);
			}
			memcpy (pcBuffer, cNode + iSlotOffset, iLength);
			return (0);
		}
		iLength -= iSlotLength;
#if	ISAMMODE == 1
		iSlotNumber = ldint (psVarlenHeader->cFreeCont) >> 6;
		*(psVarlenHeader->cFreeCont + 1) &= 0x3f;
#else	// ISAMMODE == 1
		iSlotNumber = *(psVarlenHeader->cFreeCont);
#endif	// ISAMMODE == 1
		*(psVarlenHeader->cFreeCont) = 0;
		tNodeNumber = ldquad (psVarlenHeader->cFreeCont);
	}
	return (0);
}

// MUST populate psVBFile [iHandle]->tVarlenNode
// MUST populate psVBFile [iHandle]->iVarlenSlot
// MUST populate psVBFile [iHandle]->iVarlenLength
int
iVBVarlenWrite (int iHandle, char *pcBuffer, int iLength)
{
	int	iSlotNumber;
	off_t	tNewNode,
		tNodeNumber = 0;

	psVBFile [iHandle]->iVarlenLength = iLength;
	psVBFile [iHandle]->tVarlenNode = 0;
	// Write out 'FULL' nodes first
	while (iLength > 0)
	{
		if (iLength > (psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + sizeof (struct SVARLEN))))
		{
			tNewNode = tVBNodeCountGetNext (iHandle);
			if (tNewNode == -1)
				return (-1);
			if (tNodeNumber)
			{
				stquad (tNewNode, psVarlenHeader-> cFreeCont);
				if (iVBBlockWrite (iHandle, TRUE, tNodeNumber, cNode))
					return (-1);
			}
			else
			{
				psVBFile [iHandle]->tVarlenNode = tNewNode;
				psVBFile [iHandle]->iVarlenSlot = 0;
			}
			stint (0, psVarlenHeader->cRFU);
			stint (0x7e26, psVarlenHeader->cConst);
			stquad (0, psVarlenHeader->cFreeNext);
			stquad (0, psVarlenHeader->cFreePrev);
			stint (0, psVarlenHeader->cFreeThis);
			stint (psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE), psVarlenHeader->cFreeOffset);
			psVarlenHeader->cFlag = 0x01;
#if	ISAMMODE == 1
			stint (1, psVarlenHeader->cUsedCount);
#else	// ISAMMODE == 1
			psVarlenHeader->cUsedCount = 1;
#endif	// ISAMMODE == 1
			psVarlenHeader->cGroup = 0x00;
			memcpy (&(psVarlenHeader->cGroup) + 1, pcBuffer, psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + sizeof (struct SVARLEN)));
			pcBuffer += psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + sizeof (struct SVARLEN));
			iLength -= psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + sizeof (struct SVARLEN));
			// Length
			stint (psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + sizeof (struct SVARLEN)), cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE));
			// Offset
			stint (&(psVarlenHeader->cGroup) + 1 - cNode, cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE));
			*(cNode + psVBFile [iHandle]->iNodeSize - 3) = 0x7c;
			*(cNode + psVBFile [iHandle]->iNodeSize - 2) = 0x0;
			*(cNode + psVBFile [iHandle]->iNodeSize - 1) = 0x0;
			tNodeNumber = tNewNode;
			continue;
		}
		if (!psVBFile [iHandle]->tVarlenNode)
		{
			psVBFile [iHandle]->tVarlenNode = tNodeNumber;
			psVBFile [iHandle]->iVarlenSlot = 0;
		}
		// If tNodeNumber is != 0, we still need to write it out!
		if (tNodeNumber && !iLength)
			return (iVBBlockWrite (iHandle, TRUE, tNodeNumber, cNode));
		// Now, to deal with the 'tail'
		tNewNode = tTailNode (iHandle, pcBuffer, iLength, &iSlotNumber);
		if (tNewNode == -1)
			return (-1);
		if (tNodeNumber)
		{
			stquad (tNewNode, psVarlenHeader->cFreeCont);
#if	ISAMMODE == 1
			stint (iSlotNumber << 6 + ldint (psVarlenHeader->cFreeCont), psVarlenHeader->cFreeCont);
#else	// ISAMMODE == 1
			*psVarlenHeader->cFreeCont = iSlotNumber;
#endif	// ISAMMODE == 1
			if (iVBBlockWrite (iHandle, TRUE, tNodeNumber, cNode))
				return (-1);
			if (!psVBFile [iHandle]->tVarlenNode)
			{
				psVBFile [iHandle]->tVarlenNode = tNodeNumber;
				psVBFile [iHandle]->iVarlenSlot = 0;
			}
		}
		if (!psVBFile [iHandle]->tVarlenNode)
		{
			psVBFile [iHandle]->tVarlenNode = tNewNode;
			psVBFile [iHandle]->iVarlenSlot = iSlotNumber;
		}
		return (0);
	}
	return (-1);
}

// MUST update the group number (if applicable)
// MUST update the dictionary node (if applicable)
int
iVBVarlenDelete (int iHandle, off_t tNodeNumber, int iSlotNumber, int iLength)
{
	int	iFreeThis,
		iFreeOffset,
		iGroup,
		iIsAnyUsed,
		iLoop,
		iMoveLength,
		iOffset,
		iThisLength,
		iThisOffset,
		iUsedCount;
	off_t	tNodeNext,
		tNodePrev;

	while (iLength > 0)
	{
		if (iVBBlockRead (iHandle, TRUE, tNodeNumber, cNode))
			return (-1);
		iThisLength = ldint (cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + (iSlotNumber * 2 * INTSIZE)));
		iLength -= iThisLength;
		stint (0, cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + (iSlotNumber * 2 * INTSIZE)));
		iThisOffset = ldint (cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + (iSlotNumber * 2 * INTSIZE)));
		stint (0, cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + (iSlotNumber * 2 * INTSIZE)));
#if	ISAMMODE == 1
		iUsedCount = ldint (psVarlenHeader->cUsedCount);
#else	// ISAMMODE == 1
		iUsedCount = psVarlenHeader->cUsedCount;
#endif	// ISAMMODE == 1
		iIsAnyUsed = FALSE;
		for (iLoop = 0; iLoop < iUsedCount; iLoop++)
			if (ldint (cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + (iLoop * 2 * INTSIZE))))
			{
				iIsAnyUsed = TRUE;
				break;
			}
		if (!iIsAnyUsed)
		{
			tNodeNext = ldquad (psVarlenHeader->cFreeNext);
			tNodePrev = ldquad (psVarlenHeader->cFreePrev);
			iGroup = psVarlenHeader->cGroup;
			if (ldquad (psVBFile [iHandle]->sDictNode.cVarlenG0 + (iGroup * QUADSIZE)) == tNodeNumber)
				stquad (tNodeNext, psVBFile [iHandle]->sDictNode.cVarlenG0 + (iGroup * QUADSIZE));
			iVBNodeFree (iHandle, tNodeNumber);
			tNodeNumber = ldquad (psVarlenHeader->cFreeCont);
			if (tNodeNext)
			{
				if (iVBBlockRead (iHandle, TRUE, tNodeNext, cNode))
					return (-1);
				stquad (tNodePrev, psVarlenHeader->cFreePrev);
				if (iVBBlockWrite (iHandle, TRUE, tNodeNext, cNode))
					return (-1);
			}
			if (tNodePrev)
			{
				if (iVBBlockRead (iHandle, TRUE, tNodePrev, cNode))
					return (-1);
				stquad (tNodeNext, psVarlenHeader->cFreeNext);
				if (iVBBlockWrite (iHandle, TRUE, tNodePrev, cNode))
					return (-1);
			}
			continue;
		}
		iFreeThis = ldint (psVarlenHeader->cFreeThis);
		iFreeOffset = ldint (psVarlenHeader->cFreeOffset);
		if (iSlotNumber == iUsedCount)
		{
			iUsedCount--;
#if	ISAMMODE == 1
			stint (iUsedCount - 1, psVarlenHeader->cUsedCount);
#else	// ISAMMODE == 1
			psVarlenHeader->cUsedCount = iUsedCount;
#endif	// ISAMMODE == 1
			iFreeThis += (INTSIZE * 2);
		}
		iMoveLength = psVBFile [iHandle]->iNodeSize - (iThisOffset + iThisLength);
		iMoveLength -= (3 + (iUsedCount * INTSIZE * 2));
		memmove (cNode + iThisOffset, cNode + iThisOffset + iThisLength, iMoveLength);
		iFreeOffset -= iThisLength;
		iFreeThis += iThisLength;
		stint (iFreeThis, psVarlenHeader->cFreeThis);
		stint (iFreeOffset, psVarlenHeader->cFreeOffset);
		memset (cNode + iFreeOffset, 0, iFreeThis);
		for (iLoop = 0; iLoop < iUsedCount; iLoop++)
		{
			iOffset = ldint (cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + (iLoop * 2 * INTSIZE)));
			if (iOffset > iThisOffset)
				stint (iOffset - iThisLength, cNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + (iLoop * 2 * INTSIZE)));
		}
		if (iVBBlockWrite (iHandle, TRUE, tNodeNumber, cNode))
			return (-1);
		tNodeNumber = ldquad (psVarlenHeader->cFreeCont);
	}
	psVBFile [iHandle]->tVarlenNode = 0;
	return (0);
}

// Locate space for, and fill in the tail content.
// Write out the tail node, and return the tail node number.
// Fill in the slot number used in the tail node
static	off_t
tTailNode (int iHandle, char *pcBuffer, int iLength, int *piSlotNumber)
{
	char	cLclNode [MAX_NODE_LENGTH],
		cNextPrev [MAX_NODE_LENGTH],
		*pcNodePtr;
	int	iFreeThis,
		iFreeOffset,
		iGroup,
		iSlotNumber;
	struct	SVARLEN
		*psHdr = (struct SVARLEN *) cLclNode,
		*psNPHdr = (struct SVARLEN *) cNextPrev;
	off_t	tNodeNumber = 0,
		tNodeNext,
		tNodePrev;

	// Determine which group to START with
	for (iGroup = 0; iGroupSize [iGroup] < iLength + INTSIZE + INTSIZE; iGroup++)
		;	// Do nothing!
	iGroup--;
	while (!tNodeNumber)
	{
		tNodeNumber = ldquad (psVBFile [iHandle]->sDictNode.cVarlenG0 + (iGroup * QUADSIZE));
		while (tNodeNumber)
		{
			if (iVBBlockRead (iHandle, TRUE, tNodeNumber, cLclNode))
				return (-1);
			if (ldint (psHdr->cFreeThis) < (iLength + INTSIZE + INTSIZE))
				tNodeNumber = ldquad (psHdr->cFreeNext);
			else
				break;
		}
		if (tNodeNumber)
			break;
		if (iGroupSize [iGroup] >= MAX_NODE_LENGTH)
			break;
		iGroup++;
	}
	if (!tNodeNumber)
	{
		tNodeNumber = tVBNodeCountGetNext (iHandle);
		if (tNodeNumber == -1)
			return (tNodeNumber);
		memset (cLclNode, 0, MAX_NODE_LENGTH);
		stint (0x7e26, psHdr->cConst);
		stint (psVBFile [iHandle]->iNodeSize - (sizeof (struct SVARLEN) + 3 + INTSIZE + INTSIZE), psHdr->cFreeThis);
		stint (sizeof (struct SVARLEN), psHdr->cFreeOffset);
		psHdr->cGroup = -1;
		cLclNode [psVBFile [iHandle]->iNodeSize - 3] = 0x7c;
		*piSlotNumber = 0;
#if	ISAMMODE == 1
		stint (1, psHdr->cUsedCount);
#else	// ISAMMODE == 1
		psHdr->cUsedCount = 1;
#endif	// ISAMMODE == 1
	}
	else
	{
		*piSlotNumber = -1;
		pcNodePtr = cLclNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE);
#if	ISAMMODE == 1
		for (iSlotNumber = 0; iSlotNumber < ldint (psHdr->cUsedCount); iSlotNumber++)
#else	// ISAMMODE == 1
		for (iSlotNumber = 0; iSlotNumber < psHdr->cUsedCount; iSlotNumber++)
#endif	// ISAMMODE == 1
			if (ldint (pcNodePtr))
			{
				pcNodePtr -= (INTSIZE * 2);
				continue;
			}
			else
			{
				*piSlotNumber = iSlotNumber;
				break;
			}
		if (*piSlotNumber == -1)
		{
#if	ISAMMODE == 1
			*piSlotNumber = ldint (psHdr->cUsedCount);
			stint (*(piSlotNumber) + 1, psHdr->cUsedCount);
#else	// ISAMMODE == 1
			*piSlotNumber = psHdr->cUsedCount;
			psHdr->cUsedCount++;
#endif	// ISAMMODE == 1
			stint (ldint (psHdr->cFreeThis) - (INTSIZE * 2), psHdr->cFreeThis);
		}
	}
	iFreeThis = ldint (psHdr->cFreeThis);
	iFreeOffset = ldint (psHdr->cFreeOffset);
	pcNodePtr = cLclNode + psVBFile [iHandle]->iNodeSize - (3 + INTSIZE + INTSIZE + (*piSlotNumber * INTSIZE * 2));
	stint (iLength, pcNodePtr);
	stint (iFreeOffset, pcNodePtr + INTSIZE);
	memcpy (cLclNode + iFreeOffset, pcBuffer, iLength);
	iFreeThis -= iLength;
	stint (iFreeThis, psHdr->cFreeThis);
	iFreeOffset += iLength;
	stint (iFreeOffset, psHdr->cFreeOffset);
	// Determine which 'group' the node belongs in now
	for (iGroup = 0; iGroupSize [iGroup] < iFreeThis; iGroup++)
		;	// Do nothing!
	iGroup--;
	if (iGroup != psHdr->cGroup)
	{
		tNodeNext = ldquad (psHdr->cFreeNext);
		tNodePrev = ldquad (psHdr->cFreePrev);
		if (tNodePrev)
		{
			if (iVBBlockRead (iHandle, TRUE, tNodePrev, cNextPrev))
				return (-1);
			stquad (tNodeNext, psNPHdr->cFreeNext);
			if (iVBBlockWrite (iHandle, TRUE, tNodePrev, cNextPrev))
				return (-1);
		}
		if (tNodeNext)
		{
			if (iVBBlockRead (iHandle, TRUE, tNodeNext, cNextPrev))
				return (-1);
			stquad (tNodePrev, psNPHdr->cFreePrev);
			if (iVBBlockWrite (iHandle, TRUE, tNodeNext, cNextPrev))
				return (-1);
		}
		stquad (0, psHdr->cFreePrev);
		tNodeNext = ldquad (psVBFile [iHandle]->sDictNode.cVarlenG0 + (iGroup * QUADSIZE));
		stquad (tNodeNext, psHdr->cFreeNext);
		if (tNodeNext)
		{
			if (iVBBlockRead (iHandle, TRUE, tNodeNext, cNextPrev))
				return (-1);
			stquad (tNodeNumber, psNPHdr->cFreePrev);
			if (iVBBlockWrite (iHandle, TRUE, tNodeNext, cNextPrev))
				return (-1);
		}
		if (psHdr->cGroup >= 0)
			stquad (tNodeNext, psVBFile [iHandle]->sDictNode.cVarlenG0 + (psHdr->cGroup * QUADSIZE));
		stquad (tNodeNumber, psVBFile [iHandle]->sDictNode.cVarlenG0 + (iGroup * QUADSIZE));
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
		psHdr->cGroup = iGroup;
		if (iVBBlockWrite (iHandle, TRUE, tNodeNumber, cLclNode))
			return (-1);
	}
	if (iVBBlockWrite (iHandle, TRUE, tNodeNumber, cLclNode))
		return (-1);
	return (tNodeNumber);
}
