'''
This module will provied the version by checking the following in turn,
the metadata for the package when installed, the contents of the pyproject.toml
file, a fixed version is given.
'''

__all__ = ('__version__',)

import importlib.metadata
try: 
  __version__ = importlib.metadata.version(__package__)
except importlib.metadata.PackageNotFoundError:
  import pathlib
  import tomllib
  curpath = pathlib.Path(__file__).parent.parent.parent # Give ../..
  try:
    with open(curpath / 'pyproject.toml', 'rb') as ifd:
      data = tomllib.load(ifd)
      for toplevel in ('project', 'tool'):
        if toplevel in data:
          match toplevel:
            case 'project':
              projobj = data['project']
            case 'tool':
              projobj = data['tool']['poetry']
            case _:
              raise KeyError
          __version__  = projobj['version']
          break
  except:
    __version__ = '0.20dev'
