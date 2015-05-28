/*
 * Title:	vbNodeMemIO.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	25Nov2003
 * Description:
 *	This module handles the issues of reading a node into the memory-based
 *	linked-lists and writing a node to disk from the memory-based linked-
 *	lists.  The latter possibly causing a split to occur.
 * Version:
 *	$Id: vbNodeMemIO.c,v 1.4 2004/06/16 10:53:56 trev_vb Exp $
 * Modification History:
 *	$Log: vbNodeMemIO.c,v $
 *	Revision 1.4  2004/06/16 10:53:56  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
 *	
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
 *	
 */
#include	"isinternal.h"

/*
 * Prototypes
 */
int	iVBNodeLoad (int, int, struct VBTREE *, off_t, int);
int	iVBNodeSave (int, int, struct VBTREE *, off_t, int, int);
static	int	iQuickNodeSave (int, struct VBTREE *, off_t, struct keydesc *, int, int);
static	void	vFixTOFEOF (int, int);
static	int	iNodeSplit (int, int, struct VBTREE *, struct VBKEY *);
static	int	iNewRoot (int, int, struct VBTREE *, struct VBTREE *, struct VBTREE *, struct VBKEY *[], off_t, off_t);
#define		TCC	(' ')		// Trailing Compression Character
/*
 * Local scope variables
 */


