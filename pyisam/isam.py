#!/usr/bin/python3
'''
This module provides a ctypes based interface to the IBM C-ISAM library without
requiring an explicit extension to be compiled. The only requirement is that
the existing libifisam/libifisamx libraries are combined such that the SONAME
can be verified.

This variant makes use of a decorator that is used to lookup the underlying
ISAM function and then bind the arguments and return type appropriately without
causing issues with the original method (the underlying library functions are
added prefixed with an underscore, isopen -> _isopen).
'''

from ctypes import c_short, c_char, c_char_p, c_long, c_int, c_void_p
from ctypes import Structure, POINTER, _SimpleCData, _CFuncPtr, cdll, Array
from ctypes import create_string_buffer, byref, pointer
import enum
import functools
import os
import struct
from .utils import ISAM_bytes,ISAM_str
##import platform

### Determine whether to use the 32-bit or 64-bit variants of the underlying libraries
##if platform.architecture()[0].startswith('32bit'):
##  isam_lib = 'lib32/libpyisam.so'
##else:
##  isam_lib = 'lib64/libpyisam.so'

# Define the structures that are usually defined by the isam.h and decimal.h
# header files.
class decimal(Structure):
  _fields_ = [('exp', c_short),
              ('pos', c_short),
              ('ndgts', c_short),
              ('dgts', c_char * 16)]

class keypart(Structure):
  _fields_ = [('start', c_short),
              ('leng', c_short),
              ('type_', c_short)]
  def __str__(self):
    return '({0.start}, {0.leng}, {0.type_})'.format(self)

class keydesc(Structure):
  _fields_ = [('flags', c_short),
              ('nparts', c_short),
              ('part', keypart * 8),
              ('leng', c_short),
              ('rootnode', c_long)]
  def __eq__(self,other):
    '''Compare if the two keydesc structures are the same'''
    if (self.flags & 1) ^ (other.flags & 1) or self.nparts != other.nparts or self.leng != other.leng:
      return False
    for spart,opart in zip(self.part,other.part):
      if spart.start != opart.start or spart.leng != opart.leng or spart.type_ != opart.type_:
        return False
    return True
  def __str__(self):
    res = ['({0.flags}, {0.nparts}, ['.format(self)]
    res.append(', '.join([str(self.part[cpart]) for cpart in range(self.nparts)]))
    res.append('], {0.leng})'.format(self))
    return ''.join(res)

class dictinfo(Structure):
  _fields_ = [('nkeys', c_short),
              ('recsize', c_short),
              ('idxsize', c_short),
              ('nrecords', c_long)]
  def __str__(self):
    return 'NKEY: {0.nkeys}; RECSIZE: {0.recsize}; ' \
           'IDXSIZE: {0.idxsize}; NREC: {0.nrecords}'.format(self)

# Decorator function that wraps the methods within the ISAMobject that
# will perform the necessary actions on the first invocation of the ISAM
# function before calling the method itself.
def ISAMfunc(*orig_args,**orig_kwd):
  def call_wrapper(func):
    def wrapper(self,*args,**kwd):
      real_func = getattr(self._lib_, func.__name__)
      if not hasattr(real_func, '_seen'):
        real_func._seen = True
        if 'restype' in orig_kwd:
          real_func.restype = orig_kwd['restype']
          if 'argtype' in orig_kwd:
            real_func.argtypes = orig_kwd['argtype']
          else:
            real_func.argtypes = orig_args if len(orig_args) > 0 else None
          real_func.errcheck = self._chkerror
        elif 'argtype' in orig_kwd:
          real_func.argtypes = orig_kwd['argtype']
          real_func.errcheck = self._chkerror
        elif len(orig_args) == 0:
          real_func.restype = real_func.argtypes = None
        else:
          real_func.argtypes = None if orig_args[0] is None else orig_args 
          real_func.errcheck = self._chkerror
      return func(self, *args, **kwd)
    return functools.wraps(func)(wrapper)
  return call_wrapper

# Define the exceptions raised by this module
class IsamException(Exception):
  'General Exception raised for ISAM'
class IsamNotOpen(IsamException):
  'Exception raised when ISAM table not open'
class IsamNotWritable(IsamNotOpen):
  'Exception raised when ISAM table not opened with writable mode'
