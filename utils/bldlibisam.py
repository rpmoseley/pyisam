'''
Build the CFFI and CTYPES backend using the appropriate library for the specified
platform and bit-size (either 32- or 64-bit). By default build for the current
platform and bit-size of architecture the script is being run on.
'''
import hashlib
import pathlib
import shutil
import sysconfig
import subprocess

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

class BuildNoLibraryFound(FileNotFoundError):
  'Exception raised when the requested ISAM library is not available'

# Determine the extension used for libraries
_soext = sysconfig.get_config_var('SHLIB_SUFFIX')

class Builder:
  'Base class providing shared CFFI and CTYPES support'
  def __init__(self, workdir, srcdir, instdir, bits, lngsz=None):
    self._workdir = pathlib.Path(workdir)
    self._srcdir = pathlib.Path(srcdir)
    self._instdir = pathlib.Path(instdir)
    self.lngsz = lngsz
    self.bits = bits

  def patchlibrary(self, libname, rpath=None):
    'Patch the given library settings its SONAME and RPATH appropriately'
    def patch_option(option, value, path_libname):
      pret = subprocess.run(['patchelf', f'--print-{option}', path_libname],
                            stdout = subprocess.PIPE,
                            stderr = subprocess.DEVNULL)
      if pret.returncode == 0 and pret.stdout.rstrip() != value:
        pret = subprocess.run(['patchelf', f'--set-{option}', value, path_libname],
                              stdout = subprocess.DEVNULL,
                              stderr = subprocess.DEVNULL)
      return pret.returncode == 0

    bin_libname = libname.soext.encode()
    wrk_libname = self._workdir / libname.soext
    ok = patch_option('soname', bin_libname, wrk_libname)
    if ok and rpath:
      ok = patch_option('rpath', rpath, wrk_libname)
    if not ok:
      raise ValueError('Patchelf update failed')

  def _copy_on_change(self, srcfile, dstfile):
    'Internal helper method'
    if not srcfile.exists():
      raise BuildException(f'No source file to copy: {srcfile}')
    newhash = hashlib.sha256(srcfile.open('rb').read())
    if dstfile.exists():
      oldhash = hashlib.sha256(dstfile.open('rb').read())
      changed = newhash.digest() != oldhash.digest()
    else:
      changed = True
    if changed:
      shutil.copyfile(srcfile, dstfile)
      shutil.copymode(srcfile, dstfile)

  def source_on_change(self, srcdir, *filename):
    # Copy a new version of the given FILENAMEs into the working
    # directory if they have changed or are not present.
    if srcdir is None:
      srcdir = self._srcdir
    if isinstance(filename[0], list):
      filename = filename[0]
    for name in filename:
      iname = pathlib.Path(name.soext if isinstance(name, _Library) else name)
      if iname.is_absolute() or len(iname.parts) > 1:
        src_file = iname
        dst_file = self._workdir / iname.parts[-1]
      else:
        src_file = srcdir / iname
        dst_file = self._workdir / iname
      self._copy_on_change(src_file, dst_file)

  def install_on_change(self, libname, subdir=None):
    'Copy a new version of the given library if it has changed or not present'
    if isinstance(libname, _Library):
      sname = libname.soext
    else:
      sname = libname.parts[-1] if libname.is_absolute() else libname
    if subdir is None:
      iname = self._instdir / sname
    else:
      iname = self._instdir / subdir / sname
    self._copy_on_change(self._workdir / sname, iname)

class CTYPES_Builder(Builder):
  'Class providing the shared methods for the CTYPES builders'
  backend = 'ctypes'

  def prepare(self):
    'Prepare the work directory for building CTYPES support'
    libdir = self._srcdir / self._libdir
    if not libdir.exists():
      raise BuildNoLibraryFound(self.variant)
    self.source_on_change(libdir, self._libs)
    
  def compile(self):
    'Call the linker to create the interface library'
    # Make the names relative to the work directory
    wrk_outname = (self._workdir / self._mod_so.soext).as_posix()
    cmd = [
      'gold',                         # Use gold linker
      '-shared',                      # Create shared library
      '-rpath' ,'$ORIGIN/../lib',     # Dependant libraries
      '-soname', self._mod_so.soext,  # Set the SONAME in library
      '--output', wrk_outname,        # Name the library 
      '-L', str(self._workdir)        # Location of dependant libraris
    ]
    if self.bits == 32:
      cmd += ['--oformat', 'elf32-i386']
    if isinstance(self._libs, list):
      cmd += [lname.link for lname in self._libs]
    else:
      cmd.append(self._libs.link)
     
    # Call the linker using the return code to determine if it has failed
    pret = subprocess.run(cmd)
    if pret.returncode:
      raise BuildException('Linker failed')

