/*
 * Title:	vbCheck.c
 * Copyright:	(C) 2004 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	04Jun2004
 * Description:
 *	This program tests and if needed, rebuilds indexes
 * Version:
 *	$Id: vbCheck.c,v 1.3 2004/06/16 10:53:56 trev_vb Exp $
 * Modification History:
 *	$Log: vbCheck.c,v $
 *	Revision 1.3  2004/06/16 10:53:56  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
 *	
 *	Revision 1.2  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.1  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 */
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	"isinternal.h"

char	*gpsDataRow,		// Buffer to hold rows read
	*gpsDataMap [2],	// Bitmap of 'used' data rows
	*gpsIndexMap [2];	// Bitmap of 'used' index nodes
int	giRebuildDataFree,	// If set, we need to rebuild data free list
	giRebuildIndexFree,	// If set, we need to rebuild index free list
	giRebuildKey [MAXSUBS];	// For any are SET, we need to rebuild that key
off_t	gtLastUsedData,		// Last row USED in data file
	gtDataSize,		// # Rows in data file
	gtIndexSize;		// # Nodes in index file

static	int
iBitTestAndSet (char *psMap, off_t tBit)
{
	tBit--;
	switch (tBit % 8)
	{
	case	0:
		if (psMap [tBit / 8] & 0x80)
			return (1);
		psMap [tBit / 8] |= 0x80;
		break;
	case	1:
		if (psMap [tBit / 8] & 0x40)
			return (1);
		psMap [tBit / 8] |= 0x40;
		break;
	case	2:
		if (psMap [tBit / 8] & 0x20)
			return (1);
		psMap [tBit / 8] |= 0x20;
		break;
	case	3:
		if (psMap [tBit / 8] & 0x10)
			return (1);
		psMap [tBit / 8] |= 0x10;
		break;
	case	4:
		if (psMap [tBit / 8] & 0x08)
			return (1);
		psMap [tBit / 8] |= 0x08;
		break;
	case	5:
		if (psMap [tBit / 8] & 0x04)
			return (1);
		psMap [tBit / 8] |= 0x04;
		break;
	case	6:
		if (psMap [tBit / 8] & 0x02)
			return (1);
		psMap [tBit / 8] |= 0x02;
		break;
	case	7:
		if (psMap [tBit / 8] & 0x01)
			return (1);
		psMap [tBit / 8] |= 0x01;
		break;
	}
	return (0);
}

static	int
iBitTestAndReset (char *psMap, off_t tBit)
{
	tBit--;
	switch (tBit % 8)
	{
	case	0:
		if (!(psMap [tBit / 8] & 0x80))
			return (1);
		psMap [tBit / 8] ^= 0x80;
		break;
	case	1:
		if (!(psMap [tBit / 8] & 0x40))
			return (1);
		psMap [tBit / 8] ^= 0x40;
		break;
	case	2:
		if (!(psMap [tBit / 8] & 0x20))
			return (1);
		psMap [tBit / 8] ^= 0x20;
		break;
	case	3:
		if (!(psMap [tBit / 8] & 0x10))
			return (1);
		psMap [tBit / 8] ^= 0x10;
		break;
	case	4:
		if (!(psMap [tBit / 8] & 0x08))
			return (1);
		psMap [tBit / 8] ^= 0x08;
		break;
	case	5:
		if (!(psMap [tBit / 8] & 0x04))
			return (1);
		psMap [tBit / 8] ^= 0x04;
		break;
	case	6:
		if (!(psMap [tBit / 8] & 0x02))
			return (1);
		psMap [tBit / 8] ^= 0x02;
		break;
	case	7:
		if (!(psMap [tBit / 8] & 0x01))
			return (1);
		psMap [tBit / 8] ^= 0x01;
		break;
	}
	return (0);
}

