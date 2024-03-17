'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying DISAM
library.
'''

import os
from ._disam_cffi import ffi, lib
<<<<<<< HEAD
from .common import ISAMcommonMixin, ISAMindexMixin
from .common import ISAMdictinfo, ISAMkeydesc
=======
from .common import ISAMcommonMixin, ISAMindexMixin, dictinfo, keydesc
>>>>>>> 0e0ce1de0e43ea5ea064f83e49dfa01459030c0a
from ...error import IsamNotOpen
from ...utils import ISAM_str

<<<<<<< HEAD
__all__ = 'ISAMobjectMixin', 'ISAMindexMixin'
=======
__all__ = 'ISAMdisamMixin', 'ISAMindexMixin'
>>>>>>> 0e0ce1de0e43ea5ea064f83e49dfa01459030c0a

def create_record(recsz):
  return ffi.buffer(ffi.new('char[]', recsz+1))

class ISAMobjectMixin(ISAMcommonMixin):
  ''' This provides the common CFFI interface which provides the IFISAM specific
      library with all functions and variables availabe.
  '''
  __slots__ = ()
<<<<<<< HEAD
  _lib = lib
  _ffi = ffi
=======
  _lib_ = lib
>>>>>>> 0e0ce1de0e43ea5ea064f83e49dfa01459030c0a

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

  @property
  def isversnumber(self):
    return ISAM_str(self._ffi.string(self._lib.isversnumber))

  @property
  def iscopyright(self):
    return ISAM_str(self._ffi.string(self._lib.iscopyright))

  @property
  def isserial(self):
    return ISAM_str(self._ffi.string(self._lib.isserial))

  @property
  def issingleuser(self):
    return bool(self._lib.issingleuser)

  @property
  def is_nerr(self):
    return self._lib.is_nerr

  def strerror(self, errno=None):
    if errno is None:
      errno = self.iserrno()
    if self._vld_errno[0] <= errno < self._vld_errno[1]:
<<<<<<< HEAD
      return ISAM_str(self._ffi.string(self._lib.is_errlist[errno - 100]))
=======
      return ISAM_str(ffi.string(self._lib_.is_errlist[errno - 100]))
>>>>>>> 0e0ce1de0e43ea5ea064f83e49dfa01459030c0a
    else:
      return os.strerror(errno)

  def is_errlist(self):
    return self._lib.is_errlist

<<<<<<< HEAD
  def _dictinfo(self):
    dinfo = self._ffi.new('struct dictinfo *')
    self._chkerror(self._lib.isisaminfo(self._fd, dinfo), 'isdictinfo')
    return ISAMdictinfo(dinfo)

  def _keyinfo(self, keynum):
    kinfo = self._ffi.new('struct keydesc *')
    self._chkerror(self._lib.isindexinfo(self._fd, kinfo, keynum+1), 'isindexinfo')
    return ISAMkeydesc(kinfo)

  def isdictinfo(self):
    'New method introduced in CISAM 7.26, returning dictinfo'
    if self._fd is None:
      raise IsamNotOpen
    return self._dictinfo()
=======
  def _dictinfo_(self):
    dinfo = ffi.new('struct dictinfo *')
    self._chkerror(self._lib_.isisaminfo(self._fd, dinfo), 'isdictinfo')
    return dictinfo(dinfo)

  def _keyinfo_(self, keynum):
    kinfo = ffi.new('struct keydesc *')
    self._chkerror(self._lib_.isindexinfo(self._fd, kinfo, keynum+1), 'isindexinfo')
    return keydesc(kinfo)

  def isdictinfo(self):
    'New method introduced in CISAM 7.26, returning dictinfo'
    if self._fd is None: raise IsamNotOpen
    return self._dictinfo_()
>>>>>>> 0e0ce1de0e43ea5ea064f83e49dfa01459030c0a

  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    return False

  def isindexinfo(self, keynum):
    'Original method combining both isdictinfo and iskeyinfo'
<<<<<<< HEAD
    if self._fd is None:
      raise IsamNotOpen
=======
    if self._fd is None: raise IsamNotOpen
>>>>>>> 0e0ce1de0e43ea5ea064f83e49dfa01459030c0a
    if keynum is None:
      return self._dictinfo()   
    if keynum < 0:
      raise ValueError('Index must be a positive number or None for dictinfo')
<<<<<<< HEAD
    return self._keyinfo(keynum)
    
  def iskeyinfo(self, keynum):
    'New method introduced in CISAM 7.26, returning key description'
    if self._fd is None:
      raise IsamNotOpen
=======
    return self._keyinfo_(keynum)
    
  def iskeyinfo(self, keynum):
    'New method introduced in CISAM 7.26, returning key description'
    if self._fd is None: raise IsamNotOpen
>>>>>>> 0e0ce1de0e43ea5ea064f83e49dfa01459030c0a
    if keynum < 0:
      raise ValueError('Index must be a positive number starting from 0')
    return self._keyinfo(keynum)

  def islangchk(self):
    'Switch on language checks'
    # TODO: self._chkerror(self._lib.islangchk(), 'islangchk')
    return False

  def islanginfo(self, tabname):
    'Return the collation sequence if any in use'
    # TODO: return ISAM_str(self._lib.islanginfo(ISAM_bytes(tabname)), 'islanginfo')
    return

  def isnlsversion(self, tabname):
    # TODO: Add documentation for function
    # TODO: self._chkerror(self._lib_.isnlsversion(ISAM_bytes(tabname)), 'isnlsversion')
    return b''

  def isnolangchk(self):
    'Switch off language checks'
    # TODO: self._chkerror(self._lib.isnolangchk(), 'isnolangchk')
    return False