/*
 * Name:
 *	int	iVBNodeLoad (int iHandle, int iKeyNumber, struct VBTREE *psTree, off_t tNodeNumber, int iPrevLvl);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The index number in question
 *	struct VBTREE	*psTree
 *		The VBTREE structure to be filled in with this call
 *	off_t	tNodeNumber
 *		The absolute node number within the index file (1..n)
 *	int	iPrevLvl
 *		-1	We're loading the root node
 *		Other	The node we're loading should be one less than this
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	EBADFILE
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 */
int
iVBNodeLoad (int iHandle, int iKeyNumber, struct VBTREE *psTree, off_t tNodeNumber, int iPrevLvl)
{
	char	cPrevKey [VB_MAX_KEYLEN],
		cHighKey [VB_MAX_KEYLEN],
		*pcNodePtr;
	int	iCountLC = 0,		// Leading compression
		iCountTC = 0,		// Trailing compression
		iDups = FALSE,
		iNodeLen,
		iResult;
	struct	VBKEY
		*psKey,
		*psKeyNext;
	struct	keydesc
		*psKeydesc;
#if	ISAMMODE == 1
	off_t	tTransNumber;
#endif	// ISAMMODE == 1

	psKeydesc = psVBFile [iHandle]->psKeydesc[iKeyNumber];
	vVBKeyValueSet (0, psKeydesc, cPrevKey);
	vVBKeyValueSet (1, psKeydesc, cHighKey);
	iResult = iVBBlockRead (iHandle, TRUE, tNodeNumber, cVBNode [0]);
	if (iResult)
		return (iResult);
	if (iPrevLvl != -1)
		if (*(cVBNode [0] + psVBFile [iHandle]->iNodeSize - 2) != iPrevLvl - 1)
			return (EBADFILE);
	psTree->tNodeNumber = tNodeNumber;
	psTree->sFlags.iLevel = *(cVBNode [0] + psVBFile [iHandle]->iNodeSize - 2);
	iNodeLen = ldint (cVBNode [0]);
#if	ISAMMODE == 1
	pcNodePtr = cVBNode [0] + INTSIZE + QUADSIZE;
	tTransNumber = ldquad (cVBNode [0] + INTSIZE);
	if (tTransNumber == psTree->tTransNumber)
		return (0);
#else	// ISAMMODE == 1
	pcNodePtr = cVBNode [0] + INTSIZE;
#endif	// ISAMMODE == 1
	for (psKey = psTree->psKeyFirst; psKey; psKey = psKeyNext)
	{
		if (psKey->psChild)
			vVBTreeAllFree (iHandle, iKeyNumber, psKey->psChild);
		psKey->psChild = VBTREE_NULL;
		psKeyNext = psKey->psNext;
		vVBKeyFree (iHandle, iKeyNumber, psKey);
	}
	psTree->psKeyFirst = psTree->psKeyCurr = psTree->psKeyLast = VBKEY_NULL;
	psTree->sFlags.iKeysInNode = 0;
	while (pcNodePtr - cVBNode [0] < iNodeLen)
	{
		psKey = psVBKeyAllocate (iHandle, iKeyNumber);
		if (!psKey)
			return (errno);
		if (!iDups)
		{
			if (psKeydesc->iFlags & LCOMPRESS)
			{
#if	ISAMMODE == 1
				iCountLC = ldint (pcNodePtr);
				pcNodePtr += INTSIZE;
#else	// ISAMMODE == 1
				iCountLC = *(pcNodePtr);
				pcNodePtr++;
#endif	// ISAMMODE == 1
			}
			if (psKeydesc->iFlags & TCOMPRESS)
			{
#if	ISAMMODE == 1
				iCountTC = ldint (pcNodePtr);
				pcNodePtr += INTSIZE;
#else	// ISAMMODE == 1
				iCountTC = *(pcNodePtr);
				pcNodePtr++;
#endif	// ISAMMODE == 1
			}
			memcpy (cPrevKey + iCountLC, pcNodePtr, psKeydesc->iKeyLength - (iCountLC + iCountTC));
			memset (cPrevKey + psKeydesc->iKeyLength - iCountTC, TCC, iCountTC);
			pcNodePtr += psKeydesc->iKeyLength - (iCountLC + iCountTC);
		}
		if (psKeydesc->iFlags & ISDUPS)
		{
			psKey->tDupNumber = ldquad (pcNodePtr);
			pcNodePtr += QUADSIZE;
		}
		else
			psKey->tDupNumber = 0;
		if (psKeydesc->iFlags & DCOMPRESS)
		{
			if (*pcNodePtr & 0x80)
				iDups = TRUE;
			else
				iDups = FALSE;
			*pcNodePtr &= ~0x80;
		}
		psKey->tRowNode = ldquad (pcNodePtr);
		pcNodePtr += QUADSIZE;
		psKey->psParent = psTree;
		if (psTree->psKeyFirst)
			psTree->psKeyLast->psNext = psKey;
		else
			psTree->psKeyFirst = psTree->psKeyCurr = psKey;
		psTree->psKeyList [psTree->sFlags.iKeysInNode] = psKey;
		psTree->sFlags.iKeysInNode++;
		psKey->psPrev = psTree->psKeyLast;
		psTree->psKeyLast = psKey;
		memcpy (psKey->cKey, cPrevKey, psKeydesc->iKeyLength);
	}
	if (psTree->sFlags.iLevel)
	{
		psTree->psKeyLast->sFlags.iIsHigh = 1;
		memcpy (psTree->psKeyLast->cKey, cHighKey, psKeydesc->iKeyLength);
	}
	psKey = psVBKeyAllocate (iHandle, iKeyNumber);
	if (!psKey)
		return (errno);
	psKey->psParent = psTree;
	psKey->sFlags.iIsDummy = 1;
	if (psTree->psKeyFirst)
		psTree->psKeyLast->psNext = psKey;
	else
		psTree->psKeyFirst = psTree->psKeyCurr = psKey;
	psKey->psPrev = psTree->psKeyLast;
	psTree->psKeyLast = psKey;
	psTree->psKeyList [psTree->sFlags.iKeysInNode] = psKey;
	psTree->sFlags.iKeysInNode++;
	return (0);
}

