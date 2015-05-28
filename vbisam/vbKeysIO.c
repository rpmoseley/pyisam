/*
 * Title:	vbKeysIO.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	25Nov2003
 * Description:
 *	This module handles ALL the key manipulation for the VBISAM library.
 * Version:
 *	$Id: vbKeysIO.c,v 1.12 2004/06/16 10:53:56 trev_vb Exp $
 * Modification History:
 *	$Log: vbKeysIO.c,v $
 *	Revision 1.12  2004/06/16 10:53:56  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
 *	
 *	Revision 1.11  2004/06/13 06:32:33  trev_vb
 *	TvB 12June2004 See CHANGELOG 1.03 (Too lazy to enumerate)
 *	
 *	Revision 1.10  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.9  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.8  2004/03/23 15:13:19  trev_vb
 *	TvB 23Mar2004 Changes made to fix bugs highlighted by JvN's test suite.  Many thanks go out to JvN for highlighting my obvious mistakes.
 *	
 *	Revision 1.7  2004/01/06 14:31:59  trev_vb
 *	TvB 06Jan2004 Added in VARLEN processing (In a fairly unstable sorta way)
 *	
 *	Revision 1.6  2004/01/05 07:36:17  trev_vb
 *	TvB 05Feb2002 Added licensing et al as Johann v. N. noted I'd overlooked it
 *	
 *	Revision 1.5  2004/01/03 07:14:43  trev_vb
 *	TvB 02Jan2004 Ooops, I should ALWAYS try to remember to be in the RIGHT
 *	TvB 02Jan2003 directory when I check code back into CVS!!!
 *	
 *	Revision 1.4  2004/01/03 02:28:48  trev_vb
 *	TvB 02Jan2004 WAY too many changes to enumerate!
 *	TvB 02Jan2003 Transaction processing done (excluding iscluster)
 *	
 *	Revision 1.3  2003/12/23 03:08:56  trev_vb
 *	TvB 22Dec2003 Minor compilation glitch 'fixes'
 *	
 *	Revision 1.2  2003/12/22 04:48:44  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:24  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"

/*
 * Prototypes
 */
void	vVBMakeKey (int, int, char *, char *);
int	iVBKeySearch (int, int, int, int, char *, off_t);
int	iVBKeyLocateRow (int, int, off_t);
int	iVBKeyLoad (int, int, int, int, struct VBKEY **);
void	vVBKeyValueSet (int, struct keydesc *, char *);
int	iVBKeyInsert (int, struct VBTREE *, int, char *, off_t, off_t, struct VBTREE *);
int	iVBKeyDelete (int, int);
int	iVBKeyCompare (int, int, int, unsigned char *, unsigned char *);
static	int	iTreeLoad (int, int, int, char *, off_t);
#ifdef	DEBUG
static	void	vDumpKey (struct VBKEY *, struct VBTREE *, int);
static	void	vDumpTree (struct VBTREE *, int);
int	iDumpTree (int, int);
static	int	iChkTree2 (struct VBTREE *, int, struct VBKEY *, int *);
int	iChkTree (int, int);
#endif	// DEBUG

/*
 * Name:
 *	void	vVBMakeKey (int iHandle, int iKeyNumber, char *pcRowBuffer, char *pcKeyValue);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The absolute key number within the index file (0-n)
 *	char	*pcRowBuffer
 *		A pointer to the raw row data containing the key parts
 *	char	*pcKeyValue
 *		A pointer to the receiving area for the contiguous key
 * Prerequisites:
 *	Assumes pcKeyValue is *LARGE* enough to store the result
 * Returns:
 *	NOTHING
 * Problems:
 *	NONE known
 * Comments:
 *	Extracts the various parts from a pcRowBuffer to create a contiguous key
 *	in pcKeyValue
 */
void
vVBMakeKey (int iHandle, int iKeyNumber, char *pcRowBuffer, char *pcKeyValue)
{
	char	*pcSource;
	int	iPart;
	struct	keydesc
		*psKeydesc;

	// Wierdly enough, a single keypart *CAN* span multiple instances
	// EG: Part number 1 might contain 4 long values
	psKeydesc = psVBFile [iHandle]->psKeydesc [iKeyNumber];
	for (iPart = 0; iPart < psKeydesc->iNParts; iPart++)
	{
		pcSource = pcRowBuffer + psKeydesc->sPart [iPart].iStart;
		memcpy (pcKeyValue, pcSource, psKeydesc->sPart [iPart].iLength);
		pcKeyValue += psKeydesc->sPart [iPart].iLength;
	}
}

/*
 * Name:
 *	int	iVBKeySearch (int iHandle, int iMode, int iKeyNumber, int iLength, char *pcKeyValue, off_t tDupNumber)
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	struct	keydesc	*psKey
 *		The key description to be tested
 *	int	iMode
 *		ISFIRST
 *		ISLAST
 *		ISNEXT
 *		ISPREV
 *		ISCURR
 *		ISEQUAL
 *		ISGREAT
 *		ISGTEQ
 *	int	iKeyNumber
 *		The absolute key number within the index file (0-n)
 *	int	iLength
 *		The length of the key to compare (0 = FULL KEY!)
 *	char	*pcKeyValue
 *		The key value being searched for
 *	off_t	tDupNumber
 *		The duplicate number being searched for
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success, pcKeyValue < psVBFile[iHandle]->psKeyCurr[iKeyNumber]
 *	1	Success, pcKeyValue = psVBFile[iHandle]->psKeyCurr[iKeyNumber]
 *	2	Index file is EMPTY
 * Problems:
 *	NONE known
 */
