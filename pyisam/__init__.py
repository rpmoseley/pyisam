'''
This package provides a python interface to the IBM Informix CISAM library (or alternatively the
open source VBISAM replacement library). Additional modules add the ability to add table layout
definitions and the idea of rowsets to the underlying library of choice.
'''

from .backend.common import MaxKeyParts
from .constants import IndexFlags, LockMode, OpenMode, ReadMode, StartMode
from .error import IsamException, IsamNotOpen, IsamOpen, IsamReadOnly, IsamRecordMutable, IsamFunctionFailed, IsamNoRecord
from .isam import ISAMobject

__all__ = ('ISAMobject', 
           'IndexFlags', 'LockMode', 'OpenMode', 'ReadMode', 'StartMode',
           'IsamException', 'IsamNotOpen', 'IsamOpen', 'IsamReadonly',
           'IsamRecordMutable', 'IsamFunctionFailed', 'IsamNoRecord',
           'MaxKeyParts',
           '__version__',
          )
__version__ = '0.10'
