'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying IBM C-ISAM, DISAM
or open source VBISAM library and is designed to be a direct replacement for
the ctypes based module in that it aims to provide the same classes for
situations when performance is required.
'''

from ..common import MaxKeyParts
from ...error import IsamNotOpen, IsamNoRecord, IsamFunctionFailed, IsamEndFile
from ...constants import OpenMode, LockMode, ReadMode, StartMode, IndexFlags
from ...utils import ISAM_bytes, ISAM_str
import os

__all__ = 'ISAMcommonMixin', 'ISAMindexMixin', 'dictinfo', 'keydesc'

# Provide a raw buffer used to pass record area to ISAM library
class RecordBuffer:
  def __init__(self, size):
    self.size = size + 1

  def __call__(self):
    from .. import cffi_obj
    return cffi_obj.buffer(cffi_obj.new('char[]', self.size))

# Provide same information as the ctypes backend provides
class dictinfo:
  'Class that provides the dictinfo as expected by the rest of the package'
  def __init__(self, dinfo):
    self.nkeys = dinfo.di_nkeys
    self.recsize = dinfo.di_recsize
    self.idxsize = dinfo.di_idxsize
    self.nrecords = dinfo.di_nrecords

  def __str__(self):
    return 'NKEY: {0.nkeys}; RECSIZE: {0.recsize}; ' \
           'IDXSIZE: {0.idxsize}; NREC: {0.nrecords}'.format(self)

class keypart:
  def __init__(self, kpart):
    self.start = kpart.kp_start
    self.leng  = kpart.kp_leng
    self.type  = kpart.kp_type

  def __str__(self):
    return f'({self.start}, {self.leng}, {self.type})'

class keydesc:
  'Class that provides the keydesc as expected by the rest of the package'
  def __init__(self, kinfo=None):
    from .. import cffi_obj
    if kinfo is None:
      self._kinfo = cffi_obj.new('struct keydesc *')
      self._kpart = [None] * MayKeyParts
    else:
      self._kinfo = kinfo
      self._kpart = [None] * kinfo.k_nparts

  @property
  def nparts(self):
    return self._kinfo.k_nparts

  """ NOT USED:
  @nparts.setter
  def nparts(self, nparts):
    if 0 <= nparts < 9:
      cparts = self._kinfo.k_nparts
      self._kinfo.k_nparts = nparts
    else:
      raise ValueError('An index can only have a maximum of 8 parts')
    if nparts > cparts:
      self._kpart = self._kpart[:] + [None] * (nparts - cparts)
    elif nparts < cparts:
      self._kpart = self._kpart[:nparts]
  END NOT USED """

  @property
  def flags(self):
    return self._kinfo.k_flags

  """ NOT USED:
  @flags.setter
  def flags(self, flags):
    self._kinfo.k_flags = flags
  END NOT USED """

  @property
  def length(self):
    return self._kinfo.k_len

  """ NOT USED:
  @length.setter
  def length(self, leng):
    self._kinfo.k_len = leng
  END NOT USED"""

  def __getitem__(self, part):
    if not isinstance(part, int):
      raise ValueError('Expecting an integer key part number')
    if part < 0:
      part = self._kinfo.k_nparts + part
      if self._kinfo.k_nparts < part:
        raise ValueError('Cannot refer beyound first key part')
    elif self._kinfo.k_nparts < part:
      raise ValueError('Cannot refer beyond last key part')
    kpart = self._kpart[part]
    if kpart is None:
      kpart = self._kpart[part] = keypart(self._kinfo.k_part[part])
    return kpart

  """ NOT USED:
  def __setitem__(self, part, kpart):
    if not isinstance(part, int):
      raise ValueError('Expecting an integer key part number')
    elif part < -self._kinfo.k_nparts:
      raise ValueError('Cannot refer beyond first key part')
    elif part < 0:
      part = self._kinfo.k_nparts + part
    elif part >= self._kinfo.k_nparts:
      raise ValueError('Cannot refer beyond last key part')
    if not isinstance(kpart, keypart):
      raise ValueError('Expecting an instance of keypart')
    self._kinfo.k_part[part].kp_start = kpart.start
    self._kinfo.k_part[part].kp_leng = kpart.length
    self._kinfo.k_part[part].kp_type = kpart.type
    self._kpart[part] = keypart(self._kinfo.k_part[part])
  END NOT USED """

  def __eq__(self, other):
    'Compare the given keydesc to check if they are identical'
    if isinstance(other, keydesc):
      if self._kinfo.k_nparts != other._kinfo.k_nparts:
        return False
      if self._kinfo.k_flags != other._kinfo.k_flags:
        return False
      for part in range(self._kinfo.k_nparts):
        self_part = self._kinfo.k_part[part]
        other_part = other._kinfo.k_part[part]
        if self_part.kp_start != other_part.kp_start:
          return False
        if self_part.kp_leng != other_part.kp_leng:
          return False
        if self_part.kp_type != other_part.kp_type:
          return False
      return True
    else:
      raise NotImplementedError('Comparison not implemented')

  def __ne__(self, other):
    'Compare the given keydesc to check if they are not identical'
    if isinstance(other, keydesc):
      if self._kinfo.k_nparts != other._kinfo.k_nparts:
        return True
      if self._kinfo.k_flags != other._kinfo.k_flags:
        return True
      for part in range(self._kinfo.k_nparts):
        self_part = self._kinfo.k_part[part]
        other_part = other._kinfo.k_part[part]
        if self_part.kp_start != other_part.kp_start:
          return True
        if self_part.kp_leng != other_part.kp_leng:
          return True
        if self_part.kp_type != other_part.kp_type:
          return True
      return False
    else:
      raise NotImplementedError('Comparison not implemented')

  @property
  def value(self):
    return self._kinfo

  def __str__(self):
    'Generate a string representation of the underlying keydesc structure'
    prt = []
    for cpart in range(self._kinfo.k_nparts):
      kpart = self._kinfo.k_part[cpart]
      prt.append(f'({kpart.kp_start}, {kpart.kp_leng}, {kpart.kp_type})')
    prt = ', '.join(prt)
    return f'({self._kinfo.k_nparts}, [{prt}], 0x{self._kinfo.k_flags:02x})'

class ISAMcommonMixin:
  ''' This provides the interface to underlying ISAM libraries adding the context of the
      current file to avoid having to remember it separately.
  '''
  __slots__ = ()

  def_openmode = OpenMode.ISINOUT
  def_lockmode = LockMode.ISMANULOCK
  _vld_errno = (100, 172)

  _const_ = (
    'iserrno', 'iserrio', 'isrecnum', 'isreclen'
  )

  def __getattr__(self, name):
    if not isinstance(name, str):
      raise AttributeError(name)
    if name in self._const_:
      val = getattr(self._lib_, name)
      if callable(val):
        val = val()
      return val
    raise AttributeError(name)

  def _chkerror(self, result, func):
    '''Perform checks on the running of the underlying ISAM function by
       checking the iserrno provided by the ISAM library.'''
    if result < 0:
      errno = self.iserrno
      if errno == 101:
        raise IsamNotOpen
      elif errno == 110:
        raise IsamEndFile
      elif errno == 111:
        raise IsamNoRecord
      elif errno:
        raise IsamFunctionFailed(func, errno, self.strerror(errno))
    return result

  """ NOT CODE:
  def strerror(self, errno=None):
    'Return the error message related to the error number given'
    if errno is None:
      errno = self.iserrno()
    if 100 <= errno < self.is_nerr:
      return ISAM_str(self._ffi_.string(self.is_errlist()[errno - 100]))
    else:
      return os.strerror(errno)
  END NOT CODE"""

  def isaddindex(self, kdesc):
    'Add an index to an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(kdesc, keydesc): raise ValueError('Must be an instance of keydesc')
    self._chkerror(self._lib_.isaddindex(self._isfd_, kdesc.value), 'isaddindex')

  def isaudit(self, mode, audname=None):
    'Perform audit trail related processing'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, str): raise ValueError('Must provide a string value')
    if mode == 'AUDSETNAME':
      self._chkerror(self._lib_.isaudit(self._isfd_, ISAM_bytes(audname), 0), 'isaudit')
    elif mode == 'AUDGETNAME':
      buff = bytes(256)
      self._chkerror(self._lib_.isaudit(self._isfd_, buff, 1), 'isaudit')
      return buff
    elif mode == 'AUDSTART':
      return bool(self._chkerror(self._lib_.isaudit(self._isfd_, b'', 2), 'isaudit'))
    elif mode == 'AUDSTOP':
      return bool(self._chkerror(self._lib_.isaudit(self._isfd_, b'', 3), 'isaudit'))
    elif mode == 'AUDINFO':
      buff = bytes(1)
      ret = self._lib_.isaudit(self._isfd_, buff, 4)
      return bool(buff[0])
    elif mode == 'AUDRECVR':
      pass
    else:
      raise ValueError('Unhandled audit mode specified')

  def isbegin(self):
    'Begin a transaction'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isbegin(self._isfd_), 'isbegin')

  def isbuild(self, tabpath, reclen, kdesc, varlen=None):
    'Build a new table in exclusive mode'
    if self._isfd_ is not None: raise IsamOpen()
    if not isinstance(kdesc, keydesc):
      raise ValueError('Must provide instance of keydesc for primary index')
    self._fdmode_ = OpenMode.ISINOUT
    self._fdlock_ = LockMode.ISEXCLLOCK
    fdmode = OpenMode.ISINOUT.value | LockMode.ISEXCLLOCK.value
    if varlen:
      self._isobj_.isreclen = varlen
    self._isfd_ = self._chkerror(self._lib_.isbuild(ISAM_bytes(tabpath), reclen, kdesc._kinfo, fdmode), 'isbuild')

  def iscleanup(self):
    'Cleanup the ISAM library'
    self._chkerror(self._lib_.iscleanup(), 'iscleanup')

  def isclose(self):
    'Close an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isclose(self._isfd_), 'isclose')
    self._isfd_ = self._fdmode_ = self._fdlock_ = None

  def iscluster(self, kdesc):
    'Create a clustered index'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.iscluster(self._isfd_, kdesc.raw()), 'iscluster')

  def iscommit(self):
    'Commit the current transaction'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.iscommit(self._isfd_), 'iscommit')

  def isdelcurr(self):
    'Delete the current record from the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isdelcurr(self._isfd_), 'isdelcurr')

  def isdelete(self, keybuff):
    'Delete a record by using its key'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(keybuff, bytes): raise ValueError('Expected a bytes array for record')
    self._chkerror(self._lib_.isdelete(self._isfd_, keybuff), 'isdelete')

  def isdelindex(self, kdesc):
    'Remove the given index from the table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(kdesc, keydesc): raise ValueError('Must provide an instance of keydesc')
    self._chkerror(self._lib_.isdelindex(self._isfd_, kdesc.value), 'isdelindex')

  def iserase(self, tabname):
    'Remove the table from the filesystem'
    self._chkerror(self._lib_.iserase(ISAM_bytes(tabname)), 'iserase')

  def isflush(self):
    'Flush the data out to the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isflush(self._isfd_), 'isflush')

  def islock(self):
    'Lock the entire table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.islock(self._isfd_), 'islock')

  def islogclose(self):
    'Close the transaction logfile'
    self._chkerror(self._lib_.islogclose(), 'islogclose')

  def islogopen(self, logname):
    'Open a transaction logfile'
    self._chkerror(self._lib_.islogopen(ISAM_bytes(logname)), 'islogopen')

  def isopen(self, tabname, mode=None, lock=None):
    'Open an ISAM table'
    if mode is None: mode = self.def_openmode
    if lock is None: lock = self.def_lockmode
    if not isinstance(mode, OpenMode) or not isinstance(lock, LockMode):
      raise ValueError('Must provide an OpenMode and/or LockMode values')
    opnmde = mode.value | lock.value
    try:
      # Try a fixed length table first
      self._isfd_ = self._chkerror(self._lib_.isopen(ISAM_bytes(tabname), opnmde), 'isopen')
    except IsamFunctionFailed as exc:
      if exc.errno != 102:
        raise
      # Try a variable length table second
      opnmde |= OpenMode.ISVARLEN.value
      mode |= OpenMode.ISVARLEN
      self._isfd_ = self._chkerror(self._lib_.isopen(ISAM_bytes(tabname), opnmde), 'isopen')
    self._fdmode_ = mode
    self._fdlock_ = lock

  def isread(self, recbuff, mode=ReadMode.ISNEXT):
    'Read a record from an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isread(self._isfd_, self._ffi_.from_buffer(recbuff), mode.value), 'isread')

  def isrecover(self):
    'Recover a transaction'
    self._chkerror(self._lib_.isrecover(), 'isrecover')

  def isrelease(self):
    'Release all locks on table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isrelease(self._isfd_), 'isrelease')

  def isrename(self, oldname, newname):
    'Rename an ISAM table'
    self._chkerror(self._lib_.isrename(ISAM_bytes(oldname), ISAM_bytes(newname)), 'isrename')

  def isrewcurr(self, recbuff):
    'Rewrite the current record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isrewcurr(self._isfd_, recbuff), 'isrewcurr')

  def isrewrec(self, recnum, recbuff):
    'Rewrite the specified record'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recnum, int): raise ValueError('Expected a numeric rowid' )
    self._chkerror(self._lib_.isrewrec(self._isfd_, recnum, recbuff), 'isrewrec')

  def isrewrite(self, recbuff):
    'Rewrite the record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isrewrite(self._isfd_, recbuff), 'isrewrite')

  def isrollback(self):
    'Rollback the current transaction'
    self._chkerror(self._lib_.isrollback(), 'isrollback')

  def issetunique(self, uniqnum):
    'Set the unique number on the table'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    self._chkerror(self._lib_.issetunique(self._isfd_, uniqnum), 'issetunique')

  def isstart(self, kdesc, mode, recbuff, keylen=0):
    'Start using a different index'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, StartMode):
      raise ValueError('Must provide a StartMode value')
    self._chkerror(self._lib_.isstart(self._isfd_, kdesc.value, keylen, self._ffi_.from_buffer(recbuff), mode.value), 'isstart')

  def isuniqueid(self):
    'Return the unique id for the table'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    self._chkerror(self._lib_.isuniqueid(self._isfd_, val), 'isuniqueid')
    return val 

  def isunlock(self):
    'Unlock the current table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.isunlock(self._isfd_), 'isunlock')

  def iswrcurr(self, recbuff):
    'Write the current record'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.iswrcurr(self._isfd_, recbuff), 'iswrcurr')

  def iswrite(self, recbuff):
    'Write a new record'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(self._lib_.iswrite(self._isfd_, recbuff), 'iswrite')

class ISAMindexMixin:
  'This class provides the cffi specific methods for ISAMindex'
  def create_keydesc(self, isobj, record, optimize=False):
    'Create a new keydesc using the column information in RECORD'
    # NOTE: The information stored in an instance of _TableIndexCol
    #       is relative to the associated column within in the
    #       record object not to the overall record in general, thus
    #       an offset of 0 indicates that the key starts at the start
    #       of the column and length indicates how much of the column
    #       is involved in the index, permitting a part of the column
    #       to be stored in the index. An offset and length of None
    #       implies the complete column is involved in the index.
    def _idxpart(idxno, idxcol):
      colinfo = record._flddict[idxcol.name]
      if idxcol.length is None and idxcol.offset is None:
        # Use the whole column in the index
        kdesc.k_part[idxno].kp_start = colinfo.offset
        kdesc.k_part[idxno].kp_leng = colinfo.size
      elif idxcol.offset is None:
        # Use the prefix part of column in the index
        if idxcol.length > colinfo._size:
          raise ValueError('Index part is larger than the specified column')
        kdesc.k_part[idxno].kp_start = colinfo.offset
        kdesc.k_part[idxno].kp_leng = idxcol.length
      else:
        # Use the length of column from the given offset in the index
        if idxcol.offset + idxcol.length > colinfo.size:
          raise ValueError('Index part too long for the specified column')
        kdesc.k_part[idxno].kp_start = colinfo.offset + idxcol.offset
        kdesc.k_part[idxno].kp_leng = idxcol.length
      kdesc.k_part[idxno].kp_type = colinfo.type.value
      return kdesc.k_part[idxno].kp_leng

    kdesc = isobj._ffi_.new('struct keydesc *')
    kdesc.k_flags = IndexFlags.DUPS if self.dups else IndexFlags.NO_DUPS
    if self.desc: kdesc.k_flags |= IndexFlags.DESCEND
    if isinstance(self._colinfo, (tuple, list)):
      # Multi-part index comprising the given columns
      kdesc_leng, kdesc.k_nparts = 0, len(self._colinfo)
      for idxno, idxcol in enumerate(self._colinfo):
        kdesc_leng += _idxpart(idxno, idxcol)
    else:
      # Single part index comprising the given column
      kdesc.k_nparts = 1
      kdesc_leng = _idxpart(0, self._colinfo)
    if optimize and kdesc_leng > 8:
      kdesc.k_flags |= IndexFlags.ALL_COMPRESS
    return keydesc(kdesc)
