'''
This package provides a python interface to the IBM Informix CISAM library (or alternatively the
open source VBISAM replacement library). Additional modules add the ability to add table layout
definitions and the idea of rowsets to the underlying library of choice.
'''

from .isam import ISAMobject
from .constants import IndexFlags, LockMode, OpenMode, ReadMode, StartMode
from .error import IsamException, IsamNotOpen, IsamOpened, IsamNotWritable, IsamRecordMutable, IsamFunctionFailed, IsamNoRecord

MaxKeyParts = 8  # Define the maximum number of parts a key may contain

__all__ = ('ISAMobject', 
           'IndexFlags', 'LockMode', 'OpenMode', 'ReadMode', 'StartMode',
           'IsamException', 'IsamNotOpen', 'IsamOpened', 'IsamNotWritable',
           'IsamRecordMutable', 'IsamFunctionFailed', 'IsamNoRecord',
           'MaxKeyParts',
          )