int
iVBKeySearch (int iHandle, int iMode, int iKeyNumber, int iLength, char *pcKeyValue, off_t tDupNumber)
{
	char	cBuffer [QUADSIZE],
		cKeyValue [VB_MAX_KEYLEN];
	int	iResult;
	struct	VBKEY
		*psKey;
	struct	keydesc
		*psKeydesc;

	psKeydesc = psVBFile [iHandle]->psKeydesc [iKeyNumber];
	if (iLength == 0)
		iLength = psKeydesc->iKeyLength;
	switch (iMode)
	{
	case	ISFIRST:
		vVBKeyValueSet (0, psKeydesc, cKeyValue);
		tDupNumber = -1;
		return (iTreeLoad (iHandle, iKeyNumber, iLength, cKeyValue, tDupNumber));

	case	ISLAST:
		vVBKeyValueSet (1, psKeydesc, cKeyValue);
		memset (cBuffer, 0xff, QUADSIZE);
		cBuffer [0] = 0x7f;
		tDupNumber = ldquad (cBuffer);
		return (iTreeLoad (iHandle, iKeyNumber, iLength, cKeyValue, tDupNumber));

	case	ISNEXT:
		iResult = iVBKeyLoad (iHandle, iKeyNumber, ISNEXT, TRUE, &psKey);
		iserrno = iResult;
		if (iResult == EENDFILE)
			iResult = 0;
		if (iResult)
			return (-1);
		return (1);		// "NEXT" can NEVER be an exact match!

	case	ISPREV:
		iResult = iVBKeyLoad (iHandle, iKeyNumber, ISPREV, TRUE, &psKey);
		iserrno = iResult;
		if (iResult == EENDFILE)
			iResult = 0;
		if (iResult)
			return (-1);
		return (1);		// "PREV" can NEVER be an exact match

	case	ISCURR:
		return (iTreeLoad (iHandle, iKeyNumber, iLength, pcKeyValue, tDupNumber));

	case	ISEQUAL:	// Falls thru to ISGTEQ
		tDupNumber = 0;
	case	ISGTEQ:
		return (iTreeLoad (iHandle, iKeyNumber, iLength, pcKeyValue, tDupNumber));

	case	ISGREAT:
		memset (cBuffer, 0xff, QUADSIZE);
		cBuffer [0] = 0x7f;
		tDupNumber = ldquad (cBuffer);
		return (iTreeLoad (iHandle, iKeyNumber, iLength, pcKeyValue, tDupNumber));

	default:
		fprintf (stderr, "HUGE ERROR! File %s, Line %d iMode %d\n", __FILE__, __LINE__, iMode);
		exit (1);
	}
}

/*
 * Name:
 *	static	int	iVBKeyLocateRow (int iHandle, int iKeyNumber, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The absolute key number within the index file (0-n)
 *	off_t	tRowNumber
 *		The row number to locate within the key
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success (Fills the psVBFile [iHandle]->psTree linked list)
 * Problems:
 *	NONE known
 * Comments:
 *	The purpose of this function is to populate the VBTREE linked list
 *	associated with iKeyNumber such that the index entry being pointed to
 *	matches the entry for the row number passed into this function.
 */
int
iVBKeyLocateRow (int iHandle, int iKeyNumber, off_t tRowNumber)
{
	char	cKeyValue [VB_MAX_KEYLEN];
	int	iResult;
	struct	VBKEY
		*psKey;
	struct	VBTREE
		*psTree;

	/*
	 * Step 1:
	 *	The easy way out...
	 *	If it is already the current index pointer *AND*
	 *	the index file has remained unchanged since then,
	 *	we don't need to do anything
	 */
	iResult = TRUE;
	psKey = psVBFile [iHandle]->psKeyCurr [iKeyNumber];
	if (psKey && psKey->tRowNode == tRowNumber)
	{
		psKey->psParent->psKeyCurr = psKey;
		// Position psKeyCurr all the way up to the root to point at us
		psTree = psKey->psParent;
		while (psTree->psParent)
		{
			for (psTree->psParent->psKeyCurr = psTree->psParent->psKeyFirst; psTree->psParent->psKeyCurr && psTree->psParent->psKeyCurr->psChild != psTree; psTree->psParent->psKeyCurr = psTree->psParent->psKeyCurr->psNext)
				;
			if (!psTree->psParent->psKeyCurr)
				iResult = FALSE;
			psTree = psTree->psParent;
		}
		if (iResult)
			return (0);
	}

	/*
	 * Step 2:
	 *	It's a valid and non-deleted row.  Therefore, let's make a
	 *	contiguous key from it to search by.
	 *	Find the damn key!
	 */
	vVBMakeKey (iHandle, iKeyNumber, *(psVBFile [iHandle]->ppcRowBuffer), cKeyValue);
	iResult = iVBKeySearch (iHandle, ISGTEQ, iKeyNumber, 0, cKeyValue, 0);
	if (iResult < 0 || iResult > 1)
	{
		iserrno = ENOREC;
		return (-1);
	}
	while (psVBFile [iHandle]->psKeyCurr [iKeyNumber]->tRowNode != tRowNumber)
	{
		iserrno = iVBKeyLoad (iHandle, iKeyNumber, ISNEXT, TRUE, &psKey);
		if (iserrno == EENDFILE)
		{
			iserrno = ENOREC;
			return (-1);
		}
		if (iserrno)
			return (-1);
		if (iVBKeyCompare (iHandle, iKeyNumber, 0, cKeyValue, psVBFile [iHandle]->psKeyCurr [iKeyNumber]->cKey))
		{
			iserrno = ENOREC;
			return (-1);
		}
	}
	return (0);
}

