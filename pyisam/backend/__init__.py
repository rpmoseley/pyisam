'''
This module provides backend configuration for the pyisam package.

If the prefered backend fails to load, then the first variant of
the prefered backend is tried, if this also fails to load then the
package will fail with the ModuleImport exception.
'''

import importlib

_all_conf = ('ctypes', 'cffi')   # TODO: , 'cython')
_all_isam = ('vbisam', 'ifisam', 'disam')

# Pickup the interface to use
try:
  confmod = importlib.import_module('.conf', 'pyisam.backend')
  rawconf = getattr(confmod, 'backend', '')
except ModuleNotFoundError:
  rawconf = ''

# Validate the configuration and variant and default to the first allowed
use_conf, *rest = rawconf.split('.', 1)
if use_conf in _all_conf:
  use_isam = rest[0] if rest else _all_isam[0]
elif use_conf in _all_isam:
  use_isam, use_conf = use_conf, _all_conf[0]
else:
  use_isam, use_conf = _all_isam[0], _all_conf[0]

# Load the specific module to retrieve those objects specified in the __all__ variable,
# switching to the default variant if an error occurs
try:
  print(f'Trying {use_conf}.{use_isam} backend')
  _backend = importlib.import_module(f'.{use_conf}.{use_isam}', 'pyisam.backend')
except ImportError as exc:
  if use_isam != _all_isam[0]:
    print(f'Trying {use_conf}.{_all_isam[0]} backend, due to "{exc}"')
    _backend = importlib.import_module(f'.{use_conf}.{_all_isam[0]}', 'pyisam.backend')
    use_isam = _all_isam[0]
  else:
    raise exc
