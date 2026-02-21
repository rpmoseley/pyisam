# Define the actions to enable the C-ISAM libraries to be used

libifisam_dir = '-L@0@'.format(meson.current_source_dir())

# Install the libraries required by all backends
install_data(
  'libifisam.so',
  'libifisamx.so', 
  install_dir: py.get_install_dir() + 'pyisam/backend/lib',
  install_mode: 'rwxr-xr-x',
  install_tag: 'runtime',
)

libifisam_link = [libifisam_dir, '-lifisam', '-lifisamx']

# Build the CTYPES helper library
if pkgconf.get('ctypes')
  libifisam_ctypes = shared_library('pyifisam',
    '_dummy.c',
    link_args: libifisam_link,
    install: true,
    install_dir: py.get_install_dir() + 'pyisam/backend/ctypes',
    install_mode: 'r-xr-xr-x',
    install_rpath: '$ORIGIN/../lib',
    install_tag: 'runtime',
  )
endif

# Build the CFFI extension if required
if pkgconf.get('cffi') and gen_cffi_src.found()
  libifisam_cffi_src = custom_target(
    command: [
      gen_cffi_src,
      'read-sources',
      '_ifisam_cffi',       # WAS: pyisam.backend.cffi._ifisam_cffi
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

  libifisam_cffi = py.extension_module(
    '_ifisam_cffi',
    libifisam_cffi_src,
    #CHK#dependencies: py.dependency(),
    link_args: libifisam_link,
    install: true,
    subdir: 'pyisam/backend/cffi',
    install_mode: 'rwxr-xr-x',
    install_rpath: '$ORIGIN/../lib',
    install_tag: 'runtime',
  )
endif
