'''
This module provides the exceptions raised by the package, this avoids a cyclic dependancy from being
created between the vaious sub-modules.
'''

class IsamException(Exception):
  'General exception raised by ISAM'

class IsamIterError(IsamException):
  'Iterator based error'
  
class IsamNotOpen(IsamException):
  'Exception raised when ISAM table not open'

class IsamOpen(IsamException):
  'Exception when ISAM table already opened'

class IsamNotWritable(IsamNotOpen):
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
    return '{0.tabname}: {0.errno}' if self.errstr is None else '{0.tabname}: {0.errstr} ({0.errno})'.format(self)

class IsamNoRecord(IsamException):
  'Exception raised when no record was found'

class IsamNoIndex(IsamException):
  'Exception raised when an index is missing from a table instance'
  def __init__(self, tabname, idxname):
    self.tabname = tabname._name_ if hasattr(tabname, '_name_') else tabname
    self.idxname = idxname._name_ if hasattr(idxname, '_name_') else idxname

  def __str__(self):
    return "Index '{0.idxname}' is not available on table '{0.tabname}'".format(self)

class TableDefnError(IsamException):
  'General exception raised during table definition'
