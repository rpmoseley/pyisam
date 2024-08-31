/*
 * This file provides extra variables to make the libvbisam library look like the
 * commerical libifisam library. In particular, the handling of error codes, which
 * are not translated.
 */

#define NEED_VBINLINE_INT_LOAD 1
#define NEED_VBINLINE_QUAD_LOAD 1
#include "isinternal.h"

/* Provide the error messages for the standard CISAM error codes */
static const int   _is_nerr = 172;
static const char *_is_errlist[] = {
  "Duplicate record",                       /* 100 - EDUPL */
  "File not open",                          /* 101 - ENOTOPEN */
  "Illegal argument",                       /* 102 - EBADARG */
  "Bad key descriptor",                     /* 103 - EBADKEY */
  "Too many files",                         /* 104 - ETOOMANY */
  "Corrupted isam file",                    /* 105 - EBADFILE */
  "Need exclusive access",                  /* 106 - ENOTEXCL */
  "Record or file locked",                  /* 107 - ELOCKED */
  "Index already exists",                   /* 108 - EKEXISTS */
  "Primary index",                          /* 109 - EPRIMARY */
  "End of file",                            /* 110 - EENDFILE */
  "Record not found",                       /* 111 - ENOREC */
  "No current record",                      /* 112 - ENOCURR */
  "File is in use",                         /* 113 - EFLOCKED */
  "File name too long",                     /* 114 - EFNAME */
  "Bad lock device",                        /* 115 - ENOLOK */
  "Can't allocate memory",                  /* 116 - EBADMEM */
  "Can't read log record",                  /* 118 - ELOGREAD */
  "Bad log record",                         /* 119 - EBADLOG */
  "Can't open log file",                    /* 120 - ELOGOPEN */
  "Can't write log record",                 /* 121 - ELOGWRIT */
  "No transaction",                         /* 122 - ENOTRANS */
  "No begin work yet",                      /* 124 - ENOBEGIN */
  "No primary key",                         /* 127 - ENOPRIM */
  "No logging",                             /* 128 - ENOLOG */
  "No free disk space",                     /* 131 - ENOFREE */
  "Rowsize too big",                        /* 132 - EROWSIZE */
  "Audit trail exists",                     /* 133 - EAUDIT */
  "No more locks",                          /* 134 - ENOLOCKS */
  "Deadlock detected",                      /* 143 - EDEADLOK */
  "Not in ISMANULOCK mode",                 /* 153 - ENOMANU */
  "Interrupted isam call",                  /* 157 - EINTERUPT */
  "isam file format change detected",       /* 171 - EBADFORMAT */
};
/* The following are the globals defined in ifisam/isam.h:
 *
 * For system call errors
 *   iserrno = errno (system error code 1-99)
 *   iserrio = IO_call + IO_file
 *		IO_call  = what system call
 *		IO_file  = which file caused error
 *
 * #define IO_OPEN	 0x10		* open()	*
 * #define IO_CREA	 0x20		* creat()	*
 * #define IO_SEEK	 0x30		* lseek()	*
 * #define IO_READ	 0x40		* read()	*
 * #define IO_WRIT	 0x50		* write()	*
 * #define IO_LOCK	 0x60		* locking()	*
 * #define IO_IOCTL  0x70		* ioctl()	*
 *
 * #define IO_IDX	   0x01		* index file *
 * #define IO_DAT	   0x02		* data file	*
 * #define IO_AUD	   0x03		* audit file *
 * #define IO_LOK	   0x04		* lock file	*
 * #define IO_SEM	   0x05		* semaphore file *

 * extern  int iserrno;		* isam error return code	*
 * extern  int iserrio;		* system call error code	*
 * extern  int isrecnum;	* record number of last call	*
 * extern  int isreclen;	* actual record length, or	*
 *                        * minimum (isbuild, isindexinfo) *
 *                				* or maximum (isopen )		*
 * extern  char isstat1;	* cobol status characters	*
 * extern  char isstat2;
 * extern  char isstat3;
 * extern  char isstat4;
 * extern  char *isversnumber;	* C-ISAM version number	*
 * extern  char *iscopyright;	  * RDS copyright		*
 * extern  char *isserial;		  * C-ISAM software serial number *
 * extern  int  issingleuser;	  * set for single user access	*
 * extern  int  is_nerr;		    * highest C-ISAM error code	*
 * extern  char *is_errlist[];	* C-ISAM error messages	*
 *
 *  error message usage:
 *	if (iserrno >= 100 && iserrno < is_nerr)
 *	    printf("ISAM error %d: %s\n", iserrno, is_errlist[iserrno-100]);
 *
 * #define EDUPL	  100		* duplicate record	*
 * #define ENOTOPEN  101		* file not open	*
 * #define EBADARG   102		* illegal argument	*
 * #define EBADKEY   103		* illegal key desc	*
 * #define ETOOMANY  104		* too many files open	*
 * #define EBADFILE  105		* bad isam file format	*
 * #define ENOTEXCL  106		* non-exclusive access	*
 * #define ELOCKED   107		* record locked	*
 * #define EKEXISTS  108		* key already exists	*
 * #define EPRIMKEY  109		* is primary key	*
 * #define EENDFILE  110		* end/begin of file	*
 * #define ENOREC    111		* no record found	*
 * #define ENOCURR   112		* no current record	*
 * #define EFLOCKED  113		* file locked		*
 * #define EFNAME    114		* file name too long	*
 * #define ENOLOK    115		* can't create lock file *
 * #define EBADMEM   116		* can't alloc memory	*
 * #define EBADCOLL  117		* bad custom collating	*
 * #define ELOGREAD  118		* cannot read log rec  *
 * #define EBADLOG   119		* bad log record	*
 * #define ELOGOPEN  120		* cannot open log file	*
 * #define ELOGWRIT  121		* cannot write log rec *
 * #define ENOTRANS  122		* no transaction	*
 * #define ENOSHMEM  123		* no shared memory	*
 * #define ENOBEGIN  124		* no begin work yet	*
 * #define ENONFS    125		* can't use nfs 	*
 * #define EBADROWID 126		* reserved for future use *
 * #define ENOPRIM   127		* no primary key	*
 * #define ENOLOG    128		* no logging		*
 * #define EUSER     129		* reserved for future use *
 * #define ENODBS    130		* reserved for future use *
 * #define ENOFREE   131		* no free disk space	*
 * #define EROWSIZE  132		* row size too big	*
 * #define EAUDIT	  133		* audit trail exists   *
 * #define ENOLOCKS  134		* no more locks	*
 * #define ENOPARTN  135		* reserved for future use *
 * #define ENOEXTN   136		* reserved for future use *
 * #define EOVCHUNK  137		* reserved for future use *
 * #define EOVDBS    138		* reserved for future use *
 * #define EOVLOG    139		* reserved for future use *
 * #define EGBLSECT  140		* global section disallowing access - VMS *
 * #define EOVPARTN  141		* reserved for future use *
 * #define EOVPPAGE  142		* reserved for future use *
 * #define EDEADLOK  143		* reserved for future use *
 * #define EKLOCKED  144		* reserved for future use *
 * #define ENOMIRROR 145           * reserved for future use *
 * #define EDISKMODE 146           * reserved for future use *
 * #define EARCHBUSY 147		* reserved for future use *
 * #define ENEMPTY	  148		* reserved for future use *
 * #define EDEADDEM  149		* reserved for future use *
 * #define EDEMO	  150		* demo limits have been exceeded *
 * #define EBADVCLEN 151		* reserved for future use *
 * #define EBADRMSG  152		* reserved for future use *
 * #define ENOMANU   153		* must be in ISMANULOCK mode *
 * #define EDEADTIME 154           * lock timeout expired *
 * #define EPMCHKBAD 155           * primary and mirror chunk bad *
 * #define EBADSHMEM 156           * can't attach to shared memory*
 * #define EINTERUPT 157           * interrupted isam call *
 * #define ENOSMI    158           * operation disallowed on SMI pseudo table *
 * #define ECOL_SPEC 159		* invalid collation specifier *
 * #define ENLS_LANG ECOL_SPEC	* retained for backward compatibility *
 * #define EB_BUSY	  160		* reserved for future use *
 * #define EB_NOOPEN 161		* reserved for future use *
 * #define EB_NOBS	  162		* reserved for future use *
 * #define EB_PAGE	  163		* reserved for future use *
 * #define EB_STAMP  164		* reserved for future use *
 * #define EB_NOCOL  165		* reserved for future use *
 * #define EB_FULL   166		* reserved for future use *
 * #define EB_PSIZE  167		* reserved for future use *
 * #define EB_ARCH   168		* reserved for future use *
 * #define EB_CHKNLOG 169		* reserved for future use *
 * #define EB_IUBS	  170		* reserved for future use *
 * #define EBADFORMAT 171		* locking or NODESIZE change *
 * #define ERECFULL      21501     * Record overflow  *
 * #define ELOGINCOMPAT  21502     * Incompatiable log file *
 * #define ENODEFULL     26061     * NODE overflow *
 * #define ETREEFULL     26062     * BTREE limit exceeded *
 */

