/*
 * Title:	isbuild.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	24Nov2003
 * Description:
 *	This is the module that deals with creation of a completely NEW VBISAM
 *	table.  It also implements the isaddindex () and isdelindex () as these
 *	are predominantly only called at isbuild () time.
 * Version:
 *	$Id: isbuild.c,v 1.8 2004/06/13 06:32:33 trev_vb Exp $
 * Modification History:
 *	$Log: isbuild.c,v $
 *	Revision 1.8  2004/06/13 06:32:33  trev_vb
 *	TvB 12June2004 See CHANGELOG 1.03 (Too lazy to enumerate)
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
 *	Revision 1.2  2003/12/22 04:45:50  trev_vb
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
int	isbuild (char *, int, struct keydesc *, int);
int	isaddindex (int, struct keydesc *);
int	isdelindex (int, struct keydesc *);
static	int	iAddKeydescriptor (int, struct keydesc *);
static	off_t	tDelKeydescriptor (int, struct keydesc *, int);
static	int	iDelNodes (int, int, off_t);
static	int	iMakeKeysFromData (int, int);

/*
 * Name:
 *	int	isbuild (char *pcFilename, int iMaxRowLen, struct keydesc *psKey, int iMode);
 * Arguments:
 *	char	*pcFilename
 *		The null terminated filename to be built / opened
 *	int	iMaxRowLength
 *		The MAXIMUM row length to be used
 *	struct keydesc *psKey
 *		The definition of the primary key
 *	int	iMode
 *		See isam.h
 * Prerequisites:
 *	If iMode includes ISVARLEN then isreclen should be set to min row length
 * Returns:
 *	-1	An error occurred.  iserrno contains the reason
 *	Other	The handle to be used for accessing this file
 * Problems:
 *	NONE known
 * Comments:
 *	I've chosen to make VBISAM *NOT* overwrite a pre-existing VBISAM file
 *	The iserase () call allows a user to remove a pre-existing VBISAM file
 */