class CFFI_Builder(Builder):
  'Class providing the shared methods for the CFFI builders'
  decimal_h_code = None
  isam_h_code = None
  max_key_parts = 8
  backend = 'cffi'

  def __init__(self, workdir, srcdir, instdir, bits, lngsz=None):
    import cffi
    Builder.__init__(self, workdir, srcdir, instdir, bits, lngsz)
    self._ffi = cffi.FFI()

  def prepare(self):
    libdir = self._srcdir / self._libdir
    if not libdir.exists():
      raise BuildNoLibraryFound(self.variant)
    incldir = libdir / 'include'
    if not incldir.exists():
      incldir = libdir
    self.source_on_change(incldir, self._hdrs)
    self.source_on_change(libdir, self._libs)

  def compile(self):
    if self.decimal_h_code:
      self._ffi.cdef(self.decimal_h_code.format(self=self))
    if self.isam_h_code:
      self._ffi.cdef(self.isam_h_code.format(self=self))
    self._mod_so = pathlib.Path(self._ffi.compile(tmpdir=self._workdir))

class _Library:
  'Class providing a means to handle library names according to use'
  def __init__(self, libname):
    self.short = libname
    self.long  = f'lib{libname}'
    self.soext = f'lib{libname}{_soext}'
    self.link  = f'-l{libname}'

class IFISAM_Mixin:
  'Class providing common settings for ifisam variant'
  _ifisam_so = _Library('ifisam')
  _ifisamx_so = _Library('ifisamx')
  _hdrs = ['isam.h', 'decimal.h']
  _libs = [_ifisam_so, _ifisamx_so]
  variant = 'ifisam'

  def __init__(self, bits):
    self._libdir = pathlib.Path('libifisam') / str(self.bits)

  def prepare(self):
    libdir = self._srcdir / self._libdir
    if not libdir.exists():
      raise BuildNoLibraryFound(self.variant)
    incldir = libdir / 'include'
    if not incldir.exists():
      incldir = libdir
    self.source_on_change(incldir, self._hdrs)
    self.source_on_change(libdir, self._libs)

  def install(self, addmissing=True):
    if addmissing:
      self.patchlibrary(self._ifisam_so, b'$ORIGIN')
      self.patchlibrary(self._ifisamx_so)
    self.install_on_change(self._mod_so, self.backend)
    self.install_on_change(self._ifisam_so, 'lib')
    self.install_on_change(self._ifisamx_so, 'lib')

class CTYPES_IFISAM_Builder(CTYPES_Builder, IFISAM_Mixin):
  'Class encapsulating the information to compile the IFISAM CTYPES module'
  _mod_so = _Library('pyifisam')

  def __init__(self, workdir, srcdir, instdir, bits=64):
    CTYPES_Builder.__init__(self, workdir, srcdir, instdir, bits, 'int32_t')
    IFISAM_Mixin.__init__(self, bits)
    
class CFFI_IFISAM_Builder(CFFI_Builder, IFISAM_Mixin):
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
    short          k_flags;
    short          k_nparts;
    struct keypart k_part[{self.max_key_parts}];
    short          k_len;
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
    CFFI_Builder.__init__(self, workdir, srcdir, instdir, bits, 'int32_t')
    IFISAM_Mixin.__init__(self, bits)

  def compile(self):
    self._ffi.set_source(
      f'_{self.variant}_cffi',
      '#include <stdint.h>\n#include "isam.h"',
      library_dirs=[str(self._workdir)],
      runtime_library_dirs=['$ORIGIN/../lib'],
      libraries=['ifisam', 'ifisamx'],
      include_dirs=[self._workdir],
    )
    super().compile()

class VBISAM_Mixin:
  _vbisam_so = _Library('vbisam')
  _hdrs = ['byteswap.h', 'isinternal.h', 'vbdecimal.h', 'vbisam.h']
  _libs = _vbisam_so
  _libdir = pathlib.Path('libvbisam')
  variant = 'vbisam'

  def install(self, addmissing=False):
    self.install_on_change(self._mod_so, self.backend)
    self.install_on_change(self._vbisam_so, 'lib')

class CTYPES_VBISAM_Builder(CTYPES_Builder, VBISAM_Mixin):
  'Class encapsulating the information to compile the VBISAM CTYPES module'
  _mod_so = _Library('pyvbisam')

  def __init__(self, workdir, srcdir, instdir, bits=64):
    CTYPES_Builder.__init__(self, workdir, srcdir, instdir, bits)

