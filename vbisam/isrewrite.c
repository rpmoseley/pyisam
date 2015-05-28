/*
 * Title:	isrewrite.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	11Dec2003
 * Description:
 *	This is the module that deals with all the rewriting to a file in the
 *	VBISAM library.
 * Version:
 *	$Id: isrewrite.c,v 1.7 2004/06/16 10:53:56 trev_vb Exp $
 * Modification History:
 *	$Log: isrewrite.c,v $
 *	Revision 1.7  2004/06/16 10:53:56  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
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
 *	Revision 1.2  2003/12/22 04:47:34  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:17  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"

/*
 * Prototypes
 */
int	isrewrite (int, char *);
int	isrewcurr (int, char *);
int	isrewrec (int, off_t, char *);
static	int	iRowUpdate (int, char *, off_t);

/*
 * Name:
 *	int	isrewrite (int iHandle, char *pcRow);
 * Arguments:
 *	int	iHandle
 *		The open VBISAM file handle
 *	char	*pcRow
 *		The data row to be rewritten
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
isrewrite (int iHandle, char *pcRow)
{
	char	cKeyValue [VB_MAX_KEYLEN];
	int	iDeleted,
		iNewRecLen,
		iOldRecLen = 0,
		iResult = 0;
	off_t	tRowNumber;

	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (-1);
	if (psVBFile [iHandle]->iOpenMode & ISVARLEN && (isreclen > psVBFile [iHandle]->iMaxRowLength || isreclen < psVBFile [iHandle]->iMinRowLength))
	{
		iserrno = EBADARG;
		return (-1);
	}

	iNewRecLen = isreclen;
	if (psVBFile [iHandle]->psKeydesc [0]->iFlags & ISDUPS)
	{
		iResult = -1;
		iserrno = ENOREC;
	}
	else
	{
		vVBMakeKey (iHandle, 0, pcRow, cKeyValue);
		iResult = iVBKeySearch (iHandle, ISEQUAL, 0, 0, cKeyValue, 0);
		switch (iResult)
		{
		case	1:	// Exact match
			iResult = 0;
			psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
			tRowNumber = psVBFile [iHandle]->psKeyCurr [0]->tRowNode;
			if (psVBFile [iHandle]->iOpenMode & ISTRANS)
			{
				iserrno = iVBDataLock (iHandle, VBWRLOCK, tRowNumber);
				if (iserrno)
				{
					iResult = -1;
					goto ISRewriteExit;
				}
			}
			iserrno = iVBDataRead (iHandle, (void *) *(psVBFile [iHandle]->ppcRowBuffer), &iDeleted, tRowNumber, TRUE);
			if (!iserrno && iDeleted)
				iserrno = ENOREC;
			if (iserrno)
				iResult = -1;
			else
				iOldRecLen = isreclen;

			if (!iResult)
				iResult = iRowUpdate (iHandle, pcRow, tRowNumber);
			if (!iResult)
			{
				isrecnum = tRowNumber;
				isreclen = iNewRecLen;
				iResult = iVBDataWrite (iHandle, (void *) pcRow, FALSE, isrecnum, TRUE);
			}
			if (!iResult)
			{
				if (psVBFile [iHandle]->iOpenMode & ISVARLEN)
					iResult = iVBTransUpdate (iHandle, tRowNumber, iOldRecLen, iNewRecLen, pcRow);
				else
					iResult = iVBTransUpdate (iHandle, tRowNumber, psVBFile [iHandle]->iMinRowLength, psVBFile [iHandle]->iMinRowLength, pcRow);
			}
			break;

		case	0:		// LESS than
		case	2:		// GREATER than
		case	3:		// EMPTY file
			iserrno = ENOREC;
			iResult = -1;
			break;

		default:
			iserrno = EBADFILE;
			iResult = -1;
			break;
		}
	}

ISRewriteExit:
	iResult |= iVBExit (iHandle);
	return (iResult);
}

/*
 * Name:
 *	int	isrewcurr (int iHandle, char *pcRow);
 * Arguments:
 *	int	iHandle
 *		The open VBISAM file handle
 *	char	*pcRow
 *		The data row to be rewritten
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
isrewcurr (int iHandle, char *pcRow)
{
	int	iDeleted,
		iNewRecLen,
		iOldRecLen = 0,
		iResult = 0;

	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (-1);

	if (psVBFile [iHandle]->iOpenMode & ISVARLEN && (isreclen > psVBFile [iHandle]->iMaxRowLength || isreclen < psVBFile [iHandle]->iMinRowLength))
	{
		iserrno = EBADARG;
		return (-1);
	}

	iNewRecLen = isreclen;
	if (psVBFile [iHandle]->tRowNumber > 0)
	{
		if (psVBFile [iHandle]->iOpenMode & ISTRANS)
		{
			iserrno = iVBDataLock (iHandle, VBWRLOCK, psVBFile [iHandle]->tRowNumber);
			if (iserrno)
			{
				iResult = -1;
				goto ISRewcurrExit;
			}
		}
		iserrno = iVBDataRead (iHandle, (void *) *(psVBFile [iHandle]->ppcRowBuffer), &iDeleted, psVBFile [iHandle]->tRowNumber, TRUE);
		if (!iserrno && iDeleted)
			iserrno = ENOREC;
		else
			iOldRecLen = isreclen;
		if (iserrno)
			iResult = -1;
		if (!iResult)
			iResult = iRowUpdate (iHandle, pcRow, psVBFile [iHandle]->tRowNumber);
		if (!iResult)
		{
			isrecnum = psVBFile [iHandle]->tRowNumber;
			isreclen = iNewRecLen;
			iResult = iVBDataWrite (iHandle, (void *) pcRow, FALSE, isrecnum, TRUE);
		}
		if (!iResult)
		{
			if (psVBFile [iHandle]->iOpenMode & ISVARLEN)
				iResult = iVBTransUpdate (iHandle, psVBFile [iHandle]->tRowNumber, iOldRecLen, iNewRecLen, pcRow);
			else
				iResult = iVBTransUpdate (iHandle, psVBFile [iHandle]->tRowNumber, psVBFile [iHandle]->iMinRowLength, psVBFile [iHandle]->iMinRowLength, pcRow);
		}
	}
ISRewcurrExit:
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	iResult |= iVBExit (iHandle);
	return (iResult);
}

/*
 * Name:
 *	int	isrewrec (int iHandle, off_t tRowNumber, char *pcRow);
 * Arguments:
 *	int	iHandle
 *		The open VBISAM file handle
 *	off_t	tRowNumber
 *		The data row number to be deleted
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
isrewrec (int iHandle, off_t tRowNumber, char *pcRow)
{
	int	iDeleted,
		iNewRecLen,
		iOldRecLen = 0,
		iResult = 0;

	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (-1);

	if (psVBFile [iHandle]->iOpenMode & ISVARLEN && (isreclen > psVBFile [iHandle]->iMaxRowLength || isreclen < psVBFile [iHandle]->iMinRowLength))
	{
		iserrno = EBADARG;
		return (-1);
	}

	iNewRecLen = isreclen;
	if (tRowNumber < 1)
	{
		iResult = -1;
		iserrno = ENOREC;
	}
	else
	{
		if (psVBFile [iHandle]->iOpenMode & ISTRANS)
		{
			iserrno = iVBDataLock (iHandle, VBWRLOCK, tRowNumber);
			if (iserrno)
			{
				iResult = -1;
				goto ISRewrecExit;
			}
		}
		iserrno = iVBDataRead (iHandle, (void *) *(psVBFile [iHandle]->ppcRowBuffer), &iDeleted, tRowNumber, TRUE);
		if (!iserrno && iDeleted)
			iserrno = ENOREC;
		if (iserrno)
			iResult = -1;
		else
			iOldRecLen = isreclen;
		if (!iResult)
			iResult = iRowUpdate (iHandle, pcRow, tRowNumber);
		if (!iResult)
		{
			isrecnum = tRowNumber;
			isreclen = iNewRecLen;
			iResult = iVBDataWrite (iHandle, (void *) pcRow, FALSE, isrecnum, TRUE);
		}
		if (!iResult)
		{
			if (psVBFile [iHandle]->iOpenMode & ISVARLEN)
				iResult = iVBTransUpdate (iHandle, tRowNumber, iOldRecLen, iNewRecLen, pcRow);
			else
				iResult = iVBTransUpdate (iHandle, tRowNumber, psVBFile [iHandle]->iMinRowLength, psVBFile [iHandle]->iMinRowLength, pcRow);
		}
	}
ISRewrecExit:
	if (!iResult)
		psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	iResult |= iVBExit (iHandle);
	return (iResult);
}

/*
 * Name:
 *	static	int	iRowUpdate (int iHandle, char *pcRow, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	char	*pcRow
 *		A pointer to the buffer containing the updated row
 *	off_t	tRowNumber
 *		The row number to be updated
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
static	int
iRowUpdate (int iHandle, char *pcRow, off_t tRowNumber)
{
	char	cKeyValue [VB_MAX_KEYLEN];
	int	iKeyNumber,
		iResult;
	struct	VBKEY
		*psKey;
	off_t	tDupNumber = 0;

	/*
	 * Step 1:
	 *	For each index that's changing, confirm that the NEW value
	 *	doesn't conflict with an existing ISNODUPS flag.
	 */
	for (iKeyNumber = 0; iKeyNumber < psVBFile [iHandle]->iNKeys; iKeyNumber++)
	{
		if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iNParts == 0)
			continue;
		if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iFlags & ISDUPS)
			continue;
		vVBMakeKey (iHandle, iKeyNumber, pcRow, cKeyValue);
		iResult = iVBKeySearch (iHandle, ISGTEQ, iKeyNumber, 0, cKeyValue, 0);
		if (iResult != 1 || tRowNumber == psVBFile [iHandle]->psKeyCurr [iKeyNumber]->tRowNode || psVBFile [iHandle]->psKeyCurr[iKeyNumber]->sFlags.iIsDummy)
			continue;
		iserrno = EDUPL;
		return (-1);
	}

	/*
	 * Step 2:
	 *	Check each index for existance of tRowNumber
	 *	This 'preload' additionally helps determine which indexes change
	 */
	for (iKeyNumber = 0; iKeyNumber < psVBFile [iHandle]->iNKeys; iKeyNumber++)
	{
		if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iNParts == 0)
			continue;
		iResult = iVBKeyLocateRow (iHandle, iKeyNumber, tRowNumber);
		iserrno = EBADFILE;
		if (iResult)
			return (-1);
	}

	/*
	 * Step 3:
	 *	Perform the actual deletion / insertion with each index
	 *	But *ONLY* for those indexes that have actually CHANGED!
	 */
	for (iKeyNumber = 0; iKeyNumber < psVBFile [iHandle]->iNKeys; iKeyNumber++)
	{
		if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iNParts == 0)
			continue;
		// pcRow is the UPDATED key!
		vVBMakeKey (iHandle, iKeyNumber, pcRow, cKeyValue);
		iResult = memcmp (cKeyValue, psVBFile [iHandle]->psKeyCurr [iKeyNumber]->cKey, psVBFile [iHandle]->psKeydesc [iKeyNumber]->iKeyLength);
		// If NEW key is DIFFERENT than CURRENT, remove CURRENT
		if (iResult)
			iResult = iVBKeyDelete (iHandle, iKeyNumber);
		else
			continue;
		if (iResult)
		{
		// Eeek, an error occured.  Let's put back what we removed!
			while (iKeyNumber >= 0)
			{
// BUG - We need to do SOMETHING sane here? Dunno WHAT
				iVBKeyInsert (iHandle, VBTREE_NULL, iKeyNumber, cKeyValue, tRowNumber, tDupNumber, VBTREE_NULL);
				iKeyNumber--;
				vVBMakeKey (iHandle, iKeyNumber, *(psVBFile [iHandle]->ppcRowBuffer), cKeyValue);
			}
			iserrno = EBADFILE;
			return (-1);
		}
		iResult = iVBKeySearch (iHandle, ISGREAT, iKeyNumber, 0, cKeyValue, 0);
		tDupNumber = 0;
		if (iResult >= 0)
		{
			iResult = iVBKeyLoad (iHandle, iKeyNumber, ISPREV, TRUE, &psKey);
			if (!iResult)
			{
				if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iFlags & ISDUPS && !memcmp (psKey->cKey, cKeyValue, psVBFile [iHandle]->psKeydesc [iKeyNumber]->iKeyLength))
					tDupNumber = psKey->tDupNumber + 1;
				psVBFile [iHandle]->psKeyCurr [iKeyNumber] = psVBFile [iHandle]->psKeyCurr [iKeyNumber]->psNext;
				psVBFile [iHandle]->psKeyCurr [iKeyNumber]->psParent->psKeyCurr = psVBFile [iHandle]->psKeyCurr [iKeyNumber];
			}
			iResult = iVBKeySearch (iHandle, ISGTEQ, iKeyNumber, 0, cKeyValue, tDupNumber);
			iResult = iVBKeyInsert (iHandle, VBTREE_NULL, iKeyNumber, cKeyValue, tRowNumber, tDupNumber, VBTREE_NULL);
		}
		if (iResult)
		{
		// Eeek, an error occured.  Let's remove what we added
			while (iKeyNumber >= 0)
			{
// BUG - This is WRONG, we should re-establish what we had before!
				//iVBKeyDelete (iHandle, iKeyNumber);
				iKeyNumber--;
			}
			return (iResult);
		}
	}

	return (0);
}
