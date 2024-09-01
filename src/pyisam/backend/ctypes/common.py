'''
This module provides the common definitions for both the variants of the ISAM
library, these will be pulled by the __init__ module to create the necessary
objects for the package to use.
'''

import functools
from ctypes import create_string_buffer, Structure, POINTER, _SimpleCData
from ctypes import c_char, c_short, c_int, c_int32, c_char_p
from ..common import MaxKeyParts, MaxKeyLength, check_keypart
from ...constants import IndexFlags, LockMode, OpenMode, ReadMode, StartMode
from ...constants import dflt_lockmode, dflt_openmode
from ...error import IsamNotOpen, IsamOpen, IsamFunctionFailed, IsamEndFile, IsamReadOnly, IsamNoRecord
from ...utils import ISAM_bytes, ISAM_str

_all__ = ('decimal', 'keypart', 'ISAMkeydesc' ,'ISAMdictinfo',
          'ISAMcommonMixin', 'ISAMindexMixin')

def create_record(recsz):
  return create_string_buffer(recsz+1)

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

class ISAMkeydesc(Structure):
  _fields_ = [('flags', c_short),
              ('nparts', c_short),
              ('part', keypart * MaxKeyParts),
              ('length', c_short),           # Total length of key
              ('_pad', c_char * 16)]         # NOTE: Allow for additional internal information

  def __getitem__(self, part):
    'Return the specified keydesc part object'
    return self.part[check_keypart(self, part)]

  def __setitem__(self, part, kpart):
    'Set the specified key part to the definition provided'
    if not isinstance(kpart, keypart):
      raise ValueError('Must pass an instance of KEYPART')
    self.part[check_keypart(self, part)] = kpart

  def as_keydesc(self, ffiobj):
    'Return an internal keydesc structure'
    return self

  def __str__(self):
    'Generate a string representation of the underlying keydesc structure'
    res = [f'({self.nparts}, [']
    res.append(', '.join([str(self.part[cpart]) for cpart in range(self.nparts)]))
    res.append(f'], 0x{self.flags:02x})')
    return ''.join(res)

class ISAMdictinfo(Structure):
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
      if func.__name__ not in self._lib.__dict__:
        # Avoid the recursive nature of getattr/hasattr to check
        # if the underlying library has already looked this up.
        lib_func = getattr(self._lib, func.__name__)
        if 'restype' in orig_kwd:
          lib_func.restype = orig_kwd['restype']
          if 'argtypes' in orig_kwd:
            lib_func.argtypes = orig_kwd['argtypes']
          else:
            lib_func.argtypes = orig_args if orig_args else None
        elif 'argtypes' in orig_kwd:
          lib_func.argtypes = orig_kwd['argtypes']
        elif orig_args:
          lib_func.argtypes = orig_args if orig_args[0] else None
        else:
          lib_func.restype = lib_func.argtypes = None
        if lib_func.restype is not None:
          lib_func.errcheck = self._chkerror
      return func(self, *args, **kwd)
    return functools.wraps(func)(wrapper)
  return call_wrapper

