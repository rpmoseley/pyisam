'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying DISAM
library.
'''

from ._disam_cffi import ffi, lib
from .common import ISAMcommonMixin, ISAMindexMixin, RecordBuffer, dictinfo, keydesc
from ...error import IsamNotOpen
from ...utils import ISAM_bytes, ISAM_str

__all__ = 'ISAMdisamMixin', 'ISAMindexMixin', 'RecordBuffer', 'dictinfo', 'keydesc', 'ffi'

class ISAMdisamMixin(ISAMcommonMixin):
  ''' This provides the common CFFI interface which provides the IFISAM specific
      library with all functions and variables availabe.
  '''
  __slots__ = ()
  _ffi_ = ffi
  _lib_ = lib

  @property
  def iserrno(self):
    return self._lib_.iserrno

  @property
  def iserrio(self):
    return self._lib_.iserrio

  @property
  def isrecnum(self):
    return self._lib_.isrecnum

  @property
  def isreclen(self):
    return self._lib_.isreclen

  @property
  def isversnumber(self):
    return ISAM_str(ffi.string(self._lib_.isversnumber))

  @property
  def iscopyright(self):
    return ISAM_str(ffi.string(self._lib_.iscopyright))

  @property
  def isserial(self):
    return ISAM_str(ffi.string(self._lib_.isserial))

  @property
  def issingleuser(self):
    return bool(self._lib_.issingleuser)

  @property
  def is_nerr(self):
    return self._lib_.is_nerr

  def strerror(self, errno=None):
    if errno is None:
      errno = self.iserrno()
    if self._vld_errno[0] <= errno < self._vld_errno[1]:
      return ISAM_str(self._ffi_.string(self._lib_.is_errlist[errno - 100]))
    else:
      return os.strerror(errno)

  def is_errlist(self):
    return self._lib_.is_errlist

  def _dictinfo_(self):
    dinfo = ffi.new('struct dictinfo *')
    self._chkerror(self._lib_.isuserinfo(self._isfd_, dinfo), 'isdictinfo')
    return dictinfo(dinfo)

  def _keyinfo_(self, keynum):
    kinfo = ffi.new('struct keydesc *')
    self._chkerror(self._lib_.isindexinfo(self._isfd_, kinfo, keynum+1), 'isindexinfo')
    return keydesc(kinfo)

  def isdictinfo(self):
    'New method introduced in CISAM 7.26, returning dictinfo'
    if self._isfd_ is None: raise IsamNotOpen
    return self._dictinfo_()

  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    return bool(self._lib_.isglsversion(ISAM_bytes(tabname)), 'isglsversion')

  def isindexinfo(self, keynum):
    'Original method combining both isdictinfo and iskeyinfo'
    if self._isfd_ is None: raise IsamNotOpen
    if keynum is None:
      return self._dictinfo_()   
    if keynum < 0:
      raise ValueError('Index must be a positive number or None for dictinfo')
    return self._keyinfo_(keynum+1)
    
  def iskeyinfo(self, keynum):
    'New method introduced in CISAM 7.26, returning key description'
    if self._isfd_ is None: raise IsamNotOpen
    if keynum < 0:
      raise ValueError('Index must be a positive number starting from 0')
    return self._keyinfo_(keynum)

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
    # TODO: self._chkerror(self._lib_.isnlsversion(ISAM_bytes(tabname)), 'isnlsversion')
    return b''

  def isnolangchk(self):
    'Switch off language checks'
    # TODO: self._chkerror(self._lib_.isnolangchk(), 'isnolangchk')
    return False
