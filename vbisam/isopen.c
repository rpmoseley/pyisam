/*
 * Title:	isopen.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	24Nov2003
 * Description:
 *	This module deals with the opening and closing of VBISAM files
 * Version:
 *	$Id: isopen.c,v 1.11 2004/06/16 10:53:55 trev_vb Exp $
 * Modification History:
 *	$Log: isopen.c,v $
 *	Revision 1.11  2004/06/16 10:53:55  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
 *	
 *	Revision 1.10  2004/06/13 07:52:17  trev_vb
 *	TvB 13June2004
 *	Implemented sharing of open files.
 *	Changed the locking strategy slightly to allow table-level locking granularity
 *	(i.e. A process opening the same table more than once can now lock itself!)
 *	
 *	Revision 1.9  2004/06/13 06:32:33  trev_vb
 *	TvB 12June2004 See CHANGELOG 1.03 (Too lazy to enumerate)
 *	
 *	Revision 1.8  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.7  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.6  2004/01/10 16:21:27  trev_vb
 *	JvN 10Jan2004 Johann the 'super-sleuth detective' found an errant semi-colon
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
 *	Revision 1.2  2003/12/22 04:47:11  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:21  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"

static	int	iInitialized = FALSE;

/*
 * Prototypes
 */
int	iscleanup (void);
int	isclose (int);
int	iVBClose2 (int);
int	iVBClose3 (int);
int	isindexinfo (int, struct keydesc *, int);
int	isopen (char *, int);
static	off_t	tCountRows (int);

/*
 * Name:
 *	int	iscleanup (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
iscleanup (void)
{
	int	iLoop,
		iResult,
		iResult2 = 0;

	for (iLoop = 0; iLoop <= iVBMaxUsedHandle; iLoop++)
		if (psVBFile [iLoop])
		{
			if (psVBFile [iLoop]->iIsOpen == 0)
			{
				iResult = isclose (iLoop);
				if (iResult)
					iResult2 = iserrno;
			}
			if (psVBFile [iLoop]->iIsOpen == 1)
			{
				iResult = iVBClose2 (iLoop);
				if (iResult)
					iResult2 = iserrno;
			}
			iVBClose3 (iLoop);
		}
	if (iVBLogfileHandle != -1)
	{
		iResult = islogclose ();
		if (iResult)
			iResult2 = iserrno;
	}
	return (iResult2);
}

/*
 * Name:
 *	int	isclose (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The currently open VBISAM handle
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
isclose (int iHandle)
{
	if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen)
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	if (psVBFile [iHandle]->iOpenMode & ISEXCLLOCK)
		iVBForceExit (iHandle);	// BUG retval
	iVBBlockFlush (iHandle);	// BUG retval
	vVBBlockInvalidate (iHandle);
	psVBFile [iHandle]->sFlags.iIndexChanged = 0;
	psVBFile [iHandle]->iIsOpen = 1;
	if (!(iVBInTrans == VBBEGIN || iVBInTrans == VBNEEDFLUSH || iVBInTrans == VBRECOVER))
		if (iVBClose2 (iHandle))
			return (-1);
	return (0);
}

/*
 * Name:
 *	int	iVBClose2 (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The currently open VBISAM handle
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 * Comments:
 *	The isclose () function does not *COMPLETELY* close a table *IF* the
 *	call to isclose () occurred during a transaction.  This is to make sure
 *	that rowlocks held during a transaction are retained.  This function is
 *	the 'middle half' that performs the (possibly delayed) physical close.
 *	The 'lower half' (iVBClose3) frees up the cached stuff.
 */
