'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying IBM C-ISAM or
the open source VBISAM library using an complied extension.
'''

""" NOT USED :
import importlib
from .. import use_variant

__all__ = ('cffi_obj',)

cffi_obj = importlib.import_module(f'.{use_variant}', 'pyisam.backend.cffi')
if use_variant == 'ifisam':
  from .ifisam import ISAMifisamMixin as ISAMobjectMixin
  from .ifisam import ffi, lib
elif use_variant == 'vbisam':
  from .vbisam import ISAMvbisamMixin as ISAMobjectMixin
  from .vbisam import ffi, lib
else:
  raise ModuleNotFoundError('Invalid variant of ISAM library configured')
END NOT USED """
