'''
This module provides the exceptions raised by the package, this avoids a cyclic dependancy from being
created between the vaious sub-modules.
'''

class IsamException(Exception):
  'General exception raised by ISAM'

class IsamError(IsamException):
  'General error from package'

class IsamIterError(IsamException):
  'Iterator based error'
  
class IsamNotOpen(IsamException):
  'Exception raised when ISAM table not open'

class IsamOpen(IsamException):
  'Exception when ISAM table already opened'

class IsamReadOnly(IsamNotOpen):
  'Exception raised when ISAM table not opened with writable mode'

class IsamRecordMutable(IsamException):
  'Exception raised when given a not mutable buffer'

class IsamFunctionFailed(IsamException):
  'Exception raised when an ISAM function is not found in the libaray'
  def __init__(self, tabname, errno, errstr=None):
    self.tabname = tabname
    self.errno = errno
    self.errstr = errstr

  def __str__(self):
    if self.errstr is None:
      return f'{self.tabname}: {self.errno}'
    return f'{self.tabname}: {self.errstr} ({self.errno})'

class IsamVariableLength(IsamException):
  'Exception raised when opening a variable length file if not enabled'

class IsamNoRecord(IsamException):
  'Exception raised when no record was found'

class IsamEndFile(IsamException):
  'End of file reached'

class IsamNoPrimaryIndex(IsamException):
  'Exception raised when no primary index has been defined on table'
  def __init__(self, tabname):
    self.tabname = tabname._name_ if hasattr(tabname, '_name_') else tabname

  def __str__(self):
    return f"Table '{self.tabname}' does not have a primary index defined"

class IsamNoIndex(IsamException):
  'Exception raised when an index is missing from a table instance'
  def __init__(self, tabname, idxname):
    self.tabname = tabname._name_ if hasattr(tabname, '_name_') else tabname
    self.idxname = idxname._name_ if hasattr(idxname, '_name_') else idxname

  def __str__(self):
    return f"Index '{self.idxname}' is not available on table '{self.tabname}'"

class TableDefnError(IsamException):
  'General exception raised during table definition'
