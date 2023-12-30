# pyisam build file for meson
project(
  'pyisam', 'c', 
  version : '0.10',
)

# Get the compiler
cc = meson.get_compiler('c')

# Determine the list of compiler arguments
cflags = cc.get_supported_arguments([
  '-O',
  '-finline-functions',
  '-fomit-frame-pointer',
  '-fsigned-char',
  '-Wall',
  '-Wwrite-strings',
  '-Wmissing-prototypes',
  '-Wno-format-y2k',
])

# Create the configuration to store settings in
conf = configuration_data()

# Create the configuration for the pyisam package
pyisam_conf = configuration_data()

# Select whether to compile vbisam or ifisam support
use_vbisam = get_option('vbisam')
if use_vbisam
  add_project_arguments(['-imacros', meson.project_build_root() / 'config.h'], language: 'c')
  #conf.set10('ISAMMODE', true, description: 'Set if compiling vbisam in CISAM compatible mode')
  pyisam_conf.set('PYISAM_ISAMLIB', 'vbisam', description: 'Default backend to be used')
  conf.set10('HAVE_LFS64', true, description: 'Set if the system supports 64-bit I/O')
  conf.set10('VDEBUG', false, description: 'Enable internal debug of vbisam')
  conf.set10('HAVE_STRUCT_FLOCK64', cc.has_header_symbol('fcntl.h', 'flock64'), description: 'Define if have the struct flock64')
  std_hdrs = ['fcntl.h', 'unistd.h']
  req_func = []
else
  pyisam_conf.set('PYISAM_ISAMLIB', 'ifisam', description: 'Default backend to be used')
  std_hdrs = []
  req_func = []
endif
pyisam_conf.set('PYISAM_BACKEND', 'cffi', description: 'Set the default backend')

# Check for the required headers
foreach hdr: std_hdrs
  conf.set10('HAVE_' + hdr.underscorify().to_upper(),
    cc.has_header(hdr),
    description: 'Define if have the @hdr@ header')
endforeach
foreach fun: req_func
  conf.set10('HAVE_' + fun.underscorify().to_upper(),
    cc.has_function(fun),
    description: 'Define if have the @fun@ function')
endforeach

# Create the config.h file for compilations
conf_h = configure_file(configuration: conf, output: 'config.h')
pyisam_conf = configure_file(
  configuration: pyisam_conf,
  output: 'conf.py',
  input: meson.project_source_root() / 'pyisam' / 'backend' / 'conf.py.in'
)

# Include the library source being compiled
if use_vbisam
  # Include the libvbisam support
  subdir('libvbisam')
else
  # Include the libifisam/libifisamx support
  subdir('libifisam')
endif
