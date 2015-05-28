/*
 * Title:	isrecover.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	30May2004
 * Description:
 *	This is the module that deals solely with the isrecover function in the
 *	VBISAM library.
 * Version:
 *	$Id: isrecover.c,v 1.2 2004/06/11 22:16:16 trev_vb Exp $
 * Modification History:
 *	$Log: isrecover.c,v $
 *	Revision 1.2  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.1  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.5  2004/01/06 14:31:59  trev_vb
 *	
 */
#include	"isinternal.h"

/*
 * Prototypes
 */
static	void	vDelayedDataFree (int, int, off_t);
static	void	vFreeDelayed (int);

struct	VBDELAY
{
	struct	VBDELAY
		*psNext;
	int	iHandle;
	off_t	tRowNumber;
};
#define	VBDELAY_NULL	((struct VBDELAY *) 0)

	struct
	{
		off_t	tOffset;
		int	iPID;
		struct	VBDELAY
			*psDelayHead;
	} sOpenTrans [MAX_OPEN_TRANS];

/*
 * Name:
 *	int	isrecover (void)
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 * Comments:
 *	At present, isrecover () has a limitation of MAX_OPEN_TRANS 'open'
 *	transactions at any given point in time.  If this limit is exceeded,
 *	isrecover will FAIL with iserrno = EBADMEM
 */
