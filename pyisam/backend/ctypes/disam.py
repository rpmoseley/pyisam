'''
This is the CTYPES specific implementation of the package

This module provides a ctypes based interface to the IBM C-ISAM library without
requiring an explicit extension to be compiled. The only requirement is that
the existing libifisam/libifisamx libraries are combined such that the SONAME
can be verified.
'''

import os
from ctypes import c_char_p, c_int, c_int32
from ctypes import _SimpleCData, CDLL, _dlopen, POINTER
from .common import ISAMcommonMixin, ISAMindexMixin, ISAMfunc, dictinfo, keydesc, RecordBuffer
from ...error import IsamNotOpen, IsamNoRecord, IsamFunctionFailed
from ...utils import ISAM_str

__all__ = 'ISAMdisamMixin'

_lib_so = os.path.join(os.path.dirname(__file__), 'libpydisam.so')

class ISAMdisamMixin(ISAMcommonMixin):
  '''This provides the interface to the underlying ISAM libraries.
     The underlying ISAM routines are loaded on demand with a
     prefix of an underscore, so isopen becomes _isopen.
  '''
  __slots__ = ()
  _vld_errno = (500, 560)
  
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
  _lib_ = CDLL('libpydisam', handle=_dlopen(_lib_so))

  def __getattr__(self, name):
    '''Lookup the ISAM function and return the entry point into the library
       or define and return the numeric equivalent'''
    if not isinstance(name, str):
      raise AttributeError(name)
    return super().__getattr__(name)

  # Override the isdictinfo and iskeyinfo functions
  @ISAMfunc(c_int, POINTER(dictinfo))
  def isisaminfo(self):
    '''Return the dictionary information for table'''
    if self._isfd_ is None: raise IsamNotOpen
    ptr = dictinfo()
    self._isisaminfo(self._isfd_, ptr)
    return ptr

  @ISAMfunc(c_int, POINTER(keydesc), c_int)
  def isindexinfo(self, keynum):
    if self._isfd_ is None: raise IsamNotOpen
    if keynum < 0: raise ValueError('Index must be a positive number')
    ptr = keydesc()
    self._isindexinfo(self._isfd_, ptr, keynum+1)
    return ptr
    
  def isdictinfo(self):
    'Provide the same name as expected'
    return self.isisaminfo()
  
  def iskeyinfo(self, keynum):
    'Provide the same name as expected'
    return self.isindexinfo(keynum)

  def strerror(self, errcode=None):
    if errcode is None:
      errcode = self.iserrno
    errnum = errcode - self._vld_errno[0]
    if self._vld_errno[0] <= errcode < self._vld_errno[1]:
      return ISAM_str(self.is_errlist[errnum])
    else:
      return os.strerror(errcode)
