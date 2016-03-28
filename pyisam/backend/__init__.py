'''
This module provides backend configuration for the pyisam package
'''

__all__ = 'ISAMobject', 'ISAMindexMixin', 'RecordBuffer', 'dictinfo', 'keydesc'

# Set the following variable to True to enable the use of the CFFI interface, otherwise
# leave as False to use the ctypes interface.
use_cffi = True

if use_cffi:
  from .cffi import ISAMobjectMixin, ISAMindexMixin, RecordBuffer, dictinfo, keydesc
else:
  from .ctypes import ISAMobjectMixin, ISAMindexMixin, RecordBuffer, dictinfo, keydesc

"""
The following is a way to accept extra backends without having to modify the source manually:

import importlib
_interface = 'cffi' # or 'ctypes'
module_obj = importlib.import_module('.' + _interface, 'pyisam.backend')
ISAMobjectMixin = module_obj.ISAMobjectMixin
ISAMindexMixin = module_obj.ISAMindexMixin
RecordBuffer = module_obj.RecordBuffer
dictinfo = module_obj.dictinfo
keydesc = module_obj.keydesc
"""
