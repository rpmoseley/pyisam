'''
This package provides a python interface to the IBM Informix CISAM library,
the open source VBISAM replacement library, or the commerical D-ISAM library.
Additional modules add the ability to add table layout definitions and the idea
of rowsets to the underlying library of choice.
'''

from .backend.common import MaxKeyParts
from .constants import IndexFlags, LockMode, OpenMode, ReadMode
from .error import IsamException, IsamNotOpen, IsamOpen, IsamReadOnly
from .error import IsamRecordMutable, IsamFunctionFailed, IsamNoRecord
from .isam import ISAMobject
from .version import __version__

__all__ = ('ISAMobject', 
           'IndexFlags', 'LockMode', 'OpenMode', 'ReadMode', 'StartMode',
           'IsamException', 'IsamNotOpen', 'IsamOpen', 'IsamReadOnly',
           'IsamRecordMutable', 'IsamFunctionFailed', 'IsamNoRecord',
           'MaxKeyParts',
           '__version__',
          )
