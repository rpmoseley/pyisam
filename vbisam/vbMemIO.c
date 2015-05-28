/*
 * Title:	vbMemIO.c
 * Copyright:	(C) 2003 Trevor van Bremen
 * License:	LGPL - See COPYING.LIB
 * Author:	Trevor van Bremen
 * Created:	08Dec2003
 * Description:
 *	This is the module that deals with *ALL* memory (de-)allocation for the
 *	VBISAM library.
 * Version:
 *	$Id: vbMemIO.c,v 1.8 2004/06/11 22:16:16 trev_vb Exp $
 * Modification History:
 *	$Log: vbMemIO.c,v $
 *	Revision 1.8  2004/06/11 22:16:16  trev_vb
 *	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
 *	checkin that will not be immediately made into a release.
 *	
 *	Revision 1.7  2004/06/06 20:52:21  trev_vb
 *	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
 *	
 *	Revision 1.6  2004/01/06 14:31:59  trev_vb
 *	TvB 06Jan2004 Added in VARLEN processing (In a fairly unstable sorta way)
 *	
 *	Revision 1.5  2004/01/05 07:36:17  trev_vb
 *	TvB 05Feb2002 Added licensing et al as Johann v. N. noted I'd overlooked it
 *	
 *	Revision 1.4  2004/01/03 07:28:54  trev_vb
 *	TvB 02Jan2004 Oooops, I forgot to vVBFree the pcWriteBuffer
 *	
 *	Revision 1.3  2004/01/03 02:28:48  trev_vb
 *	TvB 02Jan2004 WAY too many changes to enumerate!
 *	TvB 02Jan2003 Transaction processing done (excluding iscluster)
 *	
 *	Revision 1.2  2003/12/22 04:49:30  trev_vb
 *	TvB 21Dec2003 Modified header to correct case ('Id')
 *	
 *	Revision 1.1.1.1  2003/12/20 20:11:20  trev_vb
 *	Init import
 */
#define	VBISAMMAIN
#include	"isinternal.h"
#include	<assert.h>

static	int	iCurrHandle = -1;

/*
 * Prototypes
 */
struct	VBLOCK *psVBLockAllocate (int);
void	vVBLockFree (struct VBLOCK *);
struct	VBTREE	*psVBTreeAllocate (int);
void	vVBTreeAllFree (int, int, struct VBTREE *);
struct	VBKEY *psVBKeyAllocate (int, int);
void	vVBKeyAllFree (int, int, struct VBTREE *);
void	vVBKeyFree (int, int, struct VBKEY *);
void	vVBKeyUnMalloc (int, int);
void	*pvVBMalloc (size_t);
void	vVBFree (void *, size_t);
void	vVBUnMalloc (void);
#ifdef	DEBUG
void	vVBMallocReport (void);
#endif	// DEBUG

static	size_t
	tMallocUsed = 0,
	tMallocMax = 0;
static	struct	VBLOCK
	*psLockFree = VBLOCK_NULL;
static	struct	VBTREE
	*psTreeFree = VBTREE_NULL;

/*
 * Name:
 *	int	psVBLockAllocate (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The corresponding VBISAM file handle
 * Prerequisites:
 *	NONE
 * Returns:
 *	VBLOCK_NULL Ran out out memory
 *	OTHER Pointer to the allocated structure
 * Problems:
 *	NONE known
 */
struct	VBLOCK	*
psVBLockAllocate (int iHandle)
{
	struct	VBLOCK
		*psLock = psLockFree;

	iCurrHandle = iHandle;
	if (psLockFree != VBLOCK_NULL)
		psLockFree = psLockFree->psNext;
	else
		psLock = (struct VBLOCK *) pvVBMalloc (sizeof (struct VBLOCK));
	iCurrHandle = -1;
	if (psLock)
		memset (psLock, 0, sizeof (struct VBLOCK));
	return (psLock);
}

/*
 * Name:
 *	void	vVBLockFree (struct VBLOCK *psLock);
 * Arguments:
 *	struct	VBLOCK	*psLock
 *		A previously allocated lock structure
 * Prerequisites:
 *	NONE
 * Problems:
 *	NONE known
 */
void
vVBLockFree (struct VBLOCK *psLock)
{
	psLock->psNext = psLockFree;
	psLockFree = psLock;
	return;
}

/*
 * Name:
 *	struct VBTREE	*psVBTreeAllocate (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The corresponding VBISAM file handle
 * Prerequisites:
 *	NONE
 * Returns:
 *	VBTREE_NULL Ran out out memory
 *	OTHER Pointer to the allocated structure
 * Problems:
 *	NONE known
 */
