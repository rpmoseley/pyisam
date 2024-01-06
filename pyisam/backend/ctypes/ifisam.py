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
from .common import ISAMcommonMixin, ISAMindexMixin, dictinfo, keydesc, RecordBuffer
from ...error import IsamNotOpen, IsamNoRecord, IsamFunctionFailed
from ...utils import ISAM_str

__all__ = 'ISAMifisamMixin', 'ISAMindexMixin', 'dictinfo', 'keydesc', 'RecordBuffer'

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

  def strerror(self, errcode=None):
    if errcode is None:
      errcode = self.iserrno
    errnum = errcode - self._vld_errno[0]
    if self._vld_errno[0] <= errcode < self._vld_errno[1]:
      return ISAM_str(self.is_errlist[errnum])
    else:
      return os.strerror(errcode)
