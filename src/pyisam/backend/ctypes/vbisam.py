'''
This is the CTYPES specific implementation of the package

This module provides a ctypes based interface to the open source VBISAM
library without requiring an explicit extension to be compiled.
'''

import pathlib
import sysconfig
from ctypes import CDLL, _dlopen, Structure, POINTER
from ctypes import c_int, c_longlong, c_char_p, c_int, c_int32
from .common import ISAMcommonMixin, ISAMfunc, ISAMindexMixin, ISAMdictinfo
from .common import ISAMkeydesc
from ...utils import ISAM_str

__all__ = ('ISAMobjectMixin', 'ISAMindexMixin', 'ISAMdictinfo', 'ISAMkeydesc',
           'RecordBuffer')

# The name of the library used to load the underlying ISAM
_soext = sysconfig.get_config_var('SHLIB_SUFFIX')
_lib_nm = pathlib.PurePath('libpyvbisam')
_lib_so = pathlib.Path(__file__).parent / _lib_nm.with_suffix(_soext)

# Declare the internal context structure which provides direct access to
# the various global variables used by the ISAM library
class vb_rtd(Structure):
  _fields_ = [('iserrno',  c_int),
              ('iserrio',  c_int),
              ('isreclen', c_int),
              ('isrecnum', c_longlong)]

class ISAMobjectMixin(ISAMcommonMixin):
  '''This provides the interface to the underlying ISAM libraries.
     The underlying ISAM routines are loaded on demand with a
     prefix of an underscore, so isopen becomes _isopen.
  '''
  __slots__ = ()
  
  # The _const list contains all the variables available within the underlying
  # library that are treated as property functions.
  _const = {
    'iserrno' : c_int, 'iserrio' : c_int
  }

  # Load the VBISAM library once and share it in other instances
  # To make use of vbisam instead link the libpyisam.so accordingly
  _lib = CDLL(_lib_nm, handle=_dlopen(_lib_so))
  _lib.vb_get_rtd.restype = POINTER(vb_rtd)      # Define the return type
  _rtd = _lib.vb_get_rtd()    # Get the internal context pointer for the library

  def __getattr__(self, name):
    '''Lookup the ISAM function and return the entry point into the library
       or define and return the numeric equivalent'''
    if not isinstance(name, str) or name.startswith('_'):
      return super().__getattr__(name)
    try:
      return getattr(self._rtd.contents, name)
    except AttributeError:
      return super().__getattr__(name)

  def __setattr__(self, name, value):
    '''Enable the setting of those updatable global variables within the
       library'''
    if not isinstance(name, str) or name.startswith('_'):
      super().__setattr__(name, value)
    elif name in self._const:
      raise AttributeError(name)
    elif hasattr(self._rtd.contents, name):
      setattr(self._rtd.contents, name, value)
    else:
      raise AttributeError(name)

  """ NOT USED :
  @property
  @ISAMfunc(restype=c_int)
  def iserrno(self):
    return self._lib_.iserrno()
  
  @property
  @ISAMfunc(restype=c_int)
  def iserrio(self):
    return self._lib_.iserrio()

  @ISAMfunc(restype=c_int32)
  def isrecnum(self):
    return self._lib.isrecnum()
  
  @ISAMfunc(c_int32, restype=c_int)
  def set_isrecnum(self, recnum):
    return self._lib.set_isrecnum(recnum)

  isrecnum = property(isrecnum, set_isrecnum)

  @property
  @ISAMfunc(restype=c_int)
  def isreclen(self):
    return self._lib.isreclen()

  @ISAMfunc(c_int, restype=c_char_p)
  def is_strerror(self, errcode):
    return self._lib.is_strerror(errcode)
  END NOT USED """

  def strerror(self, errcode=None):
    if errcode is None:
      errcode = self.iserrno()
    #UNUSED:errnum = errcode - self._vld_errno[0]
    if self._vld_errno[0] <= errcode < self._vld_errno[1]:
      return ISAM_str(self.is_strerror(errcode))
    else:
      return os.strerror(errcode)

  @property
  @ISAMfunc(restype=c_char_p)
  def is_errlist(self):
    return self._lib.is_errlist()

  @property
  def isversnumber(self):
    return 'VBISAM 2.1.1'

  @property
  def iscopyright(self):
    return "(c) 2003-2023 Trevor van Bremen"