static	int
iPreamble (iHandle)
{
	struct	stat
		sStat;

	printf ("Table node size: %d bytes\n", psVBFile [iHandle]->iNodeSize);
	gpsDataRow = (char *) pvVBMalloc (psVBFile [iHandle]->iMaxRowLength);
	if (!gpsDataRow)
	{
		printf ("Unable to allocate data row buffer!\n");
		return (-1);
	}

	if (fstat (sVBFile [psVBFile [iHandle]->iDataHandle].iHandle, &sStat))
	{
		printf ("Unable to get data status!\n");
		return (-1);
	}
	if (psVBFile [iHandle]->iOpenMode & ISVARLEN)
		gtDataSize = (off_t) (sStat.st_size + psVBFile [iHandle]->iMinRowLength + INTSIZE + QUADSIZE) / (psVBFile [iHandle]->iMinRowLength + INTSIZE + QUADSIZE + 1);
	else
		gtDataSize = (off_t) (sStat.st_size + psVBFile [iHandle]->iMinRowLength) / (psVBFile [iHandle]->iMinRowLength + 1);
	gpsDataMap [0] = (char *) pvVBMalloc ((int) ((gtDataSize + 7) / 8));
	if (gpsDataMap [0] == (char *) 0)
	{
		printf ("Unable to allocate node map!\n");
		return (-1);
	}
	gpsDataMap [1] = (char *) pvVBMalloc ((int) ((gtDataSize + 7) / 8));
	if (gpsDataMap [1] == (char *) 0)
	{
		printf ("Unable to allocate node map!\n");
		return (-1);
	}
	memset (gpsDataMap [0], 0, (int) ((gtDataSize + 7) / 8));

	if (fstat (sVBFile [psVBFile [iHandle]->iIndexHandle].iHandle, &sStat))
	{
		printf ("Unable to get index status!\n");
		return (-1);
	}
	gtIndexSize = (off_t) (sStat.st_size + psVBFile [iHandle]->iNodeSize - 1) / psVBFile [iHandle]->iNodeSize;
	gpsIndexMap [0] = (char *) pvVBMalloc ((int) ((gtIndexSize + 7) / 8));
	if (gpsIndexMap [0] == (char *) 0)
	{
		printf ("Unable to allocate node map!\n");
		return (-1);
	}
	gpsIndexMap [1] = (char *) pvVBMalloc ((int) ((gtIndexSize + 7) / 8));
	if (gpsIndexMap [1] == (char *) 0)
	{
		printf ("Unable to allocate node map!\n");
		return (-1);
	}
	memset (gpsIndexMap [0], 0, (int) ((gtIndexSize + 7) / 8));
	switch (gtIndexSize % 8)
	{
	case	1:
		*(gpsIndexMap [0] + ((gtIndexSize - 1) / 8)) = 0x7f;
		break;
	case	2:
		*(gpsIndexMap [0] + ((gtIndexSize - 1) / 8)) = 0x3f;
		break;
	case	3:
		*(gpsIndexMap [0] + ((gtIndexSize - 1) / 8)) = 0x1f;
		break;
	case	4:
		*(gpsIndexMap [0] + ((gtIndexSize - 1) / 8)) = 0x0f;
		break;
	case	5:
		*(gpsIndexMap [0] + ((gtIndexSize - 1) / 8)) = 0x07;
		break;
	case	6:
		*(gpsIndexMap [0] + ((gtIndexSize - 1) / 8)) = 0x03;
		break;
	case	7:
		*(gpsIndexMap [0] + ((gtIndexSize - 1) / 8)) = 0x01;
		break;
	}
	iBitTestAndSet (gpsIndexMap [0], 1);	// Dictionary node!

	return (0);
}

static	int
iDataCheck (int iHandle)
{
	int	iDeleted;
	off_t	tLoop;

	// Mark the entries used by *LIVE* data rows
	for (tLoop = 1; tLoop <= gtDataSize; tLoop++)
	{
		if (iVBDataRead (iHandle, gpsDataRow, &iDeleted, tLoop, FALSE))
			continue; // A data file read error! Leave it as free!
		if (!iDeleted)
		{
			gtLastUsedData = tLoop;
			// MAYBE we could add index verification here
			// That'd be in SUPER THOROUGH mode only!
			iBitTestAndSet (gpsDataMap [0], tLoop);
		}
		// BUG - We also need to set gpsIndexMap [1] for those node(s)
		// BUG - that were at least partially consumed by VARLEN data!
	}
	return (0);
}