/* Provide the newer iskeyinfo() and isdictinfo() */
static off_t my_tcountrows(int ihandle, struct DICTINFO *fptr) {
    off_t   tnodenumber, tdatacount;
    int     inodeused;
    VB_CHAR cvbnodetmp[MAX_NODE_LENGTH];

    tnodenumber = inl_ldquad((VB_CHAR *)fptr->sdictnode.cdatafree);
    tdatacount = inl_ldquad((VB_CHAR *)fptr->sdictnode.cdatacount);
    while (tnodenumber) {
        if (ivbblockread(ihandle, 1, tnodenumber, cvbnodetmp)) {
            return -1;
        }
        inodeused = inl_ldint(cvbnodetmp);
        inodeused -= INTSIZE + QUADSIZE;
        tdatacount -= (inodeused / QUADSIZE);
        tnodenumber = inl_ldquad(cvbnodetmp + INTSIZE);
    }
    return tdatacount;
}

int
isdictinfo (int ihandle, struct dictinfo *psdictinfo)
{
    vb_rtd_t *vb_rtd = VB_GET_RTD;
    struct DICTINFO *psvbfptr;
    struct dictinfo sdict;

    if (unlikely(ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle)) {
        vb_rtd->iserrno = EBADARG;
        return -1;
    }
    if (unlikely(!psdictinfo)) {
        vb_rtd->iserrno = EBADARG;
        return -1;
    }
    psvbfptr = vb_rtd->psvbfile[ihandle];
    if (!psvbfptr || psvbfptr->iisopen) {
        vb_rtd->iserrno = ENOTOPEN;
        return -1;
    }
    vb_rtd->iserrno = 0;
    if (ivbenter (ihandle, 0))
        return -1;
    sdict.di_nkeys = psvbfptr->inkeys;
    if (psvbfptr->iopenmode & ISVARLEN)
        sdict.di_nkeys |= 0x80;
    sdict.di_recsize = psvbfptr->imaxrowlength;
    sdict.di_idxsize = psvbfptr->inodesize;
    sdict.di_nrecords = my_tcountrows(ihandle, psvbfptr);
    vb_rtd->isreclen = psvbfptr->iminrowlength;
    memcpy (psdictinfo, &sdict, sizeof (struct dictinfo));
    ivbexit (ihandle);
    return 0;
}

