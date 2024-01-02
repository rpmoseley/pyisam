'''
Build the CFFI and CTYPES backend using the appropriate library for the specified
platform and bit-size (either 32- or 64-bit). By default build for the current
platform and bit-size of architecture the script is being run on.
'''
from cffi import FFI
import hashlib
import pathlib
import shutil
import sysconfig
import subprocess
import os

# Define the working directory
WORKDIR=pathlib.Path('/tmp/pyisam')
INSTDIR=pathlib.Path('pyisam/backend')
SOURCEDIR=pathlib.Path('.')

# Define an exception that will be raised when a build fails
class BuildException(Exception):
  def __init__(self, msg):
    self._msg = msg

  def __str__(self):
    return self._msg

class Builder:
  'Base class providing shared CFFI and CTYPES support'
  _soext = sysconfig.get_config_var('SHLIB_SUFFIX')

  def __init__(self, workdir, srcdir, instdir, lngsz):
    self._workdir = pathlib.Path(workdir)
    self._srcdir = pathlib.Path(srcdir)
    self._instdir = pathlib.Path(instdir)
    self.lngsz = lngsz

  def patchlibrary(self, libname, rpath=None):
    'Patch the given library settings its SONAME and RPATH appropriately'
    def patch_option(option, value, path_libname):
      pret = subprocess.run(['patchelf', f'--print-{option}', path_libname],
                            stdout = subprocess.PIPE,
                            stderr = subprocess.DEVNULL)
      if pret.returncode == 0 and pret.stdout != value:
        pret = subprocess.run(['patchelf', f'--set-{option}', value, path_libname],
                              stdout = subprocess.DEVNULL,
                              stderr = subprocess.DEVNULL)
      return pret.returncode == 0

    bin_libname = libname.as_posix().encode()
    ok = patch_option('soname', bin_libname, self._workdir / libname)
    if ok and rpath:
      ok = patch_option('rpath', rpath, self._workdir / libname)
    if not ok:
      raise ValueError('Patchelf update failed')

  def _copy_on_change(self, srcfile, dstfile):
    'Internal helper method'
    newhash = hashlib.sha256(srcfile.open('rb').read())
    if dstfile.exists():
      oldhash = hashlib.sha256(dstfile.open('rb').read())
      changed = newhash.digest() != oldhash.digest()
    else:
      changed = True
    if changed:
      shutil.copyfile(srcfile, dstfile)
      shutil.copymode(srcfile, dstfile)

  def source_on_change(self, filename):
    '''Copy a new version of the given FILENAME into the working direcroy if
    has changed or not present'''
    if filename.is_absolute() or len(filename.parts) > 1:
      src_file = filename
      dst_file = self._workdir / filename.parts[-1]
    else:
      src_file = self._srcdir / filename
      dst_file = self._workdir / filename
    self._copy_on_change(src_file, dst_file)

  def install_on_change(self, libname, subdir=None):
    'Copy a new version of the given library if it has changed or not present'
    sname = libname.parts[-1] if libname.is_absolute() else libname
    if subdir is None:
      iname = self._instdir / sname
    else:
      iname = self._instdir / subdir / sname
    self._copy_on_change(self._workdir / sname, iname)

class CFFI_Builder(Builder):
  'Class providing the shared methods for the CFFI builders'
  decimal_h_code = None
  isam_h_code = None
  max_key_parts = 8

  def __init__(self, workdir, srcdir, instdir, lngsz):
    super().__init__(workdir, srcdir, instdir, lngsz)
    self._ffi = FFI()
    self._mod_so = None

  def prepare(self):
    raise NotImplementedError

  def compile(self):
    raise NotImplementedError

  def _compile(self):
    if self.decimal_h_code:
      self._ffi.cdef(self.decimal_h_code.format(self=self))
    if self.isam_h_code:
      self._ffi.cdef(self.isam_h_code.format(self=self))
    self._mod_so = pathlib.Path(self._ffi.compile(tmpdir=self._workdir))

  def install(self, addmissing=False):
    raise NotImplementedError