static	int
iDataFreeCheck (int iHandle)
{
	int	iLoop,
		iResult;
	off_t	tFreeHead,
		tFreeRow,
		tHoldHead,
		tLoop;

	giRebuildDataFree = TRUE;
	// Mark the entries used by the free data list
	tHoldHead = ldquad (psVBFile [iHandle]->sDictNode.cDataFree);
	stquad (0, psVBFile [iHandle]->sDictNode.cDataFree);
	tFreeHead = tHoldHead;
	memcpy (gpsDataMap [1], gpsDataMap [0], (int) ((gtDataSize + 7) / 8));
	memcpy (gpsIndexMap [1], gpsIndexMap [0], (int) ((gtIndexSize + 7) / 8));
	while (tFreeHead)
	{
		// If the freelist node is > index.EOF, it must be bullshit!
		if (tFreeHead > gtIndexSize)
			return (0);
		iResult = iVBBlockRead (iHandle, TRUE, tFreeHead, cVBNode [0]);
		if (iResult)
			return (0);
		/*
		 * If the node has the WRONG signature, then we've got
		 * a corrupt data free list.  We'll rebuild it later!
		 */
// Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO)
#if	ISAMMODE == 1
		if (cVBNode [0][psVBFile [iHandle]->iNodeSize - 2] != 0x7f)
			return (0);
#endif	// ISAMMODE == 1
		if (cVBNode [0][psVBFile [iHandle]->iNodeSize - 3] != -1)
			return (0);
		if (ldint (cVBNode [0]) > (psVBFile [iHandle]->iNodeSize - 3))
			return (0);
		/*
		 * If the node is already 'used' then we have a corrupt
		 * data free list (circular reference).
		 * We'll rebuild the free list later
		 */
		if (iBitTestAndSet (gpsIndexMap [1], tFreeHead))
			return (0);
		for (iLoop = INTSIZE + QUADSIZE; iLoop < ldint (cVBNode [0]); iLoop += QUADSIZE)
		{
			tFreeRow = ldquad (cVBNode [0] + iLoop);
			/*
			 * If the row is NOT deleted, then the free
			 * list is screwed so we ignore it and rebuild it
			 * later.
			 */
			if (iBitTestAndSet (gpsDataMap [1], tFreeRow))
				return (0);
		}
		tFreeHead = ldquad (cVBNode [0] + INTSIZE);
	}
	// Set the few bits between the last row used and EOF to 'used'
	for (tLoop = gtLastUsedData + 1; tLoop <= ((gtDataSize + 7) / 8) * 8; tLoop++)
		iBitTestAndSet (gpsDataMap [1], tLoop);
	for (tLoop = 0; tLoop < (gtDataSize + 7) / 8; tLoop++)
		if (gpsDataMap [1] [tLoop] != -1)
			return (0);
	// Seems the data file is 'intact' so we'll keep the allocation lists!
	memcpy (gpsDataMap [0], gpsDataMap [1], (int) ((gtDataSize + 7) / 8));
	memcpy (gpsIndexMap [0], gpsIndexMap [1], (int) ((gtIndexSize + 7) / 8));
	stquad (tHoldHead, psVBFile [iHandle]->sDictNode.cDataFree);
	giRebuildDataFree = FALSE;

	return (0);
}