struct	VBTREE	*
psVBTreeAllocate (int iHandle)
{
	struct	VBTREE
		*psTree = psTreeFree;

	iCurrHandle = iHandle;
	if (psTreeFree == VBTREE_NULL)
		psTree = (struct VBTREE *) pvVBMalloc (sizeof (struct VBTREE));
	else
	{
		psTreeFree = psTreeFree->psNext;
#ifdef	DEBUG
		if (psTree->tNodeNumber != -1)
		{
			printf ("TreeAllocated that doesn't seem to be free!\n");
			assert (FALSE);
		}
#endif	// DEBUG
	}
	iCurrHandle = -1;
	if (psTree)
		memset (psTree, 0, sizeof (struct VBTREE));
	return (psTree);
}

/*
 * Name:
 *	void	vVBTreeAllFree (int iHandle, int iKeyNumber, struct VBTREE *psTree);
 * Arguments:
 *	int	iHandle
 *		The currently open VBISAM file handle
 *	int	iKeyNumber
 *		The key number in question
 *	struct	VBTREE	*psTree
 *		The head entry of the list of VBTREE's to be de-allocated
 * Prerequisites:
 *	NONE
 * Problems:
 *	NONE known
 * Comments:
 *	Simply transfers an *ENTIRE* Tree to the free list
 *	Any associated VBKEY structures are moved to the relevant entry
 *	(psVBFile [iHandle]->psKeyFree [iKeyNumber])
 */
void
vVBTreeAllFree (int iHandle, int iKeyNumber, struct VBTREE *psTree)
{
	if (!psTree)
		return;
#ifdef	DEBUG
	if (psTree->tNodeNumber == -1)
	{
		printf ("TreeFreed that seems to be already free!\n");
		assert (FALSE);
	}
#endif	// DEBUG
	vVBKeyAllFree (iHandle, iKeyNumber, psTree);
	psTree->psNext = psTreeFree;
	psTreeFree = psTree;
	psTree->tNodeNumber = -1;
}

/*
 * Name:
 *	struct	VBKEY	*psVBKeyAllocate (int iHandle, int iKeyNumber);
 * Arguments:
 *	int	iHandle
 *		The currently open VBISAM file handle
 *	int	iKeyNumber
 *		The key number in question
 * Prerequisites:
 *	NONE
 * Problems:
 *	NONE known
 * Returns:
 *	VBKEY_NULL Ran out out memory
 *	OTHER Pointer to the allocated structure
 */
struct	VBKEY	*
psVBKeyAllocate (int iHandle, int iKeyNumber)
{
	int	iLength = 0;
	struct	VBKEY
		*psKey;

	psKey = psVBFile [iHandle]->psKeyFree [iKeyNumber];
	if (psKey == VBKEY_NULL)
	{
		iCurrHandle = iHandle;
		iLength = psVBFile [iHandle]->psKeydesc [iKeyNumber]->iKeyLength;
		psKey = (struct VBKEY *) pvVBMalloc (sizeof (struct VBKEY) + iLength);
		iCurrHandle = -1;
	}
	else
	{
#ifdef	DEBUG
		if (psKey->tRowNode != -1)
		{
			printf ("KeyAllocated that doesn't seem to be free!\n");
			assert (FALSE);
		}
#endif	// DEBUG
		psVBFile [iHandle]->psKeyFree [iKeyNumber] = psVBFile [iHandle]->psKeyFree [iKeyNumber]->psNext;
	}
	if (psKey)
		memset (psKey, 0, (sizeof (struct VBKEY) + iLength));
	return (psKey);
}

/*
 * Name:
 *	void	vVBKeyAllFree (int iHandle, int iKeyNumber, struct VBKEY *psKey);
 * Arguments:
 *	int	iHandle
 *		The currently open VBISAM file handle
 *	int	iKeyNumber
 *		The key number in question
 *	struct	VBKEY	*psKey
 *		The head pointer of a list of keys to be moved to the free list
 * Prerequisites:
 *	NONE
 * Problems:
 *	NONE known
 */
