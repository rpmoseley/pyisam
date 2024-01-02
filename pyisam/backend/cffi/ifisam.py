'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying IBM C-ISAM
library.
'''

from ._ifisam_cffi import ffi, lib
from .common import ISAMcommonMixin, dictinfo, keydesc
from ...error import IsamNotOpen
from ...utils import ISAM_bytes, ISAM_str

__all__ = 'ISAMifisamMixin', 'ffi', 'lib'

class ISAMifisamMixin(ISAMcommonMixin):
  ''' This provides the common CFFI interface which provides the IFISAM specific
      library with all functions and variables availabe.
  '''
  __slots__ = ()
  _ffi_ = ffi
  _lib_ = lib

  """ NOT USED:
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
  END NOT USED """

  @property
  def isversnumber(self):
    return ffi.string(self._lib_.isversnumber).decode('utf-8')

  @property
  def iscopyright(self):
    return ffi.string(self._lib_.iscopyright).decode('utf-8')

  @property
  def isserial(self):
    return ffi.string(self._lib_.isserial).decode('utf-8')

  @property
  def issingleuser(self):
    return bool(self._lib_.issingleuser)

  @property
  def is_nerr(self):
    return self._lib_.is_nerr

  def is_errlist(self):
    return self._lib_.is_errlist

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
      self._chkerror(self._lib_.iskeyinfo(self._isfd_, kinfo, keynum+1))
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
