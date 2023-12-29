'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying IBM C-ISAM or
VBISAM library and is designed to be a direct replacement for the ctypes based
module in that it aims to provide the same classes for situations when
performance is required.
'''

from ._isam_cffi import ffi, lib
from ...error import IsamNotOpen, IsamNoRecord, IsamFunctionFailed, IsamRecordMutable
from ...constants import OpenMode, LockMode, ReadMode, StartMode, IndexFlags
from ...utils import ISAM_bytes, ISAM_str
from operator import attrgetter
import os

__all__ = 'ISAMobjectMixin', 'ISAMindexMixin', 'dictinfo', 'keydesc', 'RecordBuffer'

# Represent a raw buffer that is used for the low-level access of the underlying
# ISAM library and can be created when required.
class RecordBuffer:
  def __init__(self, size):
    self.size = size + 1

  def __call__(self):
    'Return a rewritable array to represent a single record in ISAM'
    return ffi.buffer(ffi.new('char[]', self.size))

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
    return '({0.start}, {0.leng}, {0.type})'.format(self)

class keydesc:
  'Class that provides the keydesc as expected by the rest of the package'
  def __init__(self, kinfo=None):
    self._kinfo = ffi.new('struct keydesc *') if kinfo is None else kinfo
    self._kpart = [None] * self._kinfo.k_nparts

  @property
  def nparts(self):
    return self._kinfo.k_nparts

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

  @property
  def flags(self):
    return self._kinfo.k_flags

  @flags.setter
  def flags(self, flags):
    self._kinfo.k_flags = flags

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

  def _dump(self):
    'Generate a string representation of the underlying keydesc structure'
    prt = []
    for cpart in range(self._kinfo.k_nparts):
      prt.append('({{0.k_part[{0}].kp_start}}, {{0.k_part[{0}].kp_leng}}, {{0.k_part[{0}].kp_type}})'.format(cpart))
    res = '({{0.k_nparts}}, [{0}], 0x{{0.k_flags:02x}})'.format(', '.join(prt))
    return res.format(self._kinfo)

  __str__ = _dump 

class ISAMobjectMixin:
  ''' This provides the interface to underlying ISAM libraries adding the context of the
      current file to avoid having to remember it separately.
  '''
  __slots__ = ()
  #iserrno = attrgetter('lib.iserrno')
  @property
  def iserrno(self):
    return lib.iserrno

  @property
  def iserrio(self):
    return lib.iserrio

  @property
  def isrecnum(self):
    return lib.isrecnum

  @property
  def isreclen(self):
    return lib.isreclen

  @property
  def isversnumber(self):
    return ffi.string(lib.isversnumber).decode('utf-8')

  @property
  def iscopyright(self):
    return ffi.string(lib.iscopyright).decode('utf-8')

  @property
  def isserial(self):
    return ffi.string(lib.isserial).decode('utf-8')

  @property
  def issingleuser(self):
    return bool(lib.issingleuser)

  @property
  def is_nerr(self):
    return lib.is_nerr

  def _chkerror(self, result, args=None):
    '''Perform checks on the running of the underlying ISAM function by
       checking the iserrno provided by the ISAM library, if ARGS is
       given return that on sucessfull completion of this method'''
    if result < 0:
      if self.iserrno == 101:
        raise IsamNotOpen
      elif self.iserrno == 111:
        raise IsamNoRecord
      elif self.iserrno != 0:
        raise IsamFunctionFailed(ISAM_str(args), self.iserrno, self.strerror(self.iserrno))
    return result

  def strerror(self, errno=None):
    'Return the error message related to the error number given'
    if errno is None:
      errno = self.iserrno
    if 100 <= errno < self.is_nerr:
      return ISAM_str(ffi.string(lib.is_errlist[errno - 100]))
    else:
      return os.strerror(errno)

  def isaddindex(self, kdesc):
    'Add an index to an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(kdesc, keydesc): raise ValueError('Must be an instance of keydesc')
    self._chkerror(lib.isaddindex(self._isfd_, kdesc.value), 'isaddindex')

  def isaudit(self, mode, audname=None):
    'Perform audit trail related processing'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, str): raise ValueError('Must provide a string value')
    if mode == 'AUDSETNAME':
      self._chkerror(lib.isaudit(self._isfd_, ISAM_bytes(audname), 0), 'isaudit')
    elif mode == 'AUDGETNAME':
      buff = bytes(256)
      self._chkerror(lib.isaudit(self._isfd_, buff, 1), 'isaudit')
      return buff
    elif mode == 'AUDSTART':
      return bool(self._chkerror(lib.isaudit(self._isfd_, b'', 2), 'isaudit'))
    elif mode == 'AUDSTOP':
      return bool(self._chkerror(lib.isaudit(self._isfd_, b'', 3), 'isaudit'))
    elif mode == 'AUDINFO':
      buff = bytes(1)
      ret = lib.isaudit(self._isfd_, buff, 4)
      return bool(buff[0])
    elif mode == 'AUDRECVR':
      pass
    else:
      raise ValueError('Unhandled audit mode specified')

  def isbegin(self):
    'Begin a transaction'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isbegin(self._isfd_), 'isbegin')

  def isbuild(self, tabname, reclen, kdesc, varlen=None):
    'Build a new table in exclusive mode'
    if self._isfd_ is not None: raise IsamException('Attempt to build with open table')
    if not isinstance(kdesc, keydesc): raise ValueError('Must provide instance of keydesc for primary index')
    self._fdmode_ = OpenMode.ISINOUT
    self._fdlock_ = LockMode.ISEXCLLOCK
    self._isfd_ = self._chkerror(lib.isbuild(ISAM_bytes(tabname), reclen, kdesc, self._fdmode_.value | self._fdlock_.value), 'isbuild')

  def iscleanup(self):
    'Cleanup the ISAM library'
    self._chkerror(lib.iscleanup(), 'iscleanup')

  def isclose(self):
    'Close an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isclose(self._isfd_), 'isclose')
    self._isfd_ = self._fdmode_ = self._fdlock_ = None

  def iscluster(self, kdesc):
    'Create a clustered index'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.iscluster(self._isfd_, kdesc.raw()), 'iscluster')

  def iscommit(self):
    'Commit the current transaction'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.iscommit(self._isfd_), 'iscommit')

  def isdelcurr(self):
    'Delete the current record from the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isdelcurr(self._isfd_), 'isdelcurr')

  def isdelete(self, keybuff):
    'Delete a record by using its key'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(keybuff, bytes): raise ValueError('Expected a bytes array for record')
    self._chkerror(lib.isdelete(self._isfd_, keybuff), 'isdelete')

  def isdelindex(self, kdesc):
    'Remove the given index from the table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(kdesc, keydesc): raise ValueError('Must provide an instance of keydesc')
    self._chkerror(lib.isdelindex(self._isfd_, kdesc.value), 'isdelindex')

  def isdictinfo(self):
    'Return the current dictionary information'
    if self._isfd_ is None: raise IsamNotOpen
    dinfo = ffi.new('struct dictinfo *')
    self._chkerror(lib.isdictinfo(self._isfd_, dinfo), 'isdictinfo')
    return dictinfo(dinfo)

  def iserase(self, tabname):
    'Remove the table from the filesystem'
    self._chkerror(lib.iserase(ISAM_bytes(tabname)), 'iserase')

  def isflush(self):
    'Flush the data out to the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isflush(self._isfd_), 'isflush')

  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    return bool(lib.isglsversion(ISAM_bytes(tabname)), 'isglsversion')

  def isindexinfo(self, keynum):
    'Backwards compatible method for version of ISAM < 7.26'
    if keynum < 0:
      raise ValueError('Index must be a positive number or 0 for dictinfo')
    elif keynum > 0:
      return self.iskeyinfo(keynum-1)
    else:
      return self.isdictinfo()

  def iskeyinfo(self, keynum):
    'Return the keydesc for the specified key'
    if self._isfd_ is None: raise IsamNotOpen
    keyinfo = ffi.new('struct keydesc *')
    self._chkerror(lib.iskeyinfo(self._isfd_, keyinfo, keynum+1), 'iskeyinfo')
    return keydesc(keyinfo)

  def islangchk(self):
    'Switch on language checks'
    self._chkerror(lib.islangchk(), 'islangchk')

  def islanginfo(self, tabname):
    'Return the collation sequence if any in use'
    return ISAM_str(lib.islanginfo(ISAM_bytes(tabname)), 'islanginfo')

  def islock(self):
    'Lock the entire table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.islock(self._isfd_), 'islock')

  def islogclose(self):
    'Close the transaction logfile'
    self._chkerror(lib.islogclose(), 'islogclose')

  def islogopen(self, logname):
    'Open a transaction logfile'
    self._chkerror(lib.islogopen(ISAM_bytes(logname)), 'islogopen')

  def isnlsversion(self, tabname):
    # TODO: Add documentation for function
    self._chkerror(lib.isnlsversion(ISAM_bytes(tabname)), 'isnlsversion')

  def isnolangchk(self):
    'Switch off language checks'
    self._chkerror(lib.isnolangchk(), 'isnolangchk')

  def isopen(self, tabname, mode=None, lock=None):
    'Open an ISAM table'
    if mode is None: mode = self.def_openmode
    if lock is None: lock = self.def_lockmode
    if not isinstance(mode, OpenMode) or not isinstance(lock, LockMode):
      raise ValueError('Must provide an OpenMode and/or LockMode values')
    self._isfd_ = self._chkerror(lib.isopen(ISAM_bytes(tabname), mode.value | lock.value), 'isopen')
    self._fdmode_ = mode
    self._fdlock_ = lock

  def isread(self, recbuff, mode=ReadMode.ISNEXT):
    'Read a record from an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isread(self._isfd_, ffi.from_buffer(recbuff), mode.value), 'isread')

  def isrecover(self):
    'Recover a transaction'
    self._chkerror(lib.isrecover(), 'isrecover')

  def isrelease(self):
    'Release all locks on table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isrelease(self._isfd_), 'isrelease')

  def isrename(self, oldname, newname):
    'Rename an ISAM table'
    self._chkerror(lib.isrename(ISAM_bytes(oldname), ISAM_bytes(newname)), 'isrename')

  def isrewcurr(self, recbuff):
    'Rewrite the current record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isrewcurr(self._isfd_, recbuff), 'isrewcurr')

  def isrewrec(self, recnum, recbuff):
    'Rewrite the specified record'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recnum, int): raise ValueError('Expected a numeric rowid' )
    self._chkerror(lib.isrewrec(self._isfd_, recnum, recbuff), 'isrewrec')

  def isrewrite(self, recbuff):
    'Rewrite the record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isrewrite(self._isfd_, recbuff), 'isrewrite')

  def isrollback(self):
    'Rollback the current transaction'
    self._chkerror(lib.isrollback(), 'isrollback')

  def issetunique(self, uniqnum):
    'Set the unique number on the table'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    self._chkerror(lib.issetunique(self._isfd_, uniqnum), 'issetunique')

  def isstart(self, kdesc, mode, recbuff, keylen=0):
    'Start using a different index'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, StartMode):
      raise ValueError('Must provide a StartMode value')
    self._chkerror(lib.isstart(self._isfd_, kdesc.value, keylen, ffi.from_buffer(recbuff), mode.value), 'isstart')

  def isuniqueid(self):
    'Return the unique id for the table'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    self._chkerror(lib.isuniqueid(self._isfd_, val), 'isuniqueid')
    return val 

  def isunlock(self):
    'Unlock the current table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isunlock(self._isfd_), 'isunlock')

  def iswrcurr(self, recbuff):
    'Write the current record'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.iswrcurr(self._isfd_, recbuff), 'iswrcurr')

  def iswrite(self, recbuff):
    'Write a new record'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.iswrite(self._isfd_, recbuff), 'iswrite')

class ISAMindexMixin:
  'This class provides the cffi specific methods for ISAMindex'
  def create_keydesc(self, record, optimize=False):
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
      colinfo = record._colinfo(idxcol.name)
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

    kdesc = ffi.new('struct keydesc *')
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