void	vVBKeyAllFree (int iHandle, int iKeyNumber, struct VBTREE *psTree)
{
	struct	VBKEY
		*psKeyCurr = psTree->psKeyFirst,
		*psKeyNext;

	while (psKeyCurr)
	{
#ifdef	DEBUG
		if (psKeyCurr->tRowNode == -1)
		{
			printf ("KeyFreed that already appears to be free!\n");
			assert (FALSE);
		}
#endif	// DEBUG
		psKeyNext = psKeyCurr->psNext;
		if (psKeyCurr->psChild)
			vVBTreeAllFree (iHandle, iKeyNumber, psKeyCurr->psChild);
		psKeyCurr->psNext = psVBFile [iHandle]->psKeyFree [iKeyNumber];
		psVBFile [iHandle]->psKeyFree [iKeyNumber] = psKeyCurr;
		psKeyCurr->tRowNode = -1;
		psKeyCurr = psKeyNext;
	}
	return;
}

/*
 * Name:
 *	void	vVBKeyFree (int iHandle, int iKeyNumber, struct VBKEY *psKey);
 * Arguments:
 *	int	iHandle
 *		The currently open VBISAM file handle
 *	int	iKeyNumber
 *		The key number in question
 *	struct	VBKEY	*psKey
 *		The VBKEY structure to be moved to the free list
 * Prerequisites:
 *	NONE
 * Problems:
 *	NONE known
 */
void	vVBKeyFree (int iHandle, int iKeyNumber, struct VBKEY *psKey)
{
#ifdef	DEBUG
	if (psKey->tRowNode == -1)
	{
		printf ("KeyFreed that already seems to be free!\n");
		assert (FALSE);
	}
#endif	// DEBUG
	if (psKey->psChild)
		vVBTreeAllFree (iHandle, iKeyNumber, psKey->psChild);
	if (psKey->psNext)
		psKey->psNext->psPrev = psKey->psPrev;
	if (psKey->psPrev)
		psKey->psPrev->psNext = psKey->psNext;
	psKey->psNext = psVBFile [iHandle]->psKeyFree [iKeyNumber];
	psVBFile [iHandle]->psKeyFree [iKeyNumber] = psKey;
psKey->tRowNode = -1;
	return;
}

/*
 * Name:
 *	void	vVBKeyUnMalloc (int iHandle);
 * Arguments:
 *	int	iHandle
 *		The currently open VBISAM file handle
 * Prerequisites:
 *	NONE
 * Problems:
 *	NONE known
 */
void
vVBKeyUnMalloc (int iHandle, int iKeyNumber)
{
	int	iLength;
	struct	VBKEY
		*psKeyCurr = psVBFile [iHandle]->psKeyFree [iKeyNumber];

	iLength = sizeof (struct VBKEY) + psVBFile [iHandle]->psKeydesc [iKeyNumber]->iKeyLength;
	while (psKeyCurr)
	{
		psVBFile [iHandle]->psKeyFree [iKeyNumber] = psVBFile [iHandle]->psKeyFree [iKeyNumber]->psNext;
		vVBFree (psKeyCurr, iLength);
		psKeyCurr = psVBFile [iHandle]->psKeyFree [iKeyNumber];
	}
}

/*
 * Name:
 *	void	*pvVBMalloc (size_t tLength);
 * Arguments:
 *	size_t	tLength
 *		The desired length (in bytes) to allocate
 * Prerequisites:
 *	NONE
 * Returns:
 *	(void *) 0
 *		Ran out of RAM, errno = ENOMEN
 *	OTHER
 *		Pointer to the allocated memory
 * Problems:
 *	None known
 */
