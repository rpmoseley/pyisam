/*
 * Copyright (C) 2003 Trevor van Bremen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1,
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA
 */

#include	"isinternal.h"

#ifdef	VBDEBUG
#include	<assert.h>
#endif

#ifdef _MSC_VER
    #define COB_THREAD __declspec( thread ) 
    #define HAVE__THEAD_ATTR 1
COB_THREAD vb_rtd_t vb_rtd_data = {0};
static vb_rtd_t vb_rtd_data_xp = {0};
static OSVERSIONINFO osvi = {0};
#else
    #ifdef HAVE__THEAD_ATTR
__thread vb_rtd_t vb_rtd_data = {0};
    #elif defined(HAVE_PTHREAD_H)
        #include <pthread.h>
static pthread_key_t tlsKey;
static pthread_once_t tlsIndex_once = PTHREAD_ONCE_INIT;
    #else 
vb_rtd_t vb_rtd_data = {0};
    #endif
#endif

static void
vb_allocate_rtd (void)
{
#if defined(HAVE_PTHREAD_H) && !defined(HAVE__THEAD_ATTR)
    pthread_key_create(&tlsKey, NULL);
#endif
}

void
vb_init_rtd (void)
{
	vb_rtd_t * vb_rtd;

	vb_rtd =VB_GET_RTD;

	vb_rtd->ivblogfilehandle = -1;		/* Handle of the current logfile */
	vb_rtd->ivbmaxusedhandle = -1;		/* The highest opened file handle */
#ifdef	VBDEBUG
	vb_rtd->icurrhandle = -1;
#endif
}

vb_rtd_t *
vb_get_rtd (void)
{
	vb_rtd_t * res;
#ifdef _MSC_VER
    if ( osvi.dwOSVersionInfoSize == 0 ) {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);
    }
    if ( osvi.dwMajorVersion < 6 ) {
        res = &vb_rtd_data_xp;
        goto end_get_rtd;
    }

#endif
#if defined(HAVE_PTHREAD_H) && !defined(HAVE__THEAD_ATTR)
	int err;

	(void) pthread_once(&tlsIndex_once, vb_allocate_rtd);
	res = (vb_rtd_t * ) pthread_getspecific (tlsKey);	
	if (res == NULL) {
		res = calloc(1, sizeof(vb_rtd_t));
		if (!res) {
			fprintf (stderr, "vb_init_rtd :Memory allocation failed\n") ;
			exit (-1);
		}
		if((err = pthread_setspecific (tlsKey, res)) != 0){	
			fprintf (stderr, "Internal Error: pthread_setspecific (key=%d) failed (error: 0%d)\n", tlsKey, err);
			exit (-1);
		}
	}
#else
	res = &vb_rtd_data;
#endif
end_get_rtd:
    if (!(res->vb_isinit)) {
        res->vb_isinit =  1;
        vb_init_rtd();
    }
	return res;
}

struct VBLOCK *
psvblockallocate (const int ihandle)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct VBLOCK *pslock = vb_rtd->pslockfree;

#ifdef	VBDEBUG
	vb_rtd->icurrhandle = ihandle;
#endif
	if (vb_rtd->pslockfree != NULL) {
		vb_rtd->pslockfree = vb_rtd->pslockfree->psnext;
		memset (pslock, 0, sizeof (struct VBLOCK));
	} else {
		pslock = pvvbmalloc (sizeof (struct VBLOCK));
	}
#ifdef	VBDEBUG
	vb_rtd->icurrhandle = -1;
#endif
	return pslock;
}

void
vvblockfree (struct VBLOCK *pslock)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	pslock->psnext = vb_rtd->pslockfree;
	vb_rtd->pslockfree = pslock;
}

struct VBTREE *
psvbtreeallocate (const int ihandle)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct VBTREE *pstree = vb_rtd->pstreefree;

#ifdef	VBDEBUG
	vb_rtd->icurrhandle = ihandle;
#endif
	if (vb_rtd->pstreefree == NULL) {
		pstree = pvvbmalloc (sizeof (struct VBTREE));
	} else {
		vb_rtd->pstreefree = vb_rtd->pstreefree->psnext;
		memset (pstree, 0, sizeof (struct VBTREE));
#ifdef	VBDEBUG
		if (pstree->tnodenumber != -1) {
			printf ("TreeAllocated that doesn't seem to be free!\n");
			assert (0);
		}
#endif	/* VBDEBUG */
	}
#ifdef	VBDEBUG
	vb_rtd->icurrhandle = -1;
#endif
	return pstree;
}

void
vvbtreeallfree (const int ihandle, const int ikeynumber, struct VBTREE *pstree)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	if (!pstree) {
		return;
	}
