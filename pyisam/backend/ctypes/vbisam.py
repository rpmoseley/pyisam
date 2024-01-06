'''
This is the CTYPES specific implementation of the package

This module provides a ctypes based interface to the open source VBISAM
library without requiring an explicit extension to be compiled.
'''

import os
from ctypes import c_char_p, c_int, c_int32
from ctypes import _SimpleCData, CDLL, _dlopen
from .common import ISAMcommonMixin, ISAMfunc, ISAMindexMixin, dictinfo, keydesc, RecordBuffer
from ...error import IsamNotOpen, IsamNoRecord, IsamFunctionFailed
from ...utils import ISAM_str

__all__ = 'ISAMvbisamMixin', 'ISAMindexMixin', 'dictinfo', 'keydesc', 'RecordBuffer'

class ISAMvbisamMixin(ISAMcommonMixin):
  '''This provides the interface to the underlying ISAM libraries.
     The underlying ISAM routines are loaded on demand with a
     prefix of an underscore, so isopen becomes _isopen.
  '''
  __slots__ = ()
  
  # Load the ISAM library once and share it in other instances
  # To make use of vbisam instead link the libpyisam.so accordingly
  _lib_ = CDLL('libpyvbisam', handle=_dlopen(os.path.normpath(os.path.join(os.path.dirname(__file__), 'libpyvbisam.so'))))

  def __getattr__(self,name):
    '''Lookup the ISAM function and return the entry point into the library
       or define and return the numeric equivalent'''
    if not isinstance(name, str):
      raise AttributeError(name)
    elif name.startswith('_is'):
      return getattr(self._lib_, name[1:])
    elif name == 'is_errlist':
      return getattr(self._lib_, 'is_errlist')
    raise AttributeError(name)

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

  @ISAMfunc(c_int, restype=c_char_p)
  def is_strerror(self, errcode):
    return self._lib_.is_strerror(errcode)

  def strerror(self, errcode=None):
    if errcode is None:
      errcode = self.iserrno()
    errnum = errcode - self._vld_errno[0]
    if self._vld_errno[0] <= errcode < self._vld_errno[1]:
      return ISAM_str(self.is_strerror(errcode))
    else:
      return os.strerror(errcode)

  @property
  @ISAMfunc(restype=c_char_p)
  def is_errlist(self):
    return self._lib_is_errlist()

  @property
  def isversnumber(self):
    return 'VBISAM 2.1.1'

  @property
  def iscopyright(self):
    return "(c) 2003-2023 Trevor van Bremen"
