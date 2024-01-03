'''
This is the CTYPES specific implementation of the package

This module provides a ctypes based interface to the IBM C-ISAM library without
requiring an explicit extension to be compiled. The only requirement is that
the existing libifisam/libifisamx libraries are combined such that the SONAME
can be verified.
'''

import os
from ctypes import c_char_p, c_int, c_int32
from ctypes import _SimpleCData, CDLL, _dlopen
from .common import ISAMcommonMixin
from ...error import IsamNotOpen, IsamNoRecord, IsamFunctionFailed
from ...utils import ISAM_str

__all__ = 'ISAMifisamMixin'

class ISAMifisamMixin(ISAMcommonMixin):
  '''This provides the interface to the underlying ISAM libraries.
     The underlying ISAM routines are loaded on demand with a
     prefix of an underscore, so isopen becomes _isopen.
  '''
  __slots__ = ()
  
  # The _const_ dictionary initially consists of the ctypes type
  # which will be mapped to the correct variable when accessed.
  _const_ = {
    'iserrno'      : c_int,    'iserrio'      : c_int,
    'isrecnum'     : c_int32,  'isreclen'     : c_int,
    'isversnumber' : c_char_p, 'iscopyright'  : c_char_p,
    'isserial'     : c_char_p, 'issingleuser' : c_int,
    'is_nerr'      : c_int,    'is_errlist'   : None
  }
  
  # Load the ISAM library once and share it in other instances
  # To make use of vbisam instead link the libpyifisam.so accordingly
  _lib_ = CDLL('libpyifisam', handle=_dlopen(os.path.normpath(os.path.join(os.path.dirname(__file__), 'libpyifisam.so'))))

  def __getattr__(self,name):
    '''Lookup the ISAM function and return the entry point into the library
       or define and return the numeric equivalent'''
    if not isinstance(name, str):
      raise AttributeError(name)
    elif name.startswith('_is'):
      return getattr(self._lib_, name[1:])
    elif name.startswith('_'):
      raise AttributeError(name)
    elif name == 'is_errlist':
      val = self._const_['is_errlist']
      if val is None:
        errlist = c_char_p * (self.is_nerr - 100)
        val = self._const_['is_errlist'] = errlist.in_dll(self._lib_, 'is_errlist')
    else:
      val = self._const_.get(name)
      if val is None:
        raise AttributeError(name)
      elif not isinstance(val, _SimpleCData) and hasattr(val, 'in_dll'):
        val = self._const_[name] = val.in_dll(self._lib_, name)
      if hasattr(val, 'value'):
        val = val.value
      if isinstance(val, bytes):
        val = ISAM_str(val)
    return val

  """ NOT USED:
  def _chkerror(self, result=None, func=None, args=None):
    '''Perform checks on the running of the underlying ISAM function by
       checking the iserrno provided by the ISAM library, if ARGS is
       given return that on successfull completion of this method'''
    print('CHK: R=', result, 'F=', func.__name__, 'A=', args)
    if result is None or result < 0:
      if self.iserrno == 101:
        raise IsamNotOpen
      elif self.iserrno == 111:
        raise IsamNoRecord
    elif result is not None and (result < 0 or self.iserrno != 0):
      raise IsamFunctionFailed(func.__name__, self.iserrno, self.strerror(self.iserrno))
    return args
  END NOT USED """

  def strerror(self, errno=None):
    '''Return the error message related to the error number given'''
    if errno is None:
      errno = self.iserrno
    if 100 <= errno < self.is_nerr:
      return ISAM_str(self.is_errlist[errno - 100])
    else:
      return os.strerror(errno)
