'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying IBM C-ISAM
library.
'''

import os
from ._ifisam_cffi import ffi, lib
from .common import ISAMcommonMixin, ISAMindexMixin
from .common import dictinfo, keydesc
from ...error import IsamNotOpen
from ...utils import ISAM_bytes, ISAM_str

__all__ = 'ISAMifisamMixin', 'ISAMindexMixin'

class ISAMifisamMixin(ISAMcommonMixin):
  ''' This provides the common CFFI interface which provides the IFISAM specific
      library with all functions and variables availabe.
  '''
  __slots__ = ()
  _lib = lib

  """ NOT USED:
  @property
  def iserrno(self):
    return self._lib.iserrno

  @property
  def iserrio(self):
    return self._lib.iserrio

  @property
  def isrecnum(self):
    return self._lib.isrecnum

  @property
  def isreclen(self):
    return self._lib.isreclen
  END NOT USED"""

  @property
  def isversnumber(self):
    return ISAM_str(ffi.string(self._lib.isversnumber))

  @property
  def iscopyright(self):
    return ISAM_str(ffi.string(self._lib.iscopyright))

  @property
  def isserial(self):
    return ISAM_str(ffi.string(self._lib.isserial))

  @property
  def issingleuser(self):
    return bool(self._lib.issingleuser)

  @property
  def is_nerr(self):
    return self._lib.is_nerr

  def strerror(self, errno=None):
    'Return the error description for the given ERRNO or current one if None'
    if errno is None:
      errno = self.iserrno()
    if self._vld_errno[0] <= errno < self._vld_errno[1]:
      return ISAM_str(FFI.string(self._lib.is_errlist[errno - self._vld_errno[0]]))
    else:
      return os.strerror(errno)
      
  def isdictinfo(self):
    'New method introduced in CISAM 7.26, returning dictinfo'
    if self._fd is None: raise IsamNotOpen
    dinfo = ffi.new('struct dictinfo *')
    self._chkerror(self._lib.isdictinfo(self._fd, dinfo), 'isdictinfo')
    return dictinfo(dinfo)

  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    return bool(self._lib.isglsversion(ISAM_bytes(tabname)), 'isglsversion')

  def isindexinfo(self, keynum):
    'Original method combining both isdictinfo and iskeyinfo'
    if keynum is None:
      return self.isdictinfo()
    else:
      return self.iskeyinfo(keynum)
    
  def iskeyinfo(self, keynum):
    'New method introduced in CISAM 7.26, returning key description'
    if self._fd is None: raise IsamNotOpen
    if keynum is None or keynum < 0:
      raise ValueError('Index must be a positive number')
    kinfo = ffi.new('struct keydesc *')
    self._chkerror(self._lib.iskeyinfo(self._fd, kinfo, keynum+1), 'iskeyinfo')
    return keydesc(kinfo)

  def islangchk(self):
    'Switch on language checks'
    self._chkerror(self._lib.islangchk(), 'islangchk')

  def islanginfo(self, tabname):
    'Return the collation sequence if any in use'
    return ISAM_str(self._lib.islanginfo(ISAM_bytes(tabname)), 'islanginfo')

  def isnlsversion(self, tabname):
    # TODO: Add documentation for function
    self._chkerror(self._lib.isnlsversion(ISAM_bytes(tabname)), 'isnlsversion')

  def isnolangchk(self):
    'Switch off language checks'
    self._chkerror(self._lib.isnolangchk(), 'isnolangchk')
