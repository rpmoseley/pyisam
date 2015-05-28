/*
 * Title:	isHelper.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	26Nov2003
 * Description:
 *	This is the module where all the various 'helper' functions are defined.
 *	Only functions with external linkage (i.e. is*, ld* and st*) should be
 *	defined within this module.
 * Version:
 *	$Id: isHelper.c,v 1.12 2004/06/16 10:53:55 trev_vb Exp $
 * Modification History:
 *	$Log: isHelper.c,v $
 *	Revision 1.12  2004/06/16 10:53:55  trev_vb
 *	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
 *	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
 *	
 *	Revision 1.11  2004/06/13 07:52:17  trev_vb
 *	TvB 13June2004
 *	Implemented sharing of open files.
 *	Changed the locking strategy slightly to allow table-level locking granularity
 *	(i.e. A process opening the same table more than once can now lock itself!)
 *	
 *	Revision 1.10  2004/06/13 06:32:33  trev_vb
 *	TvB 12June2004 See CHANGELOG 1.03 (Too lazy to enumerate)
 *	
 *	Revision 1.9  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.8  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.7  2004/03/23 22:14:28  trev_vb
 *	TvB 23Mar2004 Ooops, forgot the 2byte short->int offset in stint code!
 *	
 *	Revision 1.6  2004/03/23 21:55:56  trev_vb
 *	TvB 23Mar2004 Endian on SPARC (Phase I).  Makefile changes for SPARC.
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
 *	Revision 1.2  2003/12/22 04:45:31  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:19  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"

/*
 * Prototypes
 */
int	iscluster (int, struct keydesc *);
int	iserase (char *);
int	isflush (int);
int	islock (int);
int	isrelcurr (int);		// Added for JvN
int	isrelease (int);
int	isrelrec (int, off_t);		// Added for JvN
int	isrename (char *, char *);
int	issetunique (int, off_t);
int	isuniqueid (int, off_t *);
int	isunlock (int);
void	ldchar (char *, int, char *);
void	stchar (char *, char *, int);
int	ldint (char *);
void	stint (int, char *);
long	ldlong (char *);
void	stlong (long, char *);
off_t	ldquad (char *);
void	stquad (off_t, char *);
double	ldfloat (char *);
void	stfloat (double, char *);
double	ldfltnull (char *, short *);
void	stfltnull (double, char *, short);
double	lddbl (char *);
void	stdbl (double, char *);
double	lddblnull (char *, short *);
void	stdblnull (double, char *, short);

/*
 * Name:
 *	int	iscluster (int iHandle, struct keydesc *psKeydesc)
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 *	struct	keydesc	*psKeydesc
 *		The index to order the data by
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	NONE known
 */
int
iscluster (int iHandle, struct keydesc *psKeydesc)
{
	// BUG Write iscluster() and don't forget to call iVBTransCluster
	return (0);
}

/*
 * Name:
 *	int	iserase (char *pcFilename)
 * Arguments:
 *	char	*pcFilename
 *		The name (sans extension) of the file to be deleted
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	NONE known
 */
int
iserase (char *pcFilename)
{
	char	cBuffer [256];

	sprintf (cBuffer, "%s.idx", pcFilename);
	if (iVBUnlink (cBuffer))
		goto EraseExit;
	sprintf (cBuffer, "%s.dat", pcFilename);
	if (iVBUnlink (cBuffer))
		goto EraseExit;
	return (iVBTransErase (pcFilename));
EraseExit:
	iserrno = errno;
	return (-1);
}

/*
 * Name:
 *	int	isflush (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The handle of the VBISAM file about to be flushed
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 * Problems:
 *	NONE known
 * Comments:
 *	Since we write out data in real-time, no flushing is needed
 */
int
isflush (int iHandle)
{
	if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen)
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	return (iVBBlockFlush (iHandle));
}

/*
 * Name:
 *	int	islock (int iHandle)
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	NONE known
 */
int
islock (int iHandle)
{
	if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen)
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	return (iVBDataLock (iHandle, VBWRLOCK, 0));
}

/*
 * Name:
 *	int	isrelcurr (int iHandle)
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	We can NOT do a vVBEnter / vVBExit here as this is called from
 *	OTHER is* functions (isstart and isread)
 */