void	*
pvVBMalloc (size_t tLength)
{
	int	iLoop,
		iLoop2;
	void	*pvPointer;
	struct	VBKEY
		*psKey;
	struct	VBLOCK
		*psLock;
	struct	VBTREE
		*psTree;

	pvPointer = malloc (tLength);
	if (!pvPointer)
	{
	// Firstly, try by freeing up the TRUELY free data
		for (iLoop = 0; iLoop <= iVBMaxUsedHandle; iLoop++)
		{
			if (psVBFile [iLoop])
			{
				for (iLoop2 = 0; iLoop2 < psVBFile [iLoop]->iNKeys; iLoop2++)
				{
					psKey = psVBFile [iLoop]->psKeyFree [iLoop2];
					while (psKey)
					{
						psVBFile [iLoop]->psKeyFree [iLoop2] = psKey->psNext;
						vVBFree (psKey, sizeof (struct VBKEY) + psVBFile [iLoop]->psKeydesc [iLoop2]->iKeyLength);
						psKey = psVBFile [iLoop]->psKeyFree [iLoop2];
					}
				}
			}
		}
		psLock = psLockFree;
		while (psLock)
		{
			psLockFree = psLockFree->psNext;
			vVBFree (psLock, sizeof (struct VBLOCK));
			psLock = psLockFree;
		}
		psTree = psTreeFree;
		while (psTree)
		{
			psTreeFree = psTreeFree->psNext;
			vVBFree (psTree, sizeof (struct VBTREE));
			psTree = psTreeFree;
		}
	}
	if (!pvPointer)
		pvPointer = malloc (tLength);
	if (!pvPointer)
	{
	// Nope, that wasn't enough, try harder!
		for (iLoop = 0; iCurrHandle != -1 && iLoop <= iVBMaxUsedHandle; iLoop++)
		{
			if (psVBFile [iLoop] && iLoop != iCurrHandle)
			{
				for (iLoop2 = 0; iLoop2 < psVBFile [iLoop]->iNKeys; iLoop2++)
				{
					vVBTreeAllFree (iLoop, iLoop2, psVBFile [iLoop]->psTree [iLoop2]);
					psVBFile [iLoop]->psTree [iLoop2] = VBTREE_NULL;
					psVBFile [iLoop]->psKeyCurr [iLoop2] = VBKEY_NULL;
					psKey = psVBFile [iLoop]->psKeyFree [iLoop2];
					while (psKey)
					{
						psVBFile [iLoop]->psKeyFree [iLoop2] = psKey->psNext;
						vVBFree (psKey, sizeof (struct VBKEY) + psVBFile [iLoop]->psKeydesc [iLoop2]->iKeyLength);
						psKey = psVBFile [iLoop]->psKeyFree [iLoop2];
					}
				}
			}
		}
		psTree = psTreeFree;
		while (psTree)
		{
			psTreeFree = psTreeFree->psNext;
			vVBFree (psTree, sizeof (struct VBTREE));
			psTree = psTreeFree;
		}
	}
	if (!pvPointer)
		pvPointer = malloc (tLength);
	// Note from Robin: "Holy pointers batman, we're REALLY out of memory!"
	if (!pvPointer)
		fprintf (stderr, "MALLOC FAULT!\n");
	else
		tMallocUsed += tLength;
	if (tMallocUsed > tMallocMax)
		tMallocMax = tMallocUsed;
	fflush (stderr);
	return (pvPointer);
}

/*
 * Name:
 *	void	vVBFree (void *pvPointer, size_t tLength);
 * Arguments:
 *	void	*pvPointer
 *		A pointer to the previously pvVBMalloc()'d memory
 *	size_t	tLength
 *		The length of the data being released
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
vVBFree (void *pvPointer, size_t tLength)
{
	tMallocUsed -= tLength;
	free (pvPointer);
}

/*
 * Name:
 *	void	vVBUnMalloc (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 */
void
vVBUnMalloc (void)
{
	struct	VBLOCK
		*psLockCurr;
	struct	VBTREE
		*psTreeCurr;

	iscleanup ();
	if (pcRowBuffer)
		vVBFree (pcRowBuffer, iVBRowBufferLength);
	if (pcWriteBuffer)
		vVBFree (pcWriteBuffer, iVBRowBufferLength);
	pcRowBuffer = (char *) 0;
	psLockCurr = psLockFree;
	while (psLockCurr)
	{
		psLockFree = psLockFree->psNext;
		vVBFree (psLockCurr, sizeof (struct VBLOCK));
		psLockCurr = psLockFree;
	}
	psTreeCurr = psTreeFree;
	while (psTreeCurr)
	{
		psTreeFree = psTreeFree->psNext;
		vVBFree (psTreeCurr, sizeof (struct VBTREE));
		psTreeCurr = psTreeFree;
	}
	vVBBlockDeinit ();
#ifdef	DEBUG
	vVBMallocReport ();
#endif
}

#ifdef	DEBUG
/*
 * Name:
 *	void	vVBMallocReport (void);
 * Arguments:
 *	NONE
 * Prerequisites:
 *	NONE
 * Returns:
 *	NONE
 * Problems:
 *	NONE known
 * Comments:
 *	Simply allows a progam to determine the MAXIMUM allocate RAM usage
 */
void
vVBMallocReport (void)
{
	fprintf (stderr, "Maximum RAM allocation during this run was: 0x%08lX\n", (long) tMallocMax);
	fprintf (stderr, "RAM still allocated at termination is: 0x%08lX\n", (long) tMallocUsed);
	fflush (stderr);
}
#endif	// DEBUG