/*
 * Name:
 *	int	iVBKeyLoad (int iHandle, int iKeyNumber, int iMode, int iSetCurr, struct VBKEY **ppsKey)
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The index number in question
 *	int	iMode
 *		ISPREV - Search previous
 *		ISNEXT - Search next
 *	int	iSetCurr
 *		0 - Leave psVBFile [iHandle]->psTree [iKeyNumber]->psKeyCurr
 *		Other - SET it!
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 */
int
iVBKeyLoad (int iHandle, int iKeyNumber, int iMode, int iSetCurr, struct VBKEY **ppsKey)
{
	int	iResult;
	struct	VBKEY
		*psKey,
		*psKeyHold;
	struct	VBTREE
		*psTree;

 	psKey = psVBFile [iHandle]->psKeyCurr [iKeyNumber];
	if (psKey->psParent->sFlags.iLevel)
		return (EBADFILE);
	switch (iMode)
	{
	case	ISPREV:
		if (psKey->psPrev)
		{
			*ppsKey = psKey->psPrev;
			if (iSetCurr)
			{
 				psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psKey->psPrev;
				psKey->psParent->psKeyCurr = psKey->psPrev;
			}
			return (0);
		}
		psTree = psKey->psParent;
		if (psTree->sFlags.iIsTOF)
			return (EENDFILE);
		// Back up the tree until we find a node where there is a <
		while (psTree->psKeyCurr == psTree->psKeyFirst)
		{
			if (psTree->psParent)
				psTree = psTree->psParent;
			else
				break;
		}
		// Back down the tree selecting the LAST valid key of each
		psKey = psTree->psKeyCurr->psPrev;
		if (iSetCurr)
			psTree->psKeyCurr = psTree->psKeyCurr->psPrev;
		while (psTree->sFlags.iLevel && psKey)
		{
			if (iSetCurr)
				psTree->psKeyCurr = psKey;
			if (!psKey->psChild || psVBFile [iHandle]->sFlags.iIndexChanged)
			{
				if (!psKey->psChild)
				{
					psKey->psChild = psVBTreeAllocate (iHandle);
					if (!psKey->psChild)
						return (errno);
					psKey->psChild->psParent = psKey->psParent;
					if (psKey->psParent->sFlags.iIsTOF && psKey == psKey->psParent->psKeyFirst)
						psKey->psChild->sFlags.iIsTOF = 1;
					if (psKey->psParent->sFlags.iIsEOF && psKey == psKey->psParent->psKeyLast->psPrev)
						psKey->psChild->sFlags.iIsEOF = 1;
				}
				psKeyHold = psKey;
				iResult = iVBNodeLoad (iHandle, iKeyNumber, psKey->psChild, psKey->tRowNode, psTree->sFlags.iLevel);
				if (iResult)
				{
				// Ooops, make sure the tree is not corrupt
					vVBTreeAllFree (iHandle, iKeyNumber, psKeyHold->psChild);
					psKeyHold->psChild = VBTREE_NULL;
					return (iResult);
				}
			}
			psTree = psKey->psChild;
			// Last key is always the dummy, so backup by one
			psKey = psTree->psKeyLast->psPrev;
		}
		if (iSetCurr)
		{
			psTree->psKeyCurr = psKey;
			psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psKey;
		}
		*ppsKey = psKey;
		break;

	case	ISNEXT:
		if (psKey->psNext && !psKey->psNext->sFlags.iIsDummy)
		{
			*ppsKey = psKey->psNext;
			if (iSetCurr)
			{
 				psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psKey->psNext;
				psKey->psParent->psKeyCurr = psKey->psNext;
			}
			return (0);
		}
		psTree = psKey->psParent;
		if (psTree->sFlags.iIsEOF)
			return (EENDFILE);
		psTree = psTree->psParent;
		// Back up the tree until we find a node where there is a >
		while (1)
		{
			if (psTree->psKeyLast->psPrev != psTree->psKeyCurr)
				break;
			psTree = psTree->psParent;
		}
		psKey = psTree->psKeyCurr->psNext;
		if (iSetCurr)
			psTree->psKeyCurr = psTree->psKeyCurr->psNext;
		// Back down the tree selecting the FIRST valid key of each
		while (psTree->sFlags.iLevel)
		{
			if (iSetCurr)
				psTree->psKeyCurr = psKey;
			if (!psKey->psChild || psVBFile [iHandle]->sFlags.iIndexChanged)
			{
				if (!psKey->psChild)
				{
					psKey->psChild = psVBTreeAllocate (iHandle);
					if (!psKey->psChild)
						return (errno);
					psKey->psChild->psParent = psKey->psParent;
					if (psKey->psParent->sFlags.iIsTOF && psKey == psKey->psParent->psKeyFirst)
						psKey->psChild->sFlags.iIsTOF = 1;
					if (psKey->psParent->sFlags.iIsEOF && psKey == psKey->psParent->psKeyLast->psPrev)
						psKey->psChild->sFlags.iIsEOF = 1;
				}
				psKeyHold = psKey;
				iResult = iVBNodeLoad (iHandle, iKeyNumber, psKey->psChild, psKey->tRowNode, psTree->sFlags.iLevel);
				if (iResult)
				{
				// Ooops, make sure the tree is not corrupt
					vVBTreeAllFree (iHandle, iKeyNumber, psKeyHold->psChild);
					psKeyHold->psChild = VBTREE_NULL;
					return (iResult);
				}
			}
			psTree = psKey->psChild;
			psKey = psTree->psKeyFirst;
		}
		if (iSetCurr)
		{
			psTree->psKeyCurr = psKey;
			psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psKey;
		}
		*ppsKey = psKey;
		break;

	default:
		fprintf (stderr, "HUGE ERROR! File %s, Line %d Mode %d\n", __FILE__, __LINE__, iMode);
		exit (1);
	}
	return (0);
}

