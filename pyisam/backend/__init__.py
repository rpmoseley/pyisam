'''
This module provides backend configuration for the pyisam package
'''

import importlib
from .common import MaxKeyParts, MaxKeyLength, _checkpart

__all__ = ('ISAMobjectMixin', 'ISAMindexMixin', 'RecordBuffer', 'ISAMdictinfo',
           'ISAMkeydesc', 'MaxKeyParts', 'MaxKeyLength', '_checkpart', 
           'use_conf', 'use_isamlib')
_allowed_conf = ('cffi', 'ctypes', 'cython')
_allowed_isamlib = ('ifisam', 'vbisam')
_dflt_conf = 'cffi'
_dflt_isamlib = 'ifisam'

# Pickup the interface to use, defaulting to CFFI
try:
  confmod = importlib.import_module('.conf', 'pyisam.backend')
  use_conf = getattr(confmod, 'backend')
except ModuleNotFoundError:
  use_conf = None

# Split the configuration into two parts, the backend and the isam library, defaulting
# to libifisam.
use_conf, use_isamlib = use_conf.split('.')[:2] if use_conf.find('.') >= 0 else (use_conf, None)
if use_conf in ('', None):
  use_conf = _dflt_conf
if use_isamlib in ('', None):
  use_isamlib = _dflt_isamlib

# Validate the backend to those allowed
if use_conf not in _allowed_conf or use_isamlib not in _allowed_isamlib:
  raise ModuleNotFoundError()

# Load the specific module to retrieve those objects specified in the __all__ variable
mod_obj = importlib.import_module(f'.{use_conf}', 'pyisam.backend')

# Reference the objects made available to the application
ISAMobjectMixin = getattr(mod_obj, 'ISAMobjectMixin')
ISAMindexMixin = getattr(mod_obj, 'ISAMindexMixin')
RecordBuffer = getattr(mod_obj, 'RecordBuffer')
ISAMdictinfo = getattr(mod_obj, 'dictinfo')
ISAMkeydesc = getattr(mod_obj, 'keydesc')
