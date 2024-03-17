'''
This module provides backend configuration for the pyisam package.

If the configured variant and backend fails to load, due to a missing
required library, then the following rules are followed:

   'cffi.*' -> 'ctypes.*' -> FAIL
   '*.disam' -> '*.ifisam' -> *.vbisam' -> FAIL
'''

import importlib
from .common import MaxKeyParts, MaxKeyLength

__all__ = ('MaxKeyParts', 'MaxKeyLength')

_all_conf = ('ctypes', 'cffi')   # TODO: , 'cython')
_all_isam = ('vbisam', 'ifisam', 'disam')

# Pickup the interface to use, defaulting to CFFI
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
  use_conf, use_isam = _all_conf[0], _all_isam[0]

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