#ifdef	VBDEBUG
	if (pstree->tnodenumber == -1) {
		printf ("TreeFreed that seems to be already free!\n");
		assert (0);
	}
#endif	/* VBDEBUG */
	vvbkeyallfree (ihandle, ikeynumber, pstree);
	pstree->psnext = vb_rtd->pstreefree;
	vb_rtd->pstreefree = pstree;
	pstree->tnodenumber = -1;
}

struct VBKEY *
psvbkeyallocate (const int ihandle, const int ikeynumber)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct VBKEY	*pskey;
	struct DICTINFO	*psvbptr;
	int		ilength = 0;

	psvbptr = vb_rtd->psvbfile[ihandle];
	pskey = psvbptr->pskeyfree[ikeynumber];
	if (pskey == NULL) {
#ifdef	VBDEBUG
		vb_rtd->icurrhandle = ihandle;
#endif
		ilength = psvbptr->pskeydesc[ikeynumber]->k_len;
		pskey = pvvbmalloc (sizeof (struct VBKEY) + ilength);
#ifdef	VBDEBUG
		vb_rtd->icurrhandle = -1;
#endif
	} else {
#ifdef	VBDEBUG
		if (pskey->trownode != -1) {
			printf ("KeyAllocated that doesn't seem to be free!\n");
			assert (0);
		}
#endif	/* VBDEBUG */
		psvbptr->pskeyfree[ikeynumber] =
		    psvbptr->pskeyfree[ikeynumber]->psnext;
		memset (pskey, 0, (sizeof (struct VBKEY) + ilength));
	}
	return pskey;
}

void
vvbkeyallfree (const int ihandle, const int ikeynumber, struct VBTREE *pstree)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;
	struct VBKEY *pskeycurr = pstree->pskeyfirst, *pskeynext;

	psvbptr = vb_rtd->psvbfile[ihandle];
	while (pskeycurr) {
#ifdef	VBDEBUG
		if (pskeycurr->trownode == -1) {
			printf ("KeyFreed that already appears to be free!\n");
			assert (0);
		}
#endif	/* VBDEBUG */
		pskeynext = pskeycurr->psnext;
		if (pskeycurr->pschild) {
			vvbtreeallfree (ihandle, ikeynumber, pskeycurr->pschild);
		}
		pskeycurr->pschild = NULL;
		pskeycurr->psnext = psvbptr->pskeyfree[ikeynumber];
		psvbptr->pskeyfree[ikeynumber] = pskeycurr;
		pskeycurr->trownode = -1;
		pskeycurr = pskeynext;
	}
	pstree->pskeyfirst = NULL;
	pstree->pskeylast = NULL;
	pstree->pskeycurr = NULL;
	pstree->ikeysinnode = 0;
}

void
vvbkeyfree (const int ihandle, const int ikeynumber, struct VBKEY *pskey)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;

#ifdef	VBDEBUG
	if (pskey->trownode == -1) {
		printf ("KeyFreed that already seems to be free!\n");
		assert (0);
	}
#endif	/* VBDEBUG */
	psvbptr = vb_rtd->psvbfile[ihandle];
	if (pskey->pschild) {
		vvbtreeallfree (ihandle, ikeynumber, pskey->pschild);
	}
	pskey->pschild = NULL;
	if (pskey->psnext) {
		pskey->psnext->psprev = pskey->psprev;
	}
	if (pskey->psprev) {
		pskey->psprev->psnext = pskey->psnext;
	}
	pskey->psnext = psvbptr->pskeyfree[ikeynumber];
	psvbptr->pskeyfree[ikeynumber] = pskey;
	pskey->trownode = -1;
}

void
vvbkeyunmalloc (const int ihandle, const int ikeynumber)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct DICTINFO	*psvbptr;
	struct VBKEY	*pskeycurr;
	int		ilength;

	psvbptr = vb_rtd->psvbfile[ihandle];
	pskeycurr = psvbptr->pskeyfree[ikeynumber];
	ilength = sizeof (struct VBKEY) + psvbptr->pskeydesc[ikeynumber]->k_len;
	while (pskeycurr) {
		psvbptr->pskeyfree[ikeynumber] =
		    psvbptr->pskeyfree[ikeynumber]->psnext;
		vvbfree (pskeycurr, ilength);
		pskeycurr = psvbptr->pskeyfree[ikeynumber];
	}
}

