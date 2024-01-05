'''
This module provides the common definitions for both the variants of the ISAM
library, these will be pulled by the __init__ module to create the necessary
objects for the package to use.
'''

import functools
from ctypes import create_string_buffer, Structure, POINTER
from ctypes import c_char, c_short, c_int, c_int32, c_char_p
from ..common import _checkpart, MaxKeyParts, MaxKeyLength
from ...constants import IndexFlags, LockMode, OpenMode, ReadMode, StartMode
from ...error import IsamNotOpen, IsamOpen, IsamFunctionFailed
from ...utils import ISAM_bytes, ISAM_str

_all__ = ('RecordBuffer', 'decimal', 'keypart', 'keydesc' ,'dictinfo',
          'ISAMcommonMixin', 'ISAMindexMixin')

class RecordBuffer:
  '''Represents the raw buffer used for low-level access of the underlying 
  ISAM library can can be created when required'''
  def __init__(self, size):
    self.size = size + 1

  def __call__(self):
    'Return a rewritable area to represent a single record in ISAM'
    return create_string_buffer(self.size)

# Define the structures that are usually defined by the isam.h
# and decimal.h header files.
class decimal(Structure):
  _fields_ = [('exp', c_short),
              ('pos', c_short),
              ('ndgts', c_short),
              ('dgts', c_char * 16)]

class keypart(Structure):
  _fields_ = [('start', c_short),
              ('leng', c_short),
              ('type', c_short)]

  def __str__(self):
    return f'({self.start}, {self.leng}, {self.type})'

class keydesc(Structure):
  _fields_ = [('flags', c_short),
              ('nparts', c_short),
              ('part', keypart * MaxKeyParts),
              ('length', c_short),           # Total length of key
              ('_pad', c_char * 16)]         # NOTE: Allow for additional internal information

  def __getitem__(self, part):
    'Return the specified keydesc part object'
    return self.part[_checkpart(self, part)]

  def __setitem__(self, part, kpart):
    'Set the specified key part to the definition provided'
    part = _checkpart(self, part)
    self.part[part].start = kpart.start
    self.part[part].leng = kpart.leng
    self.part[part].type = kpart.type

  def __eq__(self, other):
    'Compare if the two keydesc structures are the same'
    if (self.flags & IndexFlags.DUPS) ^ (other.flags & IndexFlags.DUPS):
      return False
    if self.nparts != other.nparts:
      return False
    for spart, opart in zip(self.part, other.part):
      if spart.start != opart.start:
        return False
      if spart.leng != opart.leng:
        return False
      if spart.type != opart.type:
        return False
    return True
    
  def _dump(self):
    'Generate a string representation of the underlying keydesc structure'
    res = [f'({self.nparts}, [']
    res.append(', '.join([str(self.part[cpart]) for cpart in range(self.nparts)]))
    res.append(f'], 0x{self.flags:02x})')
    return ''.join(res)
  __str__ = _dump

  @property
  def value(self):
    return self        # Return ourselves as the value

class dictinfo(Structure):
  _fields_ = [('nkeys', c_short),
              ('recsize', c_short),
              ('idxsize', c_short),
              ('nrecords', c_int32)]

  def __str__(self):
    return f'NKEY: {self.nkeys}; RECSIZE: {self.recsize}; ' \
           f'IDXSIZE: {self.idxsize}; NREC: {self.nrecords}'

# Decorator function that wraps the methods within the ISAMobject that
# will perform the necessary actions on the first invocation of the ISAM
# function before calling the method itself.
# ORIG_ARGS is the list of parameters passed to the underlying ISAM library
# including the file descriptor. ORIG_KWD is an alternative way of passing
# both the return type and arguments of the underlying ISAM library routine
# when the later does not return an 'int', as expected by the ctypes module,
# in this case the 'restype' is the return type and 'argtypes' is the same
# list of arguments including the file descriptor passed to the underlying
# ISAM function.
def ISAMfunc(*orig_args, **orig_kwd):
  def call_wrapper(func):
    def wrapper(self, *args, **kwd):
      if func.__name__ not in self._lib_.__dict__:
        # Avoid the recursive nature of getattr/hasattr to check
        # if the underlying library has already looked this up.
        lib_func = getattr(self._lib_, func.__name__)
        if 'restype' in orig_kwd:
          lib_func.restype = orig_kwd['restype']
          if 'argtypes' in orig_kwd:
            lib_func.argtypes = orig_kwd['argtypes']
          else:
            lib_func.argtypes = orig_args if orig_args else None
          lib_func.errcheck = self._chkerror
        elif 'argtypes' in orig_kwd:
          lib_func.argtypes = orig_kwd['argtypes']
          lib_func.errcheck = self._chkerror
        elif orig_args:
          lib_func.argtypes = orig_args if orig_args[0] else None
          lib_func.errcheck = self._chkerror
        else:
          lib_func.restype = lib_func.argtypes = None
      return func(self, *args, **kwd)
    return functools.wraps(func)(wrapper)
  return call_wrapper