int	isrecover (void)
{
	char	*pcBuffer,
		*pcRow;
	int	iFreeNow,
		iHandle,
		iLoop,
		iLocalHandle [VB_MAX_FILES + 1];
	off_t	tLength,
		tLength2,
		tLength3,
		tOffset,
		tRBOffset,
		tRowNumber;

	// Initialize by stating that *ALL* transactions are 'closed'.
	for (iLoop = 0; iLoop < MAX_OPEN_TRANS; iLoop++)
		sOpenTrans [iLoop].tOffset = -1;
	for (iLoop = 0; iLoop <= VB_MAX_FILES; iLoop++)
	{
		if (psVBFile [iLoop])
			iLocalHandle [iLoop] = iLoop;
		else
			iLocalHandle [iLoop] = -1;
	}
	iVBInTrans = VBRECOVER;
	psVBLogHeader = (struct SLOGHDR *) (cVBTransBuffer - INTSIZE);
	pcBuffer = cVBTransBuffer - INTSIZE + sizeof (struct SLOGHDR);
	// Begin by reading the header of the first transaction
	iserrno = EBADFILE;
	if (tVBLseek (iVBLogfileHandle, 0, SEEK_SET) != 0)
		return (-1);
	if (tVBRead (iVBLogfileHandle, cVBTransBuffer, INTSIZE) != INTSIZE)
		return (0);	// Nothing to do if the file is empty
	tOffset = INTSIZE;
	tLength = ldint (cVBTransBuffer);
	// Now, recurse forwards
	while (1)
	{
		tLength2 = tVBRead (iVBLogfileHandle, cVBTransBuffer, tLength);
		iserrno = EBADFILE;
		if (tLength2 != tLength && tLength2 != tLength - INTSIZE)
			return (-1);
		iHandle = ldint (pcBuffer);
		tRowNumber = ldquad (pcBuffer + INTSIZE);
		if (!memcmp (psVBLogHeader->cOperation, VBL_BEGIN, 2))
		{
			/*
			 * Handle the weird case where we get a 2nd BW for the
			 * SAME PID before we encounter the CW/RW of the 1st
			 */
			for (iLoop = 0; iLoop < MAX_OPEN_TRANS; iLoop++)
				if (sOpenTrans [iLoop].tOffset != -1 && ldint (psVBLogHeader->cPID) == sOpenTrans [iLoop].iPID)
				{
					vFreeDelayed (iLoop);
					iserrno = iVBRollMeBack (sOpenTrans [iLoop].tOffset, sOpenTrans [iLoop].iPID, TRUE);
					sOpenTrans [iLoop].tOffset = -1;
					if (iserrno)
						return (-1);	// Bug RestoreState
				}
			for (iLoop = 0; iLoop < MAX_OPEN_TRANS; iLoop++)
			{
				if (sOpenTrans [iLoop].tOffset == -1)
				{
					sOpenTrans [iLoop].tOffset = tOffset + tLength2 - INTSIZE;
					sOpenTrans [iLoop].iPID = ldint (psVBLogHeader->cPID);
					sOpenTrans [iLoop].psDelayHead = VBDELAY_NULL;
					break;
				}
			}
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_COMMIT, 2))
		{
			for (iLoop = 0; iLoop < MAX_OPEN_TRANS; iLoop++)
				if (sOpenTrans [iLoop].tOffset != -1 && ldint (psVBLogHeader->cPID) == sOpenTrans [iLoop].iPID)
				{
					vFreeDelayed (iLoop);
					sOpenTrans [iLoop].tOffset = -1;
				}
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_ROLLBACK, 2))
		{
			tLength3 = ldint (cVBTransBuffer + tLength - INTSIZE);
			tRBOffset = tVBLseek (iVBLogfileHandle, 0, SEEK_CUR);
			for (iLoop = 0; iLoop < MAX_OPEN_TRANS; iLoop++)
				if (sOpenTrans [iLoop].tOffset != -1 && ldint (psVBLogHeader->cPID) == sOpenTrans [iLoop].iPID)
				{
					vFreeDelayed (iLoop);
					iserrno = iVBRollMeBack (sOpenTrans [iLoop].tOffset, sOpenTrans [iLoop].iPID, TRUE);
					sOpenTrans [iLoop].tOffset = -1;
					if (iserrno)
						return (-1);	// Bug RestoreState
				}
			tVBLseek (iVBLogfileHandle, tRBOffset, SEEK_SET);
			tOffset += tLength2;
			if (tLength2 == tLength - INTSIZE)
				break;
			tLength = tLength3;
			psVBLogHeader = (struct SLOGHDR *) (cVBTransBuffer - INTSIZE);
			continue;
		}
		else
		{
			for (iLoop = 0; iLoop < MAX_OPEN_TRANS; iLoop++)
			{
				if (sOpenTrans [iLoop].tOffset != -1 && ldint (psVBLogHeader->cPID) == sOpenTrans [iLoop].iPID)
				{
					sOpenTrans [iLoop].tOffset = tOffset + tLength2 - INTSIZE;
					break;
				}
			}
		}
		if (!memcmp (psVBLogHeader->cOperation, VBL_FILEOPEN, 2))
		{
			if (iLocalHandle [iHandle] != -1)
				return (-1);
			iLocalHandle [iHandle] = isopen (pcBuffer + INTSIZE + INTSIZE, ISAUTOLOCK+ISINOUT+ISTRANS);
			if (iLocalHandle [iHandle] == -1)
				return (-1);	// Probably ETOOMANY!
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_FILECLOSE, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (-1);
			isclose (iLocalHandle [iHandle]);
			iLocalHandle [iHandle] = -1;
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_INSERT, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (-1);
			isreclen = ldint (pcBuffer + INTSIZE + QUADSIZE);
			pcRow = pcBuffer + INTSIZE + QUADSIZE + INTSIZE;
			iVBEnter (iLocalHandle [iHandle], TRUE);
			psVBFile [iLocalHandle [iHandle]]->sFlags.iIsDictLocked |= 0x02;
			if (iVBForceDataAllocate (iLocalHandle [iHandle], tRowNumber))
			{
				iserrno = EBADFILE;
				return (-1);
			}
			if (iVBWriteRow (iLocalHandle [iHandle], pcRow, tRowNumber))
				return (-1);
			iVBExit (iLocalHandle [iHandle]);
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_UPDATE, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (-1);
			isreclen = ldint (pcBuffer + INTSIZE + QUADSIZE);
			pcRow = pcBuffer + INTSIZE + QUADSIZE + INTSIZE + INTSIZE + isreclen;
			isreclen = ldint (pcBuffer + INTSIZE + QUADSIZE + INTSIZE);
			// BUG - Should we READ the row first and compare it?
			if (isrewrec (iLocalHandle [iHandle], tRowNumber, pcRow))
				return (-1);
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_DELETE, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (-1);
			// BUG - Should we READ the row first and compare it?
			if (isdelrec (iLocalHandle [iHandle], tRowNumber))
				return (-1);

//If the isdel* was not called within an isbegin, then free the data row NOW
//Otherwise, delay it till the commit / rollback is processed

			iFreeNow = TRUE;
			for (iLoop = 0; iLoop < MAX_OPEN_TRANS; iLoop++)
				if (sOpenTrans [iLoop].tOffset != -1 && ldint (psVBLogHeader->cPID) == sOpenTrans [iLoop].iPID)
				{
					vDelayedDataFree (iLoop, iLocalHandle [iHandle], tRowNumber);
					iFreeNow = FALSE;
					break;	// exit for loop
				}
			if (iFreeNow)
			{
				if (iVBEnter (iLocalHandle [iHandle], TRUE))
					return (-1);
				if (iVBDataFree (iLocalHandle [iHandle], tRowNumber))
					return (-1);
				if (iVBExit (iLocalHandle [iHandle]))
					return (-1);
			}
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_BUILD, 2))
		{
fprintf (stderr, "BUG: VBL_BUILD - isrecover is incomplete!\n");
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_CREINDEX, 2))
		{
fprintf (stderr, "BUG: VBL_CREINDEX - isrecover is incomplete!\n");
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_DELINDEX, 2))
		{
fprintf (stderr, "BUG: VBL_DELINDEX - isrecover is incomplete!\n");
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_CLUSTER, 2))
		{
fprintf (stderr, "BUG: VBL_CLUSTER - isrecover is incomplete!\n");
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_FILEERASE, 2))
		{
fprintf (stderr, "BUG: VBL_FILEERASE - isrecover is incomplete!\n");
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_RENAME, 2))
		{
fprintf (stderr, "BUG: VBL_RENAME - isrecover is incomplete!\n");
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_SETUNIQUE, 2))
		{
fprintf (stderr, "BUG: VBL_SETUNIQUE - isrecover is incomplete!\n");
		}
		else if (!memcmp (psVBLogHeader->cOperation, VBL_UNIQUEID, 2))
		{
fprintf (stderr, "BUG: VBL_UNIQUEID - isrecover is incomplete!\n");
		}
		tOffset += tLength2;
		if (tLength2 == tLength - INTSIZE)
			break;
		tLength = ldint (cVBTransBuffer + tLength - INTSIZE);
	}
	// Finally, we can rollback any transactions that were never 'closed'
	for (iLoop = 0; iLoop < MAX_OPEN_TRANS; iLoop++)
	{
		if (sOpenTrans [iLoop].tOffset != -1)
		{
			vFreeDelayed (iLoop);
			iserrno = iVBRollMeBack (sOpenTrans [iLoop].tOffset, sOpenTrans [iLoop].iPID, TRUE);
			if (iserrno)
				return (-1);	// Bug RestoreState
		}
	}
	// Return the list of open files to what it was before the isrecover
	for (iLoop = 0; iLoop <= VB_MAX_FILES; iLoop++)
		if (iLocalHandle [iLoop] == -1 && !psVBFile [iLoop])
			isclose (iLoop);
	iserrno = 0;
	return (0);
}