int
isrelcurr (int iHandle)
{
	if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen)
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	if (iVBInTrans != VBNOTRANS)
		return (0);
	if (!psVBFile [iHandle]->tRowNumber)
	{
		iserrno = ENOREC;
		return (-1);
	}
	iserrno = iVBDataLock (iHandle, VBUNLOCK, psVBFile [iHandle]->tRowNumber);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	isrelease (int iHandle)
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	We can NOT do a vVBEnter / vVBExit here as this is called from
 *	OTHER is* functions (isstart and isread)
 */
int
isrelease (int iHandle)
{
	if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen)
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	if (iVBInTrans != VBNOTRANS)
		return (0);
	iVBDataLock (iHandle, VBUNLOCK, 0);	// Ignore the return
	return (0);
}

/*
 * Name:
 *	int	isrelrec (int iHandle, off_t tRowNumber)
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 *	off_t	tRowNumber
 *		The absolute rowid of the row to be unlocked
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	We can NOT do a vVBEnter / vVBExit here as this is called from
 *	OTHER is* functions (isstart and isread)
 */
int
isrelrec (int iHandle, off_t tRowNumber)
{
	if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen)
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	iserrno = iVBDataLock (iHandle, VBUNLOCK, tRowNumber);
	if (iserrno)
		return (-1);
	return (0);
}

/*
 * Name:
 *	int	isrename (char *pcOldName, char *pcNewName);
 * Arguments:
 *	char	*pcOldName
 *		The current filename (sans extension)
 *	char	*pcNewName
 *		The new filename (sans extension)
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	NONE known
 */
int
isrename (char *pcOldName, char *pcNewName)
{
	char	cBuffer [2][256];
	int	iResult;

	sprintf (cBuffer [0], "%s.idx", pcOldName);
	sprintf (cBuffer [1], "%s.idx", pcNewName);
	iResult = iVBLink (cBuffer [0], cBuffer [1]);
	if (iResult == -1)
		goto RenameExit;
	iResult = iVBUnlink (cBuffer [0]);
	if (iResult == -1)
		goto RenameExit;
	sprintf (cBuffer [0], "%s.dat", pcOldName);
	sprintf (cBuffer [1], "%s.dat", pcNewName);
	iResult = iVBLink (cBuffer [0], cBuffer [1]);
	if (iResult == -1)
		goto RenameExit;
	iResult = iVBUnlink (cBuffer [0]);
	if (iResult == -1)
		goto RenameExit;
	return (iVBTransRename (pcOldName, pcNewName));
RenameExit:
	iserrno = errno;
	return (-1);
}

/*
 * Name:
 *	int	issetunique (int iHandle, off_t tUniqueID);
 * Arguments:
 *	int	iHandle
 *		The handle of the VBISAM file about to be set
 *	off_t	tUniqueID
 *		The *NEW* value to be set into the unique ID
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	OTHER	Failure (iserrno)
 * Problems:
 *	NONE known
 */
int
issetunique (int iHandle, off_t tUniqueID)
{
	int	iResult,
		iResult2;

	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (iResult);
	iResult = iVBUniqueIDSet (iHandle, tUniqueID);

	if (!iResult)
		iResult = iVBTransSetUnique (iHandle, tUniqueID);
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	iResult2 = iVBExit (iHandle);
	if (iResult)
		return (iResult);
	return (iResult2);
}

/*
 * Name:
 *	int	isuniqueid (int iHandle, off_t *ptUniqueID);
 * Arguments:
 *	int	iHandle
 *		The handle of the VBISAM file about to be set
 *	off_t	*ptUniqueID
 *		A pointer to the receiving location to be used
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success (and ptUniqueID is filled in)
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	NONE known
 */
int
isuniqueid (int iHandle, off_t *ptUniqueID)
{
	int	iResult;
	off_t	tValue;

	iResult = iVBEnter (iHandle, TRUE);
	if (iResult)
		return (iResult);

	tValue = tVBUniqueIDGetNext (iHandle);

	if (!iResult)
		iResult = iVBTransUniqueID (iHandle, tValue);
	psVBFile [iHandle]->sFlags.iIsDictLocked |= 0x02;
	iResult = iVBExit (iHandle);
	*ptUniqueID = tValue;
	return (iResult);
}

