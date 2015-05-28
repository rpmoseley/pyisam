/*
 * Title:	MVTest.c
 * Copyright:	(C) 2004 Mikhail Verkhovski
 * License:	LGPL - See COPYING.LIB
 * Author:	Mikhail Verkhovski
 * Created:	??May2004
 * Description:
 *	This module tests a bunch of the features of VBISAM
 * Version:
 *	$Id: vbRecover.c,v 1.2 2004/06/06 20:59:21 trev_vb Exp $
 * Modification History:
 *	$Log: vbRecover.c,v $
 *	Revision 1.2  2004/06/06 20:59:21  trev_vb
 *	06Jun2004 TvB Grrr, had a 'wee issue' in vbRecover... Sorry!
 *	
 *	Revision 1.1  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	TvB 30May2004 Many thanks go out to MV for writing this test program
 */
#include	<stdio.h>
#include	"vbisam.h"

int
main (int iArgc, char **ppcArgv)
{
	int	iResult;
	char	cLogfileName [100];

	if (iArgc != 2)
	{
		printf ("Usage: %s <LOGFILE>\n", ppcArgv [0]);
		exit (1);
	}
	sprintf (cLogfileName, "%s", ppcArgv [1]);
	iResult = islogopen (cLogfileName);
	if (iResult < 0)
	{
		fprintf (stdout, "Error opening log: %d\n", iserrno);
		exit (-1);
	}
	printf ("Recovering... Please wait\n");
	iResult = isrecover ();
	if (iResult)
		printf ("Recovery failed with error %d\n", iserrno);
	else
		printf ("Recovery SUCCESSFUL!\n");
	islogclose ();

	return (0);
}
