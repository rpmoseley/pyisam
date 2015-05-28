/*
 * Title:	istrans.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	11Dec2003
 * Description:
 *	This is the module that deals with all the transaction processing for
 *	a file in the VBISAM library.
 * Version:
 *	$Id: istrans.c,v 1.8 2004/06/16 10:53:56 trev_vb Exp $
 * Modification History:
 *	$Log: istrans.c,v $
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
 *	Revision 1.2  2003/12/22 04:47:51  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:18  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"
#include	<time.h>

/*
 * Prototypes
 */
int	isbegin (void);
int	iscommit (void);
int	islogclose (void);
int	islogopen (char *);
int	isrollback (void);
int	iVBRollMeBack (off_t, int, int);
int	iVBRollMeForward (off_t, int);
int	iVBTransBuild (char *, int, int, struct keydesc *, int);
int	iVBTransCreateIndex (int, struct keydesc *);
int	iVBTransCluster (void);	// BUG - Unknown args
int	iVBTransDelete (int, off_t, int);
int	iVBTransDeleteIndex (int, struct keydesc *);
int	iVBTransErase (char *);
int	iVBTransClose (int, char *);
int	iVBTransOpen (int, char *);
int	iVBTransInsert (int, off_t, int, char *);
int	iVBTransRename (char *, char *);
int	iVBTransSetUnique (int, off_t);
int	iVBTransUniqueID (int, off_t);
int	iVBTransUpdate (int, off_t, int, int, char *);
static	void	vTransHdr (char *);
static	int	iWriteTrans (int, int);
static	int	iWriteBegin (void);
static	int	iDemoteLocks (void);

