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
vbisam_incl = include_directories(['.'])
vbflags = cc.get_supported_arguments([
  '-finline-functions',
  '-fomit-frame-pointer',
  '-fsigned-char',
  '-Wall',
  '-Wwrite-strings',
  '-Wmissing-prototypes',
  '-Wno-format-y2k',
])
configure_file(configuration: pkgconf, output: 'config.h', input: 'config.h.in')
add_project_arguments(['-imacros', 'config.h'], language: 'c')

# Define the shared library
libvbisam = shared_library('vbisam',
  vbisam_src, vbisam_hdr,
  c_args: vbflags,
  include_directories: vbisam_incl,
)

# Build the CFFI extension if required
if gen_cffi_src.found()
  libvbisam_cffi_src = custom_target(
    command: [
        gen_cffi_src,
        'read-sources',
        'pyisam.backend.cffi._vbisam_cffi',
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

  libvbisam_cffi = py.extension_module(
    '_vbisam_cffi',
    libvbisam_cffi_src,
    subdir: 'pyisam.backend.cffi',
    install: true,
    dependencies: [py.dependency()],
    link_with: libvbisam,
  ) 
endif

# Build the binaries
##subdir('bin')