#ifdef	VBDEBUG
void *
pvvbmalloc (const size_t tlength)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	void		*pvpointer;
	struct VBKEY	*pskey;
	struct VBLOCK	*pslock;
	struct VBTREE	*pstree;
	int		iloop, iloop2;

	pvpointer = malloc (tlength);
	if (!pvpointer) {
		/* Firstly, try by freeing up the truely free data */
		for (iloop = 0; iloop <= vb_rtd->ivbmaxusedhandle; iloop++) {
			if (vb_rtd->psvbfile[iloop]) {
				for (iloop2 = 0; iloop2 < vb_rtd->psvbfile[iloop]->inkeys; iloop2++) {
					pskey = vb_rtd->psvbfile[iloop]->pskeyfree[iloop2];
					while (pskey) {
						vb_rtd->psvbfile[iloop]->pskeyfree[iloop2] =
						    pskey->psnext;
						vvbfree (pskey,
							 sizeof (struct VBKEY) +
							 vb_rtd->psvbfile[iloop]->pskeydesc[iloop2]->
							 k_len);
						pskey = vb_rtd->psvbfile[iloop]->pskeyfree[iloop2];
					}
				}
			}
		}
		pslock = vb_rtd->pslockfree;
		while (pslock) {
			vb_rtd->pslockfree = vb_rtd->pslockfree->psnext;
			vvbfree (pslock, sizeof (struct VBLOCK));
			pslock = vb_rtd->pslockfree;
		}
		pstree = vb_rtd->pstreefree;
		while (pstree) {
			vb_rtd->pstreefree = vb_rtd->pstreefree->psnext;
			vvbfree (pstree, sizeof (struct VBTREE));
			pstree = vb_rtd->pstreefree;
		}
	}
	if (!pvpointer) {
		pvpointer = malloc (tlength);
	}
	if (!pvpointer) {
		/* Nope, that wasn't enough, try harder! */
		for (iloop = 0; vb_rtd->icurrhandle != -1 && iloop <= vb_rtd->ivbmaxusedhandle; iloop++) {
			if (vb_rtd->psvbfile[iloop] && iloop != vb_rtd->icurrhandle) {
				for (iloop2 = 0; iloop2 < vb_rtd->psvbfile[iloop]->inkeys; iloop2++) {
					vvbtreeallfree (iloop, iloop2,
							vb_rtd->psvbfile[iloop]->pstree[iloop2]);
					vb_rtd->psvbfile[iloop]->pstree[iloop2] = NULL;
					vb_rtd->psvbfile[iloop]->pskeycurr[iloop2] = NULL;
					pskey = vb_rtd->psvbfile[iloop]->pskeyfree[iloop2];
					while (pskey) {
						vb_rtd->psvbfile[iloop]->pskeyfree[iloop2] =
						    pskey->psnext;
						vvbfree (pskey,
							 sizeof (struct VBKEY) +
							 vb_rtd->psvbfile[iloop]->pskeydesc[iloop2]->
							 k_len);
						pskey = vb_rtd->psvbfile[iloop]->pskeyfree[iloop2];
					}
				}
			}
		}
		pstree = vb_rtd->pstreefree;
		while (pstree) {
			vb_rtd->pstreefree = vb_rtd->pstreefree->psnext;
			vvbfree (pstree, sizeof (struct VBTREE));
			pstree = vb_rtd->pstreefree;
		}
	}
	if (!pvpointer) {
		pvpointer = malloc (tlength);
	}
	if (!pvpointer) {
		fprintf (stderr, "MALLOC FAULT!\n");
		fflush (stderr);
	} else {
		vb_rtd->tmallocused += tlength;
		memset (pvpointer, 0, tlength);
	}
	if (vb_rtd->tmallocused > vb_rtd->tmallocmax) {
		vb_rtd->tmallocmax = vb_rtd->tmallocused;
	}
	return pvpointer;
}

void
vvbfree (void *pvpointer, size_t tlength)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	vb_rtd->tmallocused -= tlength;
	free (pvpointer);
}

static void
vvbmallocreport (void)
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	fprintf (stderr, "Maximum RAM allocation during this run was: 0x%08lX\n",
		 (long)vb_rtd->tmallocmax);
	fprintf (stderr, "RAM still allocated at termination is: 0x%08lX\n", (long)vb_rtd->tmallocused);
	fflush (stderr);
}

void
vvbunmalloc ()
{
	vb_rtd_t *vb_rtd =VB_GET_RTD;
	struct VBLOCK *pslockcurr;
	struct VBTREE *pstreecurr;

	iscleanup ();
	pslockcurr = vb_rtd->pslockfree;
	while (pslockcurr) {
		vb_rtd->pslockfree = vb_rtd->pslockfree->psnext;
		vvbfree (pslockcurr, sizeof (struct VBLOCK));
		pslockcurr = vb_rtd->pslockfree;
	}
	pstreecurr = vb_rtd->pstreefree;
	while (pstreecurr) {
		vb_rtd->pstreefree = vb_rtd->pstreefree->psnext;
		vvbfree (pstreecurr, sizeof (struct VBTREE));
		pstreecurr = vb_rtd->pstreefree;
	}
	vvbmallocreport ();
}
#endif	/* VBDEBUG */