class ISAMcommonMixin:
  '''Class providing the shared functionality between all variants of the library.
  The underlying ISAM routines are loaded on demand and added to the dictionary
  with a prefix of underscore once the arguments and return type have been set'''
  
  _vld_errno = (100, 172)

  def __getattr__(self, name):
    'Provide the command __getattr__ functionality'
    if name.startswith('_is'):
      return getattr(self._lib, name[1:])
    elif name.startswith('_'):
      raise AttributeError(name)
    elif hasattr(self, '_const') and name in self._const:
      val = self._const[name]
      if name == 'is_errlist':
        if val is None:
          val = c_char_p * (self._vld_errno[1] - self._vld_errno[0])
          val = self._const[name] = val.in_dll(self._lib, name)
        return val
      if not isinstance(val, _SimpleCData) and hasattr(val, 'in_dll'):
        val = self._const[name] = val.in_dll(self._lib, name)
      if hasattr(val, 'value'):
        val = val.value
      if not isinstance(val, int):
        val = ISAM_str(val)
      return val
    return AttributeError(name)

  def _chkerror(self, result=None, func=None, args=None):
    '''Perform checks on the running of the underlying ISAM function by
       checking the iserrno provided by the ISAM library, if ARGS is
       given return that on successful completion of this method'''
    if isinstance(result, (str, bytes)):
      return result
    elif not isinstance(result, int):
      raise ValueError(f'Unexpected result: {result}')
    elif result < 0:
      # Make the error code relative to range for variant
      errcode = self.iserrno
      errnum = errcode - self._vld_errno[0]
      if errnum == 1:
        raise IsamNotOpen
      elif errnum == 10:
        raise IsamEndFile
      elif errnum == 11:
        raise IsamNoRecord
      elif self._vld_errno[0] <= errcode < self._vld_errno[1]:
        raise IsamFunctionFailed(func.__name__, errcode, ISAM_str(self.is_errlist[errnum]))
      elif errnum:
        raise IsamFunctionFailed(func.__name__, errcode, 'Unknown')
    return result

  @ISAMfunc(c_int, POINTER(ISAMkeydesc))
  def isaddindex(self, kdesc):
    '''Add an index to an open ISAM table'''
    if self._fd is None:
      raise IsamNotOpen
    return self._isaddindex(self._fd, kdesc)

  @ISAMfunc(c_int, c_char_p, c_int)
  def isaudit(self, mode, audname=None):
    '''Perform audit trail related processing'''
    if self._fd is None:
      raise IsamNotOpen
    if not isinstance(mode, str):
      raise ValueError('Must provide a string value')
    if mode == 'AUDSETNAME':
      return self._isaudit(self._fd, ISAM_bytes(audname), 0)
    elif mode == 'AUDGETNAME':
      resbuff = create_string_buffer(256)
      self._isaudit(self._fd, resbuff, 1)
      return resbuff.value()
    elif mode == 'AUDSTART':
      return self._isaudit(self._fd, b'', 2)
    elif mode == 'AUDSTOP':
      return self._isaudit(self._fd, b'', 3)
    elif mode == 'AUDINFO':
      resbuff = create_string_buffer(1)
      self._isaudit(self._fd, resbuff, 4)
      return resbuff[0] != '\0'
    elif mode == 'AUDRECVR':
      raise NotImplementedError('Audit recovery is not implemented')
    else:
      raise ValueError('Unhandled audit mode specified')

  @ISAMfunc(None)
  def isbegin(self):
    '''Begin a transaction'''
    if self._fd is None:
      raise IsamNotOpen
    self._isbegin()

  @ISAMfunc(c_char_p, c_int, POINTER(ISAMkeydesc), c_int)
  def isbuild(self, tabpath, reclen, kdesc, varlen=None):
    '''Build a new table in exclusive mode'''
    if self._fd is not None:
      raise IsamOpen()
    if not isinstance(kdesc, ISAMkeydesc):
      raise ValueError('Must provide instance of ISAMkeydesc for the primary index')
    self._fdmode = OpenMode.ISINOUT
    self._fdlock = LockMode.ISEXCLLOCK
    fdmode = OpenMode.ISINOUT.value | LockMode.ISEXCLLOCK.value
    if varlen:
      self.isreclen = varlen
      self._fdmode |= OpenMode.ISVARLEN
      fdmode |= OpenMode.ISVARLEN.value
    self._fd = self._isbuild(ISAM_bytes(tabpath), reclen, kdesc, fdmode)

  @ISAMfunc(None)
  def iscleanup(self):
    '''Cleanup the ISAM library'''
    self._iscleanup()

  @ISAMfunc(c_int)
  def isclose(self):
    '''Close an open ISAM table'''
    if self._fd is None:
      raise IsamNotOpen
    self._isclose(self._fd)
    self._fd = self._fdmode = self._fdlock = None

  @ISAMfunc(c_int, POINTER(ISAMkeydesc))
  def iscluster(self, kdesc):
    '''Create a clustered index'''
    if self._fd is None:
      raise IsamNotOpen
    self._iscluster(self._fd, kdesc)

  @ISAMfunc(None)
  def iscommit(self):
    '''Commit the current transaction'''
    if self._fd is None:
      raise IsamNotOpen
    self._iscommit()

  @ISAMfunc(c_int)
  def isdelcurr(self):
    '''Delete the current record from the table'''
    if self._fd is None:
      raise IsamNotOpen
    self._isdelcurr(self._fd)

  @ISAMfunc(c_int, c_char_p)
  def isdelete(self, keybuff):
    '''Remove a record by using its key'''
    if self._fd is None:
      raise IsamNotOpen
    self._isdelete(self._fd, keybuff)

  @ISAMfunc(c_int, POINTER(ISAMkeydesc))
  def isdelindex(self, kdesc):
    '''Remove the given index from the table'''
    if self._fd is None:
      raise IsamNotOpen
    self._isdelindex(self._fd, kdesc)

  @ISAMfunc(c_int, c_int32)
  def isdelrec(self, recnum):
    '''Delete the specified row from the table'''
    if self._fd is None:
      raise IsamNotOpen
    self._isdelrec(self._fd, recnum)

  @ISAMfunc(c_int, POINTER(ISAMdictinfo))
  def isdictinfo(self):
    '''Return the dictionary information for table'''
    if self._fd is None:
      raise IsamNotOpen
    ptr = ISAMdictinfo()
    self._isdictinfo(self._fd, ptr)
    return ptr

  @ISAMfunc(c_char_p)
  def iserase(self, tabname):
    '''Remove the table from the filesystem'''
    self._iserase(tabname)

  @ISAMfunc(c_int)
  def isflush(self):
    '''Flush the data out to the table'''
    if self._fd is None:
      raise IsamNotOpen
    self._isflush(self._fd)

  @ISAMfunc(c_char_p)
  def isglsversion(self, tabname):
    'Return whether GLS is in use with tabname'
    return self._isglsversion(ISAM_bytes(tabname))

  def isindexinfo(self, keynum):
    'Backwards compatible method for version of ISAM < 7.26'
    if keynum is None:
      return self.isdictinfo()
    else:
      return self.iskeyinfo(keynum)

  @ISAMfunc(c_int, POINTER(ISAMkeydesc), c_int)
  def iskeyinfo(self, keynum):
    'Return the ISAMkeydesc for the specified key'
    if self._fd is None:
      raise IsamNotOpen
    if keynum is None or keynum < 0:
      raise ValueError('Index must be a positive number')
    ptr = ISAMkeydesc()
    self._iskeyinfo(self._fd, ptr, keynum+1)
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
    if self._fd is None:
      raise IsamNotOpen
    self._islock(self._fd)

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
      mode = dflt_openmode
    if lock is None:
      lock = dflt_lockmode
    if not isinstance(mode, OpenMode) or not isinstance(lock, LockMode):
      raise ValueError('Must provide an OpenMode and/or LockMode values')    
    opnmde = mode.value | lock.value
    """ NOT USED:
    try:
      # Try a fixed length table first
      self._fd = self._isopen(ISAM_bytes(tabname), opnmde)
    except IsamFunctionFailed as exc:
      if exc.errno != 102:
        raise
      # Try a variable length table second
      opnmde += OpenMode.ISVARLEN.value
      mode |= OpenMode.ISVARLEN
      self._fd = self._isopen(ISAM_bytes(tabname), opnmde)
    END NOT USED"""
    self._fd = self._isopen(ISAM_bytes(tabname), opnmde)
    self._fdmode = mode
    self._fdlock = lock
    self._recsize = self.isreclen

  @ISAMfunc(c_int, c_char_p, c_int)
  def isread(self, recbuff, mode=ReadMode.ISNEXT):
    'Read a record from an open ISAM table'
    if self._fd is None:
      raise IsamNotOpen
    self._isread(self._fd, recbuff, mode.value)

  @ISAMfunc(None)
  def isrecover(self):
    'Recover a transaction'
    self._isrecover()

  @ISAMfunc(c_int)
  def isrelease(self):
    'Release locks on table'
    if self._fd is None:
      raise IsamNotOpen
    self._isrelease(self._fd)

  @ISAMfunc(c_char_p, c_char_p)
  def isrename(self, oldname, newname):
    'Rename an ISAM table'
    self._isrename(ISAM_bytes(oldname), ISAM_bytes(newname))

  @ISAMfunc(c_int, c_char_p)
  def isrewcurr(self, recbuff):
    'Rewrite the current record on the table'
    if self._fd is None:
      raise IsamNotOpen
    self._isrewcurr(self._fd, recbuff)

  @ISAMfunc(c_int, c_int32, c_char_p)
  def isrewrec(self, recnum, recbuff):
    'Rewrite the specified record'
    if self._fd is None:
      raise IsamNotOpen
    self._isrewrec(self._sfd, recnum, recbuff)

  @ISAMfunc(c_int, c_char_p)
  def isrewrite(self, recbuff):
    'Rewrite the record on the table'
    if self._fd is None:
      raise IsamNotOpen
    self._isrewrite(self._fd, recbuff)

  @ISAMfunc(None)
  def isrollback(self):
    'Rollback the current transaction'
    self._isrollback()

  @ISAMfunc(c_int, c_int32)
  def issetunique(self, uniqnum):
    'Set the unique number'
    if self._fd is None:
      raise IsamNotOpen
    if self._fdmode is OpenMode.ISINPUT:
      raise IsamReadOnly
    if not isinstance(uniqnum, c_int32):
      uniqnum = c_int32(uniqnum)
    self._issetunique(self._fd, uniqnum)

  @ISAMfunc(c_int, POINTER(ISAMkeydesc), c_int, c_char_p, c_int)
  def isstart(self, kdesc, mode, recbuff, keylen=0):
    'Start using a different index'
    if self._fd is None:
      raise IsamNotOpen
    if not isinstance(mode, ReadMode):
      raise ValueError('Must provide a ReadMode value')
    elif mode in (ReadMode.ISNEXT, ReadMode.ISPREV, ReadMode.ISCURR):
      raise ValueError('Cannot request a directional start')
    self._isstart(self._fd, kdesc, keylen, recbuff.raw, mode.value)

  @ISAMfunc(c_int, restype=int)
  def isuniqueid(self):
    'Return the unique id for the table'
    if self._fd is None:
      raise IsamNotOpen
    if self._fdmode is OpenMode.ISINPUT:
      raise IsamReadOnly
    ptr = c_int32(0)
    self._isuniqueid(self._fd, ptr)
    return ptr.value

  @ISAMfunc(c_int)
  def isunlock(self):
    'Unlock the current table'
    if self._fd is None:
      raise IsamNotOpen
    self._isunlock(self._fd)

  @ISAMfunc(c_int, c_char_p)
  def iswrcurr(self, recbuff):
    'Write the current record'
    if self._fd is None:
      raise IsamNotOpen
    return self._iswrcurr(self._fd, recbuff)

  @ISAMfunc(c_int, c_char_p)
  def iswrite(self, recbuff):
    'Write a new record'
    if self._fd is None:
      raise IsamNotOpen
    return self._iswrite(self._fd, recbuff)

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
    kdesc = ISAMkeydesc()
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
      raise ValueError(f'Maximum length of an index cannot exceed {MaxKeyLength} bytes')
    if optimize and kdesc_leng > 8:
      kdesc.flags += IndexFlags.ALL_COMPRESS
    return kdesc
