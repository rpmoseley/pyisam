project('libdisam', 'c',
        version: '7.2',
        meson_version: '>1.6.0')

# Determine backend required
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

# Load the python support for meson-python and cffi-buildtool
py = import('python').find_installation()
gen_cffi_src = find_program('gen-cffi-src', required: use_cffi)

# Define the distributed libraries and include directories
disam_incl = include_directories(['.'])
disam_lib = files('libdisam72.so')
disam_hdr = files('disam.h', 'isconfig.h', 'isintstd.h', 'iswrap.h')

libdisam_dep = declare_dependency(
  include_directories: disam_incl,
  link_with: disam_lib,
)

# Define the CFFI extension module if required
if use_cffi and gen_cffi_src.found()
  disam_cffi_src = custom_target(
    command: [
      gen_cffi_src,
      'read-sources',
      '_disam_cffi',
      '@INPUT0@',
      '@INPUT1@',
      '@OUTPUT@',
    ],
    output: '_disam_cffi.c',
    input: [
      'disam_cffi.cdef.txt',
      'disam_cffi.csrc.c',
    ],
  )

  disam_cffi_mod = py.extension_module(
    '_disam_cffi',
    disam_cffi_src,
    subdir: 'pyisam/backend/cffi',
    install: true
    dependencies: [ libdisam_dep, py.dependency() ],
  )
endif

if use_ctypes
  disam_ctypes_mod = custom_target(
    command: [
      'gold',
      '-shared',
      '-rpath', '$ORIGIN/../lib',
      '-soname', '@OUTPUT0@',
      '--output', '@OUTPUT0@',
      '-L', '@OUTPUT1@',
      '@INPUT@',
    ],
    output: [
      'libpydisam.so',
      meson.current_build_dir(),
    ],
    input: disam_lib,
    install_dir: 'pyisam/backend/ctypes',
    dependencies: libdisam_dep,
  )
endif