int
isbuild (char *pcFilename, int iMaxRowLength, struct keydesc *psKey, int iMode)
{
	char	*pcTemp;
	int	iFlags,
		iFound = FALSE,
		iHandle,
		iLoop,
		iMinRowLength;
	struct	stat
		sStat;

	// STEP 1: Sanity checks
	if (iMode & ISVARLEN)
		iMinRowLength = isreclen;
	else
		iMinRowLength = iMaxRowLength;
	iFlags = iMode & 0x03;
	if (iFlags == 3)
	{
	// Cannot be BOTH ISOUTPUT and ISINOUT
		iserrno = EBADARG;
		return (-1);
	}
	iserrno = EFNAME;
	if (strlen (pcFilename) > MAX_PATH_LENGTH - 4)
		return (-1);
	iserrno = EBADARG;
	if (psKey == (struct keydesc *) 0)
		return (-1);

	// Sanity checks passed (so far)
	for (iHandle = 0; iHandle <= iVBMaxUsedHandle; iHandle++)
	{
		if (psVBFile [iHandle] == (struct DICTINFO *) 0)
		{
			iFound = TRUE;
			break;
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
		goto BUILD_ERR;
	memset (psVBFile [iHandle], 0, sizeof (struct DICTINFO));
	iserrno = EBADARG;
	if (iVBCheckKey (iHandle, psKey, 0, iMinRowLength, TRUE))
		return (-1);
	sprintf (cVBNode [0], "%s.dat", pcFilename);
	if (!iVBStat (cVBNode [0], &sStat))
	{
		errno = EEXIST;
		goto BUILD_ERR;
	}
	sprintf (cVBNode [0], "%s.idx", pcFilename);
	if (!iVBStat (cVBNode [0], &sStat))
	{
		errno = EEXIST;
		goto BUILD_ERR;
	}
	psVBFile [iHandle]->iIndexHandle = iVBOpen (cVBNode [0], iFlags | O_CREAT, 0666);
	if (psVBFile [iHandle]->iIndexHandle < 0)
		goto BUILD_ERR;
	sprintf (cVBNode [0], "%s.dat", pcFilename);
	psVBFile [iHandle]->iDataHandle = iVBOpen (cVBNode [0], iFlags | O_CREAT, 0666);
	if (psVBFile [iHandle]->iDataHandle < 0)
	{
		iVBClose (psVBFile [iHandle]->iIndexHandle);	// Ignore ret
		goto BUILD_ERR;
	}
	psVBFile [iHandle]->iNKeys = 1;
	psVBFile [iHandle]->iNodeSize = MAX_NODE_LENGTH;
	psVBFile [iHandle]->iOpenMode = iMode;
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x01;

	// Setup root (dictionary) node (Node 1)
	memset (cVBNode [0], 0, MAX_NODE_LENGTH);
	memset ((void *) &psVBFile [iHandle]->sDictNode, 0, sizeof (struct DICTNODE));
#if	ISAMMODE == 1
	psVBFile [iHandle]->sDictNode.cValidation [0] = 'V';
	psVBFile [iHandle]->sDictNode.cValidation [1] = 'B';
	psVBFile [iHandle]->sDictNode.cRsvdPerKey = 0x08;
#else	// ISAMMODE == 1
	psVBFile [iHandle]->sDictNode.cValidation [0] = 0xfe;
	psVBFile [iHandle]->sDictNode.cValidation [1] = 0x53;
	psVBFile [iHandle]->sDictNode.cRsvdPerKey = 0x04;
#endif	// ISAMMODE == 1
	psVBFile [iHandle]->sDictNode.cHeaderRsvd = 0x02;
	psVBFile [iHandle]->sDictNode.cFooterRsvd = 0x02;
	psVBFile [iHandle]->sDictNode.cRFU1 = 0x04;
	stint (MAX_NODE_LENGTH - 1, psVBFile [iHandle]->sDictNode.cNodeSize);
	stint (1, psVBFile [iHandle]->sDictNode.cIndexCount);
	stint (0x0704, psVBFile [iHandle]->sDictNode.cRFU2);
	stint (iMinRowLength, psVBFile [iHandle]->sDictNode.cMinRowLength);
	stquad (2, psVBFile [iHandle]->sDictNode.cNodeKeydesc);
	stquad (0, psVBFile [iHandle]->sDictNode.cDataFree);
	stquad (0, psVBFile [iHandle]->sDictNode.cNodeFree);
	stquad (0, psVBFile [iHandle]->sDictNode.cDataCount);
	if (psKey->iNParts)
		stquad (3, psVBFile [iHandle]->sDictNode.cNodeCount);
	else
		stquad (2, psVBFile [iHandle]->sDictNode.cNodeCount);
	stquad (1, psVBFile [iHandle]->sDictNode.cTransNumber);
	stquad (1, psVBFile [iHandle]->sDictNode.cUniqueID);
	stquad (0, psVBFile [iHandle]->sDictNode.cNodeAudit);
	stint (0x0008, psVBFile [iHandle]->sDictNode.cLockMethod);
	if (iMode & ISVARLEN)
		stint (iMaxRowLength, psVBFile [iHandle]->sDictNode.cMaxRowLength);
	else
		stint (0, psVBFile [iHandle]->sDictNode.cMaxRowLength);
	memcpy (cVBNode [0], &psVBFile [iHandle]->sDictNode, sizeof (struct DICTNODE));
	if (iVBBlockWrite (iHandle, TRUE, (off_t) 1, cVBNode [0]))
	{
		iVBClose (psVBFile [iHandle]->iIndexHandle);	// Ignore ret
		iVBClose (psVBFile [iHandle]->iDataHandle);	// Ignore ret
		vVBFree (psVBFile [iHandle], sizeof (struct DICTINFO));
		return (-1);
	}

	// Setup first keydesc node (Node 2)
	memset (cVBNode [0], 0, MAX_NODE_LENGTH);
	pcTemp = cVBNode [0];
	pcTemp += INTSIZE;
	stquad (0, pcTemp);			// Next keydesc node
	pcTemp += QUADSIZE;
	stint (INTSIZE + QUADSIZE + 1 + (((INTSIZE * 2) + 1) * psKey->iNParts), pcTemp);	// Keydesc length
	pcTemp += INTSIZE;
	if (psKey->iNParts)
		stquad (3, pcTemp);		// Root node for this key
	else
		stquad (0, pcTemp);		// Root node for this key
	pcTemp += QUADSIZE;
	*pcTemp = psKey->iFlags / 2;		// Compression / Dups flags
	pcTemp++;
	for (iLoop = 0; iLoop < psKey->iNParts; iLoop++)
	{
		stint (psKey->sPart [iLoop].iLength, pcTemp);	// Length
		if (iLoop == 0 && psKey->iFlags & 1)
			*pcTemp |= 0x80;
		pcTemp += INTSIZE;
		stint (psKey->sPart [iLoop].iStart, pcTemp);	// Offset
		pcTemp += INTSIZE;
		*pcTemp = psKey->sPart [iLoop].iType;		// Type
		pcTemp++;
	}
	stint (pcTemp - cVBNode [0], cVBNode [0]);	// Length used
	stint (0xff7e, cVBNode [0] + MAX_NODE_LENGTH - 3);
	if (iVBBlockWrite (iHandle, TRUE, (off_t) 2, cVBNode [0]))
	{
		iVBClose (psVBFile [iHandle]->iIndexHandle);	// Ignore ret
		iVBClose (psVBFile [iHandle]->iDataHandle);	// Ignore ret
		vVBFree (psVBFile [iHandle], sizeof (struct DICTINFO));
		return (-1);
	}

	if (psKey->iNParts)
	{
		// Setup key root node (Node 3)
		memset (cVBNode [0], 0, MAX_NODE_LENGTH);
#if	ISAMMODE == 1
		stint (INTSIZE + QUADSIZE, cVBNode [0]);
		stquad (1, cVBNode [0] + INTSIZE);		// Transaction number
#else	// ISAMMODE == 1
		stint (INTSIZE, cVBNode [0]);
#endif	// ISAMMODE == 1
		if (iVBBlockWrite (iHandle, TRUE, (off_t) 3, cVBNode [0]))
		{
			iVBClose (psVBFile [iHandle]->iIndexHandle);	// Ignore ret
			iVBClose (psVBFile [iHandle]->iDataHandle);	// Ignore ret
			vVBFree (psVBFile [iHandle], sizeof (struct DICTINFO));
			return (-1);
		}
	}

	psVBFile [iHandle]->iIsOpen = 0;	// Mark it as FULLY open
	isclose (iHandle);
	iserrno = 0;
	iVBTransBuild (pcFilename, iMinRowLength, iMaxRowLength, psKey, iMode);
	return (isopen (pcFilename, iMode));
BUILD_ERR:
	if (psVBFile [iHandle] != (struct DICTINFO *) 0)
		vVBFree (psVBFile [iHandle], sizeof (struct DICTINFO));
	psVBFile [iHandle] = (struct DICTINFO *) 0;
	iserrno = errno;
	return (-1);
}

/*
 * Name:
 *	int	isaddindex (int iHandle, struct keydesc *psKeydesc);
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 *	struct keydesc *psKey
 *		The definition of the key to be added
 * Prerequisites:
 *	The file *MUST* be open in ISEXCLLOCK mode for this function to work
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
isaddindex (int iHandle, struct keydesc *psKeydesc)
{
	int	iResult,
		iKeyNumber;

	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (-1);

	iResult = -1;
	iserrno = ENOTEXCL;
	if (!(psVBFile [iHandle]->iOpenMode & ISEXCLLOCK))
		goto AddIndexExit;
	iserrno = EKEXISTS;
	iKeyNumber = iVBCheckKey (iHandle, psKeydesc, 1, 0, 0);
	if (iKeyNumber == -1)
		goto AddIndexExit;
	iKeyNumber = iAddKeydescriptor (iHandle, psKeydesc);
	if (iKeyNumber)
		goto AddIndexExit;
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	iKeyNumber = iVBCheckKey (iHandle, psKeydesc, 1, 0, 0);
	if (iKeyNumber < 0)
		goto AddIndexExit;
	for (iKeyNumber = 0; iKeyNumber < MAXSUBS && !psVBFile [iHandle]->psKeydesc [iKeyNumber]; iKeyNumber++)
		;
	iserrno = ETOOMANY;
	if (iKeyNumber >= MAXSUBS)
		goto AddIndexExit;
	iKeyNumber = psVBFile [iHandle]->iNKeys;
	psVBFile [iHandle]->psKeydesc [iKeyNumber] = pvVBMalloc (sizeof (struct keydesc));
	psVBFile [iHandle]->iNKeys++;
	stint (psVBFile [iHandle]->iNKeys, psVBFile [iHandle]->sDictNode.cIndexCount);
	iserrno = errno;
	if (!psVBFile [iHandle]->psKeydesc [iKeyNumber])
		goto AddIndexExit;
	memcpy (psVBFile [iHandle]->psKeydesc [iKeyNumber], psKeydesc, sizeof (struct keydesc));
	if (iMakeKeysFromData (iHandle, iKeyNumber))
	{
// BUG - Handle this better!
		iResult = iserrno;
		iVBExit (iHandle);
		isdelindex (iHandle, psKeydesc);
		iserrno = iResult;
		goto AddIndexExit;
	}
	iserrno = 0;
	iResult = iVBTransCreateIndex (iHandle, psKeydesc);

AddIndexExit:
	iResult |= iVBExit (iHandle);
	if (iResult)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	isdelindex (int iHandle, struct keydesc *psKeydesc);
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 *	struct keydesc *psKey
 *		The definition of the key to be removed
 * Prerequisites:
 *	The file *MUST* be open in ISEXCLLOCK mode for this function to work
 * Returns:
 *	0	Success
 *	-1	An error occurred.  iserrno contains the reason
 * Problems:
 *	NONE known
 */
int
isdelindex (int iHandle, struct keydesc *psKeydesc)
{
	int	iResult = -1,
		iKeyNumber,
		iLoop;
	off_t	tRootNode;

	if (iVBEnter (iHandle, TRUE))
		return (-1);

	if (!(psVBFile [iHandle]->iOpenMode & ISEXCLLOCK))
	{
		iserrno = ENOTEXCL;
		goto DelIndexExit;
	}
	iKeyNumber = iVBCheckKey (iHandle, psKeydesc, 2, 0, 0);
	if (iKeyNumber == -1)
	{
		iserrno = EKEXISTS;
		goto DelIndexExit;
	}
	if (!iKeyNumber)
	{
		iserrno = EPRIMKEY;
		goto DelIndexExit;
	}
	tRootNode = tDelKeydescriptor (iHandle, psKeydesc, iKeyNumber);
	if (tRootNode < 1)
		goto DelIndexExit;
	if (iDelNodes (iHandle, iKeyNumber, tRootNode))
		goto DelIndexExit;
	vVBFree (psVBFile [iHandle]->psKeydesc [iKeyNumber], sizeof (struct keydesc));
	vVBTreeAllFree (iHandle, iKeyNumber, psVBFile [iHandle]->psTree [iKeyNumber]);
	vVBKeyUnMalloc (iHandle, iKeyNumber);
	for (iLoop = iKeyNumber; iLoop < MAXSUBS; iLoop++)
	{
		psVBFile [iHandle]->psKeydesc [iLoop] = psVBFile [iHandle]->psKeydesc [iLoop + 1];
		psVBFile [iHandle]->psTree [iLoop] = psVBFile [iHandle]->psTree [iLoop + 1];
		psVBFile [iHandle]->psKeyFree [iLoop] = psVBFile [iHandle]->psKeyFree [iLoop + 1];
		psVBFile [iHandle]->psKeyCurr [iLoop] = psVBFile [iHandle]->psKeyCurr [iLoop + 1];
	}
	psVBFile [iHandle]->psKeydesc [MAXSUBS - 1] = (struct keydesc *) 0;
	psVBFile [iHandle]->psTree [MAXSUBS - 1] = VBTREE_NULL;
	psVBFile [iHandle]->psKeyFree [MAXSUBS - 1] = VBKEY_NULL;
	psVBFile [iHandle]->psKeyCurr [MAXSUBS - 1] = VBKEY_NULL;
	psVBFile [iHandle]->iNKeys--;
	stint (psVBFile [iHandle]->iNKeys, psVBFile [iHandle]->sDictNode.cIndexCount);
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	iResult = iVBTransDeleteIndex (iHandle, psKeydesc);

DelIndexExit:
	iResult |= iVBExit (iHandle);
	return (iResult);
}

static	int
iAddKeydescriptor (int iHandle, struct keydesc *psKeydesc)
{
	char	cKeydesc [INTSIZE + QUADSIZE + 1 + (NPARTS * ((INTSIZE * 2) + 1))],
		*pcDstPtr = cKeydesc + INTSIZE;
	int	iLoop,
		iLenKeyUncomp = 0,
		iLenKeydesc,
		iNodeUsed;
	off_t	tHeadNode,
		tNodeNumber = 0,
		tNewNode;

	/*
	 * Step 1:
	 *	Create a new 'root node' for the new index
	 */
	tNewNode = tVBNodeCountGetNext (iHandle);
	if (tNewNode == -1)
		return (-1);
	memset (cVBNode [0], 0, MAX_NODE_LENGTH);
#if	ISAMMODE == 1
	stint (INTSIZE + QUADSIZE, cVBNode [0]);
	stquad (1, cVBNode [0] + INTSIZE);
#else	// ISAMMODE == 1
	stint (INTSIZE, cVBNode [0]);
#endif	// ISAMMODE == 1
	iserrno = iVBBlockWrite (iHandle, TRUE, tNewNode, cVBNode [0]);
	if (iserrno)
		return (-1);
	/*
	 * Step 2:
	 *	Append the new key description to the keydesc list
	 */
	tHeadNode = ldquad (psVBFile [iHandle]->sDictNode.cNodeKeydesc);
	if (tHeadNode < 1)
		return (-1);
	while (tHeadNode)
	{
		tNodeNumber = tHeadNode;
		iserrno = iVBBlockRead (iHandle, TRUE, tNodeNumber, cVBNode [0]);
		if (iserrno)
			return (-1);
		tHeadNode = ldquad (cVBNode [0] + INTSIZE);
	}
	stquad (tNewNode, pcDstPtr);
	pcDstPtr += QUADSIZE;
	*pcDstPtr = psKeydesc->iFlags / 2;
	pcDstPtr++;
	for (iLoop = 0; iLoop < psKeydesc->iNParts; iLoop++)
	{
		iLenKeyUncomp += psKeydesc->sPart [iLoop].iLength;
		stint (psKeydesc->sPart [iLoop].iLength, pcDstPtr);
		if (iLoop == 0 && psKeydesc->iFlags & ISDUPS)
			*pcDstPtr |= 0x80;
		pcDstPtr += INTSIZE;
		stint (psKeydesc->sPart [iLoop].iStart, pcDstPtr);
		pcDstPtr += INTSIZE;
		*pcDstPtr = psKeydesc->sPart [iLoop].iType;
		pcDstPtr++;
	}
	iNodeUsed = ldint (cVBNode [0]);
	iLenKeydesc = pcDstPtr - cKeydesc;
	stint (iLenKeydesc, cKeydesc);
	if (psVBFile [iHandle]->iNodeSize - (iNodeUsed + 4) < iLenKeydesc)
	{
		tNewNode = tVBNodeCountGetNext (iHandle);
		if (tNewNode == -1)
			return (-1);
		memset (cVBNode [1], 0, MAX_NODE_LENGTH);
		stint (INTSIZE + QUADSIZE + iLenKeydesc, cVBNode [1]);
		memcpy (cVBNode [1] + INTSIZE + QUADSIZE, cKeydesc, iLenKeydesc);
		iserrno = iVBBlockWrite (iHandle, TRUE, tNewNode, cVBNode [0]);
		if (iserrno)
			return (-1);
		stquad (tNewNode, cVBNode [0] + INTSIZE);
		iserrno = iVBBlockWrite (iHandle, TRUE, tNodeNumber, cVBNode [0]);
		if (iserrno)
			return (-1);
		return (0);
	}
	stint (iNodeUsed + iLenKeydesc, cVBNode [0]);
	psKeydesc->iKeyLength = iLenKeyUncomp;
	psKeydesc->tRootNode = tNewNode;
	memcpy (cVBNode [0] + iNodeUsed, cKeydesc, iLenKeydesc);
	iserrno = iVBBlockWrite (iHandle, TRUE, tNodeNumber, cVBNode [0]);
	if (iserrno)
		return (-1);
	return (0);
}

static	off_t
tDelKeydescriptor (int iHandle, struct keydesc *psKeydesc, int iKeyNumber)
{
	char	*pcSrcPtr;
	int	iLoop = 0,
		iNodeUsed;
	off_t	tHeadNode;

	iserrno = EBADFILE;
	tHeadNode = ldquad (psVBFile [iHandle]->sDictNode.cNodeKeydesc);
	while (1)
	{
		if (!tHeadNode)
			return (-1);
		memset (cVBNode [0], 0, MAX_NODE_LENGTH);
		iserrno = iVBBlockRead (iHandle, TRUE, tHeadNode, cVBNode [0]);
		if (iserrno)
			return (-1);
		pcSrcPtr = cVBNode [0] + INTSIZE + QUADSIZE;
		iNodeUsed = ldint (cVBNode [0]);
		while (pcSrcPtr - cVBNode [0] < iNodeUsed)
		{
			if (iLoop < iKeyNumber)
			{
				iLoop++;
				pcSrcPtr += ldint (pcSrcPtr);
				continue;
			}
			iNodeUsed -= ldint (pcSrcPtr);
			stint (iNodeUsed, cVBNode [0]);
			memcpy (pcSrcPtr, pcSrcPtr + ldint (pcSrcPtr), MAX_NODE_LENGTH - (pcSrcPtr - cVBNode [0] + ldint (pcSrcPtr)));
			iserrno = iVBBlockWrite (iHandle, TRUE, tHeadNode, cVBNode [0]);
			if (iserrno)
				return (-1);
			return (psVBFile [iHandle]->psKeydesc [iKeyNumber]->tRootNode);
		}
		tHeadNode = ldquad (cVBNode [0] + INTSIZE);
	}
	return (-1);	// Just to keep the compiler happy :)
}

/*
 * Name:
 *	static	int	iDelNodes (int iHandle, int iKeyNumber, off_t tRootNode);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The key number being deleted
 *	off_t	tRootNode
 *		The root node of the index being deleted
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 * Comments:
 *	ONLY used by isdelindex!
 */
static	int
iDelNodes (int iHandle, int iKeyNumber, off_t tRootNode)
{
	char	cLclNode [MAX_NODE_LENGTH],
		*pcSrcPtr;
	int	iDuplicate,
		iKeyLength,
		iCompLength = 0,
		iNodeUsed,
		iResult = 0;
	struct	keydesc
		*psKeydesc;

	psKeydesc = psVBFile [iHandle]->psKeydesc [iKeyNumber];
	iResult = iVBBlockRead (iHandle, TRUE, tRootNode, cLclNode);
	if (iResult)
		return (iResult);
	// Recurse for non-leaf nodes
	if (*(cLclNode + psVBFile [iHandle]->iNodeSize - 2))
	{
		iNodeUsed = ldint (cLclNode);
#if	ISAMMODE == 1
		pcSrcPtr = cLclNode + INTSIZE + QUADSIZE;
#else	// ISAMMODE == 1
		pcSrcPtr = cLclNode + INTSIZE;
#endif	// ISAMMODE == 1
		iDuplicate = FALSE;
		while (pcSrcPtr - cLclNode < iNodeUsed)
		{
			if (iDuplicate)
			{
				if (!(*(pcSrcPtr + QUADSIZE) & 0x80))
					iDuplicate = FALSE;
				*(pcSrcPtr + QUADSIZE) &= ~0x80;
				iResult = iDelNodes (iHandle, iKeyNumber, ldquad (pcSrcPtr + QUADSIZE));	// Eeek, recursion :)
				if (iResult)
					return (iResult);
				pcSrcPtr += (QUADSIZE * 2);
			}
			iKeyLength = psKeydesc->iKeyLength;
			if (psKeydesc->iFlags & LCOMPRESS)
			{
#if	ISAMMODE == 1
				iCompLength = ldint (pcSrcPtr);
				pcSrcPtr += INTSIZE;
				iKeyLength -= (iCompLength - 2);
#else	// ISAMMODE == 1
				iCompLength = *(pcSrcPtr);
				pcSrcPtr++;
				iKeyLength -= (iCompLength - 1);
#endif	// ISAMMODE == 1
			}
			if (psKeydesc->iFlags & TCOMPRESS)
			{
#if	ISAMMODE == 1
				iCompLength = ldint (pcSrcPtr);
				pcSrcPtr += INTSIZE;
				iKeyLength -= (iCompLength - 2);
#else	// ISAMMODE == 1
				iCompLength = *pcSrcPtr;
				pcSrcPtr++;
				iKeyLength -= (iCompLength - 1);
#endif	// ISAMMODE == 1
			}
			pcSrcPtr += iKeyLength;
			if (psKeydesc->iFlags & ISDUPS)
			{
				pcSrcPtr += QUADSIZE;
				if (*pcSrcPtr & 0x80)
					iDuplicate = TRUE;
			}
			iResult = iDelNodes (iHandle, iKeyNumber, ldquad (pcSrcPtr));	// Eeek, recursion :)
			if (iResult)
				return (iResult);
			pcSrcPtr += QUADSIZE;
		}
	}
	iResult = iVBNodeFree (iHandle, tRootNode);
	return (iResult);
}

/*
 * Name:
 *	static	int	iMakeKeysFromData (int iHandle, int iKeyNumber);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	int	iKeyNumber
 *		The absolute key number within the index file (0-n)
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 * Comments:
 *	Reads in EVERY data row and creates a key entry for it
 */
static	int
iMakeKeysFromData (int iHandle, int iKeyNumber)
{
	char	cKeyValue [VB_MAX_KEYLEN];
	int	iDeleted,
		iResult;
	struct	VBKEY
		*psKey;
	off_t	tDupNumber,
		tLoop;

	// Don't have to insert if the key is a NULL key!
	if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iNParts == 0)
		return (0);

	for (tLoop = 1; tLoop < ldquad (psVBFile [iHandle]->sDictNode.cDataCount); tLoop++)
	{	
		/*
		 * Step 1:
		 *	Read in the existing data row (Just the min rowlength)
		 */
		iserrno = iVBDataRead (iHandle, (void *) *(psVBFile [iHandle]->ppcRowBuffer), &iDeleted, tLoop, FALSE);
		if (iserrno)
			return (-1);
		if (iDeleted)
			continue;
		/*
		 * Step 2:
		 *	Check the index for a potential ISNODUPS error (EDUPL)
		 *	Also, calculate the duplicate number as needed
		 */
		vVBMakeKey (iHandle, iKeyNumber, *(psVBFile [iHandle]->ppcRowBuffer), cKeyValue);
		iResult = iVBKeySearch (iHandle, ISGREAT, iKeyNumber, 0, cKeyValue, 0);
		tDupNumber = 0;
		if (iResult >= 0 && !iVBKeyLoad (iHandle, iKeyNumber, ISPREV, FALSE, &psKey) && !memcmp (psKey->cKey, cKeyValue, psVBFile [iHandle]->psKeydesc [iKeyNumber]->iKeyLength))
		{
			iserrno = EDUPL;
			if (psVBFile [iHandle]->psKeydesc [iKeyNumber]->iFlags & ISDUPS)
				tDupNumber = psKey->tDupNumber + 1;
			else
				return (-1);
		}

		/*
		 * Step 3:
		 * Perform the actual insertion into the index
		 */
		iResult = iVBKeyInsert (iHandle, VBTREE_NULL, iKeyNumber, cKeyValue, tLoop, tDupNumber, VBTREE_NULL);
		if (iResult)
			return (iResult);
	}
	return (0);
}