class CFFI_IFISAM_Builder(CFFI_Builder):
  'Class encapsulating the information to compile the IFISAM CFFI module'
  # Define items found in decimal.h
  decimal_h_code = '''
struct decimal;
extern int   decadd(struct decimal *, struct decimal *, struct decimal *);
extern int   decsub(struct decimal *, struct decimal *, struct decimal *);
extern int   decmul(struct decimal *, struct decimal *, struct decimal *);
extern int   decdiv(struct decimal *, struct decimal *, struct decimal *);
extern int   deccmp(struct decimal *, struct decimal *);
extern void  deccopy(struct decimal *, struct decimal *);
extern int   deccvasc(char *, int, struct decimal *);
extern int   deccvdbl(double, struct decimal *);
extern int   deccvint(int, struct decimal *);
extern int   deccvlong({self.lngsz}, struct decimal *);
extern char *dececvt(struct decimal *, int, int *, int *);
extern char *decfcvt(struct decimal *, int, int *, int *);
extern void  decround(struct decimal *, int);
extern int   dectoasc(struct decimal *, char *, int, int);
extern int   dectodbl(struct decimal *, double *);
extern int   dectoint(struct decimal *, int *);
extern int   dectolong(struct decimal *, {self.lngsz} *);
extern void  dectrunc(struct decimal *, int);
extern int   deccvflt(double, struct decimal *);
extern int   dectoflt(struct decimal *, float *);
'''

  # Define items found in isam.h
  isam_h_code = '''
struct keypart {{
    short kp_start;
    short kp_leng;
    short kp_type;
}};
struct keydesc {{
    short k_flags;
    short k_nparts;
    struct keypart k_part[{self.max_key_parts}];
    short k_len;
    ...;
}};
struct dictinfo {{
    short        di_nkeys;
    short        di_recsize;
    short        di_idxsize;
    {self.lngsz} di_nrecords;
}};
extern int           iserrno;
extern int           iserrio;
extern {self.lngsz}  isrecnum;
extern int           isreclen;
extern char         *isversnumber;
extern char         *iscopyright;
extern char         *isserial;	
extern int           issingleuser;
extern int           is_nerr;
extern char         *is_errlist[];
extern int           isaddindex(int, struct keydesc *);
extern int           isaudit(int, char *, int);
extern int           isbegin(void);
extern int           isbuild(char *, int, struct keydesc *, int);
extern int           iscleanup(void);
extern int           isclose(int);
extern int           iscluster(int, struct keydesc *);
extern int           iscommit(void);
extern int           isdelcurr(int);
extern int           isdelete(int, char *);
extern int           isdelindex(int, struct keydesc *);
extern int           isdelrec(int, {self.lngsz});
extern int           isdictinfo(int, struct dictinfo *);
extern int           iserase(char *);
extern int           isflush(int);
extern int           isindexinfo(int, void *, int);
extern int           iskeyinfo(int, struct keydesc *, int);
extern void          islangchk(void);
extern char         *islanginfo(char *);
extern int           islock(int);
extern int           islogclose(void);
extern int           islogopen(char *);
extern int           isnlsversion(char *);
extern int           isglsversion(char *);
extern void          isnolangchk(void);
extern int           isopen(char *, int);
extern int           isread(int, char *, int);
extern int           isrecover(void);
extern int           isrelease(int);
extern int           isrename(char *, char *);
extern int           isrewcurr(int, char *);
extern int           isrewrec(int, {self.lngsz}, char *);
extern int           isrewrite(int, char *);
extern int           isrollback(void);
extern int           issetunique(int, {self.lngsz});
extern int           isstart(int, struct keydesc *, int, char *, int);
extern int           isuniqueid(int, {self.lngsz} *);
extern int           isunlock(int);
extern int           iswrcurr(int, char *);
extern int           iswrite(int, char *);
'''
  def __init__(self, workdir, srcdir, instdir, bits=64):
    super().__init__(workdir, srcdir, instdir, 'int32_t')
    self._bits = bits

  def prepare(self):
    'Populate the working directory with the files required'
    self._ifisam_so = pathlib.Path(f'libifisam{self._soext}')
    self._ifisamx_so = pathlib.Path(f'libifisamx{self._soext}')
    libdir = self._srcdir / 'libifisam' / str(self._bits)
    incldir = libdir / 'include'
    if not incldir.exists():
      incldir = libdir 
    self.source_on_change(incldir / 'isam.h')
    self.source_on_change(incldir / 'decimal.h')
    self.source_on_change(libdir / self._ifisam_so)
    self.source_on_change(libdir / self._ifisamx_so)

  def compile(self, modname):
    self._ffi.set_source(
      modname,
      '#include <stdint.h>\n#include "isam.h"',
      library_dirs=[str(self._workdir)],
      runtime_library_dirs=['$ORIGIN/../lib'],
      libraries=['ifisam', 'ifisamx'],
      include_dirs=[self._workdir],
    )
    self._compile()

  def install(self, addmissing=True):
    if addmissing:
      self.patchlibrary(self._ifisam_so, b'$ORIGIN')
      self.patchlibrary(self._ifisamx_so)
    self.install_on_change(self._mod_so, 'cffi')
    self.install_on_change(self._ifisam_so, 'lib')
    self.install_on_change(self._ifisamx_so, 'lib')

