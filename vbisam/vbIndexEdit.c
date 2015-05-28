#include	"isinternal.h"
#include	<termio.h>

char	*gpcTableName,
	*gpcRowBuffer;
int	giCurrKey = 0,
	giHandle;

static	void
vSetup (int iMode)
{
static	struct	termios
		sTermIO,
		sNewTermIO;

	if (iMode)
	{
		tcgetattr (0, &sTermIO);
		tcgetattr (0, &sNewTermIO);
		sNewTermIO.c_iflag = BRKINT | IGNPAR | ISTRIP | IXANY;
		//sNewTermIO.c_oflag = ONLCR;
		sNewTermIO.c_lflag = 0;
		sNewTermIO.c_cc [VMIN] = 1;
		sNewTermIO.c_cc [VTIME] = 0;
		tcsetattr (0, TCSAFLUSH, &sNewTermIO);
	}
	else
	{
		tcsetattr (0, TCSAFLUSH, &sTermIO);
		printf ("\n");
	}
}

static	void
vClear (void)
{
	printf ("[H[2J");
	return;
}

static	void
vCurPos (int iRow, int iCol)
{
	printf ("[%02d;%02dH", iRow, iCol);
	return;
}

int
iGetKey (void)
{
	return (getchar ());
}

static	int
iGetHex (int iOldValue, int iNibbles)
{
	int	iChar,
		iMask1,
		iMask2,
		iNibble = iNibbles - 1,
		iValue = iOldValue;

	while (1)
	{
		iMask1 = 0x0f << (iNibble * 4);
		iMask2 = ~iMask1;
		iChar = iGetKey ();
		switch (iChar)
		{
		case	'0':
		case	'1':
		case	'2':
		case	'3':
		case	'4':
		case	'5':
		case	'6':
		case	'7':
		case	'8':
		case	'9':
			iValue &= iMask2;
			iValue |= (iChar - '0') << (iNibble * 4);
			putchar (iChar);
			if (!iNibble)
				return (iValue);
			iNibble--;
			break;

		case	'A':
		case	'B':
		case	'C':
		case	'D':
		case	'E':
		case	'F':
			iChar += ' ';
		case	'a':
		case	'b':
		case	'c':
		case	'd':
		case	'e':
		case	'f':
			iValue &= iMask2;
			iValue |= (iChar - 'W') << (iNibble * 4);
			putchar (iChar);
			if (!iNibble)
				return (iValue);
			iNibble--;
			break;

		case	0x08:	// Backspace
			if (iNibble + 1 < iNibbles)
			{
				iNibble++;
				putchar (iChar);
			}
			else
				putchar (7);
			break;

		case	' ':	// Space = advance
			if (!iNibble)
				return (iValue);
			iChar = (iValue & iMask1) >> (iNibble * 4);
			if (iChar < 10)
				putchar (iChar + '0');
			else
				putchar (iChar + 'W');
			iNibble--;
			break;

		case	0x0a:	// Enter
		case	0x0d:	// Return
			return (iValue);

		default:
			putchar (7);
			fflush (stdout);
			break;
		}
	}
}

