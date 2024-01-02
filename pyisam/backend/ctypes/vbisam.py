'''
This is the CTYPES specific implementation of the package

This module provides a ctypes based interface to the open source VBISAM
library without requiring an explicit extension to be compiled.
'''

import os
from ctypes import c_char_p, c_int, c_int32
from ctypes import _SimpleCData, CDLL, _dlopen
from .common import ISAMcommonMixin, ISAMfunc
from ...error import IsamNotOpen, IsamNoRecord, IsamFunctionFailed
from ...utils import ISAM_str

__all__ = 'ISAMvbisamMixin'

class ISAMvbisamMixin(ISAMcommonMixin):
  '''This provides the interface to the underlying ISAM libraries.
     The underlying ISAM routines are loaded on demand with a
     prefix of an underscore, so isopen becomes _isopen.
  '''
  __slots__ = ()
  
  # The _const_ dictionary initially consists of the ctypes type
  # which will be mapped to the correct variable when accessed.
  _const_ = {
    'is_nerr'      : c_int,    'is_errlist'   : None
  }
  
  # Load the ISAM library once and share it in other instances
  # To make use of vbisam instead link the libpyisam.so accordingly
  _lib_ = CDLL('libvbisam', handle=_dlopen(os.path.normpath(os.path.join(os.path.dirname(__file__), 'libvbisam.so')))) # FIXME: Make 32/64-bit correct

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
    return val.value if hasattr(val, 'value') else val

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
        # NOTE: What if args is None?
        raise IsamFunctionFailed(func.__name__, self.iserrno, self.strerror(self.iserrno))
    return args
  END NOT USED """

  def strerror(self, errno=None):
    '''Return the error message related to the error number given'''
    if errno is None:
      errno = self.iserrno()
    if 100 <= errno < self.is_nerr:
      return ISAM_str(self.is_errlist[errno - 100])
    else:
      return os.strerror(errno)
  
  @property
  @ISAMfunc(restype=c_int)
  def iserrno(self):
    return self._lib_.iserrno()
  
  @property
  @ISAMfunc(restype=c_int)
  def iserrio(self):
    return self._lib_.iserrio()

  @property  
  @ISAMfunc(restype=c_int32)
  def isrecnum(self):
    return self._lib_.isrecnum()
  
  @property
  @ISAMfunc(restype=c_int)
  def isreclen(self):
    return self._lib_.isreclen()

  @property
  def isversnumber(self):
    return 'VBISAM 2.1.1'

  @property
  def iscopyright(self):
    return "(c) 2003-2023 Trevor van Bremen"