/*
 * Name:
 *	int	iVBNodeSave (int iHandle, int iKeyNumber, struct VBTREE *psTree, off_t tNodeNumber, int iMode, int iPosn);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The index number in question
 *	struct	VBTREE	*psTree
 *		The VBTREE structure containing data to write with this call
 *	off_t	tNodeNumber
 *		The absolute node number within the index file (1..n)
 *	int	iMode
 *		-1	Key deletion
 *		0	FULL node rewrite forced
 *		1	Key insertion
 *	int	iPosn
 *		n	Insertion is at the nominated position, if no form
 *			of *COMPRESS is on the key, we can optimize it
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	EBADFILE
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 */
int
iVBNodeSave (int iHandle, int iKeyNumber, struct VBTREE *psTree, off_t tNodeNumber, int iMode, int iPosn)
{
	char	*pcKeyEndPtr,
		*pcNodePtr,
		*pcNodeHalfway,
		*pcNodeEnd,
		*pcPrevKey = (char *) 0;
	int	iCountLC = 0,		// Leading compression
		iCountTC = 0,		// Trailing compression
		iLastTC = 0,
		iKeyLen,
		iMaxTC,
		iResult;
	struct	VBKEY
		*psKey,
		*psKeyHalfway = VBKEY_NULL;
	struct	keydesc
		*psKeydesc;

	psKeydesc = psVBFile [iHandle]->psKeydesc[iKeyNumber];
	/*
	 * If it's an INSERT into a node or a DELETE from a node
	 *	*AND*
	 * there's no compression
	 *	*THEN*
	 * We can *TRY* to do a quick and dirty insertion / deletion instead!
	 * However, it's still possible that we have insufficient free space
	 * so we *MAY* still need to continue.
	 */
	if (psTree->sFlags.iLevel)
		if (psTree->psKeyLast->psPrev)
			psTree->psKeyLast->psPrev->sFlags.iIsHigh = 1;
	if (iMode && !(psKeydesc->iFlags & (DCOMPRESS | TCOMPRESS | LCOMPRESS)))
		if (iQuickNodeSave (iHandle, psTree, tNodeNumber, psKeydesc, iMode, iPosn) == 0)
		{
			vFixTOFEOF (iHandle, iKeyNumber);
			return (0);
		}
	pcNodeHalfway = cVBNode [0] + (psVBFile [iHandle]->iNodeSize / 2);
	pcNodeEnd = cVBNode [0] + psVBFile [iHandle]->iNodeSize - 2;
	memset (cVBNode [0], 0, MAX_NODE_LENGTH);
#if	ISAMMODE == 1
	stquad (ldquad (psVBFile [iHandle]->sDictNode.cTransNumber) + 1, cVBNode [0] + INTSIZE);
	pcNodePtr = cVBNode [0] + INTSIZE + QUADSIZE;
#else	// ISAMMODE == 1
	pcNodePtr = cVBNode [0] + INTSIZE;
#endif	// ISAMMODE == 1
	*pcNodeEnd = psTree->sFlags.iLevel;
	psTree->sFlags.iKeysInNode = 0;
	for (psKey = psTree->psKeyFirst; psKey && !psKey->sFlags.iIsDummy; psKey = psKey->psNext)
	{
		psTree->psKeyList [psTree->sFlags.iKeysInNode] = psKey;
		psTree->sFlags.iKeysInNode++;
		if (!psKeyHalfway)
			if (pcNodePtr >= pcNodeHalfway)
				psKeyHalfway = psKey->psPrev;
		iKeyLen = psKeydesc->iKeyLength;
		if (psKeydesc->iFlags & TCOMPRESS)
		{
			iLastTC = iCountTC;
			iCountTC = 0;
			pcKeyEndPtr = psKey->cKey + iKeyLen - 1;
			while (*pcKeyEndPtr-- == TCC && pcKeyEndPtr != psKey->cKey)
				iCountTC++;
#if	ISAMMODE == 1
			iKeyLen += INTSIZE - iCountTC;
#else	// ISAMMODE == 1
			iKeyLen += 1 - iCountTC;
#endif	// ISAMMODE == 1
		}
		if (psKeydesc->iFlags & LCOMPRESS)
		{
			iCountLC = 0;
			if (psKey != psTree->psKeyFirst)
			{
				iMaxTC = psKeydesc->iKeyLength - (iCountTC > iLastTC ? iCountTC : iLastTC);
				for (; psKey->cKey [iCountLC] == pcPrevKey [iCountLC] && iCountLC < iMaxTC; iCountLC++)
						;
			}
#if	ISAMMODE == 1
			iKeyLen += INTSIZE - iCountLC;
#else	// ISAMMODE == 1
			iKeyLen += 1 - iCountLC;
#endif	// ISAMMODE == 1
			if (psKey->sFlags.iIsHigh && psKeydesc->iFlags && LCOMPRESS)
			{
				iCountLC = psKeydesc->iKeyLength;
				iCountTC = 0;
#if	ISAMMODE == 1
				if (psKeydesc->iFlags && TCOMPRESS)
					iKeyLen = INTSIZE * 2;
				else
					iKeyLen = INTSIZE;
#else	// ISAMMODE == 1
				if (psKeydesc->iFlags && TCOMPRESS)
					iKeyLen = 2;
				else
					iKeyLen = 1;
#endif	// ISAMMODE == 1
				if (psKeydesc->iFlags & DCOMPRESS)
					iKeyLen = 0;
			}
		}
		if (psKeydesc->iFlags & ISDUPS)
		{
			iKeyLen += QUADSIZE;
			// If the key is a duplicate and it's not first in node
			if ((psKey->sFlags.iIsHigh) || (psKey != psTree->psKeyFirst && !memcmp (psKey->cKey, pcPrevKey, psKeydesc->iKeyLength)))
				if (psKeydesc->iFlags & DCOMPRESS)
					iKeyLen = QUADSIZE;
		}
		iKeyLen += QUADSIZE;
		// Split?
		if (pcNodePtr + iKeyLen >= pcNodeEnd - 1)
		{
			if (psTree->psKeyLast->psPrev->sFlags.iIsNew)
				psKeyHalfway = psTree->psKeyLast->psPrev->psPrev;
			if (psTree->psKeyLast->psPrev->sFlags.iIsHigh && psTree->psKeyLast->psPrev->psPrev->sFlags.iIsNew)
				psKeyHalfway = psTree->psKeyLast->psPrev->psPrev->psPrev;
			iResult = iNodeSplit (iHandle, iKeyNumber, psTree, psKeyHalfway);
			vFixTOFEOF (iHandle, iKeyNumber);
			return (iResult);
		}
		if (((iKeyLen == (QUADSIZE * 2)) && ((psKeydesc->iFlags & (DCOMPRESS | ISDUPS)) == (DCOMPRESS | ISDUPS))) || (psKeydesc->iFlags & DCOMPRESS && !(psKeydesc->iFlags & ISDUPS) && iKeyLen == QUADSIZE))
			*(pcNodePtr - QUADSIZE) |= 0x80;
		else
		{
			if (psKeydesc->iFlags & LCOMPRESS)
			{
#if	ISAMMODE == 1
				stint (iCountLC, pcNodePtr);
				pcNodePtr += INTSIZE;
#else	// ISAMMODE == 1
				*pcNodePtr++ = iCountLC;
#endif	// ISAMMODE == 1
			}
			if (psKeydesc->iFlags & TCOMPRESS)
			{
#if	ISAMMODE == 1
				stint (iCountTC, pcNodePtr);
				pcNodePtr += INTSIZE;
#else	// ISAMMODE == 1
				*pcNodePtr++ = iCountTC;
#endif	// ISAMMODE == 1
			}
			if (iCountLC != psKeydesc->iKeyLength)
			{
				pcPrevKey = psKey->cKey + iCountLC;
				iMaxTC = psKeydesc->iKeyLength - (iCountLC + iCountTC);
				while (iMaxTC--)
					*pcNodePtr++ = *pcPrevKey++;
			}
			pcPrevKey = psKey->cKey;
		}
		if (psKeydesc->iFlags & ISDUPS)
		{
#if	ISAMMODE == 1
			*pcNodePtr++ = (psKey->tDupNumber >> 56) & 0xff;
			*pcNodePtr++ = (psKey->tDupNumber >> 48) & 0xff;
			*pcNodePtr++ = (psKey->tDupNumber >> 40) & 0xff;
			*pcNodePtr++ = (psKey->tDupNumber >> 32) & 0xff;
#endif	// ISAMMODE == 1
			*pcNodePtr++ = (psKey->tDupNumber >> 24) & 0xff;
			*pcNodePtr++ = (psKey->tDupNumber >> 16) & 0xff;
			*pcNodePtr++ = (psKey->tDupNumber >> 8) & 0xff;
			*pcNodePtr++ = psKey->tDupNumber & 0xff;
		}
#if	ISAMMODE == 1
		*pcNodePtr++ = (psKey->tRowNode >> 56) & 0xff;
		*pcNodePtr++ = (psKey->tRowNode >> 48) & 0xff;
		*pcNodePtr++ = (psKey->tRowNode >> 40) & 0xff;
		*pcNodePtr++ = (psKey->tRowNode >> 32) & 0xff;
#endif	// ISAMMODE == 1
		*pcNodePtr++ = (psKey->tRowNode >> 24) & 0xff;
		*pcNodePtr++ = (psKey->tRowNode >> 16) & 0xff;
		*pcNodePtr++ = (psKey->tRowNode >> 8) & 0xff;
		*pcNodePtr++ = psKey->tRowNode & 0xff;
	}
	if (psKey && psKey->sFlags.iIsDummy)
	{
		psTree->psKeyList [psTree->sFlags.iKeysInNode] = psKey;
		psTree->sFlags.iKeysInNode++;
	}
	stint (pcNodePtr - cVBNode [0], cVBNode [0]);
	iResult = iVBBlockWrite (iHandle, TRUE, tNodeNumber, cVBNode [0]);
	if (iResult)
		return (iResult);
	vFixTOFEOF (iHandle, iKeyNumber);
	return (0);
}

