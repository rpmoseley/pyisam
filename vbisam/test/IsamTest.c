/*
 * Title:	IsamTest.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	19Dec2003
 * Description:
 *	This program simply reads through *EVERY* single CISAM data file that
 *	exists in an Informix SQL / Informix SE database located in the
 *	directory passed on the command line.
 *	It makes the assumption that the systables.nrows column is up-to-date.
 *	This can be achieved by way of an SQL 'update statistics;' command.
 * Version:
 *	$Id: IsamTest.c,v 1.1 2004/06/06 20:56:42 trev_vb Exp $
 * Modification History:
 *	$Log: IsamTest.c,v $
 *	Revision 1.1  2004/06/06 20:56:42  trev_vb
 *	06Jun2004 TvB Forgot to 'cvs add' a few files in subdirectories
 *	
 *	Revision 1.4  2004/01/05 07:36:17  trev_vb
 *	TvB 05Feb2002 Added licensing et al as Johann v. N. noted I'd overlooked it
 *	
 *	Revision 1.3  2004/01/03 07:15:49  trev_vb
 *	TvB 02Jan2004 Added the ability to specify a single table on the commandline
 *	
 *	Revision 1.2  2003/12/22 04:45:03  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:18  trev_vb
 *	Init import
 *	
 */
#include	"isinternal.h"

/*
 * Prototypes
 */

/*
 * Global variables (Ugh!)
 */
char	cRowBuffer [32768];	// Should be large enough huh?
int	iDBHandle;		// The handle of the systables system catalog

int
iOpenDatabase (char *pcDatabaseDir)
{
	sprintf (cRowBuffer, "%s/systables", pcDatabaseDir);
	if (isopen (cRowBuffer, ISINOUT + ISMANULOCK))
	{
		fprintf (stderr, "Error %d opening system catalog\n", iserrno);
		return (1);
	}
	return (0);
}

int
iCloseDatabase (void)
{
	if (isclose (iDBHandle))
	{
		fprintf (stderr, "Error %d closing system catalog\n", iserrno);
		return (1);
	}
	return (0);
}

void
vProcessRows (int iHandle, long lNRows, int iIndexCount)
{
	int	iIndexNumber;
	long	lRow;
	struct	keydesc
		sKeydesc;

	/*
	 * Step 1:
	 *	Check index count
	 *	Possibly skewed by 1 due to a null primary index
	 */
	if (psVBFile [iHandle]->psKeydesc [0]->iNParts == 0)
		iIndexCount++;
	if (!psVBFile [iHandle]->psKeydesc [iIndexCount - 1])
		fprintf (stderr, "\tLess indexes than systables\n");
	if (psVBFile [iHandle]->psKeydesc [iIndexCount])
		fprintf (stderr, "\tMore indexes than systables\n");
	for (iIndexNumber = 0; psVBFile [iHandle]->psKeydesc [iIndexNumber]; iIndexNumber++)
	{
		memcpy ((void *) &sKeydesc, psVBFile [iHandle]->psKeydesc [iIndexNumber], sizeof (struct keydesc));
		if (isstart (iHandle, &sKeydesc, 0, (char *) 0, ISFIRST))
		{
			fprintf (stderr, "\tError on isstart() of index %d\n", iIndexNumber);
			continue;
		}
		for (lRow = 0; lRow < lNRows; lRow++)
		{
			if (isread (iHandle, cRowBuffer, ISNEXT))
			{
				fprintf (stderr, "\tFailed with error %d reading row %ld of %ld using index %d\n", iserrno, lRow, lNRows, iIndexNumber);
				break;
			}
		}
		if (!isread (iHandle, cRowBuffer, ISNEXT))
		{
			fprintf (stderr, "\tRead additional row %ld beyond %ld using index %d\n", isrecnum, lNRows, iIndexNumber);
			break;
		}
	}
}

void
vProcessTable (char *pcDatabaseDir)
{
	char	cDirPath [64 + 1],
		cTableName [18 + 1];
	int	iHandle,
		iIndexCount;
	long	lNRows;

	if (cRowBuffer [18 + 8 + 64 + LONGSIZE + INTSIZE + INTSIZE + INTSIZE + LONGSIZE + LONGSIZE + LONGSIZE] != 'T')
		return;
	ldchar (cRowBuffer, 18, cTableName);
	ldchar (cRowBuffer + 18 + 8, 64, cDirPath);
	iIndexCount = ldint (cRowBuffer + 18 + 8 + 64 + LONGSIZE + INTSIZE + INTSIZE);
	lNRows = ldlong (cRowBuffer + 18 + 8 + 64 + LONGSIZE + INTSIZE + INTSIZE + INTSIZE);
	if (cDirPath [0] == '/')
		sprintf (cRowBuffer, "%s", cDirPath);
	else
		sprintf (cRowBuffer, "%s/%s", pcDatabaseDir, cDirPath);
	iHandle = isopen (cRowBuffer, ISINOUT + ISMANULOCK);
	if (iHandle < 0)
	{
		fprintf (stderr, "Error %d opening table %s\n", iserrno, cTableName);
		fprintf (stderr, "\tDirPath=%s\n", cDirPath);
		fprintf (stderr, "\tFullName=%s\n", cRowBuffer);
		return;
	}
	fprintf (stderr, "Processing table %s\n", cTableName);
	vProcessRows (iHandle, lNRows, iIndexCount);
	if (isclose (iHandle))
		fprintf (stderr, "Error %d closing handle for %s\n", iserrno, cTableName);
	return;
}

void
vProcessDatabase (char *pcDatabaseDir, char *pcTableName)
{
	if (!pcTableName)
	{
		while (isread (iDBHandle, cRowBuffer, ISNEXT) == 0)
			vProcessTable (pcDatabaseDir);
		if (iserrno != EENDFILE)
			fprintf (stderr, "Last read of the system catalog returned %d instead of EENDFILE\n", iserrno);
		return;
	}
	memset (cRowBuffer, 0, 177);
	stchar (pcTableName, cRowBuffer, 18);
	if (isread (iDBHandle, cRowBuffer, ISGTEQ))
	{
		fprintf (stderr, "Could not locate table %s! Error %d\n", pcTableName, iserrno);
		return;
	}
	if (memcmp (pcTableName, cRowBuffer, strlen (pcTableName)))
	{
		fprintf (stderr, "Could not locate table %s!\n", pcTableName);
		return;
	}
	vProcessTable (pcDatabaseDir);
	return;
}

int
main (int iArgc, char **ppcArgv)
{
	if (iArgc < 2 || iArgc > 3)
	{
		fprintf (stderr, "Usage: %s <DATABASE_DIR> [TABLE]\n", ppcArgv [0]);
		exit (1);
	}
	if (iOpenDatabase (ppcArgv [1]))
		exit (2);
	if (iArgc == 2)
		vProcessDatabase (ppcArgv [1], (char *) 0);
	else
		vProcessDatabase (ppcArgv [1], ppcArgv [2]);
	if (iCloseDatabase ())
		exit (3);

	return (0);
}
