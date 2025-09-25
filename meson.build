# pyisam build file for meson
project(
  'pyisam', 'c', 
  version : files('VERSION'),
  meson_version : '>1.1.0',
)

# Select which variants to build support for
isamsupp = get_option('isamsupp')
ffisupp = get_option('ffisupp')

pkgconf = configuration_data()
pkgconf.set('vbisam', 'all' in isamsupp or 'default' in isamsupp or 'vbisam' in isamsupp)
pkgconf.set('ifisam', 'all' in isamsupp or 'default' in isamsupp or 'ifisam' in isamsupp)
pkgconf.set('disam',  'all' in isamsupp or                          'disam'  in isamsupp)
pkgconf.set('cffi',   'all' in ffisupp  or 'default' in ffisupp  or 'cffi'   in ffisupp)
pkgconf.set('ctypes', 'all' in ffisupp  or 'default' in ffisupp  or 'ctypes' in ffisupp)
pkgconf.set('cython',                                               'cython' in ffisupp)

# Load the compiler and check for some header files
cc = meson.get_compiler('c')
has_fcntl_h = cc.has_header('fcntl.h')

# Load the python support for meson-python and cffi-buildtool
py = import('python').find_installation(pure: true)
gen_cffi_src = find_program('gen-cffi-src', required: pkgconf.get('cffi'))

# Load the source set support
ssmod = import('sourceset')
python_sources = ssmod.source_set()

# Configure VBISAM
if pkgconf.get('vbisam')
  pkgconf.set10('HAVE_LFS64', true,
                description: 'Set if the system supports 64-bit I/O')
  pkgconf.set10('VBDEBUG', false,
                description: 'Enable the internal debug of VBISAM')
  pkgconf.set10('ISAMMODE', get_option('vb_extended'),
                description: 'Set if compiling VBISAM in extended mode')
  pkgconf.set10('NEED_COUNT_ROWS', true,
                description: 'Enable the slower row counting routine')
  pkgconf.set10('ISOPEN_SET_ISRECLEN', true,
                description: 'Make isopen() set the isreclen variable')
  pkgconf.set10('HAVE_FCNTL_H', has_fcntl_h,
                description: 'Define if you have the fcntl.h header')
  pkgconf.set10('HAVE_UNISTD_H', cc.has_header('unistd.h'),
                description: 'Define if you have the unistd.h header')
  if has_fcntl_h
    pkgconf.set10('HAVE_STRUCT_FLOCK64', cc.has_header_symbol('fcntl.h', 'flock64'),
                  description: 'Define if using the flock64 structure')
  endif
endif

# Create the configuration for the pyisam package
pyisam_conf = configuration_data()
pyisam_conf.set('PYISAM_BACKEND', get_option('default_backend'), description: 'Default backend to be used')
pyisam_conf.set('PYISAM_ISAMLIB', get_option('default_isamlib'), description: 'Default ISAM library to be used')
pyisam_conf.set('PYISAM_VERSION', meson.project_version(),       description: 'Version of package')
conf_py = configure_file(
  configuration: pyisam_conf,
  output: 'conf.py',
  input: 'src/pyisam/backend/conf.py.in',
  install_dir: py.get_install_dir() + 'pyisam/backend',
  install_mode: 'r--r--r--',
  install_tag: 'runtime',
)

# Compile the underlying ISAM libraries as required
if pkgconf.get('vbisam')
  subdir('lib/libvbisam') 
endif
if pkgconf.get('ifisam')
  subdir('lib/libifisam')
endif
if pkgconf.get('disam')
  subdir('lib/libdisam')
endif

# Install all the various subdirectories of the package
subdir('src/pyisam')