/*
 * Name:
 *	static	int	iQuickNodeSave (int iHandle, struct VBTREE *psTree, off_t tNodeNumber, struct keydesc *, int iMode, int iPosn);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	struct	VBTREE	*psTree
 *		Used as a 'sanity' check as well as to get the key value
 *	off_t	tNodeNumber
 *		The node about to be read from and written to
 *	struct	keydesc	*psKeydesc
 *		The key descriptor for the current key
 *	int	iMode
 *		-1	Key deletion
 *		1	Key insertion
 *	int	iPosn
 *		The key number being inserted / deleted (1st in node = 0)
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 */
static	int
iQuickNodeSave (int iHandle, struct VBTREE *psTree, off_t tNodeNumber, struct keydesc *psKeydesc, int iMode, int iPosn)
{
	int	iDupsLength = 0,
		iKeyLength = psKeydesc->iKeyLength,
		iLength,
		iPosition,
		iResult;

	// Sanity checks
	if (!psTree || !psTree->psKeyList [iPosn])
		return (-1);
	if (iMode == 1 && !psTree->psKeyList [iPosn]->sFlags.iIsNew)
		return (-1);
	// Read in the node (hopefully from the cache too!)
	iResult = iVBBlockRead (iHandle, TRUE, tNodeNumber, cVBNode [0]);
	if (iResult)
		return (iResult);
	if (psKeydesc->iFlags & ISDUPS)
		iDupsLength = QUADSIZE;
	iLength = ldint (cVBNode [0]);
	// Is there enough free space in the node for an insertion?
	if (iMode == 1 && iLength + 3 + iKeyLength + iDupsLength + QUADSIZE >= psVBFile [iHandle]->iNodeSize)
		return (-1);
	stint (iLength + (iMode * (iKeyLength + iDupsLength + QUADSIZE)), cVBNode [0]);
	// Calculate position for insertion / deletion of key
#if	ISAMMODE == 1
	iPosition = INTSIZE + QUADSIZE + (iPosn * (iKeyLength + iDupsLength + QUADSIZE));
#else	// ISAMMODE == 1
	iPosition = INTSIZE + (iPosn * (iKeyLength + iDupsLength + QUADSIZE));
#endif	// ISAMMODE == 1
	if (iMode == 1)
	{
		memmove (cVBNode [0] + iPosition + iKeyLength + iDupsLength + QUADSIZE, cVBNode [0] + iPosition, iLength - iPosition);
		memcpy (cVBNode [0] + iPosition, psTree->psKeyList [iPosn]->cKey, iKeyLength);
		if (psKeydesc->iFlags & ISDUPS)
			stquad (psTree->psKeyList [iPosn]->tDupNumber, cVBNode [0] + iPosition + iKeyLength);
		stquad (psTree->psKeyList [iPosn]->tRowNode, cVBNode [0] + iPosition + iKeyLength + iDupsLength);
	}
	else
	{
		if (iLength - (iPosition + iKeyLength + iDupsLength + QUADSIZE) > 0)
			memmove (cVBNode [0] + iPosition, cVBNode [0] + iPosition + iKeyLength + iDupsLength + QUADSIZE, iLength - (iPosition + iKeyLength + iDupsLength + QUADSIZE));
		memset (cVBNode [0] + iLength - (iKeyLength + QUADSIZE), 0, iKeyLength + iDupsLength + QUADSIZE);
	}
	iResult = iVBBlockWrite (iHandle, TRUE, tNodeNumber, cVBNode [0]);
	if (iResult)
		return (iResult);
	return (0);
}