class IsamRecMutable(IsamException):
  'Exception when given a non mutable buffer'
class IsamFuncFailed(IsamException):
  'Exception raised when an ISAM function is not found in the library'
  def __init__(self,errno):
    self.errno = errno
class IsamNoRecord(IsamFuncFailed):
  'Exception raised when no record was found'

# The OpenMode and LockMode enums provide the available modes used
# when using the isbuild or isopen methods.
class OpenMode(enum.Enum):
  ISINPUT    = 0x000
  ISOUTPUT   = 0x001
  ISINOUT    = 0x002
  ISTRANS    = 0x004
  ISNOLOG    = 0x008
  ISVARLEN   = 0x010
  ISSPECAUTH = 0x020
  ISFIXLEN   = 0x000
  ISNOCKLOG  = 0x080
class LockMode(enum.IntEnum):
  ISAUTOLOCK = 0x200
  ISMANULOCK = 0x400
  ISEXCLLOCK = 0x800

# The ReadMode enum provides the available modes when using the isread
# method.
class ReadMode(enum.IntEnum):
  ISFIRST    = 0
  ISLAST     = 1
  ISNEXT     = 2
  ISPREV     = 3
  ISCURR     = 4
  ISEQUAL    = 5
  ISGREAT    = 6
  ISGTEQ     = 7
  ISLOCK     = 0x100
  ISSKIPLOCK = 0x200
  ISWAIT     = 0x400
  ISLCKW     = 0x500
  ISKEEPLOCK = 0x800

# The StartMode enum provides the available modes when using the isstart
# method.
class StartMode(enum.IntEnum):
  ISFIRST    = 0
  ISLAST     = 1
  ISEQUAL    = 5
  ISGREAT    = 6
  ISGTEQ     = 7
  ISLOCK     = 0x100
  ISSKIPLOCK = 0x200
  ISWAIT     = 0x400
  ISLCKW     = 0x500
  ISKEEPLOCK = 0x800

