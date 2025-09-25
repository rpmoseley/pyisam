python_sources = ssmod.source_set()
python_sources.add(['__init__.py', 'common.py'])
python_sources.add(when: 'ifisam', if_true: ['ifisam.py', libifisam_dep])
python_sources.add(when: 'vbisam', if_true: ['vbisam.py', libvbisam_dep])
py.install_sources(python_sources.all_sources(),
  install_tag: 'runtime', subdir: 'pyisam/backend/cffi')
