# Instructions for building the embedded version of libvbisam
conf.set10('ISAMMODE', true, description: 'Set if compiling libvbisam in CISAM compatible mode')

# Define the source and headers within the library
vbisam_src = files([
  'isaudit.c',
  'isbuild.c',
  'ischeck.c',
  'iscommon.c',
  'isdecimal.c',
  'isdelete.c',
  'ishelper.c',
  'isopen.c',
  'isread.c',
  'isrecover.c',
  'isrewrite.c',
  'istrans.c',
  'iswrite.c',
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
])
vbisam_incl = include_directories('.')

# Define the shared library
libvbisam = library('vbisam', vbisam_src, vbisam_hdr)

# Build the binaries
subdir('bin')
