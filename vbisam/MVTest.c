/*
 * Title:	MVTest.c
 * Copyright:	(C) 2004 Mikhail Verkhovski
 * License:	LGPL - See COPYING.LIB
 * Author:	Mikhail Verkhovski
 * Created:	??May2004
 * Description:
 *	This module tests a bunch of the features of VBISAM
 * Version:
 *	$Id: MVTest.c,v 1.4 2004/06/16 10:53:55 trev_vb Exp $
 * Modification History:
 *	$Log: MVTest.c,v $
 *	Revision 1.4  2004/06/16 10:53:55  trev_vb
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
 *	TvB 30May2004 Many thanks go out to MV for writing this test program
 */
#include	<time.h>
#include	<sys/types.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	"vbisam.h"
#include	"isinternal.h"

int	iVBRdCount = 0,
	iVBWrCount = 0,
	iVBDlCount = 0,
	iVBUpCount = 0;
int
main (int iArgc, char **ppcArgv)
{
	int	iResult,
		iLoop,
		iLoop2,
		iLoop3,
		iHandle;
	unsigned char
		cRecord [256];
	struct	keydesc
		sKeydesc;
	char	cLogfileName [100],
		cCommand [100];
	char	cFileName [] = "IsamTest";

	memset (&sKeydesc, 0, sizeof (sKeydesc));
	sKeydesc.k_flags = COMPRESS;
	sKeydesc.k_nparts = 1;
	sKeydesc.k_start = 0;
	sKeydesc.k_leng = 2;
	sKeydesc.k_type = CHARTYPE;

	if (iArgc == 1)
	{
		printf ("Usage:\n\t%s create\nOR\n\t%s <#iterations>\n", ppcArgv [0], ppcArgv [0]);
		exit (1);
	}

	if (iArgc > 1 && strcmp (ppcArgv [1], "create") == 0)
	{
		iserase (cFileName);
		iHandle = isbuild (cFileName, 255, &sKeydesc, ISINOUT+ISFIXLEN+ISEXCLLOCK);
		if (iHandle < 0)
		{
			fprintf (stdout, "Error creating database: %d\n", iserrno);
			exit (-1);
		}
		sKeydesc.k_flags |= ISDUPS;
		//for (sKeydesc.k_start = 1; sKeydesc.k_start < MAXSUBS; sKeydesc.k_start++)
		for (sKeydesc.k_start = 1; sKeydesc.k_start < 2; sKeydesc.k_start++)
			if (isaddindex (iHandle, &sKeydesc))
				printf ("Error adding index %d\n", sKeydesc.k_start);
		isclose (iHandle);
		return (0);
	}
	// The following is sort of cheating as it *assumes* we're running *nix
	// However, I have to admit to liking the fact that this will FAIL when
	// using WynDoze (TvB)
	sprintf (cLogfileName, "RECOVER");
	sprintf (cCommand, "rm -f %s; touch %s", cLogfileName, cLogfileName);
	system (cCommand);
	iResult = islogopen (cLogfileName);
	if (iResult < 0)
	{
		fprintf (stdout, "Error opening log: %d\n", iserrno);
		exit (-1);
	}

	srand (time (NULL));
	for (iLoop = 0; iLoop < atoi (ppcArgv [1]); iLoop++)
	{
		if (!(iLoop % 100))
			printf ("iLoop=%d\n", iLoop);

		iResult = isbegin ();
		if (iResult < 0)
		{
			fprintf (stdout, "Error begin transaction: %d\n", iserrno);
			exit (-1);
		}
		iHandle = isopen (cFileName, ISINOUT+ISFIXLEN+ISTRANS+ISAUTOLOCK);
		if (iHandle < 0)
		{
			fprintf (stdout, "Error opening database: %d\n", iserrno);
			exit (-1);
		}

		for (iLoop2 = 0; iLoop2 < 100; iLoop2++)
		{
			for (iLoop3 = 0; iLoop3 < 256; iLoop3++)
				cRecord [iLoop3] = rand () % 256;

			switch (rand () % 4)
			{
			case	0:
				if ((iResult = iswrite (iHandle, (char *) cRecord)) != 0)
				{
					if (iserrno != EDUPL && iserrno != ELOCKED)
					{
						fprintf (stdout, "Error writing: %d\n", iserrno);
						goto err;
					}
				}
				else
					iVBWrCount++;
				break;

			case	1:
				if ((iResult = isread (iHandle, (char *)cRecord, ISEQUAL)) != 0)
				{
					if (iserrno == ELOCKED)
						; //fprintf (stdout, "Locked during deletion\n");
					else if (iserrno != ENOREC)
					{
						fprintf (stdout, "Error reading: %d\n", iserrno);
						goto err;
					}
				}
				else
					iVBRdCount++;
				break;

			case	2:
				for (iLoop3 = 0; iLoop3 < 256; iLoop3++)
					cRecord [iLoop3] = rand () % 256;
				if ((iResult = isrewrite (iHandle, (char *)cRecord)) != 0)
				{
					if (iserrno == ELOCKED)
						; //fprintf (stdout, "Locked during rewrite\n");
					else if (iserrno != ENOREC)
					{
						fprintf (stdout, "Error rewriting: %d\n", iserrno);
						goto err;
					}
				}
				else
					iVBUpCount++;
				break;

			case	3:
				if ((iResult = isdelete (iHandle, (char *)cRecord)) != 0)
				{
					if (iserrno == ELOCKED)
						; //fprintf (stdout, "Locked during deletion\n");
					else if (iserrno != ENOREC)
					{
						fprintf (stdout, "Error deleting: %d\n", iserrno);
						goto err;
					}
				}
				else
					iVBDlCount++;
				break;
			}
		}

		iResult = isflush (iHandle);
		if (iResult < 0)
		{
			fprintf (stdout, "Error flush: %d\n", iserrno);
			exit (-1);
		}
		iResult = isclose (iHandle);
		if (iResult < 0)
		{
			fprintf (stdout, "Error closing database: %d\n", iserrno);
			exit (-1);
		}

		switch (rand () % 2)
		{
		case	0:
			iResult = iscommit ();
			if (iResult < 0)
			{
				fprintf (stdout, "Error commit: %d\n", iserrno);
				exit (-1);
			}
			break;

		case	1:
			iResult = isrollback ();
			if (iResult < 0)
			{
				fprintf (stdout, "Error rollback: %d\n", iserrno);
				exit (-1);
			}
			break;
		}
	}
err:
	printf ("Note:\n\tThe following figures include those that were rolled back!\n\tTherefore, you cannot sum them to match the file content!\n");
	printf ("Write  Count: %d\n", iVBWrCount);
	printf ("Read   Count: %d\n", iVBRdCount);
	printf ("Delete Count: %d\n", iVBDlCount);
	printf ("Update Count: %d\n", iVBUpCount);
	return (iResult);
}