// Add the row number to the list to be passed to iVBDataFree when the
// commit / rollback is encountered (Or, in the RARE case, EOF on the logfile)
static	void
vDelayedDataFree (int iIndex, int iHandle, off_t tRowNumber)
{
	struct	VBDELAY
		*psDelay,
		*psDelayLoop;

	psDelay = (struct VBDELAY *) pvVBMalloc (sizeof (struct VBDELAY));
	if (psDelay == VBDELAY_NULL)	// Should NEVER happen but...
	{				// At least we TRIED
		iVBEnter (iHandle, TRUE);
		iVBDataFree (iHandle, tRowNumber);
		iVBExit (iHandle);
		return;
	}
	psDelay->iHandle = iHandle;
	psDelay->tRowNumber = tRowNumber;
	// Insert at head?
	if (sOpenTrans [iIndex].psDelayHead == VBDELAY_NULL || iHandle < sOpenTrans [iIndex].psDelayHead->iHandle || (iHandle == sOpenTrans [iIndex].psDelayHead->iHandle && tRowNumber < sOpenTrans [iIndex].psDelayHead->tRowNumber))
	{
		psDelay->psNext = sOpenTrans [iIndex].psDelayHead;
		sOpenTrans [iIndex].psDelayHead = psDelay;
		return;
	}
	// Insert in middle?
	for (psDelayLoop = sOpenTrans [iIndex].psDelayHead; psDelayLoop->psNext;psDelayLoop = psDelayLoop->psNext)
	{
		if (psDelayLoop->psNext->iHandle < iHandle)
			continue;
		if (psDelayLoop->psNext->iHandle == iHandle && psDelayLoop->psNext->tRowNumber < tRowNumber)
			continue;
		psDelay->psNext = psDelayLoop->psNext;
		psDelayLoop->psNext = psDelay;
		return;
	}
	// Insert at tail!
	psDelay->psNext = VBDELAY_NULL;
	psDelayLoop->psNext = psDelay;
}

static	void
vFreeDelayed (int iIndex)
{
	int	iLastHandle = -1,
		iLastWasClosed = FALSE,
		iLoop;
	struct	VBDELAY
		*psDelay;

	for (psDelay = sOpenTrans [iIndex].psDelayHead; psDelay; psDelay = psDelay->psNext)
	{
		if (iLastHandle != psDelay->iHandle)
		{
			if (iLastHandle != -1)
			{
				iVBExit (iLastHandle);
				if (iLastWasClosed)
					psVBFile [iLastHandle]->iIsOpen = 1;
			}
			if (psVBFile [psDelay->iHandle]->iIsOpen == 1)
			{
				psVBFile [psDelay->iHandle]->iIsOpen = 0;
				iLastWasClosed = 1;
			}
			iVBEnter (psDelay->iHandle, TRUE);
			iLastHandle = psDelay->iHandle;
		}
		iVBDataFree (psDelay->iHandle, psDelay->tRowNumber);
	}
	if (iLastHandle != -1)
	{
		iVBExit (iLastHandle);
		if (iLastWasClosed)
			psVBFile [iLastHandle]->iIsOpen = 1;
	}
	while (sOpenTrans [iIndex].psDelayHead)
	{
		psDelay = sOpenTrans [iIndex].psDelayHead->psNext;
		vVBFree (sOpenTrans [iIndex].psDelayHead, sizeof (struct VBDELAY));
		sOpenTrans [iIndex].psDelayHead = psDelay;
	}
	for (iLoop = 0; iLoop <= iVBMaxUsedHandle; iLoop++)
		if (psVBFile [iLoop] && psVBFile [iLoop]->iIsOpen == 1)
		{
			iIndex = iserrno;
			if (!iVBClose2 (iLoop))
				iserrno = iIndex;
		}
}
