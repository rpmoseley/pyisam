'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying IBM C-ISAM
library.
'''

from ._ifisam_cffi import ffi, lib
from .common import ISAMcommonMixin, ISAMindexMixin, dictinfo, keydesc, RecordBuffer
from ...error import IsamNotOpen
from ...utils import ISAM_bytes, ISAM_str

__all__ = 'ISAMifisamMixin', 'ISAMindexMixin', 'dictinfo', 'keydesc', 'ffi', 'lib', 'RecordBuffer'

class ISAMifisamMixin(ISAMcommonMixin):
  ''' This provides the common CFFI interface which provides the IFISAM specific
      library with all functions and variables availabe.
  '''
  __slots__ = ()
  _ffi_ = ffi
  _lib_ = lib

  def iserrno(self):
    return self._lib_.iserrno

  def iserrio(self):
    return self._lib_.iserrio

  def isrecnum(self):
    return self._lib_.isrecnum

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
    'Return the error description for the given ERRNO or current one if None'
    if errno is None:
      errno = self.iserrno()
    if self._vld_errno[0] <= errno < self._vld_errno[1]:
      return ISAM_str(self._ffi_.string(self._lib_.is_errlist[errno - self._vld_errno[0]]))
    else:
      return os.strerror(errno)
      
  def isdictinfo(self):
    'New method introduced in CISAM 7.26, returning dictinfo'
    if self._isfd_ is None: raise IsamNotOpen
    dinfo = ffi.new('struct dictinfo *')
    self._chkerror(self._lib_.isdictinfo(self._isfd_, dinfo), 'isdictinfo')
    return dictinfo(dinfo)

  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    return bool(self._lib_.isglsversion(ISAM_bytes(tabname)), 'isglsversion')

  def isindexinfo(self, keynum):
    'Original method combining both isdictinfo and iskeyinfo'
    if self._isfd_ is None: raise IsamNotOpen
    if keynum is None:
      dinfo = ffi.new('struct dictinfo *');
      self._chkerror(self._lib_.isdictinfo(self._isfd_, dinfo), 'isindexinfo')
      return dictinfo(dinfo)
    elif keynum < 0:
      raise ValueError('Index must be a positive number or None for dictinfo')
    else:
      kinfo = ffi.new('struct keydesc *')
      self._chkerror(self._lib_.iskeyinfo(self._isfd_, kinfo, keynum+1), 'isindexinfo')
      return keydesc(kinfo)
    
  def iskeyinfo(self, keynum):
    'New method introduced in CISAM 7.26, returning key description'
    if self._isfd_ is None: raise IsamNotOpen
    if keynum < 0:
      raise ValueError('Index must be a positive number starting from 0')
    else:
      kinfo = ffi.new('struct keydesc *')
      self._chkerror(self._lib_.iskeyinfo(self._isfd_, kinfo, keynum+1), 'iskeyinfo')
    return self.isindexinfo(keynum)

  def islangchk(self):
    'Switch on language checks'
    self._chkerror(self._lib_.islangchk(), 'islangchk')

  def islanginfo(self, tabname):
    'Return the collation sequence if any in use'
    return ISAM_str(self._lib_.islanginfo(ISAM_bytes(tabname)), 'islanginfo')

  def isnlsversion(self, tabname):
    # TODO: Add documentation for function
    self._chkerror(self._lib_.isnlsversion(ISAM_bytes(tabname)), 'isnlsversion')

  def isnolangchk(self):
    'Switch off language checks'
    self._chkerror(self._lib_.isnolangchk(), 'isnolangchk')