/*
 * Name:
 *	int	isbegin (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int	isbegin (void)
{
	if (iVBLogfileHandle == -1)
	{
		iserrno = ELOGOPEN;
		return (-1);
	}
	// If we're already *IN* a transaction, don't start another!
	if (iVBInTrans)
		return (0);
	iVBInTrans = VBBEGIN;		// Just flag that we've BEGUN
	return (0);
}

/*
 * Name:
 *	int	iscommit (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int	iscommit (void)
{
	int	iHoldStatus = iVBInTrans,
		iLoop,
		iResult = 0;
	off_t	tOffset;

	iserrno = 0;
	if (iVBLogfileHandle == -1)
		return (0);
	if (!iVBInTrans)
	{
		iserrno = ENOBEGIN;
		return (-1);
	}
	iVBInTrans = VBCOMMIT;
	if (iHoldStatus != VBBEGIN)
	{
		tOffset = tVBLseek (iVBLogfileHandle, 0, SEEK_END);
		iserrno = iVBRollMeForward (tOffset, tVBPID);
	}
	for (iLoop = 0; iLoop <= iVBMaxUsedHandle; iLoop++)
		if (psVBFile [iLoop] && psVBFile [iLoop]->iIsOpen == 1)
		{
			iResult = iserrno;
			if (!iVBClose2 (iLoop))
				iserrno = iResult;
		}
	// Don't write out a 'null' transaction!
	if (iHoldStatus != VBBEGIN)
	{
		vTransHdr (VBL_COMMIT);
		iResult = iWriteTrans (0, TRUE);
		if (iResult)
			iserrno = iResult;
		iDemoteLocks ();
	}
	iVBInTrans = VBNOTRANS;
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	islogclose (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int	islogclose (void)
{
	int	iResult = 0;

	if (iVBInTrans == VBNEEDFLUSH)
		if (isrollback ())
			iResult = iserrno;
	iVBInTrans = VBNOTRANS;
	if (iVBLogfileHandle != -1)
		if (iVBClose (iVBLogfileHandle))
			iResult = errno;
	iVBLogfileHandle = -1;
	return (iResult);
}

/*
 * Name:
 *	int	islogopen (char *pcFilename);
 * Arguments:
 *	char	*pcFilename
 *		The name of the log file
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int	islogopen (char *pcFilename)
{
	if (iVBLogfileHandle != -1)
		islogclose ();		// Ignore the return value!
	iVBLogfileHandle = iVBOpen (pcFilename, O_RDWR, 0);
	if (iVBLogfileHandle == -1)
	{
		iserrno = ELOGOPEN;
		return (-1);
	}
	return (0);
}

/*
 * Name:
 *	int	isrollback (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int	isrollback (void)
{
	int	iLoop,
		iResult = 0;
	off_t	tOffset;

	if (iVBLogfileHandle == -1)
		return (0);
	if (!iVBInTrans)
	{
		iserrno = ENOBEGIN;
		return (-1);
	}
	// Don't write out a 'null' transaction!
	for (iLoop = 0; iLoop <= iVBMaxUsedHandle; iLoop++)
		if (psVBFile [iLoop] && psVBFile [iLoop]->iIsOpen == 1)
		{
			iResult = iserrno;
			if (!iVBClose2 (iLoop))
				iserrno = iResult;
			iResult = 0;
		}
	if (iVBInTrans == VBBEGIN)
		return (0);
	iVBInTrans = VBROLLBACK;
	tOffset = tVBLseek (iVBLogfileHandle, 0, SEEK_END);
	// Write out the log entry
	vTransHdr (VBL_ROLLBACK);
	iserrno = iWriteTrans (0, TRUE);
	if (!iserrno)
		iserrno = iVBRollMeBack (tOffset, tVBPID, FALSE);
	iDemoteLocks ();
	iVBInTrans = VBNOTRANS;
	if (iserrno)
		return (-1);
	for (iLoop = 0; iLoop <= iVBMaxUsedHandle; iLoop++)
	{
		if (psVBFile [iLoop] && psVBFile [iLoop]->iIsOpen == 1)
			if (iVBClose2 (iLoop))
				iResult = iserrno;
		if (psVBFile [iLoop] && psVBFile [iLoop]->sFlags.iIsDictLocked & 0x04)
			iResult |= iVBExit (iLoop);
	}
	return (iResult ? -1 : 0);
}

/*
 * Name:
 *	int	iVBTransBuild (char *pcFilename, int iMinRowLen, int iMaxRowLen, struct keydesc *psKeydesc);
 * Arguments:
 *	char	*pcFilename
 *		The name of the file being created
 *	int	iMinRowLen
 *		The minimum row length
 *	int	iMaxRowLen
 *		The maximum row length
 *	struct	keydesc	*psKeydesc
 *		The primary index being created
 *	int	iMode
 *		The open mode (Only used to determine is ISNOLOG is set)
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransBuild (char *pcFilename, int iMinRowLen, int iMaxRowLen, struct keydesc *psKeydesc, int iMode)
{
	char	*pcBuffer;
	int	iLength = 0,
		iLength2,
		iLoop;

	if (iVBLogfileHandle == -1 || iMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	vTransHdr (VBL_BUILD);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (0x0806, pcBuffer);
	stint (iMinRowLen, pcBuffer + INTSIZE);
	stint (iMaxRowLen, pcBuffer + (2 * INTSIZE));
	stint (psKeydesc->iFlags, pcBuffer + (3 * INTSIZE));
	stint (psKeydesc->iNParts, pcBuffer + (4 * INTSIZE));
	pcBuffer += (INTSIZE * 6);
	for (iLoop = 0; iLoop < psKeydesc->k_nparts; iLoop++)
	{
		stint (psKeydesc->sPart [iLoop].iStart, pcBuffer + (iLoop * 3 * INTSIZE));
		stint (psKeydesc->sPart [iLoop].iLength, pcBuffer + INTSIZE + (iLoop * 3 * INTSIZE));
		stint (psKeydesc->sPart [iLoop].iType, pcBuffer + (INTSIZE * 2) + (iLoop * 3 * INTSIZE));
		iLength += psKeydesc->sPart [iLoop].iLength;
	}
	stint (iLength, pcBuffer - INTSIZE);
	iLength = (INTSIZE * 6) + (INTSIZE * 3 * (psKeydesc->iNParts));
	iLength2 = strlen (pcFilename) + 1;
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR) + iLength;
	memcpy (pcBuffer, pcFilename, iLength2);
	iserrno = iWriteTrans (iLength + iLength2, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransCreateIndex (int iHandle, struct keydesc *psKeydesc);
 * Arguments:
 *	int	iHandle
 *		An (exclusively) open VBISAM table
 *	struct	keydesc	*psKeydesc
 *		The index being created
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransCreateIndex (int iHandle, struct keydesc *psKeydesc)
{
	char	*pcBuffer;
	int	iLength = 0,
		iLoop;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	vTransHdr (VBL_CREINDEX);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stint (psKeydesc->iFlags, pcBuffer + INTSIZE);
	stint (psKeydesc->iNParts, pcBuffer + (2 * INTSIZE));
	pcBuffer += (INTSIZE * 4);
	for (iLoop = 0; iLoop < psKeydesc->k_nparts; iLoop++)
	{
		stint (psKeydesc->sPart [iLoop].iStart, pcBuffer + (iLoop * 3 * INTSIZE));
		stint (psKeydesc->sPart [iLoop].iLength, pcBuffer + INTSIZE + (iLoop * 3 * INTSIZE));
		stint (psKeydesc->sPart [iLoop].iType, pcBuffer + (INTSIZE * 2) + (iLoop * 3 * INTSIZE));
		iLength += psKeydesc->sPart [iLoop].iLength;
	}
	stint (iLength, pcBuffer - INTSIZE);
	iLength = (INTSIZE * 4) + (INTSIZE * 3 * (psKeydesc->iNParts));
	iserrno = iWriteTrans (iLength, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransCluster (void);	// BUG - Unknown args
 * Arguments:
 *	BUG
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransCluster (void)		// BUG - Unknown args
{
	// BUG - Write iVBTransCluster
	return (0);
}

/*
 * Name:
 *	int	iVBTransDelete (int iHandle, off_t tRowNumber, int iRowLength);
 * Arguments:
 *	int	iHandle
 *		The (open) VBISAM handle
 *	off_t	tRowNumber
 *		The row number being deleted
 *	int	iRowLength
 *		The length of the row being deleted
 * Prerequisites:
 *	Assumed deleted row is in psVBFile [iHandle]->ppcRowBuffer
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransDelete (int iHandle, off_t tRowNumber, int iRowLength)
{
	char	*pcBuffer;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	if (psVBFile [iHandle]->sFlags.iTransYet == 0)
		iVBTransOpen (iHandle, psVBFile [iHandle]->cFilename);
	vTransHdr (VBL_DELETE);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stquad (tRowNumber, pcBuffer + INTSIZE);
	stint (iRowLength, pcBuffer + INTSIZE + QUADSIZE);
	memcpy (pcBuffer + INTSIZE + QUADSIZE + INTSIZE, pcRowBuffer, iRowLength);
	iRowLength += (INTSIZE * 2) + QUADSIZE;
	iserrno = iWriteTrans (iRowLength, TRUE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransDeleteIndex (int iHandle, struct keydesc *psKeydesc);
 * Arguments:
 *	int	iHandle
 *		An (exclusively) open VBISAM table
 *	struct	keydesc	*psKeydesc
 *		The index being created
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransDeleteIndex (int iHandle, struct keydesc *psKeydesc)
{
	char	*pcBuffer;
	int	iLength = 0,
		iLoop;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	vTransHdr (VBL_DELINDEX);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stint (psKeydesc->iFlags, pcBuffer + INTSIZE);
	stint (psKeydesc->iNParts, pcBuffer + (2 * INTSIZE));
	pcBuffer += (INTSIZE * 4);
	for (iLoop = 0; iLoop < psKeydesc->k_nparts; iLoop++)
	{
		stint (psKeydesc->sPart [iLoop].iStart, pcBuffer + (iLoop * 3 * INTSIZE));
		stint (psKeydesc->sPart [iLoop].iLength, pcBuffer + INTSIZE + (iLoop * 3 * INTSIZE));
		stint (psKeydesc->sPart [iLoop].iType, pcBuffer + (INTSIZE * 2) + (iLoop * 3 * INTSIZE));
		iLength += psKeydesc->sPart [iLoop].iLength;
	}
	stint (iLength, pcBuffer - INTSIZE);
	iLength = (INTSIZE * 4) + (INTSIZE * 3 * (psKeydesc->iNParts));
	iserrno = iWriteTrans (iLength, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransErase (char *pcFilename);
 * Arguments:
 *	char	*pcFilename
 *		The name of the VBISAM table to erase
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransErase (char *pcFilename)
{
	char	*pcBuffer;
	int	iLength;

	if (iVBLogfileHandle == -1)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	vTransHdr (VBL_FILEERASE);
	iLength = strlen (pcFilename) + 1;
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	memcpy (pcBuffer, pcFilename, iLength);
	iserrno = iWriteTrans (iLength, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransClose (int iHandle, char *pcFilename);
 * Arguments:
 *	int	iHandle
 *		The handle of the VBISAM table being closed
 *	char	*pcFilename
 *		The name of the VBISAM table to erase
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransClose (int iHandle, char *pcFilename)
{
	char	*pcBuffer;
	int	iLength;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBROLLBACK)
		return (0);
	if (psVBFile [iHandle]->sFlags.iTransYet == 0)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	vTransHdr (VBL_FILECLOSE);
	iLength = strlen (pcFilename) + 1;
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stint (0, pcBuffer + INTSIZE);
	memcpy (pcBuffer + INTSIZE + INTSIZE, pcFilename, iLength);
	iLength += (INTSIZE * 2);
	iserrno = iWriteTrans (iLength, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransOpen (int iHandle, char *pcFilename);
 * Arguments:
 *	int	iHandle
 *		The handle of the VBISAM table being opened
 *	char	*pcFilename
 *		The name of the VBISAM table to erase
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransOpen (int iHandle, char *pcFilename)
{
	char	*pcBuffer;
	int	iLength;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	psVBFile [iHandle]->sFlags.iTransYet = 2;
	vTransHdr (VBL_FILEOPEN);
	iLength = strlen (pcFilename) + 1;
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stint (0, pcBuffer + INTSIZE);
	memcpy (pcBuffer + INTSIZE + INTSIZE, pcFilename, iLength);
	iLength += (INTSIZE * 2);
	iserrno = iWriteTrans (iLength, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransInsert (int iHandle, off_t tRowNumber, int iRowLength, char *pcRow);
 * Arguments:
 *	int	iHandle
 *		The (open) VBISAM handle
 *	off_t	tRowNumber
 *		The row number being inserted
 *	int	iRowLength
 *		The length of the inserted row
 *	char	*pcRow
 *		The *NEW* row
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransInsert (int iHandle, off_t tRowNumber, int iRowLength, char *pcRow)
{
	char	*pcBuffer;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	if (psVBFile [iHandle]->sFlags.iTransYet == 0)
		iVBTransOpen (iHandle, psVBFile [iHandle]->cFilename);
	vTransHdr (VBL_INSERT);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stquad (tRowNumber, pcBuffer + INTSIZE);
	stint (iRowLength, pcBuffer + INTSIZE + QUADSIZE);
	memcpy (pcBuffer + INTSIZE + QUADSIZE + INTSIZE, pcRow, iRowLength);
	iRowLength += (INTSIZE * 2) + QUADSIZE;
	iserrno = iWriteTrans (iRowLength, TRUE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransRename (char *pcOldname, char *pcNewname);
 * Arguments:
 *	char	*pcOldname
 *		The original filename
 *	char	*pcNewname
 *		The new filename
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransRename (char *pcOldname, char *pcNewname)
{
	char	*pcBuffer;
	int	iLength,
		iLength1,
		iLength2;

	if (iVBLogfileHandle == -1)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	vTransHdr (VBL_RENAME);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	iLength1 = strlen (pcOldname) + 1;
	iLength2 = strlen (pcNewname) + 1;
	stint (iLength1, pcBuffer);
	stint (iLength2, pcBuffer + INTSIZE);
	memcpy (pcBuffer + (INTSIZE * 2), pcOldname, iLength1);
	memcpy (pcBuffer + (INTSIZE * 2) + iLength1, pcNewname, iLength2);
	iLength = (INTSIZE * 2) + iLength1 + iLength2;
	iserrno = iWriteTrans (iLength, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransSetUnique (int iHandle, off_t tUniqueID);
 * Arguments:
 *	int	iHandle
 *		The handle of an open VBISAM file
 *	off_t	tUniqueID
 *		The new unique ID starting number
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransSetUnique (int iHandle, off_t tUniqueID)
{
	char	*pcBuffer;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	if (psVBFile [iHandle]->sFlags.iTransYet == 0)
		iVBTransOpen (iHandle, psVBFile [iHandle]->cFilename);
	vTransHdr (VBL_SETUNIQUE);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stquad (tUniqueID, pcBuffer + INTSIZE);
	iserrno = iWriteTrans (INTSIZE + QUADSIZE, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransUniqueID (int iHandle, off_t tUniqueID);
 * Arguments:
 *	int	iHandle
 *		The handle of an open VBISAM file
 *	off_t	tUniqueID
 *		The extracted unique ID
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransUniqueID (int iHandle, off_t tUniqueID)
{
	char	*pcBuffer;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	if (psVBFile [iHandle]->sFlags.iTransYet == 0)
		iVBTransOpen (iHandle, psVBFile [iHandle]->cFilename);
	vTransHdr (VBL_UNIQUEID);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stquad (tUniqueID, pcBuffer + INTSIZE);
	iserrno = iWriteTrans (INTSIZE + QUADSIZE, FALSE);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBTransUpdate (int iHandle, off_t tRowNumber, int iOldRowLen, int iNewRowLen, char *pcRow);
 * Arguments:
 *	int	iHandle
 *		The (open) VBISAM handle
 *	off_t	tRowNumber
 *		The row number being rewritten
 *	int	iOldRowLen
 *		The length of the original row
 *	int	iNewRowLen
 *		The length of the replacement row
 *	char	*pcRow
 *		The *UPDATED* row
 * Prerequisites:
 *	Assumes non-modified row is in psVBFile [iHandle]->ppcRowBuffer
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int
iVBTransUpdate (int iHandle, off_t tRowNumber, int iOldRowLen, int iNewRowLen, char *pcRow)
{
	char	*pcBuffer;
	int	iLength;

	if (iVBLogfileHandle == -1 || psVBFile [iHandle]->iOpenMode & ISNOLOG)
		return (0);
	// Don't log transactions if we're in rollback / recover mode
	if (iVBInTrans > VBNEEDFLUSH)
		return (0);
	if (iVBInTrans == VBBEGIN)
		if (iWriteBegin ())
			return (-1);
	if (psVBFile [iHandle]->sFlags.iTransYet == 0)
		iVBTransOpen (iHandle, psVBFile [iHandle]->cFilename);
	vTransHdr (VBL_UPDATE);
	pcBuffer = cVBTransBuffer + sizeof (struct SLOGHDR);
	stint (iHandle, pcBuffer);
	stquad (tRowNumber, pcBuffer + INTSIZE);
	stint (iOldRowLen, pcBuffer + INTSIZE + QUADSIZE);
	stint (iNewRowLen, pcBuffer + INTSIZE + QUADSIZE + INTSIZE);
	memcpy (pcBuffer + INTSIZE + QUADSIZE + INTSIZE + INTSIZE, pcRowBuffer, iOldRowLen);
	memcpy (pcBuffer + INTSIZE + QUADSIZE + INTSIZE + INTSIZE + iOldRowLen, pcRow, iNewRowLen);
	iLength = INTSIZE + QUADSIZE + (INTSIZE * 2) + iOldRowLen + iNewRowLen;
	iserrno = iWriteTrans (iLength, TRUE);
	if (iserrno)
		return (-1);
	return (0);
}


/*
 * Name:
 *	static	void	vTransHdr (char *pcTransType);
 * Arguments:
 *	char	*pcTransType
 *		The two character transaction type.  One of:
 *			VBL_BUILD
 *			VBL_BEGIN
 *			VBL_CREINDEX
 *			VBL_CLUSTER
 *			VBL_COMMIT
 *			VBL_DELETE
 *			VBL_DELINDEX
 *			VBL_FILEERASE
 *			VBL_FILECLOSE
 *			VBL_FILEOPEN
 *			VBL_INSERT
 *			VBL_RENAME
 *			VBL_ROLLBACK
 *			VBL_SETUNIQUE
 *			VBL_UNIQUEID
 *			VBL_UPDATE
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	None known
 */