/*
 * Name:
 *	int	isunlock (int iHandle)
 * Arguments:
 *	int	iHandle
 *		The handle of a currently open VBISAM file
 * Prerequisites:
 *	NONE
 * Returns:
 *	0	Success
 *	-1	Failure (iserrno has more info)
 * Problems:
 *	NONE known
 */
int
isunlock (int iHandle)
{
	if (!psVBFile [iHandle] || psVBFile [iHandle]->iIsOpen)
	{
		iserrno = ENOTOPEN;
		return (-1);
	}
	return (iVBDataLock (iHandle, VBUNLOCK, 0));
}

/*
 * Name:
 *	void	ldchar (char *pcSource, int iLength, char *pcDestination);
 * Arguments:
 *	char	*pcSource
 *		The source location
 *	int	iLength
 *		The number of characters
 *	char	*pcDestination
 *		The source location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
ldchar (char *pcSource, int iLength, char *pcDestination)
{
	char	*pcDst;

	memcpy ((void *) pcDestination, (void *) pcSource, iLength);
	for (pcDst = pcDestination + iLength - 1; pcDst >= (char *) pcDestination; pcDst--)
	{
		if (*pcDst != ' ')
		{
			pcDst++;
			*pcDst = 0;
			return;
		}
	}
	*(++pcDst) = 0;
	return;
}

/*
 * Name:
 *	void	stchar (char *pcSource, char *pcDestination, int iLength);
 * Arguments:
 *	char	*pcSource
 *		The source location
 *	char	*pcDestination
 *		The source location
 *	int	iLength
 *		The number of characters
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
stchar (char *pcSource, char *pcDestination, int iLength)
{
	char	*pcSrc,
		*pcDst;
	int	iCount;

	pcSrc = pcSource;
	pcDst = pcDestination;
	for (iCount = iLength; iCount && *pcSrc; iCount--, pcSrc++, pcDst++)
		*pcDst = *pcSrc;
	for (; iCount; iCount--, pcDst++)
		*pcDst = ' ';
	return;
}

/*
 * Name:
 *	int	ldint (char *pcLocation);
 * Arguments:
 *	char	*pcLocation
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
int
ldint (char *pcLocation)
{
	short	iValue = 0;
	char	*pcTemp = (char *) &iValue;

#if	VB_ENDIAN == 1234
	*(pcTemp + 1) = *(pcLocation + 0);
	*(pcTemp + 0) = *(pcLocation + 1);
#else	// VB_ENDIAN == 1234
	*(pcTemp + 0) = *(pcLocation + 0);
	*(pcTemp + 1) = *(pcLocation + 1);
#endif	// VB_ENDIAN == 1234
	return (iValue);
}

/*
 * Name:
 *	void	stint (int iValue, char *pcLocation);
 * Arguments:
 *	int	iValue
 *		The value to be stored
 *	char	*pcLocation
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
stint (int iValue, char *pcLocation)
{
	char	*pcTemp = (char *) &iValue;

#if	VB_ENDIAN == 1234
	*(pcLocation + 0) = *(pcTemp + 1);
	*(pcLocation + 1) = *(pcTemp + 0);
#else	// VB_ENDIAN == 1234
	*(pcLocation + 0) = *(pcTemp + 0 + INTSIZE);
	*(pcLocation + 1) = *(pcTemp + 1 + INTSIZE);
#endif	// VB_ENDIAN == 1234
	return;
}

/*
 * Name:
 *	long	ldlong (char *pcLocation);
 * Arguments:
 *	char	*pcLocation
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
long
ldlong (char *pcLocation)
{
	long	lValue = 0;
	char	*pcTemp = (char *) &lValue;

#if	VB_ENDIAN == 1234
	*(pcTemp + 3) = *(pcLocation + 0);
	*(pcTemp + 2) = *(pcLocation + 1);
	*(pcTemp + 1) = *(pcLocation + 2);
	*(pcTemp + 0) = *(pcLocation + 3);
#else	// VB_ENDIAN == 1234
	*(pcTemp + 0) = *(pcLocation + 0);
	*(pcTemp + 1) = *(pcLocation + 1);
	*(pcTemp + 2) = *(pcLocation + 2);
	*(pcTemp + 3) = *(pcLocation + 3);
#endif	// VB_ENDIAN == 1234
	return (lValue);
}

/*
 * Name:
 *	void	stlong (long lValue, char *pcLocation);
 * Arguments:
 *	long	lValue
 *		The value to be stored
 *	char	*pcLocation
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
stlong (long lValue, char *pcLocation)
{
	char	*pcTemp = (char *) &lValue;

#if	VB_ENDIAN == 1234
	*(pcLocation + 0) = *(pcTemp + 3);
	*(pcLocation + 1) = *(pcTemp + 2);
	*(pcLocation + 2) = *(pcTemp + 1);
	*(pcLocation + 3) = *(pcTemp + 0);
#else	// VB_ENDIAN == 1234
	*(pcLocation + 0) = *(pcTemp + 0);
	*(pcLocation + 1) = *(pcTemp + 1);
	*(pcLocation + 2) = *(pcTemp + 2);
	*(pcLocation + 3) = *(pcTemp + 3);
#endif	// VB_ENDIAN == 1234
	return;
}

/*
 * Name:
 *	off_t	ldquad (char *);
 * Arguments:
 *	char	*pcLocation
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
off_t
ldquad (char *pcLocation)
{
	off_t	tValue = 0;
	char	*pcTemp = (char *) &tValue;

#if	VB_ENDIAN == 1234
#if	ISAMMODE == 1
	*(pcTemp + 7) = *(pcLocation + 0);
	*(pcTemp + 6) = *(pcLocation + 1);
	*(pcTemp + 5) = *(pcLocation + 2);
	*(pcTemp + 4) = *(pcLocation + 3);
	*(pcTemp + 3) = *(pcLocation + 4);
	*(pcTemp + 2) = *(pcLocation + 5);
	*(pcTemp + 1) = *(pcLocation + 6);
	*(pcTemp + 0) = *(pcLocation + 7);
#else	// ISAMMODE == 1
	*(pcTemp + 3) = *(pcLocation + 0);
	*(pcTemp + 2) = *(pcLocation + 1);
	*(pcTemp + 1) = *(pcLocation + 2);
	*(pcTemp + 0) = *(pcLocation + 3);
#endif	// ISAMMODE == 1
#else	// VB_ENDIAN == 1234
#if	ISAMMODE == 1
	*(pcTemp + 0) = *(pcLocation + 0);
	*(pcTemp + 1) = *(pcLocation + 1);
	*(pcTemp + 2) = *(pcLocation + 2);
	*(pcTemp + 3) = *(pcLocation + 3);
	*(pcTemp + 4) = *(pcLocation + 4);
	*(pcTemp + 5) = *(pcLocation + 5);
	*(pcTemp + 6) = *(pcLocation + 6);
	*(pcTemp + 7) = *(pcLocation + 7);
#else	// ISAMMODE == 1
	*(pcTemp + 0) = *(pcLocation + 0);
	*(pcTemp + 1) = *(pcLocation + 1);
	*(pcTemp + 2) = *(pcLocation + 2);
	*(pcTemp + 3) = *(pcLocation + 3);
#endif	// ISAMMODE == 1
#endif	// VB_ENDIAN == 1234
	return (tValue);
}

/*
 * Name:
 *	void	stquad (off_t, char *);
 * Arguments:
 *	off_t	tValue
 *		The value to be stored
 *	char	*pcLocation
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
stquad (off_t tValue, char *pcLocation)
{
	char	*pcTemp = (char *) &tValue;

#if	VB_ENDIAN == 1234
#if	ISAMMODE == 1
	*(pcLocation + 0) = *(pcTemp + 7);
	*(pcLocation + 1) = *(pcTemp + 6);
	*(pcLocation + 2) = *(pcTemp + 5);
	*(pcLocation + 3) = *(pcTemp + 4);
	*(pcLocation + 4) = *(pcTemp + 3);
	*(pcLocation + 5) = *(pcTemp + 2);
	*(pcLocation + 6) = *(pcTemp + 1);
	*(pcLocation + 7) = *(pcTemp + 0);
#else	// ISAMMODE == 1
	*(pcLocation + 0) = *(pcTemp + 3);
	*(pcLocation + 1) = *(pcTemp + 2);
	*(pcLocation + 2) = *(pcTemp + 1);
	*(pcLocation + 3) = *(pcTemp + 0);
#endif	// ISAMMODE == 1
#else	// VB_ENDIAN == 1234
#if	ISAMMODE == 1
	*(pcLocation + 0) = *(pcTemp + 0);
	*(pcLocation + 1) = *(pcTemp + 1);
	*(pcLocation + 2) = *(pcTemp + 2);
	*(pcLocation + 3) = *(pcTemp + 3);
	*(pcLocation + 4) = *(pcTemp + 4);
	*(pcLocation + 5) = *(pcTemp + 5);
	*(pcLocation + 6) = *(pcTemp + 6);
	*(pcLocation + 7) = *(pcTemp + 7);
#else	// ISAMMODE == 1
	*(pcLocation + 0) = *(pcTemp + 0);
	*(pcLocation + 1) = *(pcTemp + 1);
	*(pcLocation + 2) = *(pcTemp + 2);
	*(pcLocation + 3) = *(pcTemp + 3);
#endif	// ISAMMODE == 1
#endif	// VB_ENDIAN == 1234
	return;
}

/*
 * Name:
 *	double	ldfloat (char *pcLocation);
 * Arguments:
 *	char	*pcLocation
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
double
ldfloat (char *pcLocation)
{
	float	fFloat;
	double	dDouble;

	memcpy (&fFloat, pcLocation, FLOATSIZE);
	dDouble = fFloat;
	return ((double) dDouble);
}

/*
 * Name:
 *	void	stfloat (double dSource, char *pcDestination);
 * Arguments:
 *	double	dSource
 *		Technically, it's a float that was promoted to a double by
 *		the compiler.
 *	char	*pcDestination
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
stfloat (double dSource, char *pcDestination)
{
	float	fFloat;

	fFloat = dSource;
	memcpy (pcDestination, &fFloat, FLOATSIZE);
	return;
}

/*
 * Name:
 *	double	ldfltnull (char *pcLocation, short *piNullflag);
 * Arguments:
 *	char	*pcLocation
 *		The holding location
 *	short	*piNullflag
 *		Pointer to a null determining receiver
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
double
ldfltnull (char *pcLocation, short *piNullflag)
{
	double	dValue;

	*piNullflag = 0;
	dValue = ldfloat (pcLocation);
	return ((double) dValue);
}

/*
 * Name:
 *	void	stfltnull (double dSource, char *pcDestination, short iNullflag);
 * Arguments:
 *	double	dSource
 *		The double (promotoed from float) value to store
 *	char	*pcDestination
 *		The holding location
 *	short	iNullflag
 *		Null determinator
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
stfltnull (double dSource, char *pcDestination, short iNullflag)
{
	stfloat (dSource, pcDestination);
	return;
}

/*
 * Name:
 *	double	lddbl (char *pcLocation);
 * Arguments:
 *	char	*pcLocation
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
double
lddbl (char *pcLocation)
{
	double	dDouble;

	memcpy (&dDouble, pcLocation, DOUBLESIZE);
	return (dDouble);
}

/*
 * Name:
 *	void	stdbl (double dSource, char *pcDestination);
 * Arguments:
 *	double	dSource
 *		The (double) value to be stored
 *	char	*pcDestination
 *		The holding location
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
stdbl (double dSource, char *pcDestination)
{
	memcpy (pcDestination, &dSource, DOUBLESIZE);
	return;
}

/*
 * Name:
 *	double	lddblnull (char *pcLocation, short *piNullflag);
 * Arguments:
 *	char	*pcLocation
 *		The holding location
 *	short	*piNullflag
 *		Pointer to a null determining receiver
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
double
lddblnull (char *pcLocation, short *piNullflag)
{
	*piNullflag = 0;
	return (lddbl (pcLocation));
}

/*
 * Name:
 *	void	stdblnull (double dSource, char *pcDestination, short iNullflag);
 * Arguments:
 *	double	dSource
 *		The double value to store
 *	char	*pcDestination
 *		The holding location
 *	short	iNullflag
 *		Null determinator
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
stdblnull (double dSource, char *pcDestination, short iNullflag)
{
	stdbl (dSource, pcDestination);
}
