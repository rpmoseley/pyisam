# TODO: Rewrite to make use of py.language_version() and appropriate library naming convention function
pycffi_sources = ssmod.source_set()
pycffi_sources.add(['__init__.py', 'common.py'])
pycffi_sources.add(when: 'ifisam', if_true: ['ifisam.py', libifisam_cffi_dep])
pycffi_sources.add(when: 'vbisam', if_true: ['vbisam.py', libvbisam_cffi_dep])
pycffi_sources.add(when: 'disam', if_true: ['disam.py', libdisam_cffi_dep])
py.install_sources(pycffi_sources.all_sources(), install_tag: 'runtime', subdir: 'pyisam/backend/cffi')
