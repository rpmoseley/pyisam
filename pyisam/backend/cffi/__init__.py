'''
This is the CFFI specific implementation of the pyisam package

This module provides a cffi based interface to the underlying IBM C-ISAM or VBISAM library and
is designed to be a direct replacement for the ctypes based module in that it aims to provide
the same objects and also functionality for situations when performance is required.
'''

from ._isam_cffi import ffi, lib
from ...error import IsamNotOpen, IsamNoRecord, IsamFunctionFailed, IsamRecordMutable
from ...enums import OpenMode, LockMode, ReadMode, StartMode, IndexFlags
from ...utils import ISAM_bytes, ISAM_str

__all__ = 'ISAMobjectMixin', 'ISAMindexMixin', 'dictinfo', 'keydesc', 'RecordBuffer'

# Return a raw buffer that is used for the low-level access of the underlying
# ISAM library and can be created when required.
def RecordBuffer(size):
  'Return a rewritable array to represent a single record in ISAM'
  return ffi.buffer(ffi.new('char[]', size + 1))

# Provide same information as the ctypes version by wrapping the underlying implementation.
class dictinfo:
  'Class that provides the names expected by the rest of the package'
  def __init__(self, dinfo):
    self._dinfo = dinfo
  @property
  def nkeys(self):
    return self._dinfo.di_nkeys
  @property
  def recsize(self):
    return self._dinfo.di_recsize
  @property
  def idxsize(self):
    return self._dinfo.di_idxsize
  @property
  def nrecords(self):
    return self._dinfo.di_nrecords
  def __str__(self):
    return 'NKEY: {0.nkeys}; RECSIZE: {0.recsize}; ' \
           'IDXSIZE: {0.idxsize}; NREC: {0.nrecords}'.format(self)

class keydesc:
  'Class that provides the names expected by the package using ctypes'
  def __init__(self, kinfo):
    self._kinfo = kinfo
    print(dir(kinfo))  #DEBUG
  def __str__(self):
    out = ['(', ]
    
