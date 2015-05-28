/*
 * Title:	isaudit.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	11Dec2003
 * Description:
 *	This is the module that deals with all the auditting component within
 *	the VBISAM library.
 * Version:
 *	$Id: isaudit.c,v 1.2 2004/01/05 07:36:17 trev_vb Exp $
 * Modification History:
 *	$Log: isaudit.c,v $
 *	Revision 1.2  2004/01/05 07:36:17  trev_vb
 *	TvB 05Feb2002 Added licensing et al as Johann v. N. noted I'd overlooked it
 *	
 *	Revision 1.1  2004/01/03 02:28:48  trev_vb
 *	TvB 02Jan2004 WAY too many changes to enumerate!
 *	TvB 02Jan2003 Transaction processing done (excluding iscluster)
 *	
 */
#include	"isinternal.h"
#include	<time.h>

/*
 * Prototypes
 */
int	isaudit (int, char *, int);

/*
 * Name:
 *	int	isaudit (int iHandle, char *pcFilename, int iMode);
 * Arguments:
 *	int	iHandle
 *		The open file descriptor of the VBISAM file (Not the dat file)
 *	char	*pcFilename
 *		The name of the audit file
 *	int	iMode
 *		AUDSETNAME
 *		AUDGETNAME
 *		AUDSTART
 *		AUDSTOP
 *		AUDINFO
 *		AUDRECVR	BUG? Huh?
 * Prerequisites:
 *	NONE
 * Returns:
 *	-1	Failure (iserrno contains more info)
 *	0	Success
 * Problems:
 *	NONE known
 */
int	isaudit (int iHandle, char *pcFilename, int iMode)
{
	// BUG - Write isaudit
	return (0);
}