class CFFI_VBISAM_Builder(CFFI_Builder, VBISAM_Mixin):
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
    short          k_flags;
    short          k_nparts;
    struct keypart k_part[{self.max_key_parts}];
    short          k_len;
    ...;
}};
struct dictinfo {{
    short        di_nkeys;
    short        di_recsize;
    short        di_idxsize;
    {self.lngsz} di_nrecords;
}};
extern void         *vb_get_rtd(void);     /* Used to initialise library correctly */
extern int           is_nerr(void);
extern int           iserrno(void);
extern int           iserrio(void);
extern {self.lngsz}  isrecnum(void);
extern int           isreclen(void);
extern const char   *is_strerror(int);
extern int           isaddindex(int, struct keydesc *);
extern int           isaudit(int, signed char *, int);
extern int           isbegin(void);
extern int           isbuild(signed char *, int, struct keydesc *, int);
extern int           iscleanup(void);
extern int           isclose(int);
extern int           iscluster(int, struct keydesc *);
extern int           iscommit(void);
extern int           isdelcurr(int);
extern int           isdelete(int, signed char *);
extern int           isdelindex(int, struct keydesc *);
extern int           isdelrec(int, {self.lngsz});
extern int           isdictinfo(int, struct dictinfo *);
extern int           iserase(signed char *);
extern int           isflush(int);
extern int           isindexinfo(int, void *, int);
extern int           iskeyinfo(int, struct keydesc *, int);
/*extern void          islangchk(void);   -- Not implemented */
/*extern char         *islanginfo(char *);   -- Not implemented */
extern int           islock(int);
extern int           islogclose(void);
extern int           islogopen(signed char *);
/*extern int           isnlsversion(char *);   -- Not implemented */
/*extern int           isglsversion(char *);   -- Not implemented */
/*extern void          isnolangchk(void);   -- Not implemented */
extern int           isopen(signed char *, int);
extern int           isread(int, signed char *, int);
extern int           isrecover(void);
extern int           isrelease(int);
extern int           isrename(signed char *, signed char *);
extern int           isrewcurr(int, signed char *);
extern int           isrewrec(int, {self.lngsz}, signed char *);
extern int           isrewrite(int, signed char *);
extern int           isrollback(void);
extern int           issetunique(int, {self.lngsz});
extern int           isstart(int, struct keydesc *, int, signed char *, int);
extern int           isuniqueid(int, {self.lngsz} *);
extern int           isunlock(int);
extern int           iswrcurr(int, signed char *);
extern int           iswrite(int, signed char *);
'''
  def __init__(self, workdir, srcdir, instdir, bits=64):
    CFFI_Builder.__init__(self, workdir, srcdir, instdir, bits, 'long long int')

  def compile(self):
    self._ffi.set_source(
      f'_{self.variant}_cffi',
      '#include <stdint.h>\n#include "vbisam.h"',
      library_dirs=[str(self._workdir)],
      libraries=['vbisam'],
      runtime_library_dirs=['$ORIGIN/../lib'],
      include_dirs=[self._workdir],
      define_macros=[('NEED_IFISAM_COMPAT', '1'), ('NEED_ROW_COUNT', 1)],
    )
    super().compile()

class DISAM_Mixin:
  _disam_so = _Library('disam72')
  _hdrs = ['disam.h', 'isconfig.h', 'isintstd.h', 'iswrap.h']
  _libs = _disam_so
  _libdir = pathlib.Path('libdisam')
  variant = 'disam'

class CTYPES_DISAM_Builder(CTYPES_Builder, DISAM_Mixin):
  'Class encapsulating the information to compile the DISAM CTYPES module'
  _mod_so = _Library('pydisam')

class CFFI_DISAM_Builder(CFFI_Builder, DISAM_Mixin):
  'Class encapsulating the information to compile the DISAM CFFI module'
  # This can support more key parts
  max_key_parts = 20

  # Define items found in disam.h
  isam_h_code = '''
