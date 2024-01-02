'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying IBM C-ISAM or
the open source VBISAM library using an complied extension.

The particular library is chosen depending of the setting of the use_isamlib
variable that is set when the backend system is initialised, appropriate
replacement methods are provided for the VBISAM backed to match the C-ISAM one.
'''

from .common import dictinfo, keydesc, ISAMindexMixin, RecordBuffer
from .. import use_isamlib

__all__ = 'ISAMobjectMixin', 'ISAMindexMixin', 'dictinfo', 'keydesc', 'RecordBuffer', 'ffi', 'lib'

if use_isamlib == 'ifisam':
  from .ifisam import ISAMifisamMixin as ISAMobjectMixin
  from .ifisam import ffi, lib
elif use_isamlib == 'vbisam':
  from .vbisam import ISAMvbisamMixin as ISAMobjectMixin
  from .vbisam import ffi, lib
else:
  raise ModuleNotFoundError('Invalid variant of ISAM library configured')