class ISAMobjectMixin:
  ''' This provides the interface to underlying ISAM libraries adding the context of the
      current file to avoid having to remember it separately.
  '''
  __slots__ = ()
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
  def _chkerror(self, result):
    '''Perform checks on the running of the underlying ISAM function by
       checking the iserrno provided by the ISAM library, if ARGS is
       given return that on sucessfull completion of this method'''
    if result < 0:
      if self.iserrno == 101:
        raise IsamNotOpen
      elif self.iserrno == 111:
        raise IsamNoRecord
      elif self.iserrno != 0:
        raise IsamFunctionFailed(ISAM_str(args[0]), self.iserrno, self.strerror(self.iserrno))
    return result
  def strerror(self, errno=None):
    'Return the error message related to the error number given'
    if errno is None:
      errno = self.iserrno
    if 100 <= errno < self.is_nerr:
      return ISAM_str(self.is_errlist[errno - 100]) # NOTE: May need to review
    else:
      return os.strerror(errno)
  def isaddindex(self, kdesc):
    'Add an index to an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(kdesc, keydesc): raise ValueError('Must be an instance of keydesc')
    self._chkerror(lib.isaddindex(self._isfd_, kdesc.raw))
  def isaudit(self, mode, audname=None):
    'Perform audit trail related processing'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, str): raise ValueError('Must provide a string value')
    if mode == 'AUDSETNAME':
      self._chkerror(lib.isaudit(self._isfd_, ISAM_bytes(audname), 0))
    elif mode == 'AUDGETNAME':
      buff = bytes(256)
      self._chkerror(lib.isaudit(self._isfd_, buff, 1))
      return buff
    elif mode == 'AUDSTART':
      return bool(self._chkerror(lib.isaudit(self._isfd_, b'', 2)))
    elif mode == 'AUDSTOP':
      return bool(self._chkerror(lib.isaudit(self._isfd_, b'', 3)))
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
    self._chkerror(lib.isbegin(self._isfd_))
  def isbuild(self, tabname, reclen, kdesc, varlen=None):
    'Build a new table in exclusive mode'
    if self._isfd_ is not None: raise IsamException('Attempt to build with open table')
    if not isinstance(kdesc, keydesc): raise ValueError('Must provide instance of keydesc for primary index')
    self._fdmode_ = OpenMode.ISINOUT
    self._fdlock_ = LockMode.ISEXCLLOCK
    self._isfd_ = self._chkerror(lib.isbuild(ISAM_bytes(tabname), reclen, kdesc, self._fdmode_.value | self._fdlock_.value))
  def iscleanup(self):
    'Cleanup the ISAM library'
    self._chkerror(lib.iscleanup())
  def isclose(self):
    'Close an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isclose(self._isfd_))
    self._isfd_ = self._fdmode_ = self._fdlock_ = None
  def iscluster(self, kdesc):
    'Create a clustered index'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.iscluster(self._isfd_, kdesc.raw()))
  def iscommit(self):
    'Commit the current transaction'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.iscommit(self._isfd_))
  def isdelcurr(self):
    'Delete the current record from the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isdelcurr(self._isfd_))
  def isdelete(self, keybuff):
    'Delete a record by using its key'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(keybuff, bytes): raise ValueError('Expected a bytes array for record')
    self._chkerror(lib.isdelete(self._isfd_, keybuff))
  def isdelindex(self, kdesc):
    'Remove the given index from the table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(kdesc, keydesc): raise ValueError('Must provide an instance of keydesc')
    self._chkerror(lib.isdelindex(self._isfd_, kdesc))
  def isdictinfo(self):
    'Return the current dictionary information'
    if self._isfd_ is None: raise IsamNotOpen
    dinfo = ffi.new('struct dictinfo *')
    self._chkerror(lib.isdictinfo(self._isfd_, dinfo))
    return dictinfo(dinfo)
  def iserase(self, tabname):
    'Remove the table from the filesystem'
    self._chkerror(lib.iserase(ISAM_bytes(tabname)))
  def isflush(self):
    'Flush the data out to the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isflush(self._isfd_))
  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    return bool(lib.isglsversion(ISAM_bytes(tabname)))
  def isindexinfo(self, keynum):
    'Backwards compatible method for version of ISAM < 7.26'
    if keynum < 0:
      raise ValueError('Index must be a positive number or 0 for dictinfo')
    elif keynum > 0:
      return self.iskeyinfo(keynum)
    else:
      return self.isdictinfo()
  def iskeyinfo(self, keynum):
    'Return the keydesc for the specified key'
    if self._isfd_ is None: raise IsamNotOpen
    keyinfo = ffi.new('struct keydesc *')
    self._chkerror(lib.iskeyinfo(self._isfd_, keyinfo, keynum))
    return keyinfo
  def islangchk(self):
    'Switch on language checks'
    self._chkerror(lib.islangchk())
  def islanginfo(self, tabname):
    'Return the collation sequence if any in use'
    return ISAM_str(lib.islanginfo(ISAM_bytes(tabname)))
  def islock(self):
    'Lock the entire table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.islock(self._isfd_))
  def islogclose(self):
    'Close the transaction logfile'
    self._chkerror(lib.islogclose())
  def islogopen(self, logname):
    'Open a transaction logfile'
    self._chkerror(lib.islogopen(ISAM_bytes(logname)))
  def isnlsversion(self, tabname):
    # TODO: Add documentation for function
    self._chkerror(lib.isnlsversion(ISAM_bytes(tabname)))
  def isnolangchk(self):
    'Switch off language checks'
    self._chkerror(lib.isnolangchk())
  def isopen(self, tabname, mode=None, lock=None):
    'Open an ISAM table'
    if mode is None: mode = self.def_openmode
    if lock is None: lock = self.def_lockmode
    if not isinstance(mode, OpenMode) or not isinstance(lock, LockMode):
      raise ValueError('Must provide an OpenMode and/or LockMode values')
    self._isfd_ = self._chkerror(lib.isopen(ISAM_bytes(tabname), mode.value | lock.value))
    self._fdmode_ = mode
    self._fdlock_ = lock
  def isread(self, recbuff, mode=ReadMode.ISNEXT):
    'Read a record from an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isread(self._isfd_, ffi.from_buffer(recbuff), mode.value))
  def isrecover(self):
    'Recover a transaction'
    self._chkerror(lib.isrecover())
  def isrelease(self):
    'Release all locks on table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isrelease(self._isfd_))
  def isrename(self, oldname, newname):
    'Rename an ISAM table'
    self._chkerror(lib.isrename(ISAM_bytes(oldname), ISAM_bytes(newname)))
  def isrewcurr(self, recbuff):
    'Rewrite the current record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isrewcurr(self._isfd_, recbuff))
  def isrewrec(self, recnum, recbuff):
    'Rewrite the specified record'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recnum, int): raise ValueError('Expected a numeric rowid' )
    self._chkerror(lib.isrewrec(self._isfd_, recnum, recbuff))
  def isrewrite(self, recbuff):
    'Rewrite the record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isrewrite(self._isfd_, recbuff))
  def isrollback(self):
    'Rollback the current transaction'
    self._chkerror(lib.isrollback())
  def issetunique(self, uniqnum):
    'Set the unique number on the table'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    self._chkerror(lib.issetunique(self._isfd_, uniqnum))
  def isstart(self, kdesc, mode, recbuff, keylen=0):
    'Start using a different index'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, StartMode):
      raise ValueError('Must provide a StartMode value')
    self._chkerror(lib.isstart(self._isfd_, kdesc, keylen, ffi.from_buffer(recbuff), mode.value))
  def isuniqueid(self):
    'Return the unique id for the table'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    self._chkerror(lib.isuniqueid(self._isfd_, val))
    return val 
  def isunlock(self):
    'Unlock the current table'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.isunlock(self._isfd_))
  def iswrcurr(self, recbuff):
    'Write the current record'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.iswrcurr(self._isfd_, recbuff))
  def iswrite(self, recbuff):
    'Write a new record'
    if self._isfd_ is None: raise IsamNotOpen
    self._chkerror(lib.iswrite(self._isfd_, recbuff))

