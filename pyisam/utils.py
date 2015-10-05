'''
This module provides the utility functions and exceptions for the pyisam package.
'''
__all__ = ('ISAM_bytes','ISAM_str','IsamException','IsamNotOpen','IsamNotWritable',
           'IsamRecMutable','IsamFuncFailed','IsamNoRecord')

# Convert the given value to a bytes value
def ISAM_bytes(value,default=None):
  if isinstance(value,str):
    value = bytes(value,'utf-8')
  elif value is None:
    value = b'' if default is None else ISAM_bytes(default)
  else:
    raise ValueError('Value is not convertable to a bytes value')
  return value

# Convert the given value to a str value
def ISAM_str(value,default=None):
  if isinstance(value,bytes):
    value = str(value,'utf-8')
  elif value is None:
    value = '' if default is None else ISAM_str(default)
  else:
    raise ValueError('Value is not convertable to a str value')
  return value

# Define the shared exceptions raised by the package
class IsamException(Exception):
  'General exception raised by ISAM'
class IsamNotOpen(IsamException):
  'Exception raised when ISAM table not open'
class IsamNotWritable(IsamNotOpen):
  'Exception raised when ISAM table not opened with writable mode'
class IsamRecMutable(IsamException):
  'Exception raised when given a not mutable buffer'
class IsamFuncFailed(IsamException):
  'Exception raised when an ISAM function is not found in the libaray'
  def __init__(self, tabname, errno, errstr=None):
    self.tabname = tabname
    self.errno = errno
    self.errstr = errstr
  def __str__(self):
    if self.errstr is None:
      return '{}: {}'.format(self.tabname, self.errno)
    return '{}: {} ({})'.format(self.tabname, self.errstr, self.errno)
class IsamNoRecord(IsamException):
  'Exception raised when no record was found'