/*
 * Name:
 *	void	vVBKeyValueSet (int iHigh, struct keydesc *psKeydesc, char *pcKeyValue)
 * Arguments:
 *	int	iHigh
 *		0	Set it low
 *		Other	Set it high
 *	struct	keydesc	*psKeydesc
 *		The keydesc structure to use
 *	char	*pcKeyValue
 *		The receiving uncompressed key
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 */
void
vVBKeyValueSet (int iHigh, struct keydesc *psKeydesc, char *pcKeyValue)
{
	char	cBuffer [QUADSIZE];
	int	iPart,
		iRemainder;

	for (iPart = 0; iPart < psKeydesc->iNParts; iPart++)
	{
		switch (psKeydesc->sPart [iPart].iType & ~ISDESC)
		{
		case	CHARTYPE:
			memset (pcKeyValue, iHigh ? 0xff : 0, psKeydesc->sPart [iPart].iLength);
			pcKeyValue += psKeydesc->sPart [iPart].iLength;
			break;

		case	INTTYPE:
			iRemainder = psKeydesc->sPart [iPart].iLength;
			while (iRemainder > 0)
			{
				stint (iHigh ? SHRT_MAX : SHRT_MIN, pcKeyValue);
				pcKeyValue += INTSIZE;
				iRemainder -= INTSIZE;
			}
			break;

		case	LONGTYPE:
			iRemainder = psKeydesc->sPart [iPart].iLength;
			while (iRemainder > 0)
			{
				stlong (iHigh ? LONG_MAX : LONG_MIN, pcKeyValue);
				pcKeyValue += LONGSIZE;
				iRemainder -= LONGSIZE;
			}
			break;

		case	QUADTYPE:
			memset (cBuffer, iHigh ? 0xff : 0, QUADSIZE);
			cBuffer [0] = iHigh ? 0x7f : 0x80;
			iRemainder = psKeydesc->sPart [iPart].iLength;
			while (iRemainder > 0)
			{
				memcpy (pcKeyValue, cBuffer, QUADSIZE);
				pcKeyValue += QUADSIZE;
				iRemainder -= QUADSIZE;
			}
			break;

		case	FLOATTYPE:
			iRemainder = psKeydesc->sPart [iPart].iLength;
			while (iRemainder > 0)
			{
				stfloat (iHigh ? FLT_MAX : FLT_MIN, pcKeyValue);
				pcKeyValue += FLOATSIZE;
				iRemainder -= FLOATSIZE;
			}
			break;

		case	DOUBLETYPE:
			iRemainder = psKeydesc->sPart [iPart].iLength;
			while (iRemainder > 0)
			{
				stdbl (iHigh ? DBL_MAX : DBL_MIN, pcKeyValue);
				pcKeyValue += DOUBLESIZE;
				iRemainder -= DOUBLESIZE;
			}
			break;

		default:	// Please pass me the samurai sword, I goofed
			fprintf (stderr, "HUGE ERROR! File %s, Line %d Type %d\n", __FILE__, __LINE__, psKeydesc->sPart [iPart].iType);
			exit (1);
		}
	}
}

/*
 * Name:
 *	int	iVBKeyInsert (int iHandle, struct VBTREE *psTree, int iKeyNumber, char *pcKeyValue, off_t tRowNode, off_t tDupNumber, struct VBTREE *psChild);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	struct	VBTREE	*psTree
 *		The VBTREE structure containing data to be used with this call
 *	int	iKeyNumber
 *		The index number in question
 *	char	*pcKeyValue
 *		The actual uncompressed key to insert
 *	off_t	tRowNode
 *		The row number (LEAF NODE) or node number to be inserted
 *	off_t	tDupNumber
 *		The duplicate number (0 = first)
 *	struct	VBTREE	*psChild
 *		The VBTREE structure that this new entry will point to
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 * Comments:
 *	Insert the key pcKeyValue into the index.
 *	Assumes that psVBFile [iHandle]->psTree [iKeyNumber] is setup
 *	Always inserts the key BEFORE psTree->psKeyCurr
 *	Sets psVBFile [iHandle]->psKeyCurr [iKeyNumber] to the newly added key
 */
int
iVBKeyInsert (int iHandle, struct VBTREE *psTree, int iKeyNumber, char *pcKeyValue, off_t tRowNode, off_t tDupNumber, struct VBTREE *psChild)
{
	int	iPosn = 0,
		iResult;
	struct	VBKEY
		*psKey,
		*psTempKey;

	psKey = psVBKeyAllocate (iHandle, iKeyNumber);
	if (!psKey)
		return (errno);
	if (!psVBFile [iHandle]->psKeyCurr [iKeyNumber])
		return (EBADFILE);
	if (!psTree)
		psTree = psVBFile [iHandle]->psKeyCurr [iKeyNumber]->psParent; 
	psKey->psParent = psTree;
	psKey->psChild = psChild;
	psKey->tRowNode = tRowNode;
	psKey->tDupNumber = tDupNumber;
	psKey->sFlags.iIsNew = 1;
	memcpy (psKey->cKey, pcKeyValue, psVBFile [iHandle]->psKeydesc [iKeyNumber]->iKeyLength);
	psKey->psNext = psTree->psKeyCurr;
	psKey->psPrev = psTree->psKeyCurr->psPrev;
	if (psTree->psKeyCurr->psPrev)
		psTree->psKeyCurr->psPrev->psNext = psKey;
	else
		psTree->psKeyFirst = psKey;
	psTree->psKeyCurr->psPrev = psKey;
	psTree->psKeyCurr = psKey;
	psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psKey;
	psTree->sFlags.iKeysInNode = 0;
	for (psTempKey = psTree->psKeyFirst; psTempKey; psTempKey = psTempKey->psNext)
	{
		if (psTempKey == psKey)
			iPosn = psTree->sFlags.iKeysInNode;
		psTree->psKeyList [psTree->sFlags.iKeysInNode] = psTempKey;
		psTree->sFlags.iKeysInNode++;
	}
	iResult = iVBNodeSave (iHandle, iKeyNumber, psKey->psParent, psKey->psParent->tNodeNumber, 1, iPosn);
	psKey->sFlags.iIsNew = 0;
	return (iResult);
}

/*
 * Name:
 *	int	iVBKeyDelete (int iHandle, int iKeyNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The index number in question
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	As per sub function calls
 * Problems:
 *	NONE known
 */