int
iVBClose2 (int iHandle)
{
	int	iLoop;
	struct	VBLOCK
		*psRowLock;

	psVBFile [iHandle]->iIsOpen = 0;	// It's a LIE, but so what!
	isrelease (iHandle);
	iserrno = iVBTransClose (iHandle, psVBFile [iHandle]->cFilename);
	if (iVBClose (psVBFile [iHandle]->iDataHandle))
		iserrno = errno;
	psVBFile [iHandle]->iDataHandle = -1;
	if (iVBClose (psVBFile [iHandle]->iIndexHandle))
		iserrno = errno;
	while (sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead)
	{
		psRowLock = sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead->psNext;
		vVBLockFree (sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead);
		sVBFile [psVBFile [iHandle]->iIndexHandle].psLockHead = psRowLock;
	}
	sVBFile [psVBFile [iHandle]->iIndexHandle].psLockTail = VBLOCK_NULL;
	psVBFile [iHandle]->iIndexHandle = -1;
	psVBFile [iHandle]->tRowNumber = -1;
	psVBFile [iHandle]->tDupNumber = -1;
	psVBFile [iHandle]->iIsOpen = 2;	// Only buffers remain!
	psVBFile [iHandle]->sFlags.iTransYet = 0;
	for (iLoop = 0; iLoop < MAXSUBS; iLoop++)
		psVBFile [iHandle]->psKeyCurr [iLoop] = VBKEY_NULL;

	return (iserrno ? -1 : 0);
}

/*
 * Name:
 *	int	iVBClose3 (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The currently 'quasi-open' VBISAM handle
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
iVBClose3 (int iHandle)
{
	int	iLoop;

	for (iLoop = 0; iLoop < MAXSUBS; iLoop++)
	{
		vVBTreeAllFree (iHandle, iLoop, psVBFile [iHandle]->psTree [iLoop]);
		if (psVBFile [iHandle]->psKeydesc [iLoop])
		{
			vVBKeyUnMalloc (iHandle, iLoop);
			vVBFree (psVBFile [iHandle]->psKeydesc [iLoop], sizeof (struct keydesc));
		}
	}
	vVBFree (psVBFile [iHandle], sizeof (struct DICTINFO));
	psVBFile [iHandle] = (struct DICTINFO *) 0;
	return (0);
}

/*
 * Name:
 *	int	isindexinfo (int iHandle, struct keydesc *psKeydesc, int iKeyNumber);
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 *	struct	keydesc	*psKeydesc
 *		The receiving keydesc struct
 *		Note that if iKeyNumber == 0, then this is a dictinfo!
 *	int	iKeyNumber
 *		The keynumber (or 0 for a dictionary!)
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
isindexinfo (int iHandle, struct keydesc *psKeydesc, int iKeyNumber)
{
	char	*pcTemp;
	int	iResult;
	struct	dictinfo
		sDict;

	if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen)
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	iserrno = EBADKEY;
	if (iKeyNumber < 0 || iKeyNumber > psVBFile [iHandle]->iNKeys)
		return (-1);
	iserrno = 0;
	if (iKeyNumber)
	{
		memcpy (psKeydesc, psVBFile [iHandle]->psKeydesc [iKeyNumber - 1], sizeof (struct keydesc));
		return (0);
	}

	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (-1);

	sDict.iNKeys = psVBFile [iHandle]->iNKeys;
	if (psVBFile [iHandle]->iOpenMode & ISVARLEN)
	{
		pcTemp = (char *) &sDict.iNKeys;
		*pcTemp |= 0x80;
	}
	sDict.iMaxRowLength = psVBFile [iHandle]->iMaxRowLength;
	sDict.iIndexLength = psVBFile [iHandle]->iNodeSize;
	sDict.tNRows = tCountRows (iHandle);
	isreclen = psVBFile [iHandle]->iMinRowLength;
	memcpy (psKeydesc, &sDict, sizeof (struct dictinfo));

	iVBExit (iHandle);
	return (0);
}

/*
 * Name:
 *	int	isopen (char *pcFilename, int iMode);
 * Arguments:
 *	char	*pcFilename
 *		The null terminated filename to be built / opened
 *	int	iMode
 *		See isam.h
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	An error occurred.  iserrno contains the reason
 *	Other	The handle to be used for accessing this file
 * Problems:
 *	NONE known
 */
