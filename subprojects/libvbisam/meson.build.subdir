project('libvbisam', 'c',
        version: '2.1.1',
        meson_version: '>1.6.0')

# Determine the list of compiler arguments
cc = meson.get_compiler('c')
vbflags = cc.get_supported_arguments([
  '-finline-functions',
  '-fomit-frame-pointer',
  '-fsigned-char',
  '-Wall',
  '-Wwrite-strings',
  '-Wmissing-prototypes',
  '-Wno-format-y2k',
])

# Configure VBISAM
conf = configuration_data()
conf.set10('HAVE_LFS64',
           true,
           description: 'Set if the system supports 64-bit I/O')
conf.set10('VBDEBUG',
           false,
           description: 'Enable internal debug of vbisam')
conf.set10('ISAMMODE',
           get_option('extended'),
           description: 'Set if compiling vbisam in extended mode')
conf.set10('NEED_COUNT_ROWS',
           true,
           description: 'Enable the slower row counting routine')
conf.set10('ISOPEN_SET_ISRELEN',
           true,
           description: 'Make isopen() set the isreclen variable')
has_fcntl_h = cc.has_header('fcntl.h')
conf.set10('HAVE_FCNTL_H',
           has_fcntl_h,
           description: 'Define if have the fcntl.h header')
conf.set10('HAVE_UNISTD_H',
           cc.has_header('unistd.h'),
           description: 'Define if have the unistd.h header')
if has_fcntl_h
  conf.set10('HAVE_STRUCT_FLOCK64',
             cc.has_header_symbol('fcntl.h', 'flock64'),
             description: 'Define if use the flock64 structure')
endif
configure_file(configuration: conf, output: 'config.h')
add_project_arguments(['-imacros', 'config.h'], language: 'c')

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

# Load the python support for meson-python and cffi-buildtool
py = import('python').find_installation(pure: false)
gen_cffi_src = find_program('gen-cffi-src', required: use_cffi)

# Define the source and headers within the library
vbisam_src = files(
  'isaudit.c',
  'isbuild.c',
  'ischeck.c',
  'isdecimal.c',
  'isdelete.c',
  'ishelper.c',
  'isopen.c',
  'isread.c',
  'isrecover.c',
  'isrewrite.c',
  'istrans.c',
  'iswrite.c',
  'vbcompat.c',
  'vbdataio.c',
  'vbindexio.c',
  'vbkeysio.c',
  'vblocking.c',
  'vblowlevel.c',
  'vbmemio.c',
  'vbnodememio.c',
)
vbisam_hdr = files(
  'byteswap.h',
  'isinternal.h',
  'vbdecimal.h',
  'vbisam.h',
)

# Define the location of the include header files
vbisam_incl = include_directories('.')

# Define the shared library
vbisam_lib = shared_library('vbisam',
  vbisam_src, vbisam_hdr,
  c_args: vbflags,
  include_directories: vbisam_incl,
  #install: true,
  #install_dir: 'pyisam/backend/lib',
)

libvbisam_dep = declare_dependency(
  include_directories: vbisam_incl,
  link_with: vbisam_lib,
)

# Define the CFFI extension module if required
if use_cffi and gen_cffi_src.found()
  message('Building CFFI support for VBISAM')
  vbisam_cffi_src = custom_target(
    command: [
              gen_cffi_src,
              'read-sources',
              '_vbisam_cffi',
              '@INPUT0@',
              '@INPUT1@',
              '@OUTPUT@',
             ],
     output: '_vbisam_cffi.c',
     input: [
             'vbisam_cffi.cdef.txt',
             'vbisam_cffi.csrc.c',
            ],
  )

  vbisam_cffi_mod = py.extension_module(
    '_vbisam_cffi',
    vbisam_cffi_src,
    subdir: 'pyisam/backend/cffi',
    install: true,
    install_rpath: '$ORIGIN/../lib',
    dependencies: [ libvbisam_dep, py.dependency() ],
    link_with: vbisam_lib,
  )
endif

if use_ctypes
  message('Building CTYPES support for VBISAM')
  vbisam_ctypes_mod = custom_target(
    command: [
      'gold',
      '-shared',
      '-rpath', '$ORIGIN/../lib',
      '-soname', '@OUTPUT@',
      '--output', '@OUTPUT@',
      '-L', meson.current_build_dir(),
      '@INPUT@',
    ],
    output: 'libpyvbisam.so',
    input: vbisam_lib,
    #install_dir: 'pyisam/backend/ctypes',
  )
endif