struct keypart {{
    short kp_start;
    short kp_leng;
    short kp_type;
}};
struct keydesc {{
    short          k_flags;
    short          k_nparts;
    struct keypart k_part[{self.max_key_parts}];
    short          k_len;
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
extern long int      isrecnum;
extern int           isreclen;
extern char         *isversnumber;
extern char         *iscopyright;
/* extern char         *isserial;	 -- Not Implemented */
/* extern int           issingleuser;  -- Not Implemented */
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
extern int           iserase(char *);
extern int           isflush(int);
extern int           isindexinfo(int, struct keydesc *, int);
extern int           isisaminfo(int, struct dictinfo *);
/* extern void          islangchk(void);  -- Not Implemented */
/* extern char         *islanginfo(char *); -- Not Implemented */
extern int           islock(int);
extern int           islogclose(void);
extern int           islogopen(char *);
/* extern int           isnlsversion(char *); -- Not Implemented */
/* extern int           isglsversion(char *); -- Not Implemented */
/* extern void          isnolangchk(void);  -- Not Implemented */
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
    super().__init__(workdir, srcdir, instdir, 'uint32_t', bits)

  def compile(self):
    self._ffi.set_source(
      f'_{self.variant}_cffi',
      '#include <stdint.h>\n#include "disam.h"',
      library_dirs=[str(self._workdir)],
      runtime_library_dirs=['$ORIGIN/../lib'],
      libraries=['disam72'],
      include_dirs=[self._workdir],
    )
    super().compile()

class ModuleGenerator:
  'Class encapsulating the module generation logic'
  def __init__(self, workdir,           # work directory
                     sourcedir,         # source directory
                     installdir,        # install directory
                     bld_cffi = True,   # enable CFFI modules
                     bld_ctypes = True, # enable CTYPES modules
                     bld_ifisam = True, # enable IFISAM variant
                     bld_vbisam = True, # enable VBISAM variant
                     bld_disam = False, # enable DISAM variant
              ):
    klasslist = list()
    if bld_cffi:
      try:
        import cffi
      except ImportError:
        bld_cffi = False

    def cond_append(flag, cffi, ctypes):
      if flag:
        if bld_cffi and cffi is not None:
          klasslst.append(cffi)
        if bld_ctypes and ctypes is not None:
          klasslst.append(ctypes)

    cond_append(bld_ifisam, CFFI_IFISAM_Builder, CTYPES_IFISAM_Builder)
    cond_append(bld_vbisam, CFFI_VBISAM_Builder, CTYPES_VBISAM_Builder)
    cond_append(bld_disam, CFFI_DISAM_Builder, CTYPES_DISAM_Builder)
    if klasslist:
      workdir.mkdir(exist_ok=True)
    
  def prepare(self):
    for mod in self._modules:
      mod.prepare()

  def compile(self):
    for mod in self._modules:
      mod.compile()

  def install(self):
    for mod in self._modules:
      mod.install()

if __name__ == '__main2__':
  mods = ModuleGenerator(workdir, sourcedir, installdir)
  mods.prepare()
  mods.compile()
  mods.install()

if __name__ == '__main__':
  bld_cffi = True      # Enable building of CFFI modules
  bld_ctypes = False    # Enable building of CTYPES modules
  bld_ifisam = False    # Enable building of IFISAM variant
  bld_vbisam = True    # Enable building of VBISAM variant
  bld_disam = False    # Enable building of DISAM variant
  do_install = False    # Enable installation of libraries

  # Store the backend/variants actually created to process later
  all_modules = []
  if bld_cffi:
    # Prepare for building the CFFI modules
    if bld_ifisam:
      all_modules.append(CFFI_IFISAM_Builder(WORKDIR, SOURCEDIR, INSTDIR))
    if bld_vbisam:
      all_modules.append(CFFI_VBISAM_Builder(WORKDIR, SOURCEDIR, INSTDIR, 'mbuild'))
    if bld_disam:
      all_modules.append(CFFI_DISAM_Builder(WORKDIR, SOURCEDIR, INSTDIR))

  if bld_ctypes:
    # Prepare for building the CTYPES modules
    if bld_ifisam:
      all_modules.append(CTYPES_IFISAM_Builder(WORKDIR, SOURCEDIR, INSTDIR))
    if bld_vbisam:
      all_modules.append(CTYPES_VBISAM_Builder(WORKDIR, SOURCEDIR, INSTDIR, 'mbuild'))
    if bld_disam:
      all_modules.append(CTYPES_DISAM_Builder(WORKDIR, SOURCEDIR, INSTDIR))

  # If we are processing anything create the working directory
  if all_modules:
    WORKDIR.mkdir(exist_ok=True)

    # Process the list of modules to bring all necessary files into place first
    for bldmod in all_modules:
      bldmod.prepare()
    for bldmod in all_modules:
      bldmod.compile()
    if do_install:
      for bldmod in all_modules:
        bldmod.install()
