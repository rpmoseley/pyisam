py.install_sources('__init__.py', 'common.py',
  install_tag: 'runtime', subdir: 'pyisam/backend')
if pkgconf.get('cffi')
  subdir('cffi')
endif
if pkgconf.get('ctypes')
  subdir('ctypes')
endif