/*
 * Name:
 *	static	void	vFixTOFEOF (int iHandle, int iKeyNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		GUESS!
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
static	void
vFixTOFEOF (int iHandle, int iKeyNumber)
{
	struct	VBTREE
		*psTree;

	psTree = psVBFile [iHandle]->psTree [iKeyNumber];
	while (psTree)
	{
		psTree->sFlags.iIsTOF = TRUE;
		if (psTree->psKeyFirst->sFlags.iIsDummy)
			break;
		psTree = psTree->psKeyFirst->psChild;
	}
	psTree = psVBFile [iHandle]->psTree [iKeyNumber];
	while (psTree)
	{
		psTree->sFlags.iIsEOF = TRUE;
		if (!psTree->sFlags.iLevel)
			break;		// Empty file huh?
		if (psTree->psKeyLast->psPrev)
			psTree = psTree->psKeyLast->psPrev->psChild;
		else
			break;
	}
}

/*
 * Name:
 *	static	int	iNodeSplit (int iHandle, int iKeyNumber, struct VBTREE *psTree, struct VBKEY *psKeyHalfway);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The index number in question
 *	struct	VBTREE	*psTree
 *		The VBTREE structure containing data to split with this call
 *	struct	VBKEY	*psKeyHalfway
 *		A pointer within the psTree defining WHERE to split
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 */
static	int
iNodeSplit (int iHandle, int iKeyNumber, struct VBTREE *psTree, struct VBKEY *psKeyHalfway)
{
	int	iResult;
	struct	VBKEY
		*psKey,
		*psKeyTemp,
		*psHoldKeyCurr,
		*psNewKey,
		*psRootKey [3];
	struct	VBTREE
		*psNewTree,
		*psRootTree = VBTREE_NULL;
	off_t	tNewNode1,
		tNewNode2 = 0;

	psNewTree = psVBTreeAllocate (iHandle);
	if (!psNewTree)
		return (errno);
	psNewTree->psParent = psTree;
	psNewKey = psVBKeyAllocate (iHandle, iKeyNumber);
	if (!psNewKey)
		return (errno);
	if (psTree->sFlags.iIsRoot)
	{
		psRootTree = psVBTreeAllocate (iHandle);
		if (!psRootTree)
			return (errno);
		psRootKey [0] = psVBKeyAllocate (iHandle, iKeyNumber);
		if (!psRootKey [0])
			return (errno);
		psRootKey [1] = psVBKeyAllocate (iHandle, iKeyNumber);
		if (!psRootKey [1])
			return (errno);
		psRootKey [2] = psVBKeyAllocate (iHandle, iKeyNumber);
		if (!psRootKey [2])
			return (errno);
		tNewNode2 = tVBNodeAllocate (iHandle);
		if (tNewNode2 == -1)
			return (iserrno);
	}
	tNewNode1 = tVBNodeAllocate (iHandle);
	if (tNewNode1 == -1)
		return (iserrno);

	if (psTree->sFlags.iIsRoot)
	{
		psNewTree->psKeyLast = psTree->psKeyLast;
		psKey = psKeyHalfway->psNext;
		psNewKey->psPrev = psKey->psPrev;
		psNewKey->psParent = psKey->psParent;
		psNewKey->sFlags.iIsDummy = 1;
		psKey->psPrev->psNext = psNewKey;
		psKey->psPrev = VBKEY_NULL;
		psTree->psKeyLast = psNewKey;
		psTree->psKeyCurr = psTree->psKeyFirst;
		psNewTree->psKeyFirst = psNewTree->psKeyCurr = psKey;
		psNewTree->sFlags.iLevel = psTree->sFlags.iLevel;
		psNewTree->psParent = psTree->psParent;
		psNewTree->sFlags.iIsEOF = psTree->sFlags.iIsEOF;
		psTree->sFlags.iIsEOF = 0;
		for (psKeyTemp = psKey; psKeyTemp; psKeyTemp = psKeyTemp->psNext)
			psKeyTemp->psParent = psNewTree;
		return (iNewRoot (iHandle, iKeyNumber, psTree, psNewTree, psRootTree, psRootKey, tNewNode1, tNewNode2));
	}
	else
	{
		psNewTree->psKeyFirst = psNewTree->psKeyCurr = psTree->psKeyFirst;
		psNewTree->psKeyLast = psNewKey;
		psTree->psKeyFirst = psTree->psKeyCurr = psKeyHalfway->psNext;
		psKeyHalfway->psNext->psPrev = VBKEY_NULL;
		psKeyHalfway->psNext = psNewKey;
		psNewKey->psPrev = psKeyHalfway;
		psNewKey->psNext = VBKEY_NULL;
psNewKey->psParent = psNewTree;	// Doubtful
		psNewKey->sFlags.iIsDummy = 1;
		for (psKey = psNewTree->psKeyFirst; psKey; psKey = psKey->psNext)
		{
			psKey->psParent = psNewTree;
			if (psKey->psChild)
				psKey->psChild->psParent = psNewTree;
		}
		psNewTree->sFlags.iLevel = psTree->sFlags.iLevel;
		psNewTree->psParent = psTree->psParent;
		psNewTree->sFlags.iIsTOF = psTree->sFlags.iIsTOF;
		/*
		 * psNewTree is the new LEAF node but is stored in the OLD node
		 * psTree is the original node and contains the HIGH half
		 */
		psNewTree->tNodeNumber = tNewNode1;
		iResult = iVBNodeSave (iHandle, iKeyNumber, psNewTree, psNewTree->tNodeNumber, 0, 0);
		if (iResult)
			return (iResult);
		iResult = iVBNodeSave (iHandle, iKeyNumber, psTree, psTree->tNodeNumber, 0, 0);
		if (iResult)
			return (iResult);
		psTree->sFlags.iIsTOF = 0;
		psHoldKeyCurr = psVBFile [iHandle]->psKeyCurr [iKeyNumber];
		psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psNewKey->psParent->psParent->psKeyCurr;
		iResult = iVBKeyInsert (iHandle, psKeyHalfway->psParent->psParent, iKeyNumber, psKeyHalfway->cKey, tNewNode1, psKeyHalfway->tDupNumber, psNewTree);
		psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psHoldKeyCurr;
		if (iResult)
			return (iResult);
		psHoldKeyCurr->psParent->psKeyCurr = psHoldKeyCurr;
	}
	return (0);
}