static	void
vEditKey (off_t tNode, int iPrevLevel)
{
	unsigned char
		cKeyValue [VB_MAX_KEYLEN],
		*pcKeyValue;
	int	iCurrKey = 0,
		iDeleted,
		iLoop,
		iValue;
	struct	VBTREE
		sTree;

	memset (&sTree, 0, sizeof (sTree));
	while (1)
	{
		iVBNodeLoad (giHandle, giCurrKey, &sTree, tNode, iPrevLevel);
		vClear ();
		printf ("Node:         %08llx (%lld)\n", tNode, tNode);
		printf ("Level:              %02x (%d) %s\n", sTree.sFlags.iLevel, sTree.sFlags.iLevel, iPrevLevel == -1 ? "[Root]" : "");
		printf ("KeyCount:         %04x (%d)\n", sTree.sFlags.iKeysInNode - 1, sTree.sFlags.iKeysInNode - 1);
		printf ("Current key:      %04x (%d)\n", iCurrKey + 1, iCurrKey + 1);
		printf ("Node/Row#:    %08llx (%lld)\n", sTree.psKeyList [iCurrKey]->tRowNode, sTree.psKeyList [iCurrKey]->tRowNode);
		printf ("Duplicate #:  %08llx (%lld)\n", sTree.psKeyList [iCurrKey]->tDupNumber, sTree.psKeyList [iCurrKey]->tDupNumber);
		printf ("Key value:    ");
		pcKeyValue = sTree.psKeyList [iCurrKey]->cKey;
		for (iLoop = 0; iLoop < 8 && iLoop < psVBFile [giHandle]->psKeydesc [giCurrKey]->iKeyLength; iLoop++)
			printf ("%02x", *(pcKeyValue++));
		putchar ('\n');
		if (sTree.sFlags.iLevel == 0 && !iVBDataRead (giHandle, gpcRowBuffer, &iDeleted, sTree.psKeyList [iCurrKey]->tRowNode, TRUE))
		{
			if (iDeleted)
				printf ("Deleted!\n");
			else
			{
				vVBMakeKey (giHandle, giCurrKey, gpcRowBuffer, cKeyValue);
				printf ("Data value:   ");
				pcKeyValue = cKeyValue;
				for (iLoop = 0; iLoop < 8 && iLoop < psVBFile [giHandle]->psKeydesc [giCurrKey]->iKeyLength; iLoop++)
					printf ("%02x", *(pcKeyValue++));
				putchar ('\n');
			}
		}

		vCurPos (24, 1);
		printf ("D(escend) A(scend) N(ext) P(revious) F(irst) L(ast) G(oto)");
		switch (iGetKey ())
		{
		case	'D':
		case	'd':
			if (sTree.sFlags.iLevel)
				vEditKey (sTree.psKeyList [iCurrKey]->tRowNode, sTree.sFlags.iLevel);
			else
				putchar (7);
			break;
		case	'A':
		case	'a':
			vVBKeyAllFree (giHandle, giCurrKey, &sTree);
			return;

		case	'N':
		case	'n':
			if (iCurrKey < sTree.sFlags.iKeysInNode - 2)
				iCurrKey++;
			else
				putchar (7);
			break;

		case	'P':
		case	'p':
			if (iCurrKey > 0)
				iCurrKey--;
			else
				putchar (7);
			break;

		case	'F':
		case	'f':
			iCurrKey = 0;
			break;

		case	'L':
		case	'l':
			iCurrKey = sTree.sFlags.iKeysInNode - 2;
			break;

		case	'G':
		case	'g':
			vCurPos (4, 19);
			iValue = iGetHex (iCurrKey, 4);
			iValue--;
			if (iValue < 0 || iValue >= sTree.sFlags.iKeysInNode - 1)
				putchar (7);
			else
				iCurrKey = iValue;
			break;

		default:
			putchar (7);
			fflush (stdout);
			break;
		}
	}
}

static	void
vProcess (void)
{
	int	iValue;
	struct	dictinfo
		sDictInfo;

	while (1)
	{
		isindexinfo (giHandle, (struct keydesc *) &sDictInfo, 0);
		vClear ();
		printf ("Filename:     %s\n", gpcTableName);
		printf ("MinRowLength:     %04x (%d)\n", psVBFile [giHandle]->iMinRowLength, psVBFile [giHandle]->iMinRowLength);
		printf ("MaxRowLength:     %04x (%d)\n", psVBFile [giHandle]->iMaxRowLength, psVBFile [giHandle]->iMaxRowLength);
		printf ("RowCount:     %08llX (%lld)\n", sDictInfo.di_nrecords, sDictInfo.di_nrecords);
		printf ("KeyCount:           %02X (%d)\n", psVBFile [giHandle]->iNKeys, psVBFile [giHandle]->iNKeys);
		printf ("CurrentKey:         %02X (%d) [Rootnode=%08llx]\n", giCurrKey, giCurrKey, psVBFile [giHandle]->psKeydesc [giCurrKey]->tRootNode);
		vCurPos (24, 1);
		printf ("R(aw node edit) S(elect key) E(dit key) Q(uit)");
		switch (iGetKey ())
		{
		case	'R':
		case	'r':
			break;
		case	'S':
		case	's':
			vCurPos (6, 21);
			iValue = iGetHex (giCurrKey, 2);
			if (iValue < 0 || iValue >= psVBFile [giHandle]->iNKeys)
				putchar (7);
			else
				giCurrKey = iValue;
			break;

		case	'E':
		case	'e':
			vEditKey (psVBFile [giHandle]->psKeydesc [giCurrKey]->tRootNode, -1);
			break;
		case	'Q':
		case	'q':
			return;
		default:
			putchar (7);
			fflush (stdout);
			break;
		}
	}
}

int
main (int iArgc, char **ppcArgv)
{
	if (iArgc != 2)
	{
		printf ("Usage: %s ISAM-TABLE-NAME\n", ppcArgv [0]);
		exit (1);
	}
	gpcTableName = strdup (ppcArgv [1]);
	giHandle = isopen (gpcTableName, ISINOUT|ISMANULOCK);
	if (giHandle < 0)
	{
		printf ("Could not open table %s\n", ppcArgv [1]);
		exit (1);
	}
	iVBFileOpenLock (giHandle, 0);	// Free the file-open lock!
	vSetup (1);	// Initialize
	gpcRowBuffer = malloc (psVBFile [giHandle]->iMaxRowLength + 1);
	vProcess ();
	vSetup (0);	// De-initialize
	isclose (giHandle);
	free (gpcRowBuffer);
	free (gpcTableName);
	return (0);
}