static	void
vTransHdr (char *pcTransType)
{
	psVBLogHeader = (struct SLOGHDR *) cVBTransBuffer;
	memcpy (psVBLogHeader->cOperation, pcTransType, 2);
	stint (tVBPID, psVBLogHeader->cPID);	// Assumes pid_t is short
	stint (tVBUID, psVBLogHeader->cUID);	// Assumes uid_t is short
	stlong (time ((time_t *) 0), psVBLogHeader->cTime);	// Assumes time_t is long
	stint (0, psVBLogHeader->cRFU1);		// BUG - WTF is this?
}

/*
 * Name:
 *	static	int	iWriteTrans (int iTransLength, int iRollBack);
 * Arguments:
 *	int	iTransLength
 *		The length of the transaction to write (exluding hdr/ftr)
 *	int	iRollback
 *		FALSE
 *			This transaction CANNOT be rolled back!
 *		TRUE
 *			Take a wild guess!
 * Prerequisites:
 *	NONE
 * Returns:
 *	0
 *		Success
 *	ELOGWRIT
 *		Ooops, some problem occurred!
 * Problems:
 *	FUTURE
 *	When we begin to support rows > 32k, the buffer is too small.
 *	In that case, we'll need to perform SEVERAL writes and this means we
 *	will need to implement a crude locking scheme to guarantee atomicity.
 */
