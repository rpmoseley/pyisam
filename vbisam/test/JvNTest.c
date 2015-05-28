/*
 * Title:	JvNTest.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	17Nov2003
 * Description:
 *	This is a simple stub test program that emulates Johann vN's test suite.
 *	It tries to eliminate any other 'clutter' from confusing me. :)
 *	It is dependant on the JvN supplied test data file that I have renamed
 *	to 'src'.
 * Version:
 *	$Id: JvNTest.c,v 1.1 2004/06/06 20:56:42 trev_vb Exp $
 * Modification History:
 *	$Log: JvNTest.c,v $
 *	Revision 1.1  2004/06/06 20:56:42  trev_vb
 *	06Jun2004 TvB Forgot to 'cvs add' a few files in subdirectories
 *	
 *	Revision 1.1  2004/03/23 15:13:19  trev_vb
 *	TvB 23Mar2004 Changes made to fix bugs highlighted by JvN's test suite.  Many thanks go out to JvN for highlighting my obvious mistakes.
 *	
 */
#include	<stdio.h>
#include	"vbisam.h"

#define	FILE_COUNT	1
#define	INDX_COUNT	2

struct	keydesc
	gsKey [10] =
	{
		{ISNODUPS,	1,  {{0,  8, 0}},},
		{ISDUPS,	1, {{42, 14, 0}},},
	};

int
main (int iArgc, char **ppcArgv)
{
	char	cBuffer [1024],
		cName [32];
	int	iHandle [10],
		iLoop,
		iLoop1,
		iResult;
	FILE	*psHandle;

	for (iLoop = 0; iLoop < INDX_COUNT; iLoop++)
	{
		gsKey [iLoop].k_len = 0;
		for (iLoop1 = 0; iLoop1 < gsKey [iLoop].k_nparts; iLoop1++)
			gsKey [iLoop].k_len += gsKey [iLoop].k_part [iLoop1].kp_leng;
	}
	for (iLoop = 0; iLoop < FILE_COUNT; iLoop++)
	{
		sprintf (cName, "File%d", iLoop);
		iserase (cName);
		iHandle [iLoop] = isbuild (cName, 170, &gsKey [0], ISINOUT + ISEXCLLOCK);
		if (iHandle [iLoop] < 0)
		{
			printf ("isbuild error %d for %s file\n", iserrno, cName);
			exit (1);
		}
	}
	for (iLoop = 0; iLoop < FILE_COUNT; iLoop++)
	{
		for (iLoop1 = 1; iLoop1 < INDX_COUNT; iLoop1++)
		{
			iResult = isaddindex (iHandle [iLoop], &gsKey [iLoop1]);
			if (iResult)
			{
				printf ("isaddindex error %d on handle %d index %d\n", iserrno, iLoop, iLoop1);
				exit (1);
			}
		}
	}
	psHandle = fopen ("src", "r");
	if (psHandle == (FILE *) 0)
	{
		printf ("Error opening source file!\n");
		exit (1);
	}
	iLoop1 = 0;
	while (fgets (cBuffer, 1024, psHandle) != NULL)
	{
if (iLoop1 == 0) printf ("[%-8.8s] [%-14.14s]\n", cBuffer, cBuffer + 42);
		iLoop1++;
		cBuffer [170] = 0;
		for (iLoop = 0; iLoop < FILE_COUNT; iLoop++)
			if (iswrite (iHandle [iLoop], cBuffer))
			{
				printf ("Error %d writing row %d to file %d\n", iserrno, iLoop1, iLoop);
				exit (1);
			}
			if (iLoop1 > atoi (ppcArgv [1]))
				break;
	}
	fclose (psHandle);
	//for (iLoop1 = 0; iLoop1 < atoi (ppcArgv [1]); iLoop1++)
	//{
		//sprintf (cBuffer, "%-42.42s%010ld%-76.76s", " ", random(), " ");
		//for (iLoop = 0; iLoop < FILE_COUNT; iLoop++)
			//if (iswrite (iHandle [iLoop], cBuffer))
			//{
				//printf ("Error %d writing row %d to file %d\n", iserrno, iLoop1, iLoop);
				//exit (1);
			//}
	//}
isclose (iHandle [0]); iHandle [0] = isopen (cName, ISINOUT | ISEXCLLOCK);
	for (iLoop = 0; iLoop < 2; iLoop++)
	{
		iResult = isread (iHandle [0], cBuffer, ISFIRST);
		if (iResult)
		{
			printf ("Error on isread ()!\n");
			exit (1);
		}
printf ("[%-8.8s] [%-14.14s]\n", cBuffer, cBuffer + 42);
		iResult = isdelcurr (iHandle [0]);
		if (iResult)
		{
			printf ("Error on isdelcurr ()!\n");
			exit (1);
		}
isclose (iHandle [0]); iHandle [0] = isopen (cName, ISINOUT | ISEXCLLOCK);
	}
	for (iLoop = 0; iLoop < FILE_COUNT; iLoop++)
		isclose (iHandle [iLoop]);
	return (0);
}