class CFFI_VBISAM_Builder(CFFI_Builder):
  'Class encapsulating the information to compile the VBISAM CFFI module'
  # Define items found in vbdecimal.h
  decimal_h_code = '''
struct decimal;
extern int          decadd(struct decimal *, struct decimal *, struct decimal *);
extern int          decsub(struct decimal *, struct decimal *, struct decimal *);
extern int          decmul(struct decimal *, struct decimal *, struct decimal *);
extern int          decdiv(struct decimal *, struct decimal *, struct decimal *);
extern int          deccmp(struct decimal *, struct decimal *);
extern void         deccopy(struct decimal *, struct decimal *);
extern int          deccvasc(signed char *, int, struct decimal *);
extern int          deccvdbl(double, struct decimal *);
extern int          deccvint(int, struct decimal *);
extern int          deccvlong(long, struct decimal *);
extern signed char *dececvt(struct decimal *, int, int *, int *);
extern signed char *decfcvt(struct decimal *, int, int *, int *);
/*extern void         decround(struct decimal *, int);  -- Not implemented */
extern int          dectoasc(struct decimal *, signed char *, int, int);
extern int          dectodbl(struct decimal *, double *);
extern int          dectoint(struct decimal *, int *);
extern int          dectolong(struct decimal *, long *);
/*extern void         dectrunc(struct decimal *, int);  -- Not implemented */
extern int          deccvflt(double, struct decimal *);
extern int          dectoflt(struct decimal *, float *);
'''

  # Define items found in vbisam.h
  isam_h_code = '''
struct keypart {{
    short kp_start;
    short kp_leng;
    short kp_type;
}};
struct keydesc {{
    short k_flags;
    short k_nparts;
    struct keypart k_part[{self.max_key_parts}];
    short k_len;
    ...;
}};
struct dictinfo {{
    short   di_nkeys;
    short   di_recsize;
    short   di_idxsize;
    {self.lngsz} di_nrecords;
}};
extern int             is_nerr(void);
extern const char    **is_errlist(void);
extern int             iserrno(void);
extern int             iserrio(void);
extern {self.lngsz}   isrecnum(void);
extern int             isreclen(void);
extern int             isaddindex(int, struct keydesc *);
extern int             isaudit(int, signed char *, int);
extern int             isbegin(void);
extern int             isbuild(signed char *, int, struct keydesc *, int);
extern int             iscleanup(void);
extern int             isclose(int);
extern int             iscluster(int, struct keydesc *);
extern int             iscommit(void);
extern int             isdelcurr(int);
extern int             isdelete(int, signed char *);
extern int             isdelindex(int, struct keydesc *);
extern int             isdelrec(int, {self.lngsz});
extern int             isdictinfo(int, struct dictinfo *);
extern int             iserase(signed char *);
extern int             isflush(int);
extern int             isindexinfo(int, void *, int);
extern int             iskeyinfo(int, struct keydesc *, int);
/*extern void            islangchk(void);   -- Not implemented */
/*extern char           *islanginfo(char *);   -- Not implemented */
extern int             islock(int);
extern int             islogclose(void);
extern int             islogopen(signed char *);
/*extern int             isnlsversion(char *);   -- Not implemented */
/*extern int             isglsversion(char *);   -- Not implemented */
/*extern void            isnolangchk(void);   -- Not implemented */
extern int             isopen(signed char *, int);
extern int             isread(int, signed char *, int);
extern int             isrecover(void);
extern int             isrelease(int);
extern int             isrename(signed char *, signed char *);
extern int             isrewcurr(int, signed char *);
extern int             isrewrec(int, {self.lngsz}, signed char *);
extern int             isrewrite(int, signed char *);
extern int             isrollback(void);
extern int             issetunique(int, {self.lngsz});
extern int             isstart(int, struct keydesc *, int, signed char *, int);
extern int             isuniqueid(int, {self.lngsz} *);
extern int             isunlock(int);
extern int             iswrcurr(int, signed char *);
extern int             iswrite(int, signed char *);
'''
  def __init__(self, workdir, srcdir, instdir, blddir, bits=64):
    super().__init__(workdir, srcdir, instdir, 'long long int')
    self._blddir = pathlib.Path(blddir)

  def prepare(self):
    self._vbisam_so = pathlib.Path(f'libvbisam{self._soext}')
    libdir = self._srcdir / 'libvbisam'
    blddir = self._blddir / 'libvbisam'
    self.source_on_change(libdir / 'vbisam.h')
    self.source_on_change(libdir / 'vbdecimal.h')
    # Prefer a newly build version of libvbisam.so rather the one part of
    # the repository.
    if blddir.exists():
      self.source_on_change(blddir / self._vbisam_so)
    else:
      self.source_on_change(libdir / self._vbisam_so)

  def compile(self, modname):
    self._ffi.set_source(
      modname,
      '#include <stdint.h>\n#include "vbisam.h"',
      library_dirs=[str(self._workdir)],
      libraries=['vbisam'],
      runtime_library_dirs=['$ORIGIN/../lib'],
      include_dirs=[self._workdir],
    )
    self._compile()

  def install(self):
    self.install_on_change(self._mod_so, 'cffi')
    self.install_on_change(self._vbisam_so, 'lib')

if __name__ == '__main__':
  WORKDIR.mkdir(exist_ok=True)
  ifisam_bld = CFFI_IFISAM_Builder(WORKDIR, SOURCEDIR, INSTDIR)
  ifisam_bld.prepare()
  ifisam_bld.compile('_ifisam_cffi')
  ifisam_bld.install()
  vbisam_bld = CFFI_VBISAM_Builder(WORKDIR, SOURCEDIR, INSTDIR, 'mbuild')
  vbisam_bld.prepare()
  vbisam_bld.compile('_vbisam_cffi')
  vbisam_bld.install()
  