static	int
iWriteTrans (int iTransLength, int iRollback)
{
static	int	iPrevLen = 0;
static	off_t	tOffset = 0;
	int	iResult;

	iTransLength += sizeof (struct SLOGHDR) + INTSIZE;
	stint (iTransLength, cVBTransBuffer);
	stint (iTransLength, cVBTransBuffer + iTransLength - INTSIZE);
	if (iRollback)
	{
		psVBLogHeader = (struct SLOGHDR *) cVBTransBuffer;
		stint (tOffset, psVBLogHeader->cLastPosn);
		stint (iPrevLen, psVBLogHeader->cLastLength);
		tOffset = tVBLseek (iVBLogfileHandle, 0, SEEK_END);
		if (tOffset == -1)
			return (ELOGWRIT);
		iPrevLen = iTransLength;
	}
	else
	{
		psVBLogHeader = (struct SLOGHDR *) cVBTransBuffer;
		stint (0, psVBLogHeader->cLastPosn);
		stint (0, psVBLogHeader->cLastLength);
		if (tVBLseek (iVBLogfileHandle, 0, SEEK_END) == -1)
			return (ELOGWRIT);
	}
	iResult = iVBLock (iVBLogfileHandle, 0, 0, VBWRLCKW);
	if (iResult)
		return (ELOGWRIT);
	if (tVBWrite (iVBLogfileHandle, (void *) cVBTransBuffer, (size_t) iTransLength) != (ssize_t) iTransLength)
		return (ELOGWRIT);
	iResult = iVBLock (iVBLogfileHandle, 0, 0, VBUNLOCK);
	if (iResult)
		return (ELOGWRIT);
	if (iVBInTrans == VBBEGIN)
		iVBInTrans = VBNEEDFLUSH;
	return (0);
}

