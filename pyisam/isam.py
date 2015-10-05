'''
This module will load the appropriate variant according to whether the builtin ctypes or cffi
version should be used.

For now the module provides the ctypes version as is
'''

__all__ = ('ISAMobject', 'dictinfo', 'keydesc', 'keypart')

from .config import use_cffi
if use_cffi:
  print('Using CFFI backend')
  from .cffi import ISAMobject, dictinfo
else:
  print('Using CTYPES backend')
  from .ctypes import ISAMobject, dictinfo
