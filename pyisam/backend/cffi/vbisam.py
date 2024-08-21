'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying VBISAM library
and is designed to be a direct replacement for the ctypes based module.
'''

import os
from ._vbisam_cffi import ffi, lib
from .common import ISAMcommonMixin, ISAMindexMixin
from .common import ISAMdictinfo, ISAMkeydesc
from ...error import IsamNotOpen
from ...utils import ISAM_bytes, ISAM_str

def create_record(recsz):
  return ffi.buffer(ffi.new('char[]', recsz+1))

class ISAMobjectMixin(ISAMcommonMixin):
  ''' This provides the interface to underlying ISAM libraries adding the context
      of the current file to avoid having to remember it separately.
  '''
  __slots__ = ('_numerr', )
  _lib = lib
  _lib.vb_get_rtd()
  _ffi = ffi

  """ NOT USED:
  @property
  def iserrno(self):
    return self._lib.iserrno()

  @property
  def iserrio(self):
    return self._lib.iserrio()

  @property
  def isrecnum(self):
    return self._lib.isrecnum()

  @property
  def isreclen(self):
    return self._lib.isreclen()
  END NOT USED """

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

  def strerror(self, errno=None):
    if errno is None:
      errno = self.iserrno()
    if self._vld_errno[0] <= errno < self._vld_errno[1]:
      return ISAM_str(ffi.string(self._lib.is_strerror(errno)))
    else:
      return os.strerror(errno)

  def isdictinfo(self):
    'Return the dictinfo for the table'
    if self._fd is None:
      raise IsamNotOpen
    dinfo = ffi.new('struct dictinfo *')
    self._chkerror(self._lib.isdictinfo(self._fd, dinfo), 'isdictinfo')
    return ISAMdictinfo(dinfo)

  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    # TODO: return bool(lib.isglsversion(ISAM_bytes(tabname)), 'isglsversion')
    return False

  def isindexinfo(self, keynum):
    'Backwards compatible method for version of ISAM < 7.26'
    if self._fd is None:
      raise IsamNotOpen
    if keynum is None:
      dinfo = ffi.new('struct dictinfo *')
      self._chkerror(self._lib.isdictinfo(self._fd, dinfo, 0), 'isindexinfo')
      return ISAMdictinfo(dinfo)
    elif keynum < 0:
      raise ValueError('Index must be a positive number or None for dictinfo')
    else:
      kinfo = ffi.new('struct keydesc *')
      self._chkerror(self._lib.iskeyinfo(self._fd, kinfo, keynum+1), 'isindexinfo')
      return ISAMkeydesc(kinfo)

  def iskeyinfo(self, keynum):
    'Return the keydesc for the specified key'
    if self._fd is None:
      raise IsamNotOpen
    kinfo = ffi.new('struct keydesc *')
    self._chkerror(self._lib.iskeyinfo(self._fd, kinfo, keynum+1), 'iskeyinfo')
    return ISAMkeydesc(kinfo)

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
    self._chkerror(self._lib.isnlsversion(ISAM_bytes(tabname)), 'isnlsversion')

  def isnolangchk(self):
    'Switch off language checks'
    self._chkerror(self._lib.isnolangchk(), 'isnolangchk')