int
iVBKeyDelete (int iHandle, int iKeyNumber)
{
	int	iForceRewrite = FALSE,
		iPosn,
		iResult;
	struct	VBKEY
		*psKey,
		*psKeyTemp;
	struct	VBTREE
		*psTree,
		*psTreeRoot;

	psKey = psVBFile [iHandle]->psKeyCurr [iKeyNumber];
	/*
	 * We're going to *TRY* to keep the index buffer populated!
	 * However, since it's technically feasible for the current node to be
	 * removed in it's entirety, we can only do this if there is at least 1
	 * other key in the node that's not the dummy entry.
	 * Since the current key is guaranteed to be at the LEAF node level (0),
	 * it's impossible to ever have an iIsHigh entry in the node.
	 */
	if (psKey->psNext && psKey->psNext->sFlags.iIsDummy == 0)
	{
		psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psKey->psNext;
		psKey->psParent->psKeyCurr = psKey->psNext;
	}
	else
	{
		if (psKey->psPrev)
		{
			psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psKey->psPrev;
			psKey->psParent->psKeyCurr = psKey->psPrev;
		}
		else
		{
			psVBFile [iHandle]->psKeyCurr [iKeyNumber] = VBKEY_NULL;
			psKey->psParent->psKeyCurr = VBKEY_NULL;
		}
	}
	while (1)
	{
		psTree = psKey->psParent;
		if (psKey->sFlags.iIsHigh)
		{
			/*
			 * Handle removal of the high key in a node.
			 * Since we're modifying a key OTHER than the one we're
			 * deleting, we need a FULL node rewrite!
			 */
			if (psKey->psPrev)
			{
				psKey->psChild = psKey->psPrev->psChild;
				psKey->tRowNode = psKey->psPrev->tRowNode;
				psKey->tDupNumber = psKey->psPrev->tDupNumber;
				psKey = psKey->psPrev;
				iForceRewrite = TRUE;
			}
			else
			{
				iResult = iVBNodeFree (iHandle, psTree->tNodeNumber);	// BUG - didn't check iResult
				psTree = psTree->psParent;
				vVBTreeAllFree (iHandle, iKeyNumber, psTree->psKeyCurr->psChild);
				psKey = psTree->psKeyCurr;
				psKey->psChild = VBTREE_NULL;
				continue;
			}
		}
		iPosn = -1;
		psKey->psParent->sFlags.iKeysInNode = 0;
		for (psKeyTemp = psKey->psParent->psKeyFirst; psKeyTemp; psKeyTemp = psKeyTemp->psNext)
		{
			if (psKey == psKeyTemp)
				iPosn = psKey->psParent->sFlags.iKeysInNode;
			else
			{
				psKey->psParent->psKeyList [psKey->psParent->sFlags.iKeysInNode] = psKeyTemp;
				psKey->psParent->sFlags.iKeysInNode++;
			}
		}
		if (psKey->psPrev)
			psKey->psPrev->psNext = psKey->psNext;
		else
			psTree->psKeyFirst = psKey->psNext;
		if (psKey->psNext)
			psKey->psNext->psPrev = psKey->psPrev;
		psKey->psParent = VBTREE_NULL;
		psKey->psChild = VBTREE_NULL;
		vVBKeyFree (iHandle, iKeyNumber, psKey);
		if (psTree->sFlags.iIsRoot && (psTree->psKeyFirst->sFlags.iIsHigh || psTree->psKeyFirst->sFlags.iIsDummy))
		{
			psKey = psTree->psKeyFirst;
			if (!psKey->psChild)
				psTreeRoot = psTree;
			else
			{
				psTreeRoot = psKey->psChild;
				psKey->psChild = VBTREE_NULL;
			}
			if (psKey->sFlags.iIsDummy)	// EMPTY FILE!
				return (iVBNodeSave (iHandle, iKeyNumber, psTreeRoot, psTreeRoot->tNodeNumber, 0, 0));
			iResult = iVBNodeLoad (iHandle, iKeyNumber, psTreeRoot, psKey->tRowNode, psTree->sFlags.iLevel);
			if (iResult)
				return (iResult);	// BUG Corrupt!
			iResult = iVBNodeFree (iHandle, psTreeRoot->tNodeNumber);
			if (iResult)
				return (iResult);	// BUG Corrupt!
			psTreeRoot->tNodeNumber = psVBFile [iHandle]->psKeydesc [iKeyNumber]->k_rootnode;
			psTreeRoot->psParent = VBTREE_NULL;
			psTreeRoot->sFlags.iIsTOF = 1;
			psTreeRoot->sFlags.iIsEOF = 1;
			psTreeRoot->sFlags.iIsRoot = 1;
			if (psTree != psTreeRoot)
				vVBTreeAllFree (iHandle, iKeyNumber, psTree);
			psVBFile [iHandle]->psTree [iKeyNumber] = psTreeRoot;
			return (iVBNodeSave (iHandle, iKeyNumber, psTreeRoot, psTreeRoot->tNodeNumber, 0, 0));
		}
		if (psTree->psKeyFirst->sFlags.iIsDummy)
		{	// Handle removal of the last key in a node
			iResult = iVBNodeFree (iHandle, psTree->tNodeNumber);
			if (iResult)
				return (iResult);	// BUG Corrupt!
			psTree = psTree->psParent;
			vVBTreeAllFree (iHandle, iKeyNumber, psTree->psKeyCurr->psChild);
			psTree->psKeyCurr->psChild = VBTREE_NULL;
			psKey = psTree->psKeyCurr;
			continue;
		}
		break;
	}
	if (iForceRewrite)
		return (iVBNodeSave (iHandle, iKeyNumber, psTree, psTree->tNodeNumber, 0, 0));
	else
		return (iVBNodeSave (iHandle, iKeyNumber, psTree, psTree->tNodeNumber, -1, iPosn));
}

/*
 * Name:
 *	int	iVBKeyCompare (int iHandle, int iKeyNumber, int iLength, char *pcKey1, char *pcKey2);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The absolute key number within the index file
 *	int	iLength
 *		The length (in bytes) of the key to be compared.  If 0, uses ALL
 *	char	*pcKey1
 *		The first (contiguous) key value
 *	char	*pcKey2
 *		The second (contiguous) key value
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	pcKey1 is LESS THAN pcKey2
 *	0	pcKey1 is EQUAL TO pcKey2
 *	1	pcKey1 is GREATER THAN pcKey2
 * Problems:
 *	NONE known
 */