static	int
iIndexFreeCheck (int iHandle)
{
	int	iLoop,
		iResult;
	off_t	tFreeHead,
		tFreeNode,
		tHoldHead;

	// Mark the entries used by the free data list
	giRebuildIndexFree = TRUE;
	tFreeHead = ldquad (psVBFile [iHandle]->sDictNode.cNodeFree);
	tHoldHead = tFreeHead;
	stquad (0, psVBFile [iHandle]->sDictNode.cNodeFree);
	memcpy (gpsIndexMap [1], gpsIndexMap [0], (int) ((gtIndexSize + 7) / 8));
	while (tFreeHead)
	{
		// If the freelist node is > index.EOF, it must be bullshit!
		if (tFreeHead > ldquad (psVBFile [iHandle]->sDictNode.cNodeCount))
			return (0);
		if (tFreeHead > gtIndexSize)
			return (0);
		iResult = iVBBlockRead (iHandle, TRUE, tFreeHead, cVBNode [0]);
		if (iResult)
			return (0);
		/*
		 * If the node has the WRONG signature, then we've got
		 * a corrupt data free list.  We'll rebuild it later!
		 */
// Guido pointed out that C-ISAM is not 100% C-ISAM compatible (LMAO)
#if	ISAMMODE == 1
		if (cVBNode [0][psVBFile [iHandle]->iNodeSize - 2] != 0x7f)
			return (0);
#endif	// ISAMMODE == 1
		if (cVBNode [0][psVBFile [iHandle]->iNodeSize - 3] != -2)
			return (0);
		if (ldint (cVBNode [0]) > (psVBFile [iHandle]->iNodeSize - 3))
			return (0);
		/*
		 * If the node is already 'used' then we have a corrupt
		 * index free list (circular reference).
		 * We'll rebuild the free list later
		 */
		if (iBitTestAndSet (gpsIndexMap [1], tFreeHead))
			return (0);
		for (iLoop = INTSIZE + QUADSIZE; iLoop < ldint (cVBNode [0]); iLoop += QUADSIZE)
		{
			tFreeNode = ldquad (cVBNode [0] + iLoop);
			/*
			 * If the row is NOT deleted, then the free
			 * list is screwed so we ignore it and rebuild it
			 * later.
			 */
			if (iBitTestAndSet (gpsIndexMap [1], tFreeNode))
				return (0);
		}
		tFreeHead = ldquad (cVBNode [0] + INTSIZE);
	}
	// Seems the index free list is 'intact' so we'll keep the allocation lists!
	memcpy (gpsIndexMap [0], gpsIndexMap [1], (int) ((gtIndexSize + 7) / 8));
	stquad (tHoldHead, psVBFile [iHandle]->sDictNode.cNodeFree);
	giRebuildIndexFree = FALSE;

	return (0);
}

static	int
iCheckKeydesc (int iHandle)
{
	off_t	tNode = ldquad (psVBFile [iHandle]->sDictNode.cNodeKeydesc);

	while (tNode)
	{
		if (tNode > gtIndexSize)
			return (1);
		if (iBitTestAndSet (gpsIndexMap [0], tNode))
			return (1);
		if (iVBBlockRead (iHandle, TRUE, tNode, cVBNode [0]))
			return (1);
		if (cVBNode [0][psVBFile [iHandle]->iNodeSize - 3] != -1)
			return (1);
		if (cVBNode [0][psVBFile [iHandle]->iNodeSize - 2] != 0x7e)
			return (1);
		if (cVBNode [0][psVBFile [iHandle]->iNodeSize - 1] != 0)
			return (1);
		tNode = ldquad (cVBNode [0] + INTSIZE);
	}
	return (0);
}

static	int
iCheckTree (int iHandle, int iKey, off_t tNode, int iLevel)
{
	int	iLoop;
	struct	VBTREE
		sTree;

	memset (&sTree, 0, sizeof (sTree));
	if (tNode > gtIndexSize)
		return (1);
	if (iBitTestAndSet (gpsIndexMap [1], tNode))
		return (1);
	sTree.tTransNumber = -1;
	if (iVBNodeLoad (iHandle, iKey, &sTree, tNode, iLevel))
		return (1);
	for (iLoop = 0; iLoop < sTree.sFlags.iKeysInNode; iLoop++)
	{
		if (sTree.psKeyList [iLoop]->sFlags.iIsDummy)
			continue;
		if (iLoop > 0 && iVBKeyCompare (iHandle, iKey, 0, sTree.psKeyList [iLoop - 1]->cKey, sTree.psKeyList [iLoop]->cKey) > 0)
		{
			printf ("Index is out of order!\n");
			vVBKeyAllFree (iHandle, iKey, &sTree);
			return (1);
		}
		if (iLoop > 0 && iVBKeyCompare (iHandle, iKey, 0, sTree.psKeyList [iLoop - 1]->cKey, sTree.psKeyList [iLoop]->cKey) == 0 && sTree.psKeyList [iLoop - 1]->tDupNumber >= sTree.psKeyList [iLoop]->tDupNumber)
		{
			printf ("Index is out of order!\n");
			vVBKeyAllFree (iHandle, iKey, &sTree);
			return (1);
		}
		if (sTree.sFlags.iLevel)
		{
			if (iCheckTree (iHandle, iKey, sTree.psKeyList [iLoop]->tRowNode, sTree.sFlags.iLevel))
			{
				vVBKeyAllFree (iHandle, iKey, &sTree);
				return (1);
			}
		}
		else
		{
			if (iBitTestAndReset (gpsDataMap [1], sTree.psKeyList [iLoop]->tRowNode))
			{
				vVBKeyAllFree (iHandle, iKey, &sTree);
				printf ("Bad data row pointer!\n");
				return (1);
			}
		}
	}
	vVBKeyAllFree (iHandle, iKey, &sTree);
	return (0);
}

