'''
This module provides backend configuration for the pyisam package
'''

import importlib
from .common import MAXKPART, MAXKLENG, _checkpart

__all__ = 'ISAMobjectMixin', 'ISAMindexMixin', 'RecordBuffer', 'ISAMdictinfo', 'ISAMkeydesc', 'MAXKPART', 'MAXKLENG', '_checkpart'
_allowed_conf = ('cffi', 'ctypes', 'cython')

# Pickup the interface to use, defaulting to CFFI
try:
  confmod = importlib.import_module('.conf', 'pyisam.backend')
  use_conf = getattr(confmod, 'backend')
except ModuleNotFoundError:
  use_conf = 'cffi'

# Validate the backend to those allowed
if use_conf not in _allowed_conf:
  raise ModuleNotFoundError()

# Load the specific module to retrieve those objects specified in the __all__ variable
mod_obj = importlib.import_module(f'.{use_conf}', 'pyisam.backend')

# Reference the objects made available to the application
ISAMobjectMixin = getattr(mod_obj, 'ISAMobjectMixin')
ISAMindexMixin = getattr(mod_obj, 'ISAMindexMixin')
RecordBuffer = getattr(mod_obj, 'RecordBuffer')
ISAMdictinfo = getattr(mod_obj, 'dictinfo')
ISAMkeydesc = getattr(mod_obj, 'keydesc')