int
iVBKeyCompare (int iHandle, int iKeyNumber, int iLength, unsigned char *pcKey1, unsigned char *pcKey2)
{
	int	iDescBias,
		iPart,
		iLengthToCompare,
		iResult = 0,
		iValue1,
		iValue2;
	long	lValue1,
		lValue2;
	float	fValue1,
		fValue2;
	double	dValue1,
		dValue2;
	off_t	tValue1,
		tValue2;
	struct	keydesc
		*psKeydesc;

	psKeydesc = psVBFile [iHandle]->psKeydesc [iKeyNumber];
	if (iLength == 0)
		iLength = psKeydesc->iKeyLength;
	for (iPart = 0; iLength > 0 && iPart < psKeydesc->iNParts; iPart++)
	{
		if (iLength >= psKeydesc->sPart [iPart].iLength)
			iLengthToCompare = psKeydesc->sPart [iPart].iLength;
		else
			iLengthToCompare = iLength;
		iLength -= iLengthToCompare;
		if (psKeydesc->sPart [iPart].iType & ISDESC)
			iDescBias = -1;
		else
			iDescBias = 1;
		iResult = 0;
		switch (psKeydesc->sPart [iPart].iType & ~ISDESC)
		{
		case	CHARTYPE:
			while (iLengthToCompare-- && !iResult)
			{
				if (*pcKey1 < *pcKey2)
					return (-iDescBias);
				if (*pcKey1++ > *pcKey2++)
					return (iDescBias);
			}
			break;

		case	INTTYPE:
			while (iLengthToCompare >= INTSIZE && !iResult)
			{
				iValue1 = ldint (pcKey1);
				iValue2 = ldint (pcKey2);
				if (iValue1 < iValue2)
					return (-iDescBias);
				if (iValue1 > iValue2)
					return (iDescBias);
				pcKey1 += INTSIZE;
				pcKey2 += INTSIZE;
				iLengthToCompare -= INTSIZE;
			}
			break;

		case	LONGTYPE:
			while (iLengthToCompare >= LONGSIZE && !iResult)
			{
				lValue1 = ldlong (pcKey1);
				lValue2 = ldlong (pcKey2);
				if (lValue1 < lValue2)
					return (-iDescBias);
				if (lValue1 > lValue2)
					return (iDescBias);
				pcKey1 += LONGSIZE;
				pcKey2 += LONGSIZE;
				iLengthToCompare -= LONGSIZE;
			}
			break;

		case	QUADTYPE:
			while (iLengthToCompare >= QUADSIZE && !iResult)
			{
				tValue1 = ldquad (pcKey1);
				tValue2 = ldquad (pcKey2);
				if (tValue1 < tValue2)
					return (-iDescBias);
				if (tValue1 > tValue2)
					return (iDescBias);
				pcKey1 += QUADSIZE;
				pcKey2 += QUADSIZE;
				iLengthToCompare -= QUADSIZE;
			}
			break;

		case	FLOATTYPE:
			while (iLengthToCompare >= FLOATSIZE && !iResult)
			{
				fValue1 = ldfloat (pcKey1);
				fValue2 = ldfloat (pcKey2);
				if (fValue1 < fValue2)
					return (-iDescBias);
				if (fValue1 > fValue2)
					return (iDescBias);
				pcKey1 += FLOATSIZE;
				pcKey2 += FLOATSIZE;
				iLengthToCompare -= FLOATSIZE;
			}
			break;

		case	DOUBLETYPE:
			while (iLengthToCompare >= DOUBLESIZE && !iResult)
			{
				dValue1 = lddbl (pcKey1);
				dValue2 = lddbl (pcKey2);
				if (dValue1 < dValue2)
					return (-iDescBias);
				if (dValue1 > dValue2)
					return (iDescBias);
				pcKey1 += DOUBLESIZE;
				pcKey2 += DOUBLESIZE;
				iLengthToCompare -= DOUBLESIZE;
			}
			break;

		default:
			fprintf (stderr, "HUGE ERROR! File %s, Line %d iType %d\n", __FILE__, __LINE__, psKeydesc->sPart [iPart].iType);
			exit (1);
		}
	}
	return (0);
}

/*
 * Name:
 *	static	int	iTreeLoad (int iHandle, int iKeyNumber, int iLength, char *pcKeyValue, off_t tDupNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The absolute key number within the index file (0-n)
 *	int	iLength
 *		The length of the key being compared
 *	char	*pcKeyValue
 *		The (uncompressed) key being searched for
 *	off_t	tDupNumber
 *		The duplicate number to search for
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success, pcKeyValue < psVBFile[iHandle]->psKeyCurr[iKeyNumber]
 *	1	Success, pcKeyValue = psVBFile[iHandle]->psKeyCurr[iKeyNumber]
 *	2	Index file is EMPTY
 * Problems:
 *	NONE known
 * Comments:
 *	The purpose of this function is to populate the VBTREE/VBKEY structures
 *	associated with iKeyNumber such that the index entry being pointed to
 *	matches the entry (>=) in the pcKeyValue passed into this function.
 */