static	int
iCheckKey (int iHandle, int iKey)
{
	off_t	tLoop;

	memcpy (gpsDataMap [1], gpsDataMap [0], (int) ((gtDataSize + 7) / 8));
	memcpy (gpsIndexMap [1], gpsIndexMap [0], (int) ((gtIndexSize + 7) / 8));
	if (iVBBlockRead (iHandle, TRUE, psVBFile [iHandle]->psKeydesc [iKey]->tRootNode, cVBNode [0]))
		return (1);
	if (iCheckTree (iHandle, iKey, psVBFile [iHandle]->psKeydesc [iKey]->tRootNode, cVBNode [0][psVBFile [iHandle]->iNodeSize - 2] + 1))
		return (1);
	for (tLoop = 0; tLoop < (gtDataSize + 7) / 8; tLoop++)
		if (gpsDataMap [1][tLoop])
			return (1);
	memcpy (gpsIndexMap [0], gpsIndexMap [1], (int) ((gtIndexSize + 7) / 8));
	return (0);
}

static	int
iIndexCheck (int iHandle)
{
	int	iKey,
		iLoop,
		iPart;

	for (iLoop = 0; iLoop < MAXSUBS; iLoop++)
		giRebuildKey [iLoop] = FALSE;
	// If the keydesc node(s) are bad, we QUIT this table altogether
	if (iCheckKeydesc (iHandle))
	{
		printf ("Corrupted Key Descriptor node(s)! Can't continue!\n");
		return (-1);
	}
	for (iKey = 0; iKey < psVBFile [iHandle]->iNKeys; iKey++)
	{
		printf ("Index %d: ", iKey + 1);
		printf ("%s ", psVBFile [iHandle]->psKeydesc [iKey]->iFlags & ISDUPS ? "ISDUPS" : "ISNODUPS");
		printf ("%s", psVBFile [iHandle]->psKeydesc [iKey]->iFlags & DCOMPRESS ? "DCOMPRESS " : "");
		printf ("%s", psVBFile [iHandle]->psKeydesc [iKey]->iFlags & LCOMPRESS ? "LCOMPRESS " : "");
		printf ("%s\n", psVBFile [iHandle]->psKeydesc [iKey]->iFlags & TCOMPRESS ? "TCOMPRESS" : "");
		for (iPart = 0; iPart < psVBFile [iHandle]->psKeydesc [iKey]->iNParts; iPart++)
		{
			printf (" Part %d: ", iPart + 1);
			printf ("%c%d,", 0x28, psVBFile [iHandle]->psKeydesc [iKey]->sPart [iPart].iStart);
			printf ("%d,", psVBFile [iHandle]->psKeydesc [iKey]->sPart [iPart].iLength);
			switch (psVBFile [iHandle]->psKeydesc [iKey]->sPart [iPart].iType & ~ISDESC)
			{
			case	CHARTYPE:
				printf ("CHARTYPE%c", 0x29);
				break;
			case	INTTYPE:
				printf ("INTTYPE%c", 0x29);
				break;
			case	LONGTYPE:
				printf ("LONGTYPE%c", 0x29);
				break;
			case	DOUBLETYPE:
				printf ("DOUBLETYPE%c", 0x29);
				break;
			case	FLOATTYPE:
				printf ("FLOATTYPE%c", 0x29);
				break;
			case	QUADTYPE:
				printf ("QUADTYPE%c", 0x29);
				break;
			default:
				printf ("UNKNOWN TYPE%c", 0x29);
				break;
			}
			if (psVBFile [iHandle]->psKeydesc [iKey]->sPart [iPart].iType & ISDESC)
				printf (" ISDESC\n");
			else
				printf ("\n");
		}
		// If the index is screwed, write out an EMPTY root node and
		// flag the index for a complete reconstruction later on
		if (iCheckKey (iHandle, iKey))
		{
			memset (cVBNode [0], 0, MAX_NODE_LENGTH);
			stint (2, cVBNode [0]);
			iVBBlockWrite (iHandle, TRUE, psVBFile [iHandle]->psKeydesc [iKey]->tRootNode, cVBNode [0]);
			iBitTestAndSet (gpsIndexMap [0], psVBFile [iHandle]->psKeydesc [iKey]->tRootNode);
			vVBTreeAllFree (iHandle, iKey, psVBFile [iHandle]->psTree [iKey]);
			psVBFile [iHandle]->psTree [iKey] = VBTREE_NULL;
			giRebuildKey [iKey] = TRUE;
		}
	}
	return (0);
}

