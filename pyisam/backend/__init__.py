'''
This module provides backend configuration for the pyisam package
'''

import importlib
from .common import MaxKeyParts, MaxKeyLength, _checkpart

__all__ = ('ISAMobjectMixin', 'ISAMindexMixin', 'RecordBuffer', 'ISAMdictinfo',
           'ISAMkeydesc', 'MaxKeyParts', 'MaxKeyLength', '_checkpart', 
           'use_conf', 'use_variant', 'cffi_obj')
_allowed_conf = ('cffi', 'ctypes')   # TODO: , 'cython')
_allowed_isamlib = ('ifisam', 'vbisam', 'disam')
_dflt_conf = 'cffi'
_dflt_variant = 'ifisam'

# Pickup the interface to use, defaulting to CFFI
try:
  confmod = importlib.import_module('.conf', 'pyisam.backend')
  use_conf, use_variant = getattr(confmod, 'backend').split('.')
  if use_conf == '':
    use_conf = _dflt_conf
  if use_variant == '':
    use_variant = _dflt_variant
except ModuleNotFoundError:
  use_conf = _dflt_conf
  use_variant = _dflt_variant

# Validate the backend to those allowed
if use_conf not in _allowed_conf or use_variant not in _allowed_isamlib:
  raise ModuleNotFoundError()

# Recombine back into one string
fullconf = f'{use_conf}.{use_variant}'

# Load the specific module to retrieve those objects specified in the __all__ variable,
# switching the default variant if an error occurs
try:
  mod_obj = importlib.import_module(f'.{fullconf}', 'pyisam.backend')
except:
  print(f'Switching to {_dflt_variant} variant')
  use_variant = _dflt_variant
  fullconf = f'{use_conf}.{_dflt_variant}'
  mod_obj = importlib.import_module(f'.{fullconf}', 'pyisam.backend')

# Reference the objects made available to the application
ISAMobjectMixin = getattr(mod_obj, f'ISAM{use_variant}Mixin')
ISAMindexMixin = mod_obj.ISAMindexMixin
RecordBuffer = mod_obj.RecordBuffer
ISAMdictinfo = mod_obj.dictinfo
ISAMkeydesc = mod_obj.keydesc
cffi_obj = getattr(mod_obj, 'ffi', None)
