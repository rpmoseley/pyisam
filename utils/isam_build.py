'''
This module provides the definitions in order to generate the CFFI interface library
which links with either IBM Informix CISAM or the open source VBISAM libraries and
provides the necessary mappings.

FIXME: This currently is restricted to the CISAM library.
TODO: Update this script to perform all the necessary copies of the libraries and
      include files into the working directory, and then copy the resultant compiled
      module back into the correct location.
'''
import platform
from cffi import FFI

# Determine whether this a 32- or 64-bit architecture
on_64bit = platform.architecture()[0] == '64bit'
if on_64bit:
  long32 = 'int'
  libdir = 'lib64'
  incdir = 'include/isam64'
else:
  long32 = 'long'
  libdir = 'lib32'
  incdir = 'include/isam32'

ffi = FFI()
lib = ffi.set_source(
  '_isam_cffi',
  '#include "isam.h"',
  library_dirs = [libdir],
  runtime_library_dirs = ['$ORIGIN/../lib'],
  libraries = ['ifisam','ifisamx'],
  include_dirs = [incdir],
)
# Define items found in decimal.h
ffi.cdef('''
struct decimal;
int decadd(struct decimal *, struct decimal *, struct decimal *);
int decsub(struct decimal *, struct decimal *, struct decimal *);
int decmul(struct decimal *, struct decimal *, struct decimal *);
int decdiv(struct decimal *, struct decimal *, struct decimal *);
int deccmp(struct decimal *, struct decimal *);
void deccopy(struct decimal *, struct decimal *);
int deccvasc(char *, int, struct decimal *);
int deccvdbl(double, struct decimal *);
int deccvint(int, struct decimal *);
int deccvlong(long, struct decimal *);
char *dececvt(struct decimal *, int, int *, int *);
char *decfcvt(struct decimal *, int, int *, int *);
void decround(struct decimal *, int);
int dectoasc(struct decimal *, char *, int, int);
int dectodbl(struct decimal *, double *);
int dectoint(struct decimal *, int *);
int dectolong(struct decimal *, {longsz} *);
void dectrunc(struct decimal *, int);
int deccvflt(double, struct decimal *);
int dectoflt(struct decimal *, float *);
'''.format(longsz=long32))
# Define items found in isam.h
ffi.cdef('''
#define NPARTS 8
struct keypart {{
    short kp_start;
    short kp_leng;
    short kp_type;
}};
struct keydesc {{
    short k_flags;
    short k_nparts;
    struct keypart k_part[NPARTS];
    ...;
}};
struct dictinfo {{
    short di_nkeys;
    short di_recsize;
    short di_idxsize;
    long di_nrecords;
}};
int    iserrno;
int    iserrio;
{longsz}    isrecnum;
int    isreclen;
char  *isversnumber;
char  *iscopyright;
char  *isserial;	
int    issingleuser;
int    is_nerr;
char  *is_errlist[];
int    isaddindex(int, struct keydesc *);
int    isaudit(int, char *, int);
int    isbegin(void);
int    isbuild(char *, int, struct keydesc *, int);
int    iscleanup(void);
int    isclose(int);
int    iscluster(int, struct keydesc *);
int    iscommit(void);
int    isdelcurr(int);
int    isdelete(int, char *);
int    isdelindex(int, struct keydesc *);
int    isdelrec(int, long);
int    isdictinfo(int, struct dictinfo *);
int    iserase(char *);
int    isflush(int);
int    isindexinfo(int, void *, int);
int    iskeyinfo(int, struct keydesc *, int);
void   islangchk(void);
char  *islanginfo(char *);
int    islock(int);
int    islogclose(void);
int    islogopen(char *);
int    isnlsversion(char *);
int    isglsversion(char *);
void   isnolangchk(void);
int    isopen(char *, int);
int    isread(int, char *, int);
int    isrecover(void);
int    isrelease(int);
int    isrename(char *, char *);
int    isrewcurr(int, char *);
int    isrewrec(int, long, char *);
int    isrewrite(int, char *);
int    isrollback(void);
int    issetunique(int, {longsz});
int    isstart(int, struct keydesc *, int, char *, int);
int    isuniqueid(int, {longsz} *);
int    isunlock(int);
int    iswrcurr(int, char *);
int    iswrite(int, char *);
'''.format(longsz=long32))
if __name__ == '__main__':
  ffi.compile()   # NOTE Passing tmpdir moves ALL relative paths as well!