static	void
vRebuildIndexFree (int iHandle)
{
	off_t	tLoop;

	printf ("Rebuilding index free list\n");
	if (gtIndexSize > ldquad (psVBFile [iHandle]->sDictNode.cNodeCount))
		stquad (gtIndexSize, psVBFile [iHandle]->sDictNode.cNodeCount);
	for (tLoop = 1; tLoop <= gtIndexSize; tLoop++)
	{
		if (iBitTestAndSet (gpsIndexMap [0], tLoop))
			continue;
		iserrno = iVBNodeFree (iHandle, tLoop);
		if (iserrno)
		{
			printf ("Error %d rebuilding index free list!\n", iserrno);
			return;
		}
	}
	return;
}

static	void
vRebuildDataFree (int iHandle)
{
	off_t	tLoop;

	printf ("Rebuilding data free list\n");
	for (tLoop = 1; tLoop <= gtLastUsedData; tLoop++)
	{
		if (iBitTestAndSet (gpsDataMap [0], tLoop))
			continue;
		iserrno = iVBDataFree (iHandle, tLoop + 1);
		if (iserrno)
		{
			printf ("Error %d rebuilding data free list!\n", iserrno);
			return;
		}
	}
	return;
}

static	void
vAddKeyForRow (int iHandle, int iKey, off_t tRowNumber)
{
	char	cKeyValue [VB_MAX_KEYLEN];
	int	iResult;
	struct	VBKEY
		*psKey;
	off_t	tDupNumber;

	if (psVBFile [iHandle]->psKeydesc [iKey]->iNParts == 0)
		return;
	vVBMakeKey (iHandle, iKey, gpsDataRow, cKeyValue);
	iResult = iVBKeySearch (iHandle, ISGREAT, iKey, 0, cKeyValue, 0);
	tDupNumber = 0;
	if (iResult >= 0 && !iVBKeyLoad (iHandle, iKey, ISPREV, FALSE, &psKey) && !memcmp (psKey->cKey, cKeyValue, psVBFile [iHandle]->psKeydesc [iKey]->iKeyLength))
	{
		iserrno = EDUPL;
		if (psVBFile [iHandle]->psKeydesc [iKey]->iFlags & ISDUPS)
			tDupNumber = psKey->tDupNumber + 1;
		else
		{
			printf ("Error! Duplicate entry in key %d\n", iKey);
			return;
		}
		iResult = iVBKeySearch (iHandle, ISGTEQ, iKey, 0, cKeyValue, tDupNumber);
	}

	iVBKeyInsert (iHandle, VBTREE_NULL, iKey, cKeyValue, tRowNumber, tDupNumber, VBTREE_NULL);
	return;
}

