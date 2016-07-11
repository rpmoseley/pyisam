'''
This program will update all those packages consider by pip to outdated.
It makes the assumption that those package that are wheels do not cause
breakage of other packages already installed.
'''
from distutils.version import LooseVersion
from subprocess import check_output, check_call
import re

test_pip_output = '''\
Pygments (2.1.1) - Latest: 2.1.3 [wheel]
setuptools (20.1.1) - Latest: 20.2.2 [wheel]
Sphinx (1.3.5) - Latest: 1.3.6 [wheel]
pip (8.0.2) - Latest: 8.1.0 [wheel]
You are using pip version 8.0.2, however version 8.1.0 is available
You should consider upgrading via the 'pip install --upgrade pip' command.
'''

pip_cmd = 'pip'
pip_line_re = re.compile('(?P<name>\w+)\s+\((?P<oldvers>[\d.]+)\)\s+-\s+Latest:\s(?P<newvers>[\d.]+)\s+\[(?P<dist>\w+)\]', flags=re.MULTILINE)

class PkgDetails:
  '''Class representing a package that is to be updated'''
  def __init__(self, mtchobj=None, pip_cmd='pip', *args):
    self.pip_cmd = pip_cmd
    if mtchobj is None:
      if len(args) != 4: raise ValueError('Must pass: name, oldvers, newvers, dist')
      self.pkgname = args[0]
      self.oldvers = LooseVersion(args[1])
      self.newvers = LooseVersion(args[2])
      self.pkgdist = args[3]
    else:
      self.pkgname = mtchobj['name']
      self.oldvers = LooseVersion(mtchobj['oldvers'])
      self.newvers = LooseVersion(mtchobj['newvers'])
      self.pkgdist = mtchobj['dist']
  def upgrade(self):
    'Upgrade the package using the pip command'
    check_call([self.pip_cmd, 'install', '--upgrade', self.pkgname])
  def __str__(self):
    return '{0.pkgname} [{0.pkgdist}] ({0.oldvers} -> {0.newvers})'.format(self) 

def get_outdated_packages(pip_cmd='pip', test_output=None):
  if isinstance(test_output, str) and len(test_output) > 0:
    print('Using testing list of packages ...')
    pip_output = test_output
  else:
    print('Fetching list of outdated packages ...')
    pip_output = check_output([pip_cmd, 'list', '-o']).decode()
  result = []
  for mtch in pip_line_re.finditer(pip_output):
    mtchdict = mtch.groupdict()
    print(mtchdict)
    if mtchdict['name'] == 'pip':
      result.insert(0, PkgDetails(mtchdict))
    else:
      result.append(PkgDetails(mtchdict))
  return result

pkg_outdated = get_outdated_packages()
if pkg_outdated:
  for pkg in pkg_outdated:
    pkg.upgrade()
else:
  print('Nothing to update')