class ISAMindexMixin:
  'This class provides the cffi specific methods for ISAMindex'
  def create_keydesc(self, tabobj, optimize=False):
    'Create a new keydesc using the column information in tabobj'
    def _idxpart(idxno, idxcol):
      colinfo = getattr(tabobj, idxcol.name)
      print(colinfo) #DEBUG
      if idxcol.length is None and idxcol.offset is None:
        # Use the whole column in the index
        kdesc.k_part[idxno].kp_start = colinfo._offset_
        kdesc.k_part[idxno].kp_leng = colinfo._size_
      elif idxcol.offset is None:
        # Use the prefix part of column in the index
        if idxcol.length > colinfo._size_:
          raise ValueError('Index part is larger than the specified column')
        kdesc.k_part[idxno].kp_start = colinfo._start_
        kdesc.k_part[idxno].kp_leng = idxcol.length
      else:
        # Use the length of column from the given offset in the index
        if idxcol.offset + idxcol.length > colinfo._size_:
          raise ValueError('Index part too long for the specified column')
        kdesc.k_part[idxno].kp_start = colinfo._offset_ + idxcol.offset
        kdesc.k_part[idxno].kp_leng = idxcol.length
      kdesc.k_part[idxno].kp_type = colinfo._type_
    kdesc = ffi.new('struct keydesc *')
    kdesc.k_flags = IndexFlags.DUPS if self._dups_ else IndexFlags.NO_DUPS
    if self.desc:
      kdesc.k_flags += IndexFlags.DESCEND
    kdesc_leng = 0
    if isinstance(self._colinfo_, (tuple, list)):
      # Multi-part index comprising the given columns
      kdesc.k_nparts = len(self._colinfo_)
      for idxno, idxcol in enumerate(self._colinfo_):
        _idxpart(idxno, idxcol)
        kdesc_leng += kdesc.k_part[idxno].kp_leng
    else:
      # Single part index comprising the given column
      kdesc.k_nparts = 1
      _idxpart(0, self._colinfo_)
      kdesc_leng = kdesc.k_part[0].kp_leng
    if optimize and kdesc_leng > 8:
      kdesc.k_flags += IndexFlags.ALL_COMPRESS
    return kdesc

  def match_keydesc(self, other, exact=True):
    'Check if the given OTHER keydesc matches ourselves'
    kd1, kd2 = cls._kdesc_, other._kdesc_
    if kd1.k_flags != kd2.k_flags:
      return False
    if kd1.k_nparts != kd2.k_nparts:
      return False
    for kp in range(kd1.k_nparts):
      if kd1.k_part[kp].kp_start != kd2.k_part[kp].kp_start:
        return False
      if kd1.k_part[kp].kp_leng != kd2.k_part[kp].kp_leng:
        return False
      if kd1.k_part[kp].kp_type != kd2.k_part[kp].kp_type:
        return False
    return True