/*
 * Name:
 *	static	int	iWriteBegin (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	0
 *		Success
 *	ELOGWRIT
 *		Ooops, some problem occurred!
 * Problems:
 *	None known
 */
static	int
iWriteBegin (void)
{
	vTransHdr (VBL_BEGIN);
	return (iWriteTrans (0, TRUE));
}

/*
 * Name:
 *	static	int	iDemoteLocks (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	0
 *		Success
 *	EBADFILE
 *		Ooops, some problem occurred!
 * Problems:
 *	See comments
 * Comments:
 *	When a transaction is completed, either with an iscommit() or an
 *	isrollback (), *ALL* held locks are released.  I'm not quite sure how
 *	valid this really is...  Perhaps only the 'transactional' locks should
 *	be released?  Or perhaps they should be retained, but demoted to a
 *	non-transactional status?  Oh well... C'est la vie!
 * Caveat:
 *	If the file is exclusively opened (ISEXCLLOCK) or has been locked with
 *	an islock () call, these locks remain in place!
 */
static	int
iDemoteLocks (void)
{
	int	iHandle,
		iResult = 0;

	for (iHandle = 0; iHandle <= iVBMaxUsedHandle; iHandle++)
	{
		if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen == 2)
			continue;
		if (psVBFile [iHandle]->iOpenMode & ISEXCLLOCK)
			continue;
		if (psVBFile [iHandle]->sFlags.iIsDataLocked)
			continue;
		// Rather a carte-blanche method huh?
		if (iVBDataLock (iHandle, VBUNLOCK, 0))	// BUG Only ours?
			iResult = -1;
	}
	if (iResult)
		iserrno = EBADFILE;
	return (iResult);
}

