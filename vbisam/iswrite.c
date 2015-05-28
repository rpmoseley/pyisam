/*
 * Title:	iswrite.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	11Dec2003
 * Description:
 *	This is the module that deals with all the writing to a file in the
 *	VBISAM library.
 * Version:
 *	$Id: iswrite.c,v 1.8 2004/06/16 10:53:56 trev_vb Exp $
 * Modification History:
 *	$Log: iswrite.c,v $
 *	Revision 1.8  2004/06/16 10:53:56  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
 *	
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
 *	Revision 1.2  2003/12/22 04:48:02  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:20  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"

/*
 * Prototypes
 */
int	iswrcurr (int, char *);
int	iswrite (int, char *);
int	iVBWriteRow (int, char *, off_t);
static	int	iRowInsert (int, char *, off_t);

/*
 * Name:
 *	int	iswrcurr (int iHandle, char *pcRow);
 * Arguments:
 *	int	iHandle
 *		The open VBISAM file handle
 *	char	*pcRow
 *		The data to be written
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
iswrcurr (int iHandle, char *pcRow)
{
	int	iResult;
	off_t	tRowNumber;


	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (iResult);

	if (psVBFile [iHandle]->iOpenMode & ISVARLEN && (isreclen > psVBFile [iHandle]->iMaxRowLength || isreclen < psVBFile [iHandle]->iMinRowLength))
	{
		iserrno = EBADARG;
		return (-1);
	}

	tRowNumber = tVBDataAllocate (iHandle);
	if (tRowNumber == -1)
		return (-1);

	iResult = iVBWriteRow (iHandle, pcRow, tRowNumber);
	if (!iResult)
		psVBFile [iHandle]->tRowNumber = tRowNumber;
	else
		iVBDataFree (iHandle, tRowNumber);

	iVBExit (iHandle);
	return (iResult);
}

/*
 * Name:
 *	int	iswrite (int iHandle, char *pcRow);
 * Arguments:
 *	int	iHandle
 *		The open VBISAM file handle
 *	char	*pcRow
 *		The data to be written
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
iswrite (int iHandle, char *pcRow)
{
	int	iResult,
		iSaveError;
	off_t	tRowNumber;

	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (iResult);

	if (psVBFile [iHandle]->iOpenMode & ISVARLEN && (isreclen > psVBFile [iHandle]->iMaxRowLength || isreclen < psVBFile [iHandle]->iMinRowLength))
	{
		iserrno = EBADARG;
		return (-1);
	}

	tRowNumber = tVBDataAllocate (iHandle);
	if (tRowNumber == -1)
		return (-1);

	iResult = iVBWriteRow (iHandle, pcRow, tRowNumber);
	if (iResult)
	{
		iSaveError = iserrno;
		iVBDataFree (iHandle, tRowNumber);
		iserrno = iSaveError;
	}

	iVBExit (iHandle);
	return (iResult);
}

/*
 * Name:
 *	int	iVBWriteRow (int iHandle, char *pcRow, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open VBISAM file handle
 *	char	*pcRow
 *		The data to be written
 *	off_t	tRowNumber
 *		The row number to write to
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
iVBWriteRow (int iHandle, char *pcRow, off_t tRowNumber)
{
	int	iResult = 0;

	isrecnum = tRowNumber;
	if (psVBFile [iHandle]->iOpenMode & ISTRANS)
	{
		iserrno = iVBDataLock (iHandle, VBWRLOCK, tRowNumber);
		if (iserrno)
			return (-1);
	}
	iResult = iRowInsert (iHandle, pcRow, tRowNumber);
	if (!iResult)
	{
		iserrno = 0;
		psVBFile [iHandle]->tVarlenNode = 0;	// Stop it from removing
		iResult = iVBDataWrite (iHandle, (void *) pcRow, FALSE, tRowNumber, TRUE);
		if (iResult)
		{
			iserrno = iResult;
			if (psVBFile [iHandle]->iOpenMode & ISTRANS)
				iVBDataLock (iHandle, VBUNLOCK, tRowNumber);
			return (-1);
		}
	}
	if (!iResult)
	{
		if (psVBFile [iHandle]->iOpenMode & ISVARLEN)
			iResult = iVBTransInsert (iHandle, tRowNumber, isreclen, pcRow);
		else
			iResult = iVBTransInsert (iHandle, tRowNumber, psVBFile [iHandle]->iMinRowLength, pcRow);
	}
	return (iResult);
}

/*
 * Name:
 *	static	int	iRowInsert (int iHandle, char *pcRowBuffer, off_t tRowNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	char	*pcRowBuffer
 *		A pointer to the buffer containing the row to be added
 *	off_t	tRowNumber
 *		The absolute row number of the row to be added
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
static	int
iRowInsert (int iHandle, char *pcRowBuffer, off_t tRowNumber)
{
	char	cKeyValue [VB_MAX_KEYLEN];
	int	iKeyNumber,
		iResult;
	struct	VBKEY
		*psKey;
	off_t	tDupNumber [MAXSUBS];

	/*
	 * Step 1:
	 *	Check each index for a potential ISNODUPS error (EDUPL)
	 *	Also, calculate the duplicate number as needed
	 */
	for (iKeyNumber = 0; iKeyNumber < psVBFile [iHandle]->iNKeys; iKeyNumber++)
	{
		if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iNParts == 0)
			continue;
		vVBMakeKey (iHandle, iKeyNumber, pcRowBuffer, cKeyValue);
		iResult = iVBKeySearch (iHandle, ISGREAT, iKeyNumber, 0, cKeyValue, 0);
		tDupNumber [iKeyNumber] = 0;
		if (iResult >= 0 && !iVBKeyLoad (iHandle, iKeyNumber, ISPREV, FALSE, &psKey) && !memcmp (psKey->cKey, cKeyValue, psVBFile [iHandle]->psKeydesc [iKeyNumber]->iKeyLength))
		{
			iserrno = EDUPL;
			if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iFlags & ISDUPS)
				tDupNumber [iKeyNumber] = psKey->tDupNumber + 1;
			else
				return (-1);
		}
		iResult = iVBKeySearch (iHandle, ISGTEQ, iKeyNumber, 0, cKeyValue, tDupNumber [iKeyNumber]);
	}

	// Step 2: Perform the actual insertion into each index
	for (iKeyNumber = 0; iKeyNumber < psVBFile [iHandle]->iNKeys; iKeyNumber++)
	{
		if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iNParts == 0)
			continue;
		vVBMakeKey (iHandle, iKeyNumber, pcRowBuffer, cKeyValue);
		iResult = iVBKeyInsert (iHandle, VBTREE_NULL, iKeyNumber, cKeyValue, tRowNumber, tDupNumber [iKeyNumber], VBTREE_NULL);
		if (iResult)
		{
// BUG - do something SANE here
		// Eeek, an error occured.  Let's remove what we added
			//while (iKeyNumber >= 0)
			//{
				//iVBKeyDelete (iHandle, iKeyNumber);
				//iKeyNumber--;
			//}
			return (iResult);
		}
	}

	return (0);
}
