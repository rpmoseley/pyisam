'''
This will check the list of files in pyisam.lst against the like-named files
in the repository located in ~/repos/git/pyisam, in order to find which files
may need to be copied across in order to permit a push to be made to the
github repository.
'''

import os
import shutil

local_repos  = os.path.expanduser('~/repos/git/pyisam')
local_work   = os.path.expanduser('~/virtenv/cffi35')
remote_repos = 'https://github.com/rpmoseley/pyisam'
file_list    = 'pyisam.lst'

with open(file_list) as fd:
  for rawname in fd:
    name = rawname.rstrip()
    own_stat = os.stat(name)
    try:
      rep_stat = os.stat(os.path.join(local_repos, name))
      print(' ' if own_stat.st_mtime == rep_stat.st_mtime else '*', name)
    except FileNotFoundError:
      print('-', name)