class ISAMcommonMixin:
  '''Class providing the shared functionality between all variants of the library.
  The underlying ISAM routines are loaded on demand and added to the dictionary
  with a prefix of underscore once the arguments and return type have been set'''
  def_openmode = OpenMode.ISINOUT
  def_lockmode = LockMode.ISMANULOCK

  def _chkerror(self, result=None, func=None, args=None):
    '''Perform checks on the running of the underlying ISAM function by
       checking the iserrno provided by the ISAM library, if ARGS is
       given return that on successful completion of this method'''
    if not isinstance(result, int):
      raise ValueError(f'Unexpected result: {result}')
    elif result < 0:
      errno = self.iserrno
      if errno == 101:
        raise IsamNotOpen
      elif errno == 111:
        raise IsamNoRecord
      elif 100 <= errno < 172:
        raise IsamFunctionFailed(func.__name__, errno, ISAM_str(self.is_errlist[errno - 100]))
      elif errno:
        raise IsamFunctionFailed(func.__name__, errno, 'Unkown')
    else:
      return args

  @ISAMfunc(c_int, POINTER(keydesc))
  def isaddindex(self, kdesc):
    '''Add an index to an open ISAM table'''
    if self._isfd_ is None: raise IsamNotOpen
    return self._isaddindex(self._isfd_, kdesc)

  @ISAMfunc(c_int, c_char_p, c_int)
  def isaudit(self, mode, audname=None):
    '''Perform audit trail related processing'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, str): raise ValueError('Must provide a string value')
    if mode == 'AUDSETNAME':
      return self._isaudit(self._isfd_, ISAM_bytes(audname), 0)
    elif mode == 'AUDGETNAME':
      resbuff = create_string_buffer(256)
      self._isaudit(self._isfd_, resbuff, 1)
      return resbuff.value()
    elif mode == 'AUDSTART':
      return self._isaudit(self._isfd_, b'', 2)
    elif mode == 'AUDSTOP':
      return self._isaudit(self._isfd_, b'', 3)
    elif mode == 'AUDINFO':
      resbuff = create_string_buffer(1)
      self._isaudit(self._isfd_, resbuff, 4)
      return resbuff[0] != '\0'
    elif mode == 'AUDRECVR':
      raise NotImplementedError('Audit recovery is not implemented')
    else:
      raise ValueError('Unhandled audit mode specified')

  @ISAMfunc(None)
  def isbegin(self):
    '''Begin a transaction'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isbegin()

  @ISAMfunc(c_char_p, c_int, POINTER(keydesc), c_int)
  def isbuild(self, tabname, reclen, kdesc, varlen=None):
    '''Build a new table in exclusive mode'''
    if self._isfd_ is not None:
      raise IsamOpen('Attempt to build with open table')
    if not isinstance(kdesc, keydesc):
      raise ValueError('Must provide instance of keydesc for the primary index')
    self._fdmode_ = OpenMode.ISINOUT
    self._fdlock_ = LockMode.ISEXCLLOCK
    self._isfd_ = self._isbuild(ISAM_bytes(tabname), reclen, kdesc, self._fdmode_.value|self._fdlock_.value)

  @ISAMfunc(None)
  def iscleanup(self):
    '''Cleanup the ISAM library'''
    self._iscleanup()

  @ISAMfunc(c_int)
  def isclose(self):
    '''Close an open ISAM table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isclose(self._isfd_)
    self._isfd_ = self._fdmode_ = self._fdlock_ = None

  @ISAMfunc(c_int, POINTER(keydesc))
  def iscluster(self, kdesc):
    '''Create a clustered index'''
    if self._isfd_ is None: raise IsamNotOpen
    self._iscluster(self._isfd_, kdesc)

  @ISAMfunc(None)
  def iscommit(self):
    '''Commit the current transaction'''
    if self._isfd_ is None: raise IsamNotOpen
    self._iscommit()

  @ISAMfunc(c_int)
  def isdelcurr(self):
    '''Delete the current record from the table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isdelcurr(self._isfd_)

  @ISAMfunc(c_int, c_char_p)
  def isdelete(self, keybuff):
    '''Remove a record by using its key'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isdelete(self._isfd_, keybuff)

  @ISAMfunc(c_int, POINTER(keydesc))
  def isdelindex(self, kdesc):
    '''Remove the given index from the table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isdelindex(self._isfd_, kdesc)

  @ISAMfunc(c_int, c_int32)
  def isdelrec(self, recnum):
    '''Delete the specified row from the table'''
    if self._isfd is None: raise IsamNotOpen
    self._isdelrec(self._isfd_, recnum)

  @ISAMfunc(c_int, POINTER(dictinfo))
  def isdictinfo(self):
    '''Return the dictionary information for table'''
    if self._isfd_ is None: raise IsamNotOpen
    ptr = dictinfo()
    self._isdictinfo(self._isfd_, ptr)
    return ptr

  @ISAMfunc(c_char_p)
  def iserase(self, tabname):
    '''Remove the table from the filesystem'''
    self._iserase(tabname)

  @ISAMfunc(c_int)
  def isflush(self):
    '''Flush the data out to the table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isflush(self._isfd_)

  @ISAMfunc(c_char_p)
  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    return self._isglsversion(ISAM_bytes(tabname))

  def isindexinfo(self, keynum):
    'Backwards compatible method for version of ISAM < 7.26'
    if keynum < 0:
      raise ValueError('Index must be a positive number or 0 for dictinfo')
    elif keynum > 0:
      return self.iskeyinfo(keynum+1)
    else:
      return self.isdictinfo()

  @ISAMfunc(c_int, POINTER(keydesc), c_int)
  def iskeyinfo(self, keynum):
    'Return the keydesc for the specified key'
    if self._isfd_ is None: raise IsamNotOpen
    if keynum < 0: raise ValueError('Index must be a positive number')
    ptr = keydesc()
    self._iskeyinfo(self._isfd_, ptr, keynum+1)
    return ptr

  @ISAMfunc(None)
  def islangchk(self):
    'Switch on language checks'
    self._islangchk()

  @ISAMfunc(c_char_p, restype=c_char_p)
  def islanginfo(self,tabname):
    'Return the collation sequence if any in use'
    return ISAM_str(self._islanginfo(ISAM_bytes(tabname)))

  @ISAMfunc(c_int)
  def islock(self):
    'Lock the entire table'
    if self._isfd_ is None: raise IsamNotOpen
    self._islock(self._isfd_)

  @ISAMfunc(None)
  def islogclose(self):
    'Close the transaction logfile'
    self._islogclose()

  @ISAMfunc(c_char_p)
  def islogopen(self,logname):
    'Open a transaction logfile'
    self._islogopen(logname)

  @ISAMfunc(c_char_p)
  def isnlsversion(self,tabname):
    self._isnlsversion(ISAM_bytes(tabname))

  @ISAMfunc(None)
  def isnolangchk(self):
    'Switch off language checks'
    self._isnolangchk()

  @ISAMfunc(c_char_p, c_int)
  def isopen(self, tabname, mode=None, lock=None):
    'Open an ISAM table'
    if mode is None:
      mode = self.def_openmode
    if lock is None:
      lock = self.def_lockmode
    if not isinstance(mode, OpenMode) or not isinstance(lock, LockMode):
      raise ValueError('Must provide an OpenMode and/or LockMode values')    
    self._isfd_ = self._isopen(ISAM_bytes(tabname), mode.value|lock.value)
    self._fdmode_ = mode
    self._fdlock_ = lock

  @ISAMfunc(c_int, c_char_p, c_int)
  def isread(self, recbuff, mode=ReadMode.ISNEXT):
    'Read a record from an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    self._isread(self._isfd_, recbuff, mode.value)

  @ISAMfunc(None)
  def isrecover(self):
    'Recover a transaction'
    self._isrecover()

  @ISAMfunc(c_int)
  def isrelease(self):
    'Release locks on table'
    if self._isfd_ is None: raise IsamNotOpen
    self._isrelease(self._isfd_)

  @ISAMfunc(c_char_p, c_char_p)
  def isrename(self, oldname, newname):
    'Rename an ISAM table'
    self._isrename(ISAM_bytes(oldname), ISAM_bytes(newname))

  @ISAMfunc(c_int, c_char_p)
  def isrewcurr(self, recbuff):
    'Rewrite the current record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._isrewcurr(self._isfd_, recbuff)

  @ISAMfunc(c_int, c_int32, c_char_p)
  def isrewrec(self, recnum, recbuff):
    'Rewrite the specified record'
    if self._isfd_ is None: raise IsamNotOpen
    self._isrewrec(self._isfd_, recnum, recbuff)

  @ISAMfunc(c_int, c_char_p)
  def isrewrite(self, recbuff):
    'Rewrite the record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    self._isrewrite(self._isfd_, recbuff)

  @ISAMfunc(None)
  def isrollback(self):
    'Rollback the current transaction'
    self._isrollback()

  @ISAMfunc(c_int, c_int32)
  def issetunique(self, uniqnum):
    'Set the unique number'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    if not isinstance(uniqnum, c_int32):
      uniqnum = c_int32(uniqnum)
    self._issetunique(self._isfd_, uniqnum)

  @ISAMfunc(c_int, POINTER(keydesc), c_int, c_char_p, c_int)
  def isstart(self, kdesc, mode, recbuff, keylen=0):
    'Start using a different index'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, StartMode):
      raise ValueError('Must provide a StartMode value')
    self._isstart(self._isfd_, kdesc, keylen, recbuff, mode.value)

  @ISAMfunc(c_int, restype=int)
  def isuniqueid(self):
    'Return the unique id for the table'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    ptr = c_int32(0)
    self._isuniqueid(self._isfd_, ptr)
    return ptr.value

  @ISAMfunc(c_int)
  def isunlock(self):
    'Unlock the current table'
    if self._isfd_ is None: raise IsamNotOpen
    self._isunlock(self._isfd_)

  @ISAMfunc(c_int, c_char_p)
  def iswrcurr(self, recbuff):
    'Write the current record'
    if self._isfd_ is None: raise IsamNotOpen
    return self._iswrcurr(self._isfd_, recbuff)

  @ISAMfunc(c_int, c_char_p)
  def iswrite(self, recbuff):
    'Write a new record'
    if self._isfd_ is None: raise IsamNotOpen
    return self._iswrite(self._isfd_, recbuff)

class ISAMindexMixin:
  'This class provides the ctypes specific methods for ISAMindex'
  def create_keydesc(self, isobj, record, optimize=False):
    'Create a new keydesc using the column information in RECORD'
    # NOTE: The information stored in an instance of _TableIndexCol
    #       is relative to the associated column within in the
    #       record object not to the overall record in general, thus
    #       an offset of 0 indicates that the key starts at the start
    #       of the column and length indicates how much of the column
    #       is involved in the index, permitting a part of the column
    #       to be stored in the index. An offset and length of None
    #       implies that the complete column is involved in the index.
    def _idxpart(idxcol):
      colinfo = record._flddict[idxcol.name]
      kpart = keypart()
      if idxcol.length is None and idxcol.offset is None:
        # Use the whole column in the index
        kpart.start = colinfo.offset
        kpart.leng = colinfo.size
      elif idxcol.offset is None:
        # Use the prefix part of column in the index
        if idxcol.length > colinfo.size:
          raise ValueError('Index part is larger than the specified column')
        kpart.start = colinfo.offset
        kpart.leng = idxcol.length
      elif idxcol.length is None:
        # Use the remainder of the column from the given offset
        if idxcol.offset > colinfo._size:
          raise ValueError('Index part is larger than the specified column')
        elif idxcol.offset == colinfo.size:
          raise ValueError('Index part cannot have a zero length')
        kpart.start = idxcol.offset
        kpart.leng = colinfo.size - idxcol.offset
      elif idxcol.length == 0:
        raise ValueError('Invalid index defined with zero length')
      else:
        # Use the length of column from the given offset in the index
        if idxcol.offset + idxcol.length > colinfo.size:
          raise ValueError('Index part too long for the specified column')
        kpart.start = colinfo.offset + idxcol.offset
        kpart.leng = idxcol.length
      kpart.type = colinfo.type.value
      return kpart
    kdesc = keydesc()
    kdesc.flags = IndexFlags.DUPS if self.dups else IndexFlags.NO_DUPS
    if self.desc:
      kdesc.flags += IndexFlags.DESCEND
    kdesc_leng = 0
    if isinstance(self._colinfo, (tuple, list)):
      # Multi-part index comprising the given columns
      kdesc.nparts = len(self._colinfo)
      for idxno, idxcol in enumerate(self._colinfo):
        kpart = kdesc[idxno] = _idxpart(idxcol)
        kdesc_leng += kpart.leng
    else:
      # Single part index comprising the given column
      kdesc.nparts = 1
      kpart = kdesc[0] = _idxpart(self._colinfo)
      kdesc_leng = kpart.leng
    if kdesc_leng >= MaxKeyLength:
      raise ValueError(f'Maximum length of an index cannot exceed {MAXKLENG} bytes')
    if optimize and kdesc_leng > 8:
      kdesc.flags += IndexFlags.ALL_COMPRESS
    return kdesc