static	int
iTreeLoad (int iHandle, int iKeyNumber, int iLength, char *pcKeyValue, off_t tDupNumber)
{
	int	iDelta,
		iIndex,
		iResult = 0;
	struct	VBKEY
		*psKey;
	struct	VBTREE
		*psTree;

	psTree = psVBFile [iHandle]->psTree [iKeyNumber];
	if (!psTree)
	{
		psTree = psVBTreeAllocate (iHandle);
		psVBFile [iHandle]->psTree [iKeyNumber] = psTree;
		iserrno = errno;
		if (!psTree)
			goto TreeLoadExit;
		psTree->sFlags.iIsRoot = 1;
		psTree->sFlags.iIsTOF = 1;
		psTree->sFlags.iIsEOF = 1;
		iserrno = iVBNodeLoad (iHandle, iKeyNumber, psTree, psVBFile [iHandle]->psKeydesc [iKeyNumber]->tRootNode, -1);
		if (iserrno)
		{
			vVBTreeAllFree (iHandle, iKeyNumber, psTree);
			psVBFile [iHandle]->psTree [iKeyNumber] = VBTREE_NULL;
			psVBFile [iHandle]->psKeyCurr [iKeyNumber] = VBKEY_NULL;
			goto TreeLoadExit;
		}
	}
	else
		if (psVBFile [iHandle]->sFlags.iIndexChanged)
		{
			iserrno = iVBNodeLoad (iHandle, iKeyNumber, psTree, psVBFile [iHandle]->psKeydesc [iKeyNumber]->tRootNode, -1);
			if (iserrno)
			{
				vVBTreeAllFree (iHandle, iKeyNumber, psTree);
				psVBFile [iHandle]->psTree [iKeyNumber] = VBTREE_NULL;
				psVBFile [iHandle]->psKeyCurr [iKeyNumber] = VBKEY_NULL;
				goto TreeLoadExit;
			}
		}
	iserrno = EBADFILE;
	if (psTree->tNodeNumber != psVBFile [iHandle]->psKeydesc [iKeyNumber]->tRootNode)
		goto TreeLoadExit;
	psTree->sFlags.iIsRoot = 1;
	psTree->sFlags.iIsTOF = 1;
	psTree->sFlags.iIsEOF = 1;
	while (1)
	{
// The following code takes a 'bisection' type approach for location of the
// key entry.  It FAR outperforms the original sequential search code.
#if ISAMMODE == 1
		iDelta = 256;		// Magic stuff based on NODE LENGTH
		iIndex = 256;		// and QUADSIZE.  Don't change
#else	//ISAMMODE == 1
		iDelta = 128;		// Magic stuff based on NODE LENGTH
		iIndex = 128;		// and QUADSIZE.  Don't change
#endif	// ISAMMODE == 1
		while (iDelta)
		{
			iDelta = iDelta >> 1;
			if (iIndex > psTree->sFlags.iKeysInNode)
			{
				iIndex -= iDelta;
				continue;
			}
			psTree->psKeyCurr = psTree->psKeyList [iIndex - 1];
			if (psTree->psKeyCurr->sFlags.iIsDummy)
				iResult = -1;
			else
				iResult = iVBKeyCompare (iHandle, iKeyNumber, iLength, pcKeyValue, psTree->psKeyCurr->cKey);
			if (iResult == 0)
			{
				if (tDupNumber > psTree->psKeyCurr->tDupNumber)
				{
					iResult = 1;
					iIndex += iDelta;
					continue;
				}
				if (tDupNumber < psTree->psKeyCurr->tDupNumber)
				{
					iResult = -1;
					iIndex -= iDelta;
					continue;
				}
				if (tDupNumber == psTree->psKeyCurr->tDupNumber)
					break;
			}
			if (iResult < 0)
			{
				iIndex -= iDelta;
				continue;
			}
			if (iResult > 0)
			{
				iIndex += iDelta;
				continue;
			}
		}
		if (iResult > 0 && psTree->psKeyCurr->psNext)
			psTree->psKeyCurr = psTree->psKeyList [iIndex];
		if (psTree->psKeyCurr->sFlags.iIsDummy && psTree->psKeyCurr->psPrev && psTree->psKeyCurr->psPrev->sFlags.iIsHigh)
			psTree->psKeyCurr = psTree->psKeyCurr->psPrev;
		iResult = iVBKeyCompare (iHandle, iKeyNumber, iLength, pcKeyValue, psTree->psKeyCurr->cKey);
		if (iResult == 0 && tDupNumber < psTree->psKeyCurr->tDupNumber)
			iResult = -1;
// The following code was WAY too s-l-o-w...
// ESPECIALLY on small indexes where there are LOTS of keys per node
//		for (psTree->psKeyCurr = psTree->psKeyFirst; psTree->psKeyCurr && psTree->psKeyCurr->psNext; psTree->psKeyCurr = psTree->psKeyCurr->psNext)
//		{
//			if (psTree->psKeyCurr->sFlags.iIsHigh)
//				break;
//			iResult = iVBKeyCompare (iHandle, iKeyNumber, iLength, pcKeyValue, psTree->psKeyCurr->cKey);
//			if (iResult < 0)
//				break;		// Exit the for loop
//			if (iResult > 0)
//				continue;
//			if (tDupNumber < psTree->psKeyCurr->tDupNumber)
//			{
//				iResult = -1;
//				break;		// Exit the for loop
//			}
//			if (tDupNumber == psTree->psKeyCurr->tDupNumber)
//				break;		// Exit the for loop
//		}
		if (!psTree->sFlags.iLevel)
			break;			// Exit the while loop
		if (!psTree->psKeyCurr)
			goto TreeLoadExit;
		if (!psTree->psKeyCurr->psChild || psVBFile [iHandle]->sFlags.iIndexChanged)
		{
			psKey = psTree->psKeyCurr;
			if (!psTree->psKeyCurr->psChild)
			{
				psKey->psChild = psVBTreeAllocate (iHandle);
				iserrno = errno;
				if (!psKey->psChild)
					goto TreeLoadExit;
				psKey->psChild->psParent = psKey->psParent;
				if (psKey->psParent->sFlags.iIsTOF && psKey == psKey->psParent->psKeyFirst)
					psKey->psChild->sFlags.iIsTOF = 1;
				if (psKey->psParent->sFlags.iIsEOF && psKey == psKey->psParent->psKeyLast->psPrev)
					psKey->psChild->sFlags.iIsEOF = 1;
			}
			iserrno = iVBNodeLoad (iHandle, iKeyNumber, psTree->psKeyCurr->psChild, psTree->psKeyCurr->tRowNode, psTree->sFlags.iLevel);
			if (iserrno)
			{
				vVBTreeAllFree (iHandle, iKeyNumber, psKey->psChild);
				psKey->psChild = VBTREE_NULL;
				goto TreeLoadExit;
			}
			psTree->psKeyCurr->psParent = psTree;
			psTree->psKeyCurr->psChild->psParent = psTree;
			if (psTree->sFlags.iIsTOF && psTree->psKeyCurr == psTree->psKeyFirst)
				psTree->psKeyCurr->psChild->sFlags.iIsTOF = 1;
			if (psTree->sFlags.iIsEOF && psTree->psKeyCurr == psTree->psKeyLast)
				psTree->psKeyCurr->psChild->sFlags.iIsEOF = 1;
		}
		psTree = psTree->psKeyCurr->psChild;
	}
	/*
	 * When we get here, iResult is set depending upon whether the located
	 * key was:
	 * -1	LESS than the desired value
	 * 0	EQUAL to the desired value (including a tDupNumber match!)
	 * 1	GREATER than the desired value
	 * By simply adding one to the value, we're cool for a NON-STANDARD
	 * comparison return value.
	 */
	psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psTree->psKeyCurr;
	if (!psTree->psKeyCurr)
	{
		iserrno = EBADFILE;
		return (-1);
	}
	iserrno = 0;
	if (psTree->psKeyCurr->sFlags.iIsDummy)
	{
		iserrno = EENDFILE;
		if (psTree->psKeyCurr->psPrev)
			return (0);	// EOF
		else
			return (2);	// Empty file!
	}
	return (iResult + 1);

TreeLoadExit:
	return (-1);
}