int
iskeyinfo (int ihandle, struct keydesc *pskeydesc, int ikeynumber)
{
    vb_rtd_t *vb_rtd = VB_GET_RTD;
    struct DICTINFO *psvbfptr;

    if (unlikely(ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle)) {
        vb_rtd->iserrno = EBADARG;
        return -1;
    }
    psvbfptr = vb_rtd->psvbfile[ihandle];
    if (!psvbfptr || psvbfptr->iisopen) {
        vb_rtd->iserrno = ENOTOPEN;
        return -1;
    }
    if (ikeynumber < 0 || ikeynumber > psvbfptr->inkeys) {
        vb_rtd->iserrno = EBADARG;
        return -1;
    }
    memcpy (pskeydesc, psvbfptr->pskeydesc[ikeynumber - 1],
        sizeof (struct keydesc));
    vb_rtd->iserrno = 0;
    return 0;
}

/* Provide functions that will behave as if they are actually variables */
int
iserrno (void)
{
    vb_rtd_t *vb_rtd = VB_GET_RTD;
    return vb_rtd->iserrno;
}

int
iserrio (void)
{
    vb_rtd_t *vb_rtd = VB_GET_RTD;
    return vb_rtd->iserrio;
}

vbisam_off_t
isrecnum (void)
{
    vb_rtd_t *vb_rtd = VB_GET_RTD;
    return vb_rtd->isrecnum;
}

int
set_isrecnum (vbisam_off_t irecnum)
{
    vb_rtd_t *vb_rtd = VB_GET_RTD;
    vb_rtd->isrecnum = irecnum;
    return 0;
}

int
isreclen (void)
{
    vb_rtd_t *vb_rtd = VB_GET_RTD;
    return vb_rtd->isreclen;
}

int
set_isreclen (int ireclen)
{
    vb_rtd_t *vb_rtd = VB_GET_RTD;
    vb_rtd->isreclen = ireclen;
    return 0;
}

const int
is_nerr (void)
{
    return _is_nerr;
}

const char *
is_strerror(int errcode)
{
    int corr = 100;
    switch (errcode) {
    case 171:
        corr += 13;
    case 157:
        corr += 3;
    case 153:
        corr += 9;
    case 143:
        corr += 8;
    case 131:
    case 132:
    case 133:
    case 134:
        corr += 2;
    case 127:
    case 128:
        corr += 2;
    case 124:
        corr++;
    case 118:
    case 119:
    case 120:
    case 121:
    case 122:
        corr++;
    case 116:
        corr++;
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
        break;
    default:
        return "";
    }
    return _is_errlist[errcode - corr];
}

#ifdef NEED_LIB_INITIALISE
void __attribute__((constructor)) vb_init_lib(void)
{
  /* Ensure that the vbisam library is initialised correctly */
  vb_get_rtd();
}
#endif