/*
 * Name:
 *	static	int	iNewRoot (int iHandle, int iKeyNumber, struct VBTREE *psTree, struct VBTREE *psNewTree, struct VBTREE *psRootTree, struct VBKEY *psRootKey[], off_t tNewNode1, off_t tNewNode2)
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The index number in question
 *	struct	VBTREE	*psTree
 *		The VBTREE structure containing data to be used with this call
 *	struct	VBTREE	*psNewTree
 *		The VBTREE structure that will contain some of the data
 *	struct	VBKEY	*psRootKey []
 *		An array of three (3) keys to be filled in for the new root
 *	off_t	tNewNode1
 *		The receiving node number for some of the old root data
 *	off_t	tNewNode2
 *		The receiving node number for the rest of the old root data
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 */
static	int
iNewRoot (int iHandle, int iKeyNumber, struct VBTREE *psTree, struct VBTREE *psNewTree, struct VBTREE *psRootTree, struct VBKEY *psRootKey[], off_t tNewNode1, off_t tNewNode2)
{
	int	iResult;
	struct	VBKEY
		*psKey;

	// Fill in the content for the new root node
	psRootKey [0]->psNext = psRootKey [1];
	psRootKey [1]->psNext = psRootKey [2];
	psRootKey [2]->psPrev = psRootKey [1];
	psRootKey [1]->psPrev = psRootKey [0];
	psRootKey [0]->psParent = psRootKey [1]->psParent = psRootKey [2]->psParent = psRootTree;
	psRootKey [0]->psChild = psTree;
	psRootKey [1]->psChild = psNewTree;
	psRootKey [0]->tRowNode = tNewNode2;
	psRootKey [1]->tRowNode = tNewNode1;
	psRootKey [1]->sFlags.iIsHigh = 1;
	psRootKey [2]->sFlags.iIsDummy = 1;
	memcpy (psRootKey [0]->cKey, psTree->psKeyLast->psPrev->cKey, psVBFile [iHandle]->psKeydesc [iKeyNumber]->iKeyLength);
	psRootKey [0]->tDupNumber = psTree->psKeyLast->psPrev->tDupNumber;
	vVBKeyValueSet (1, psVBFile [iHandle]->psKeydesc [iKeyNumber], psRootKey [1]->cKey);
	/*
	 * psRootTree is the new ROOT node
	 * psNewTree is the new LEAF node
	 * psTree is the original node (saved in a new place)
	 */
	psRootTree->psKeyFirst = psRootKey [0];
	psRootTree->psKeyCurr = psRootKey [0];
	psRootTree->psKeyLast = psRootKey [2];
	psRootTree->tNodeNumber = psTree->tNodeNumber;
	psRootTree->sFlags.iLevel = psTree->sFlags.iLevel + 1;
	psRootTree->sFlags.iIsRoot = 1;
	psRootTree->sFlags.iIsTOF = 1;
	psRootTree->sFlags.iIsEOF = 1;
	psTree->psParent = psRootTree;
	psTree->tNodeNumber = tNewNode2;
	psTree->sFlags.iIsRoot = 0;
	psTree->sFlags.iIsEOF = 0;
	psNewTree->psParent = psRootTree;
	psNewTree->tNodeNumber = tNewNode1;
	psNewTree->sFlags.iLevel = psTree->sFlags.iLevel;
	psNewTree->sFlags.iIsEOF = 1;
	psNewTree->psKeyCurr = psNewTree->psKeyFirst;
	psVBFile [iHandle]-> psTree [iKeyNumber] = psRootTree;
	for (psKey = psTree->psKeyFirst; psKey; psKey = psKey->psNext)
		if (psKey->psChild)
			psKey->psChild->psParent = psTree;
	for (psKey = psNewTree->psKeyFirst; psKey; psKey = psKey->psNext)
		if (psKey->psChild)
			psKey->psChild->psParent = psNewTree;
	iResult = iVBNodeSave (iHandle, iKeyNumber, psNewTree, psNewTree->tNodeNumber, 0, 0);
	if (iResult)
		return (iResult);
	iResult = iVBNodeSave (iHandle, iKeyNumber, psTree, psTree->tNodeNumber, 0, 0);
	if (iResult)
		return (iResult);
	return (iVBNodeSave (iHandle, iKeyNumber, psRootTree, psRootTree->tNodeNumber, 0, 0));
}