class ISAMobject:
  '''This provides the interface to the underlying ISAM libraries.
     The underlying ISAM routines are loaded on demand with a
     prefix of an underscore, so isopen becomes _isopen.
  '''
  __slots__ = ('_isfd_','_fdmode_','_fdlock_')
  # The _const_ dictionary initially consists of the ctypes type
  # which will be mapped to the correct variable when accessed.
  _const_ = {
    'iserrno'      : c_int,    'iserrio'      : c_int,
    'isrecnum'     : c_long,   'isreclen'     : c_int,
    'isversnumber' : c_char_p, 'iscopyright'  : c_char_p,
    'isserial'     : c_char_p, 'issingleuser' : c_int,
    'is_nerr'      : c_int,    'is_errlist'   : None
  }
  def_openmode = OpenMode.ISINPUT
  def_lockmode = LockMode.ISMANULOCK
  # Load the ISAM library once and share it in other instances
  # To make use of vbisam instead link the libpyisam.so accordingly
  _lib_ = cdll.LoadLibrary(os.path.join(os.path.dirname(__file__),'libpyisam.so')) # FIXME: Make 32/64-bit correct
  def __init__(self):
    self._isfd_ = self._fdmode_ = self._fdlock_ = None
  def __getattr__(self,name):
    '''Lookup the ISAM function and return the entry point into the library
       or define and return the numeric equivalent'''
    if not isinstance(name,str):
      raise AttributeError(name)
    elif name.startswith('_is'):
      return getattr(self._lib_, name[1:])
    elif name.startswith('_'):
      raise AttributeError(name)
    elif name == 'is_errlist':
      val = self._const_['is_errlist']
      if val is None:
        errlist = c_char_p * (self.is_nerr - 100)
        val = self._const_['is_errlist'] = errlist.in_dll(self._lib_, 'is_errlist')
    else:
      val = self._const_.get(name)
      if val is None:
        raise AttributeError(name)
      elif not isinstance(val, _SimpleCData) and hasattr(val, 'in_dll'):
        val = self._const_[name] = val.in_dll(self._lib_, name)
    return val.value if hasattr(val,'value') else val
  def _chkerror(self,result=None,func=None,args=None):
    '''Perform checks on the running of the underlying ISAM function by
       checking the iserrno provided by the ISAM library, if ARGS is
       given return that on successfull completion of this method'''
    if result is None or result < 0:
      if self.iserrno == 101:
        raise IsamNotOpen
      elif self.iserrno == 111:
        raise IsamNoRecord
      elif result is not None and (result < 0 or self.iserrno != 0):
        raise IsamFuncFailed(self.iserrno)
    return args
  def strerror(self, errno=None):
    '''Return the error message related to the error number given'''
    if errno is None:
      errno = self.iserrno
    if 100 <= errno < self.is_nerr:
      return ISAM_str(self.is_errlist[errno - 100])
    else:
      return '{} is not a valid ISAM error number'.format(errno)
  @ISAMfunc(c_int,POINTER(keydesc))
  def isaddindex(self,kdesc):
    '''Add an index to an open ISAM table'''
    if self._isfd_ is None: raise IsamNotOpen
    return self._isaddindex(self._isfd_,kdesc)
  @ISAMfunc(c_int,c_char_p,c_int)
  def isaudit(self,mode,audname=None):
    '''Perform audit trail related processing'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, str):
      raise ValueError('Must provide a string value')
    if mode == 'AUDSETNAME':
      return self._isaudit(self,_isfd_,ISAM_bytes(audname),0)
    elif mode == 'AUDGETNAME':
      resbuff = create_string_buffer(256)
      self._isaudit(self._isfd_,resbuff,1)
      return resbuff.value()
    elif mode == 'AUDSTART':
      return self._isaudit(self._isfd_,b'',2)
    elif mode == 'AUDSTOP':
      return self._isaudit(self._isfd_,b'',3)
    elif mode == 'AUDINFO':
      resbuff = create_string_buffer(1)
      self._isaudit(self._isfd_,resbuff,4)
      return resbuff[0] != '\0'
    elif mode == 'AUDRECVR':
      pass
    else:
      raise ValueError('Unhandled audit mode specified')
  @ISAMfunc()
  def isbegin(self):
    '''Begin a transaction'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isbegin(self._isfd_)
  @ISAMfunc(c_char_p,c_int,POINTER(keydesc))
  def isbuild(self,tabname,kdesc):
    '''Build a new table in exclusive mode'''
    if not isinstance(kdesc, keydesc):
      raise ValueError('Must provide the primary key description for table')
    self._fdmode_ = OpenMode.ISINOUT
    self._fdlock_ = LockMode.ISEXCLLOCK
    self._isfd_ = self._isbuild(ISAM_bytes(tabname), self._fdmode_.value|self._fdlock_.value, kdesc)
  @ISAMfunc()
  def iscleanup(self):
    '''Cleanup the ISAM library'''
    self._iscleanup()
  @ISAMfunc(c_int)
  def isclose(self):
    '''Close an open ISAM table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isclose(self._isfd_)
    self._isfd_ = self._fdmode_ = self._fdlock_ = None
  @ISAMfunc(c_int,POINTER(keydesc))
  def iscluster(self,kdesc):
    '''Create a clustered index'''
    if self._isfd_ is None: raise IsamNotOpen
    self._iscluster(self._isfd_,kdesc)
  @ISAMfunc()
  def iscommit(self):
    '''Commit the current transaction'''
    if self._isfd_ is None: raise IsamNotOpen
    self._iscommit(self._isfd_)
  @ISAMfunc(c_int)
  def isdelcurr(self):
    '''Delete the current record from the table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isdelcurr(self._isfd_)
  @ISAMfunc(c_int,c_char_p)
  def isdelete(self,keybuff):
    '''Remove a record by using its key'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(keybuff,Array): raise IsamRecMutable
    self._isdelete(self._isfd_,keybuff)
  @ISAMfunc(c_int,POINTER(keydesc))
  def isdelindex(self,kdesc):
    '''Remove the given index from the table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isdelindex(self._isfd_,kdesc)
  @ISAMfunc(c_int,POINTER(dictinfo))
  def isdictinfo(self):
    '''Return the dictionary information for table'''
    if self._isfd_ is None: raise IsamNotOpen
    ptr = dictinfo()
    self._isdictinfo(self._isfd_,ptr)
    return ptr
  @ISAMfunc(c_char_p)
  def iserase(self,tabname):
    '''Remove the table from the filesystem'''
    self._iserase(tabname)
  @ISAMfunc(c_int)
  def isflush(self):
    '''Flush the data out to the table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isflush(self._isfd_)
  @ISAMfunc(c_char_p)
  def isglsversion(self,tabname):
    'Return whether GLS is in use with tabname'
    return self._isglsversion(ISAM_bytes(tabname))
  def isindexinfo(self,keynum):
    'Backwards compatible method for version of ISAM < 7.26'
    if keynum < 0:
      raise ValueError('Index must be a positive number or 0 for dictinfo')
    elif keynum > 0:
      return self.iskeyinfo(keynum)
    else:
      return self.isdictinfo()
  @ISAMfunc(c_int,POINTER(keydesc),c_int)
  def iskeyinfo(self,keynum):
    'Return the keydesc for the specified key'
    if self._isfd_ is None: raise IsamNotOpen
    ptr = keydesc()
    self._iskeyinfo(self._isfd_,ptr,keynum)
    return ptr
  @ISAMfunc(None)
  def islangchk(self):
    'Switch on language checks'
    self._islangchk()
  @ISAMfunc(c_char_p,restype=c_char_p)
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
  @ISAMfunc(c_char_p,c_int)
  def isopen(self,tabname,mode=None,lock=None):
    'Open an ISAM table'
    if mode is None:
      mode = self.def_openmode
    if lock is None:
      lock = self.def_lockmode
    if not isinstance(mode, OpenMode) or not isinstance(lock, LockMode):
      raise ValueError('Must provide an OpenMode and/or LockMode values')    
    self._fdmode_ = mode
    self._fdlock_ = lock
    self._isfd_ = self._isopen(ISAM_bytes(tabname),self._fdmode_.value|self._fdlock_.value)
  @ISAMfunc(c_int,c_char_p,c_int)
  def isread(self,recbuff,mode=ReadMode.ISNEXT):
    'Read a record from an open ISAM table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff, Array): raise IsamRecMutable
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
  @ISAMfunc(c_char_p,c_char_p)
  def isrename(self,oldname,newname):
    'Rename an ISAM table'
    self._isrename(ISAM_bytes(oldname),ISAM_bytes(newname))
  @ISAMfunc(c_int,c_char_p)
  def isrewcurr(self,recbuff):
    'Rewrite the current record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    self._isrewcurr(self._isfd_,recbuff)
  @ISAMfunc(c_int,c_long,c_char_p)
  def isrewrec(self,recnum,recbuff):
    'Rewrite the specified record'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    self._isrewrec(self._isfd_,recnum,recbuff)
  @ISAMfunc(c_int,c_char_p)
  def isrewrite(self,recbuff):
    'Rewrite the record on the table'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    self._isrewrite(self._isfd_,recbuff)
  @ISAMfunc(None)
  def isrollback(self):
    'Rollback the current transaction'
    self._isrollback()
  @ISAMfunc(c_int,c_long)
  def issetunique(self,uniqnum):
    'Set the unique number'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    if not isinstance(uniqnum,c_long):
      uniqnum = c_long(uniqnum)
    self._issetunique(self._isfd_,uniqnum)
  @ISAMfunc(c_int,POINTER(keydesc),c_int,c_char_p,c_int)
  def isstart(self,kdesc,mode,recbuff=None,keylen=0):
    'Start using a different index'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(mode, StartMode):
      raise ValueError('Must provide a StartMode value')
    if recbuff is None: recbuff = self._row_.record
    self._isstart(self._isfd_,kdesc,keylen,recbuff,mode.value)
  @ISAMfunc(c_int,POINTER(c_long))
  def isuniqueid(self):
    'Return the unique id for the table'
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ is OpenMode.ISINPUT: raise IsamNotWritable
    ptr = c_long(0)
    self._isuniqueid(self._isfd_,ptr)
    return ptr.value
  @ISAMfunc(c_int)
  def isunlock(self):
    'Unlock the current table'
    if self._isfd_ is None: raise IsamNotOpen
    self._isunlock(self._isfd_)
  @ISAMfunc(c_int,c_char_p)
  def iswrcurr(self,recbuff):
    'Write the current record'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    return self._iswrcurr(self._isfd_,recbuff)
  @ISAMfunc(c_int,c_char_p)
  def iswrite(self,recbuff):
    'Write a new record'
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    return self._iswrite(self._isfd_,recbuff)
