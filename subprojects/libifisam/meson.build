project('libifisam', 'c',
        version: '7.2',
        meson_version: '>1.6.0')

# Determine backends required
ffisupp = get_option('ffisupp')
if 'all' in ffisupp
  use_cffi   = true
  use_ctypes = true
  use_cython = false
elif 'default' in ffisupp
  use_cffi   = false
  use_ctypes = true
  use_cython = false
else
  use_cffi   = 'cffi' in ffisupp
  use_ctypes = 'ctypes' in ffisupp
  use_cython = 'cython' in ffisupp
endif
message(f'use_cffi = @use_cffi@, use_ctypes = @use_ctypes@, use_cython = @use_cython@')

# Get the compiler object to perform library lookup
cc = meson.get_compiler('c')

# Load the python support for meson-python and cffi-buildtool
py = import('python').find_installation(pure: false)
gen_cffi_src = find_program('gen-cffi-src', required: use_cffi)

# Copy the ISAM support libraries
fs = import('fs')
isam_lib = fs.copyfile('libifisam.so', 'libifisam.so', ) # install: true, install_dir: 'pyisam/backend/lib')
isamx_lib = fs.copyfile('libifisamx.so', 'libifisamx.so', ) # install: true, install_dir: 'pyisam/backend/lib')

# Define the distributed libraries and include directories
ifisam_incl = include_directories(['.'])
ifisam_hdr = files('decimal.h', 'isam.h')
ifisam_lib = [isam_lib, isamx_lib]

libifisam_dep = declare_dependency(
  include_directories: ifisam_incl,
  sources: ifisam_lib
)

# Define the CFFI extension module if required
if use_cffi and gen_cffi_src.found()
  message('Building CFFI support for IFISAM')
  ifisam_cffi_src = custom_target(
    command: [
      gen_cffi_src,
      'read-sources',
      '_ifisam_cffi',
      '@INPUT0@',
      '@INPUT1@',
      '@OUTPUT@',
    ],
    output: '_ifisam_cffi.c',
    input: [
      'ifisam_cffi.cdef.txt',
      'ifisam_cffi.csrc.c',
    ],
  )

  ifisam_cffi_mod = py.extension_module(
    '_ifisam_cffi',
    ifisam_cffi_src,
    subdir: 'pyisam/backend/cffi',
    install: true,
    install_rpath: '$ORIGIN/../lib',
    dependencies: [ libifisam_dep, py.dependency() ],
  )
endif

if use_ctypes
  message('Building CTYPES support for IFISAM')
  ifisam_ctypes_mod = custom_target(
    command: [
      'gold',
      '-shared',
      '-rpath', '$ORIGIN/../lib',
      '-soname', '@OUTPUT@',
      '--output', '@OUTPUT@',
      '-L', meson.current_build_dir(),
      '@INPUT@',
    ],
    input: ifisam_lib,
    output: 'libpyifisam.so',
    #install_dir: 'pyisam/backend/ctypes',
  )
endif