int
isopen (char *pcFilename, int iMode)
{
	char	*pcTemp;
	int	iFlags,
		iFound = FALSE,
		iHandle,
		iIndexNumber = 0,
		iIndexPart,
		iKeydescLength,
		iLengthUsed,
		iLoop,
		iResult;
	struct	DICTINFO
		*psFile = (struct DICTINFO *) 0;
	struct	stat
		sStat;
	off_t	tNodeNumber;

	if (!iInitialized)
	{
		iInitialized = TRUE;
		tVBPID = getpid ();
		tVBUID = getuid ();
		atexit (vVBUnMalloc);
	}
	if (iMode & ISTRANS && iVBLogfileHandle == -1)
	{
		iserrno = EBADARG;	// I'd have expected ENOLOG or ENOTRANS!
		return (-1);
	}
	iFlags = iMode & 0x03;
	if (iFlags == 3)
	{
	// Cannot be BOTH ISOUTPUT and ISINOUT
		iserrno = EBADARG;
		return (-1);
	}
	if (strlen (pcFilename) > MAX_PATH_LENGTH - 4)
	{
		iserrno = EFNAME;
		return (-1);
	}
	/*
	 * The following for loop deals with the concept of re-opening a file
	 * that was closed within the SAME transaction.  Since we were not
	 * allowed to perform the FULL close during the transaction because we
	 * needed to retain all the transactional locks, we can tremendously
	 * simplify the re-opening process.
	 */
	for (iHandle = 0; iHandle <= iVBMaxUsedHandle; iHandle++)
	{
		if (psVBFile [iHandle] && psVBFile [iHandle]->iIsOpen != 0)
		{
			if (!strcmp (psVBFile [iHandle]->cFilename, pcFilename))
			{
				if (psVBFile [iHandle]->iIsOpen == 2)
				{
					sprintf (cVBNode [0], "%s.idx", pcFilename);
					psVBFile [iHandle]->iIndexHandle = iVBOpen (cVBNode [0], O_RDWR, 0);
					sprintf (cVBNode [0], "%s.dat", pcFilename);
					psVBFile [iHandle]->iDataHandle = iVBOpen (cVBNode [0], O_RDWR, 0);
					if (iMode & ISEXCLLOCK)
						iResult = iVBFileOpenLock (iHandle, 2);
					else
						iResult = iVBFileOpenLock (iHandle, 1);
					if (iResult)
					{
						errno = EFLOCKED;
						goto OPEN_ERR;
					}
				}
				psVBFile [iHandle]->iIsOpen = 0;
				psVBFile [iHandle]->iOpenMode = iMode;
				return (iHandle);
			}
		}
	}
	for (iHandle = 0; iHandle <= iVBMaxUsedHandle; iHandle++)
	{
		if (!psVBFile [iHandle])
		{
			iFound = TRUE;
			break;
		}
	}
	if (!iFound)
	{
		for (iHandle = 0; iHandle <= iVBMaxUsedHandle; iHandle++)
		{
			if (psVBFile [iHandle] && psVBFile [iHandle]->iIsOpen == 2)
			{
				iVBClose3 (iHandle);
				iFound = TRUE;
				break;
			}
		}
	}
	if (!iFound)
	{
		iserrno = ETOOMANY;
		if (iVBMaxUsedHandle >= VB_MAX_FILES)
			return (-1);
		iVBMaxUsedHandle++;
		iHandle = iVBMaxUsedHandle;
		psVBFile [iHandle] = (struct DICTINFO *) 0;
	}
	psVBFile [iHandle] = (struct DICTINFO *) pvVBMalloc (sizeof (struct DICTINFO));
	if (psVBFile [iHandle] == (struct DICTINFO *) 0)
		goto OPEN_ERR;
	psFile = psVBFile [iHandle];
	memset (psFile, 0, sizeof (struct DICTINFO));
	memcpy (psFile->cFilename, pcFilename, strlen (pcFilename) + 1);
	psFile->iDataHandle = -1;
	psFile->iIndexHandle = -1;
	sprintf (cVBNode [0], "%s.dat", pcFilename);
	if (iVBStat (cVBNode [0], &sStat))
	{
		errno = ENOENT;
		goto OPEN_ERR;
	}
	sprintf (cVBNode [0], "%s.idx", pcFilename);
	if (iVBStat (cVBNode [0], &sStat))
	{
		errno = ENOENT;
		goto OPEN_ERR;
	}
	psFile->iIndexHandle = iVBOpen (cVBNode [0], O_RDWR, 0);
	if (psFile->iIndexHandle < 0)
		goto OPEN_ERR;
	sprintf (cVBNode [0], "%s.dat", pcFilename);
	psFile->iDataHandle = iVBOpen (cVBNode [0], O_RDWR, 0);
	if (psFile->iDataHandle < 0)
		goto OPEN_ERR;
	psVBFile [iHandle]->iIsOpen = 0;

	psFile->iNodeSize = MAX_NODE_LENGTH;
	iResult = iVBEnter (iHandle, TRUE);	// Reads in dictionary node
	if (iResult)
		goto OPEN_ERR;
	errno = EBADFILE;
#if	ISAMMODE == 1
	if (psVBFile [iHandle]->sDictNode.cValidation [0] != 0x56 || psVBFile [iHandle]->sDictNode.cValidation [1] != 0x42)
		goto OPEN_ERR;
#else	// ISAMMODE == 1
	if (psVBFile [iHandle]->sDictNode.cValidation [0] != -2 || psVBFile [iHandle]->sDictNode.cValidation [1] != 0x53)
		goto OPEN_ERR;
#endif	// ISAMMODE == 1
	psFile->iNodeSize = ldint (psVBFile [iHandle]->sDictNode.cNodeSize) + 1;
	psFile->iNKeys = ldint (psVBFile [iHandle]->sDictNode.cIndexCount);
	psFile->iMinRowLength = ldint (psVBFile [iHandle]->sDictNode.cMinRowLength);
	if (iMode & ISVARLEN)
		psFile->iMaxRowLength = ldint (psVBFile [iHandle]->sDictNode.cMaxRowLength);
	else
		psFile->iMaxRowLength = psFile->iMinRowLength;

	if (psFile->iMaxRowLength && psFile->iMaxRowLength != psFile->iMinRowLength)
	{
		errno = EROWSIZE;
		if (!(iMode & ISVARLEN))
			goto OPEN_ERR;
	}
	else
	{
		errno = EROWSIZE;
		if (iMode & ISVARLEN)
			goto OPEN_ERR;
	}
	psFile->iOpenMode = iMode;
	if (psFile->iMinRowLength + 1 + INTSIZE + QUADSIZE > iVBRowBufferLength)
	{
		if (pcRowBuffer)
		{
			vVBFree (pcRowBuffer, iVBRowBufferLength);
			vVBFree (pcWriteBuffer, iVBRowBufferLength);
		}
		iVBRowBufferLength = psFile->iMinRowLength + 1 + INTSIZE + QUADSIZE;
		pcRowBuffer = (char *) pvVBMalloc (iVBRowBufferLength);
		if (!pcRowBuffer)
		{
			fprintf (stderr, "FATAL Memory allocation failure!\n");
			exit (-1);
		}
		pcWriteBuffer = (char *) pvVBMalloc (iVBRowBufferLength);
		if (!pcWriteBuffer)
		{
			fprintf (stderr, "FATAL Memory allocation failure!\n");
			exit (-1);
		}
	}
	psFile->ppcRowBuffer = &pcRowBuffer;
	tNodeNumber = ldquad (psVBFile [iHandle]->sDictNode.cNodeKeydesc);

	// Fill in the keydesc stuff
	while (tNodeNumber)
	{
		iResult = iVBBlockRead (iHandle, TRUE, tNodeNumber, cVBNode [0]);
		errno = iserrno;
		if (iResult)
			goto OPEN_ERR;
		pcTemp = cVBNode [0];
		errno = EBADFILE;
		if (*(cVBNode [0] + psFile->iNodeSize - 3) != -1 || *(cVBNode [0] + psFile->iNodeSize - 2) != 0x7e)
			goto OPEN_ERR;
		iLengthUsed = ldint (pcTemp);
		pcTemp += INTSIZE;
		tNodeNumber = ldquad (pcTemp);
		pcTemp += QUADSIZE;
		iLengthUsed -= (INTSIZE + QUADSIZE);
		while (iLengthUsed > 0)
		{
			errno = EBADFILE;
			if (iIndexNumber >= MAXSUBS)
				goto OPEN_ERR;
			iKeydescLength = ldint (pcTemp);
			iLengthUsed -= iKeydescLength;
			pcTemp += INTSIZE;
			psFile->psKeydesc [iIndexNumber] = (struct keydesc *) pvVBMalloc (sizeof (struct keydesc));
			if (psFile->psKeydesc [iIndexNumber] == (struct keydesc *) 0)
				goto OPEN_ERR;
			psFile->psKeydesc [iIndexNumber]->iNParts = 0;
			psFile->psKeydesc [iIndexNumber]->iKeyLength = 0;
			psFile->psKeydesc [iIndexNumber]->tRootNode = ldquad (pcTemp);
			pcTemp += QUADSIZE;
			psFile->psKeydesc [iIndexNumber]->iFlags = (*pcTemp) * 2;
			pcTemp++;
			iKeydescLength -= (QUADSIZE + INTSIZE + 1);
			iIndexPart = 0;
			if (*pcTemp & 0x80)
				psFile->psKeydesc [iIndexNumber]->iFlags |= ISDUPS;
			*pcTemp &= ~0x80;
			while (iKeydescLength > 0)
			{
				psFile->psKeydesc [iIndexNumber]->iNParts++;
				psFile->psKeydesc [iIndexNumber]->sPart [iIndexPart].iLength = ldint (pcTemp);
				psFile->psKeydesc [iIndexNumber]->iKeyLength += psFile->psKeydesc [iIndexNumber]->sPart [iIndexPart].iLength;
				pcTemp += INTSIZE;
				psFile->psKeydesc [iIndexNumber]->sPart [iIndexPart].iStart = ldint (pcTemp);
				pcTemp += INTSIZE;
				psFile->psKeydesc [iIndexNumber]->sPart [iIndexPart].iType = *pcTemp;
				pcTemp++;
				iKeydescLength -= ((INTSIZE * 2) + 1);
				errno = EBADFILE;
				if (iKeydescLength < 0)
					goto OPEN_ERR;
				iIndexPart++;
			}
			iIndexNumber++;
		}
		if (iLengthUsed < 0)
			goto OPEN_ERR;
	}
	if (iMode & ISEXCLLOCK)
		iResult = iVBFileOpenLock (iHandle, 2);
	else
		iResult = iVBFileOpenLock (iHandle, 1);
	if (iResult)
	{
		errno = EFLOCKED;
		goto OPEN_ERR;
	}
	iVBExit (iHandle);
	iResult = isstart (iHandle, psVBFile [iHandle]->psKeydesc [0], 0, (char *) 0, ISFIRST);
	if (iResult)
	{
		errno = iserrno;
		goto OPEN_ERR;
	}

	if (iVBInTrans == VBNOTRANS)
	{
		iVBTransOpen (iHandle, pcFilename);
		psVBFile [iHandle]->sFlags.iTransYet = 1;
	}
	return (iHandle);
OPEN_ERR:
	iVBExit (iHandle);
	if (psVBFile [iHandle] != (struct DICTINFO *) 0)
	{
		for (iLoop = 0; iLoop < MAXSUBS; iLoop++)
			if (psFile->psKeydesc [iLoop])
				vVBFree (psFile->psKeydesc [iLoop], sizeof (struct keydesc));
		if (psFile->iDataHandle != -1)
			iVBClose (psFile->iDataHandle);
		if (psFile->iDataHandle != -1)
			iVBClose (psFile->iIndexHandle);
		vVBFree (psVBFile [iHandle], sizeof (struct DICTINFO));
	}
	psVBFile [iHandle] = (struct DICTINFO *) 0;
	iserrno = errno;
	return (-1);
}

/*
 * Name:
 *	static	off_t	tCountRows (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 * Prerequisites:
 *	NONE
 * Returns:
 *	N	Where n is the count of rows in existance in the VBISAM file
 * Problems:
 *	NONE known
 */
static	off_t
tCountRows (int iHandle)
{
	int	iNodeUsed;
	off_t	tNodeNumber,
		tDataCount;

	tNodeNumber = ldquad ((char *) psVBFile [iHandle]->sDictNode.cDataFree);
	tDataCount = ldquad ((char *) psVBFile [iHandle]->sDictNode.cDataCount);
	while (tNodeNumber)
	{
		if (iVBBlockRead (iHandle, TRUE, tNodeNumber, cVBNode [0]))
			return (-1);
		iNodeUsed = ldint (cVBNode [0]);
		iNodeUsed -= INTSIZE + QUADSIZE;
		tDataCount -= (iNodeUsed / QUADSIZE);
		tNodeNumber = ldquad (cVBNode [0] + INTSIZE);
	}
	return (tDataCount);
}