/*
 * Name:
 *	int	iVBRollMeBack (off_t tOffset, int iPID, int iInRecover);
 * Arguments:
 *	off_t	tOffset
 *		The position within the logfile to begin the backwards scan
 *		It is assumed that this is at a true transaction 'boundary'
 *	int	iPID
 *		The process ID to 'look' for
 *	int	iInRecover
 *		FALSE	Nope
 *		TRUE	Yep, we need to force-allocate the data rows
 * Prerequisites:
 *	NONE
 * Returns:
 *	0
 *		Success
 *	OTHER
 *		Ooops, some problem occurred!
 * Problems:
 *	None known
 */
int
iVBRollMeBack (off_t tOffset, int iPID, int iInRecover)
{
	char	*pcBuffer,
		*pcRow;
	int	iErrorEncountered = FALSE,
		iFoundBegin = FALSE,
		iHandle,
		iLoop,
		iLocalHandle [VB_MAX_FILES + 1],
		iSavedHandle [VB_MAX_FILES + 1];
	off_t	tLength,
		tRowNumber;

	for (iLoop = 0; iLoop <= VB_MAX_FILES; iLoop++)
	{
		if (psVBFile [iLoop])
			iLocalHandle [iLoop] = iLoop;
		else
			iLocalHandle [iLoop] = -1;
		iSavedHandle [iLoop] = iLocalHandle [iLoop];
	}
	psVBLogHeader = (struct SLOGHDR *) (cVBTransBuffer + INTSIZE);
	pcBuffer = cVBTransBuffer + INTSIZE + sizeof (struct SLOGHDR);
	// Begin by reading the footer of the previous transaction
	tOffset -= INTSIZE;
	if (tVBLseek (iVBLogfileHandle, tOffset, SEEK_SET) != tOffset)
		return (EBADFILE);
	if (tVBRead (iVBLogfileHandle, cVBTransBuffer, INTSIZE) != INTSIZE)
		return (EBADFILE);
	// Now, recurse backwards
	while (!iFoundBegin)
	{
		tLength = ldint (cVBTransBuffer);
		if (!tLength)
			return (EBADFILE);
		tOffset -= tLength;
		// Special case: Handle where the FIRST log entry is our BW
		if (tOffset == -(INTSIZE))
		{
			if (tVBLseek (iVBLogfileHandle, 0, SEEK_SET) != 0)
				return (EBADFILE);
			if (tVBRead (iVBLogfileHandle, cVBTransBuffer + INTSIZE, tLength - INTSIZE) != tLength - INTSIZE)
				return (EBADFILE);
			if (!memcmp (psVBLogHeader->cOperation, VBL_BEGIN, 2))
				break;
			return (EBADFILE);
		}
		else
		{
			if (tOffset < INTSIZE)
				return (EBADFILE);
			if (tVBLseek (iVBLogfileHandle, tOffset, SEEK_SET) != tOffset)
				return (EBADFILE);
			if (tVBRead (iVBLogfileHandle, cVBTransBuffer, tLength) != tLength)
				return (EBADFILE);
		}
		// Is it OURS?
		if (ldint (psVBLogHeader->cPID) != iPID)
			continue;
		if (!memcmp (psVBLogHeader->cOperation, VBL_BEGIN, 2))
			break;
		iHandle = ldint (pcBuffer);
		tRowNumber = ldquad (pcBuffer + INTSIZE);
		if (!memcmp (psVBLogHeader->cOperation, VBL_FILECLOSE, 2))
		{
			if (iLocalHandle [iHandle] != -1 && psVBFile [iHandle]->iIsOpen == 0)
				return (EBADFILE);
			iLocalHandle [iHandle] = isopen (pcBuffer + INTSIZE + INTSIZE, ISMANULOCK + ISINOUT);
			if (iLocalHandle [iHandle] == -1)
				return (ETOOMANY);
		}
		if (!memcmp (psVBLogHeader->cOperation, VBL_INSERT, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (EBADFILE);
			// BUG? - Should we READ the row first and compare it?
			if (isdelrec (iLocalHandle [iHandle], tRowNumber))
				return (iserrno);
		}
		if (!memcmp (psVBLogHeader->cOperation, VBL_UPDATE, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (EBADFILE);
			isreclen = ldint (pcBuffer + INTSIZE + QUADSIZE);
			pcRow = pcBuffer + INTSIZE + QUADSIZE + INTSIZE + INTSIZE;
			// BUG? - Should we READ the row first and compare it?
			if (isrewrec (iLocalHandle [iHandle], tRowNumber, pcRow))
				return (iserrno);
		}
		if (!memcmp (psVBLogHeader->cOperation, VBL_DELETE, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (EBADFILE);
			isreclen = ldint (pcBuffer + INTSIZE + QUADSIZE);
			pcRow = pcBuffer + INTSIZE + QUADSIZE + INTSIZE;
			iVBEnter (iLocalHandle [iHandle], TRUE);
			psVBFile [iLocalHandle [iHandle]]->sFlags.iIsDictLocked |= 0x02;
			if (iInRecover && iVBForceDataAllocate (iLocalHandle [iHandle], tRowNumber))
				iErrorEncountered = EBADFILE;
			else
			{
				if (iVBWriteRow (iLocalHandle [iHandle], pcRow, tRowNumber))
				{
					iErrorEncountered = EDUPL;
					iVBDataFree (iLocalHandle [iHandle], tRowNumber);
				}
			}
			iVBExit (iLocalHandle [iHandle]);
		}
		if (!memcmp (psVBLogHeader->cOperation, VBL_FILEOPEN, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (EBADFILE);
			isclose (iLocalHandle [iHandle]);
			iLocalHandle [iHandle] = -1;
		}
	}
	for (iLoop = 0; iLoop <= VB_MAX_FILES; iLoop++)
		if (iSavedHandle [iLoop] == -1 && psVBFile [iSavedHandle [iLoop]])
			isclose (iSavedHandle [iLoop]);
	return (iErrorEncountered);
}

/*
 * Name:
 *	int	iVBRollMeForward (off_t tOffset, int iPID);
 * Arguments:
 *	off_t	tOffset
 *		The position within the logfile to begin the backwards scan
 *		It is assumed that this is at a true transaction 'boundary'
 *	int	iPID
 *		The process ID to 'look' for
 * Prerequisites:
 *	NONE
 * Returns:
 *	0
 *		Success
 *	OTHER
 *		Ooops, some problem occurred!
 * Problems:
 *	None known
 */
int
iVBRollMeForward (off_t tOffset, int iPID)
{
	char	*pcBuffer;
	int	iFoundBegin = FALSE,
		iHandle,
		iLoop,
		iLocalHandle [VB_MAX_FILES + 1],
		iSavedHandle [VB_MAX_FILES + 1];
	off_t	tLength,
		tRowNumber;

	for (iLoop = 0; iLoop <= VB_MAX_FILES; iLoop++)
	{
		if (psVBFile [iLoop])
			iLocalHandle [iLoop] = iLoop;
		else
			iLocalHandle [iLoop] = -1;
		iSavedHandle [iLoop] = iLocalHandle [iLoop];
	}
	psVBLogHeader = (struct SLOGHDR *) (cVBTransBuffer + INTSIZE);
	pcBuffer = cVBTransBuffer + INTSIZE + sizeof (struct SLOGHDR);
	// Begin by reading the footer of the previous transaction
	tOffset -= INTSIZE;
	if (tVBLseek (iVBLogfileHandle, tOffset, SEEK_SET) != tOffset)
		return (EBADFILE);
	if (tVBRead (iVBLogfileHandle, cVBTransBuffer, INTSIZE) != INTSIZE)
		return (EBADFILE);
	// Now, recurse backwards
	while (!iFoundBegin)
	{
		tLength = ldint (cVBTransBuffer);
		if (!tLength)
			return (EBADFILE);
		tOffset -= tLength;
		// Special case: Handle where the FIRST log entry is our BW
		if (tOffset == -(INTSIZE))
		{
			if (tVBLseek (iVBLogfileHandle, 0, SEEK_SET) != 0)
				return (EBADFILE);
			if (tVBRead (iVBLogfileHandle, cVBTransBuffer + INTSIZE, tLength - INTSIZE) != tLength - INTSIZE)
				return (EBADFILE);
			if (!memcmp (psVBLogHeader->cOperation, VBL_BEGIN, 2))
				break;
			return (EBADFILE);
		}
		else
		{
			if (tOffset < INTSIZE)
				return (EBADFILE);
			if (tVBLseek (iVBLogfileHandle, tOffset, SEEK_SET) != tOffset)
				return (EBADFILE);
			if (tVBRead (iVBLogfileHandle, cVBTransBuffer, tLength) != tLength)
				return (EBADFILE);
		}
		// Is it OURS?
		if (ldint (psVBLogHeader->cPID) != tVBPID)
			continue;
		if (!memcmp (psVBLogHeader->cOperation, VBL_BEGIN, 2))
			break;
		iHandle = ldint (pcBuffer);
		tRowNumber = ldquad (pcBuffer + INTSIZE);
		if (!memcmp (psVBLogHeader->cOperation, VBL_FILECLOSE, 2))
		{
			if (iLocalHandle [iHandle] != -1)
				return (EBADFILE);
			iLocalHandle [iHandle] = isopen (pcBuffer + INTSIZE + INTSIZE, ISMANULOCK + ISINOUT);
			if (iLocalHandle [iHandle] == -1)
				return (ETOOMANY);
		}
		if (!memcmp (psVBLogHeader->cOperation, VBL_DELETE, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (EBADFILE);
			iVBEnter (iLocalHandle [iHandle], TRUE);
			psVBFile [iLocalHandle [iHandle]]->sFlags.iIsDictLocked |= 0x04;
			if (tRowNumber == ldquad (psVBFile [iLocalHandle [iHandle]]->sDictNode.cDataCount))
			{
				psVBFile [iLocalHandle [iHandle]]->sFlags.iIsDictLocked |= 0x02;
				stquad (tRowNumber - 1, psVBFile [iLocalHandle [iHandle]]->sDictNode.cDataCount);
			}
			else
				if (iVBDataFree (iLocalHandle [iHandle], tRowNumber))
					return (EBADFILE);
			iVBExit (iLocalHandle [iHandle]);
		}
		if (!memcmp (psVBLogHeader->cOperation, VBL_FILEOPEN, 2))
		{
			if (iLocalHandle [iHandle] == -1)
				return (EBADFILE);
			isclose (iLocalHandle [iHandle]);
		}
	}
	for (iLoop = 0; iLoop <= VB_MAX_FILES; iLoop++)
		if (iSavedHandle [iLoop] == -1 && psVBFile [iSavedHandle [iLoop]])
			isclose (iSavedHandle [iLoop]);
	return (0);
}
