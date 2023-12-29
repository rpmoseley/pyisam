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
import os
import shutil

# Determine whether this a 32- or 64-bit architecture
on_64bit = platform.architecture()[0] == '64bit'
long32 = 'int' if on_64bit else 'long'
szlong = '64'  if on_64bit else '32'
join = os.path.join

# Build the library directly into the path expected by the pyisam package
ffi = FFI()
ffi.set_source(
  'pyisam.backend.cffi._isam_cffi',
  '#include "isam.h"',
  library_dirs = [join('libisam', szlong)],
  runtime_library_dirs = ['$ORIGIN/../lib'],
  libraries = ['ifisam', 'ifisamx'],
  include_dirs = [join('libisam', szlong, 'include')],
)

# Define items found in decimal.h
ffi.cdef('''
struct decimal;
extern int decadd(struct decimal *, struct decimal *, struct decimal *);
extern int decsub(struct decimal *, struct decimal *, struct decimal *);
extern int decmul(struct decimal *, struct decimal *, struct decimal *);
extern int decdiv(struct decimal *, struct decimal *, struct decimal *);
extern int deccmp(struct decimal *, struct decimal *);
extern void deccopy(struct decimal *, struct decimal *);
extern int deccvasc(char *, int, struct decimal *);
extern int deccvdbl(double, struct decimal *);
extern int deccvint(int, struct decimal *);
extern int deccvlong({longsz}, struct decimal *);
extern char *dececvt(struct decimal *, int, int *, int *);
extern char *decfcvt(struct decimal *, int, int *, int *);
extern void decround(struct decimal *, int);
extern int dectoasc(struct decimal *, char *, int, int);
extern int dectodbl(struct decimal *, double *);
extern int dectoint(struct decimal *, int *);
extern int dectolong(struct decimal *, {longsz} *);
extern void dectrunc(struct decimal *, int);
extern int deccvflt(double, struct decimal *);
extern int dectoflt(struct decimal *, float *);
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
extern int    iserrno;
extern int    iserrio;
extern {longsz}    isrecnum;
extern int    isreclen;
extern char  *isversnumber;
extern char  *iscopyright;
extern char  *isserial;	
extern int    issingleuser;
extern int    is_nerr;
extern char  *is_errlist[];
extern int    isaddindex(int, struct keydesc *);
extern int    isaudit(int, char *, int);
extern int    isbegin(void);
extern int    isbuild(char *, int, struct keydesc *, int);
extern int    iscleanup(void);
extern int    isclose(int);
extern int    iscluster(int, struct keydesc *);
extern int    iscommit(void);
extern int    isdelcurr(int);
extern int    isdelete(int, char *);
extern int    isdelindex(int, struct keydesc *);
extern int    isdelrec(int, long);
extern int    isdictinfo(int, struct dictinfo *);
extern int    iserase(char *);
extern int    isflush(int);
extern int    isindexinfo(int, void *, int);
extern int    iskeyinfo(int, struct keydesc *, int);
extern void   islangchk(void);
extern char  *islanginfo(char *);
extern int    islock(int);
extern int    islogclose(void);
extern int    islogopen(char *);
extern int    isnlsversion(char *);
extern int    isglsversion(char *);
extern void   isnolangchk(void);
extern int    isopen(char *, int);
extern int    isread(int, char *, int);
extern int    isrecover(void);
extern int    isrelease(int);
extern int    isrename(char *, char *);
extern int    isrewcurr(int, char *);
extern int    isrewrec(int, long, char *);
extern int    isrewrite(int, char *);
extern int    isrollback(void);
extern int    issetunique(int, {longsz});
extern int    isstart(int, struct keydesc *, int, char *, int);
extern int    isuniqueid(int, {longsz} *);
extern int    isunlock(int);
extern int    iswrcurr(int, char *);
extern int    iswrite(int, char *);
'''.format(longsz=long32))

if __name__ == '__main__':
  ffi.compile()   # NOTE Passing tmpdir moves ALL relative paths as well!
  shutil.copyfile(join('libisam', szlong, 'libifisam.so'), 'pyisam/backend/lib/libifisam.so')
  shutil.copyfile(join('libisam', szlong, 'libifisamx.so'), 'pyisam/backend/lib/libifisamx.so')
