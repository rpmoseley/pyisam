/*
 * Title:	CvtTo64.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	11Dec2003
 * Description:
 *	This program simply converts C-ISAM(R) files into VBISAM files.
 * Version:
 *	$Id: CvtTo64.c,v 1.4 2004/06/11 22:16:16 trev_vb Exp $
 * Modification History:
 *	$Log: CvtTo64.c,v $
 *	Revision 1.4  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.3  2004/01/05 07:36:17  trev_vb
 *	TvB 05Feb2002 Added licensing et al as Johann v. N. noted I'd overlooked it
 *	
 *	Revision 1.2  2003/12/22 04:44:30  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:22  trev_vb
 *	Init import
 *	
 */
#define	VBISAM_LIB
#if	ISAMMODE != 1
#error LOGIC ERROR This program is intended to be compiled *ONLY* in VBISAM mode
#endif	// ISAMMODE != 1
#include	"vbisam.h"

char	*pcBuffer;
int	iKeyCount,
	iRowLength,
	iSrcNodeLength;
long	lMaxRow,
	lUniqueID;
struct	keydesc
	sKeydesc [MAXSUBS];

int
iSetupCISAM (char *pcFilename)
{
	char	cBuffer [128],
		*pcPtr,
		*pcSrcFilename;
	int	iHandle = -1,
		iKeyLength,
		iKeyNumber = 0,
		iNodeUsed,
		iTypeError = 0;
	long	lKeydescNode;

	pcPtr = strrchr (pcFilename, '.');
	if (pcPtr && !strcmp (pcPtr, ".idx"))
		*pcPtr = 0;
	if (pcPtr && !strcmp (pcPtr, ".dat"))
		*pcPtr = 0;
	pcSrcFilename = malloc (strlen (pcFilename) + 5);
	if (!pcSrcFilename)
	{
		fprintf (stderr, "malloc error\n");
		return (-1);
	}
	sprintf (pcSrcFilename, "%s.idx", pcFilename);
	iHandle = open (pcSrcFilename, O_RDONLY);
	if (iHandle < 0)
	{
		fprintf (stderr, "Cannot open index of %s!\n", pcFilename);
		free (pcSrcFilename);
		return (-1);
	}
	if (read (iHandle, cBuffer, 121) != 121)
	{
		fprintf (stderr, "Cannot read dictionary of %s!\n", pcFilename);
		free (pcSrcFilename);
		close (iHandle);
		return (-1);
	}
	iSrcNodeLength = ldint (cBuffer + 6) + 1;
	iRowLength = ldint (cBuffer + 13) + 1;	// Includes deleted marker
	lMaxRow = ldlong (cBuffer + 33);
	iKeyCount = ldint (cBuffer + 8);
	lUniqueID = ldlong (cBuffer + 45) - 1;
	if (iKeyCount > MAXSUBS)
	{
		fprintf (stderr, "%d keys in %s exceeds max of %d\n", iKeyCount, pcFilename, MAXSUBS);
		free (pcSrcFilename);
		close (iHandle);
		return (-1);
	}
	// Allocate a dual-use row / node buffer
	if (iRowLength < iSrcNodeLength)
		pcBuffer = malloc (iSrcNodeLength);
	else
		pcBuffer = malloc (iRowLength);
	if (!pcBuffer)
	{
		fprintf (stderr, "malloc error\n");
		free (pcSrcFilename);
		close (iHandle);
		return (-1);
	}
	lKeydescNode = ldlong (cBuffer + 15);
	// Build the keydesc stuff
	while (lKeydescNode)
	{
		if (lseek (iHandle, (lKeydescNode - 1) * iSrcNodeLength, SEEK_SET) != (lKeydescNode - 1) * iSrcNodeLength)
		{
			fprintf (stderr, "Error lseeking to node %ld of %s\n", lKeydescNode, pcFilename);
			free (pcSrcFilename);
			free (pcBuffer);
			close (iHandle);
			return (-1);
		}
		if (read (iHandle, pcBuffer, iSrcNodeLength) != iSrcNodeLength)
		{
			fprintf (stderr, "Error reading node %ld of %s\n", lKeydescNode, pcFilename);
			free (pcSrcFilename);
			free (pcBuffer);
			close (iHandle);
			return (-1);
		}
		iNodeUsed = ldint (pcBuffer);
		pcPtr = pcBuffer + 6;
		sKeydesc [iKeyNumber].iNParts = 0;
		while (pcPtr - pcBuffer < iNodeUsed)
		{
			iKeyLength = ldint (pcPtr) - 7;
			sKeydesc [iKeyNumber].iFlags = (*(pcPtr + 6)) * 2;
			pcPtr += 7;
			while (iKeyLength > 0)
			{
				if (*pcPtr & 0x80)
					sKeydesc [iKeyNumber].iFlags |= ISDUPS;
				*pcPtr &= ~ 0x80;
				sKeydesc [iKeyNumber].sPart [sKeydesc [iKeyNumber].iNParts].iLength = ldint (pcPtr);
				sKeydesc [iKeyNumber].sPart [sKeydesc [iKeyNumber].iNParts].iStart = (off_t) ldint (pcPtr + 2);
				// BUG - Does VBISAM support *ALL* CISAM types?
				sKeydesc [iKeyNumber].sPart [sKeydesc [iKeyNumber].iNParts].iType = *(pcPtr + 4);
				if ((*(pcPtr + 4) & (~0x80)) > 5)
					iTypeError = iKeyNumber + 1;
				pcPtr += 5;
				iKeyLength -= 5;
				sKeydesc [iKeyNumber].iNParts++;
			}
			iKeyNumber++;
		}
		lKeydescNode = ldlong (pcBuffer + 2);
	}
	close (iHandle);
	if (iTypeError)
	{
		fprintf (stderr, "Unknown data type in key %d of %s\n", iTypeError - 1, pcFilename);
		free (pcSrcFilename);
		free (pcBuffer);
		return (-1);
	}
	sprintf (pcSrcFilename, "%s.dat", pcFilename);
	iHandle = open (pcSrcFilename, O_RDONLY);
	free (pcSrcFilename);
	if (iHandle < 0)
	{
		fprintf (stderr, "Cannot open data of %s!\n", pcFilename);
		free (pcBuffer);
		return (-1);
	}
	return (iHandle);
}

