# pyisam build file for meson
project(
  'pyisam', 'c', 
  version : '0.20',
)

# Get the compiler
cc = meson.get_compiler('c')

# Determine the list of compiler arguments
cflags = cc.get_supported_arguments([
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
add_project_arguments(['-imacros', meson.project_build_root() / 'config.h'], language: 'c')

# Create the configuration for the pyisam package
pyisam_conf = configuration_data()

# Select which variant to build support for
isamsupp = get_option('isamsupp')
if 'all' in isamsupp
  use_vbisam = true
  use_ifisam = true
  use_disam = true
elif 'default' in isamsupp
  use_vbisam = true
  use_ifisam = true
  use_disam = false
else
  use_vbisam = 'vbisam' in isamsupp
  use_ifisam = 'ifisam' in isamsupp
  use_disam = 'disam' in isamsupp
endif

# Add the standard header and functions for variants selected
std_hdrs = []
req_func = []
if use_vbisam
  conf.set10('HAVE_LFS64', true, description: 'Set if the system supports 64-bit I/O')
  conf.set10('VBDEBUG', false, description: 'Enable internal debug of vbisam')
  conf.set10('NEED_COUNT_ROWS', true, description: 'Enable the slower row counting routine')
  cont.set10('ISOPEN_SET_ISRECLEN', true, description: 'Make isopen() set the isreclen variale')
  std_hdrs += ['fcntl.h', 'unistd.h']
endif
pyisam_conf.set('PYISAM_BACKEND', get_option('default_backend',
                description: 'Set the default backend')
pyisam_conf.set('PYISAM_ISAMLIB', get_option('default_isamlib',
                description: 'Set the default ISAM library to use')

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

# Check if the fcntl.h header defines struct flock64
conf.set10('HAVE_STRUCT_FLOCK64', cc.has_header_symbol('fcntl.h', 'flock64'), description: 'Define if you use the flock64 structure')

# Create the config.h file for compilations
conf_h = configure_file(configuration: conf, output: 'config.h')
conf_py = configure_file(
  configuration: pyisam_conf,
  output: 'conf.py',
  input: meson.project_source_root() / 'src' / 'pyisam' / 'backend' / 'conf.py.in'
)

# Bring in the various libraries that are supported
subdir('lib')