#ifdef	DEBUG
static	void
vDumpKey (struct VBKEY *psKey, struct VBTREE *psTree, int iIndent)
{
	unsigned char
		cBuffer [QUADSIZE];
	int	iKey;

	memcpy (cBuffer, psKey->cKey, QUADSIZE);
	iKey = ldint (cBuffer);
#if ISAMMODE == 1
	printf
	(
		"%-*.*s KEY :%02X%02llX\t%08X DAD:%04X KID:%04X ROW:%04llX",
		iIndent, iIndent, "          ",
		cBuffer [0],
		psKey->tDupNumber,
		(int) psKey,
		(int) psKey->psParent & 0xffff,
		(int) psKey->psChild & 0xffff,
		psKey->tRowNode & 0xffff
	);
#else	//ISAMMODE == 1
	printf
	(
		"%-*.*s KEY :%02X%02llX\t%08X DAD:%04X KID:%04X ROW:%04llX",
		iIndent, iIndent, "          ",
		cBuffer [0],
		psKey->tDupNumber,
		(int) psKey,
		(int) psKey->psParent & 0xffff,
		(int) psKey->psChild & 0xffff,
		psKey->tRowNode & 0xffff
	);
#endif	//ISAMMODE == 1
	if (psKey == psTree->psKeyFirst)
		printf (" 1ST");
	if (psKey == psTree->psKeyCurr)
		printf (" CUR");
	if (psKey == psTree->psKeyLast)
		printf (" LST");
	if (psKey->sFlags.iIsNew)
		printf (" NEW");
	if (psKey->sFlags.iIsDummy)
		printf (" DMY");
	if (psKey->sFlags.iIsHigh)
		printf (" HI");
	printf ("\n");
	fflush (stdout);
	if (psKey->psChild)
		vDumpTree (psKey->psChild, iIndent + 1);
}

static	void
vDumpTree (struct VBTREE *psTree, int iIndent)
{
	struct	VBKEY
		*psKey;

#if	ISAMMODE == 1
	printf
	(
		"%-*.*sNODE:%lld  \t%08X DAD:%04X LVL:%04X CUR:%04X",
		iIndent, iIndent, "          ",
		psTree->tNodeNumber,
		(int) psTree,
		(int) psTree->psParent & 0xffff,
		psTree->sFlags.iLevel,
		(int) psTree->psKeyCurr & 0xffff
	);
#else	//ISAMMODE == 1
	printf
	(
		"%-*.*sNODE:%lld  \t%08X DAD:%04X LVL:%04X CUR:%04X",
		iIndent, iIndent, "          ",
		psTree->tNodeNumber,
		(int) psTree,
		(int) psTree->psParent & 0xffff,
		psTree->sFlags.iLevel,
		(int) psTree->psKeyCurr & 0xffff
	);
#endif	//ISAMMODE == 1
	fflush (stdout);
	if (psTree->sFlags.iIsRoot)
		printf (" RT.");
	if (psTree->sFlags.iIsTOF)
		printf (" TOF");
	if (psTree->sFlags.iIsEOF)
		printf (" EOF");
	printf ("\n");
	for (psKey = psTree->psKeyFirst; psKey; psKey = psKey->psNext)
		vDumpKey (psKey, psTree, iIndent);
}

int
iDumpTree (int iHandle, int iKeyNumber)
{
	struct	VBTREE
		*psTree = psVBFile [iHandle]->psTree [iKeyNumber];

	fflush (stdout);
	vDumpTree (psTree, 0);
	return (0);
}

static	int
iChkTree2 (struct VBTREE *psTree, int iLevel, struct VBKEY *psCurrKey, int *piCurrentInTree)
{
	int	iResult;
	struct	VBKEY
		*psKey;

	for (psKey = psTree->psKeyFirst; psKey; psKey = psKey->psNext)
	{
		if (psKey->psParent != psTree)
			return (1);
		if (psKey == psCurrKey)
			*piCurrentInTree = *piCurrentInTree + 1;
		if (!psKey->psChild)
			continue;
		if (psKey->psChild->psParent != psTree)
			return (2);
		if (psKey->psChild->sFlags.iLevel != iLevel - 1)
			return (3);
		iResult = iChkTree2 (psKey->psChild, psKey->psChild->sFlags.iLevel, psCurrKey, piCurrentInTree);
		if (iResult)
			return (iResult);
	}
	return (0);
}

int
iChkTree (int iHandle, int iKeyNumber)
{
	unsigned char
		cBuffer [QUADSIZE];
	int	iCurrInTree;
	struct	VBTREE
		*psTree = psVBFile [iHandle]->psTree [iKeyNumber];
	struct	VBKEY
		*psKey;

	if (!psTree)
		return (0);
	psKey = psVBFile [iHandle]->psKeyCurr [iKeyNumber];
	if (psKey)
	{
		iCurrInTree = 0;
		cBuffer [0] = psKey->cKey [0];
	}
	else
		iCurrInTree = 1;
	if (iChkTree2 (psTree, psTree->sFlags.iLevel, psKey, &iCurrInTree))
		printf ("Tree is invalid!\n");
	else
		if (psKey && psKey->tRowNode == -1)
			printf ("CurrentKey is invalid!\n");
		else
			if (iCurrInTree != 1)
				printf ("CurrentKey is in the tree %d times!\n", iCurrInTree);
			else
				if (psKey != psKey->psParent->psKeyCurr)
					printf ("CurrentKey is NOT the current key of its parent!\n");
	printf
	(
		"CURR KEY :%02X%02llX\t%08X DAD:%04X KID:%04X ROW:%04llX",
		cBuffer [0],
		psKey->tDupNumber,
		(int) psKey,
		(int) psKey->psParent & 0xffff,
		(int) psKey->psChild & 0xffff,
		psKey->tRowNode & 0xffff
	);
	if (psKey->sFlags.iIsNew)
		printf (" NEW");
	if (psKey->sFlags.iIsDummy)
		printf (" DMY");
	if (psKey->sFlags.iIsHigh)
		printf (" HI");
	printf ("\n");
	iDumpTree (iHandle, iKeyNumber);
	return (0);
}
#endif	//DEBUG