int
iSetupVBISAM (char *pcFilename)
{
	int	iHandle,
		iLoop;

	iserase (pcFilename);	// Just to be sure
	iHandle = isbuild (pcFilename, iRowLength - 1, &sKeydesc [0], ISINOUT + ISEXCLLOCK);
	if (iHandle == -1)
	{
		fprintf (stderr, "Couldn't build %s\n", pcFilename);
		return (-1);
	}
	iLoop = issetunique (iHandle, lUniqueID);
	if (iLoop < 0)
	{
		fprintf (stderr, "Couldn't set UniqueID on %s\n", pcFilename);
		return (-1);
	}
	for (iLoop = 1; iLoop < iKeyCount; iLoop++)
	{
		if (isaddindex (iHandle, &sKeydesc [iLoop]))
		{
			fprintf (stderr, "Couldn't add key %d to %s [%d]\n", iLoop, pcFilename, iserrno);
			isclose (iHandle);
			return (-1);
		}
	}
	return (iHandle);
}

int
iProcessRows (char *pcFilename, int iCHandle, int iVBHandle)
{
	long	lLoop,
		lOffset;

	for (lLoop = 0; lLoop < lMaxRow; lLoop++)
	{
		lOffset = lLoop * iRowLength;
		if (lseek (iCHandle, lOffset, SEEK_SET) != lOffset)
		{
			fprintf (stderr, "Couldn't lseek row %ld on %s\n", lLoop, pcFilename);
			isclose (iVBHandle);
			close (iCHandle);
			return (1);
		}
		if (read (iCHandle, pcBuffer, iRowLength) != iRowLength)
		{
			fprintf (stderr, "Couldn't read row %ld on %s\n", lLoop, pcFilename);
			isclose (iVBHandle);
			close (iCHandle);
			return (1);
		}
		if (*(pcBuffer + iRowLength - 1) != 0x0a)
			continue;
		if (iswrite (iVBHandle, pcBuffer))
		{
			fprintf (stderr, "Couldn't write row %ld on %s [%d]\n", lLoop, pcFilename, iserrno);
			isclose (iVBHandle);
			close (iCHandle);
			return (1);
		}
	}
	isclose (iVBHandle);
	close (iCHandle);
	return (0);
}

void
vConvert (char *pcFilename)
{
	char	*pcOldname;
	int	iCHandle,
		iVBHandle;

	pcOldname = malloc (strlen (pcFilename) + 4);
	if (!pcOldname)
	{
		fprintf (stderr, "malloc error\n");
		return;
	}

	iCHandle = iSetupCISAM (pcFilename);
	if (iCHandle == -1)
		return;

	iVBHandle = iSetupVBISAM ("VBISAMtmp");
	if (iVBHandle == -1)
		return;

	sprintf (pcOldname, "OLD%s", pcFilename);
	if (!iProcessRows (pcFilename, iCHandle, iVBHandle))
	{
		isrename (pcFilename, pcOldname);
		isrename ("VBISAMtmp", pcFilename);
	}
	return;
}

int
main (int iArgc, char **ppcArgv)
{
	int	iLoop;

	if (iArgc < 2)
	{
		printf ("Usage: %s <CISAM_FILENAME ...>\n", ppcArgv [0]);
		exit (1);
	}
	for (iLoop = 1; ppcArgv [iLoop]; iLoop++)
	{
		fprintf (stderr, "Converting: %s\n", ppcArgv [iLoop]);
		memset (sKeydesc, 0, MAXSUBS * sizeof (struct keydesc));
		vConvert (ppcArgv [iLoop]);
	}
	return (0);
}