static	void
vRebuildKeys (int iHandle)
{
	int	iDeleted,
		iKey;
	off_t	tRowNumber;

	// Mark the entries used by *LIVE* data rows
	for (tRowNumber = 1; tRowNumber <= gtDataSize; tRowNumber++)
	{
		if (iVBDataRead (iHandle, gpsDataRow, &iDeleted, tRowNumber, FALSE))
			continue; // A data file read error! Leave it as free!
		if (!iDeleted)
			for (iKey = 0; iKey < MAXSUBS; iKey++)
				if (giRebuildKey [iKey])
					vAddKeyForRow (iHandle, iKey, tRowNumber);
	}
	return;
}

static	int
iPostamble (int iHandle)
{
	int	iKeyCountToRebuild = 0,
		iLoop;
	off_t	tLoop;

	for (tLoop = 0; tLoop < (gtIndexSize + 7) / 8; tLoop++)
		if (gpsIndexMap [0][tLoop] != -1)
			giRebuildIndexFree = TRUE;

	if (giRebuildIndexFree)
		vRebuildIndexFree (iHandle);
	if (giRebuildDataFree)
		vRebuildDataFree (iHandle);
	for (iLoop = 0; iLoop < MAXSUBS; iLoop++)
		if (giRebuildKey [iLoop])
		{
			if (!iKeyCountToRebuild)
				printf ("Rebuilding keys: ");
			printf ("%d ", iLoop + 1);
			iKeyCountToRebuild++;
		}
	if (iKeyCountToRebuild)
	{
		vRebuildKeys (iHandle);
		printf ("\n");
	}
	stquad (gtLastUsedData, psVBFile [iHandle]->sDictNode.cDataCount);
	// Other stuff here
	vVBFree (gpsDataRow, psVBFile [iHandle]->iMaxRowLength);
	
	vVBFree (gpsDataMap [0], ((int) ((gtDataSize + 7) / 8)));
	vVBFree (gpsDataMap [1], ((int) ((gtDataSize + 7) / 8)));
	vVBFree (gpsIndexMap [0], ((int) ((gtIndexSize + 7) / 8)));
	vVBFree (gpsIndexMap [1], ((int) ((gtIndexSize + 7) / 8)));
	return (0);
}

static	void
vProcess (int iHandle)
{
	if (iPreamble (iHandle))
		return;
	if (iDataCheck (iHandle))
		return;
	if (iIndexCheck (iHandle))
		return;
	if (iDataFreeCheck (iHandle))
		return;
	if (iIndexFreeCheck (iHandle))
		return;
	if (iPostamble (iHandle))
		return;
	return;
}

int
main (int iArgc, char **ppcArgv)
{
	int	iHandle,
		iLoop,
		iResult;

	if (iArgc == 1)
	{
		printf ("Usage:\n\t%s [FILENAME...]\n", ppcArgv [0]);
		exit (1);
	}

	for (iLoop = 1; iLoop < iArgc; iLoop++)
	{
		iHandle = isopen (ppcArgv [iLoop], ISINOUT+ISEXCLLOCK);
		if (iHandle < 0 && iserrno == EROWSIZE)
		{
			printf ("Sorry, but %s doesn't process ISVARLEN files yet\n", ppcArgv [0]);
			printf ("File %s contains variable length rows!\n", ppcArgv [iLoop]);
			isclose (iHandle);
			continue;
		}
		//if (iHandle < 0 && iserrno == EROWSIZE)
			//iHandle = isopen (ppcArgv [iLoop], ISINOUT+ISVARLEN+ISEXCLLOCK);
		if (iHandle < 0)
		{
			printf ("Error %d opening %s\n", iserrno, ppcArgv [iLoop]);
			continue;
		}

		printf ("Processing: %s\n", ppcArgv [iLoop]);
		iVBEnter (iHandle, TRUE);
		vProcess (iHandle);
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x06;
		iVBExit (iHandle);

		iResult = isclose (iHandle);
		if (iResult < 0)
		{
			printf ("Error %d closing %s\n", iserrno, ppcArgv [iLoop]);
			continue;
		}

	}
	return (0);
}
