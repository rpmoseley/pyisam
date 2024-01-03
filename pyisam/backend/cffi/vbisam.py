'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying VBISAM library
and is designed to be a direct replacement for the ctypes based module.
'''

from ._vbisam_cffi import ffi, lib
from .common import ISAMcommonMixin, dictinfo, keydesc
from ...error import IsamNotOpen
from ...utils import ISAM_bytes, ISAM_str

__all__ = 'ISAMcommonMixin', 'ffi', 'lib'

class ISAMvbisamMixin(ISAMcommonMixin):
  ''' This provides the interface to underlying ISAM libraries adding the context
      of the current file to avoid having to remember it separately.
  '''
  __slots__ = ('_numerr', )
  _ffi_ = ffi
  _lib_ = lib

  def iserrno(self):
    return self._lib_.iserrno()

  def iserrio(self):
    return self._lib_.iserrio()

  def isrecnum(self):
    return self._lib_.isrecnum()

  def isreclen(self):
    return self._lib_.isreclen()

  @property
  def isversnumber(self):
    return "VBISAM 2.1.1"

  @property
  def iscopyright(self):
    return "(c) 2003-2023 Trevor van Breman"

  @property
  def isserial(self):
    return ""

  @property
  def issingleuser(self):
    return False

  @property
  def is_nerr(self):
    return 172

  @property
  def is_errlist(self):
    return self._lib_.is_errlist()
  
  def isdictinfo(self):
    'Return the dictinfo for the table'
    if self._isfd_ is None: raise IsamNotOpen
    dinfo = ffi.new('struct dictinfo *')
    self._chkerror(self._lib_.isdictinfo(self._isfd_, dinfo), 'isdictinfo')
    return dictinfo(dinfo)

  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    # TODO: return bool(lib.isglsversion(ISAM_bytes(tabname)), 'isglsversion')
    return False

  def isindexinfo(self, keynum):
    'Backwards compatible method for version of ISAM < 7.26'
    if self._isfd_ is None: raise IsamNotOpen
    if keynum is None:
      dinfo = ffi.new('struct dictinfo *')
      self._chkerror(self._lib_.isdictinfo(self._isfd_, dinfo, 0), 'isindexinfo')
      return dictinfo(dinfo)
    elif keynum < 0:
      raise ValueError('Index must be a positive number or None for dictinfo')
    else:
      kinfo = ffi.new('struct keydesc *')
      self._chkerror(self._lib_.iskeyinfo(self,_isfd_, kinfo, keynum+1), 'isindexinfo')
      return self.iskeyinfo(keynum-1)

  def iskeyinfo(self, keynum):
    'Return the keydesc for the specified key'
    if self._isfd_ is None: raise IsamNotOpen
    keyinfo = ffi.new('struct keydesc *')
    self._chkerror(self._lib_.iskeyinfo(self._isfd_, keyinfo, keynum+1), 'iskeyinfo')
    return keydesc(keyinfo)

  def islangchk(self):
    'Switch on language checks'
    # TODO: self._chkerror(self._lib_.islangchk(), 'islangchk')
    return False

  def islanginfo(self, tabname):
    'Return the collation sequence if any in use'
    # TODO: return ISAM_str(self._lib_.islanginfo(ISAM_bytes(tabname)), 'islanginfo')
    return 

  def isnlsversion(self, tabname):
    # TODO: Add documentation for function
    self._chkerror(self._lib_.isnlsversion(ISAM_bytes(tabname)), 'isnlsversion')

  def isnolangchk(self):
    'Switch off language checks'
    self._chkerror(self._lib_.isnolangchk(), 'isnolangchk')

  def is_errlist(self):
    return self._lib_.is_errlist()