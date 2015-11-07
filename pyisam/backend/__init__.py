'''
This module provides backend configuration for the pyisam package
'''

__all__ = ('ISAMobject', 'dictinfo', 'ISAMindexMixin')

# Set the following variable to True to enable the use of the CFFI interface, otherwise
# leave as False to use the ctypes interface.
use_cffi = True

if use_cffi:
  from .cffi import ISAMobjectMixin, ISAMindexMixin, dictinfo
else:
  from .ctypes import ISAMobjectMixin, ISAMindexMixin, dictinfo
