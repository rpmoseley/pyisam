'''
This module provides backend configuration for the pyisam package
'''

__all__ = ('ISAMobject', 'dictinfo', 'ISAMindexMixin', 'RecordBuffer')

# Set the following variable to True to enable the use of the CFFI interface, otherwise
# leave as False to use the ctypes interface.
use_cffi = False

if use_cffi:
  from .cffi import ISAMobjectMixin, ISAMindexMixin, dictinfo, RecordBuffer
else:
  from .ctypes import ISAMobjectMixin, ISAMindexMixin, dictinfo, RecordBuffer
