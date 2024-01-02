# Define the source and headers within the library
vbisam_src = files([
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
])
vbisam_hdr = files([
  'byteswap.h',
  'isinternal.h',
  'vbdecimal.h',
  'vbisam.h',
])
vbisam_incl = include_directories(['.'])

# Define the shared library
libvbisam = library('vbisam',
  vbisam_src, vbisam_hdr,
  c_args: cflags,
  include_directories: vbisam_incl,
)

# Build the binaries
##subdir('bin')
