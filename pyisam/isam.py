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

import collections
from ctypes import c_short, c_char, c_char_p, c_long, c_int, c_void_p
from ctypes import Structure, POINTER, _SimpleCData, _CFuncPtr, cdll, Array
from ctypes import create_string_buffer, byref
import enum
import functools
import os
import struct
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
    if (self.flags & 1) ^ (other.flags & 1):
      return False
    if self.nparts != other.nparts:
      return False
    if self.leng != other.leng:
      return False
    for spart,opart in zip(self.part,other.part):
      if spart.start != opart.start:
        return False
      if spart.leng != opart.leng:
        return False
      if spart.type_ != opart.type_:
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
          real_func.argtypes = orig_args if len(orig_args) > 0 else None
        elif orig_args[0] is None:
          real_func.restype = real_func.argtypes = None
        else:
          real_func.argtypes = orig_args
        real_func.errcheck = self._chkerror
      return func(self, *args, **kwd)
    return functools.wraps(func)(wrapper)
  return call_wrapper

# Convert the given value to a bytes value
def to_bytes(value,default=None):
  if isinstance(value,str):
    value = bytes(value,'utf-8')
  elif value is None:
    value = b'' if default is None else to_bytes(default)
  else:
    raise ValueError('Value is not convertable to a bytes value')
  return value

# Convert the given value to a str value
def to_str(value,default=None):
  if isinstance(value,bytes):
    value = str(value,'utf-8')
  elif value is None:
    value = '' if default is None else to_str(default)
  else:
    raise ValueError('Value is not convertable to a str value')
  return value

# Define the exceptions raised by this module
class IsamException(Exception):
  '''General Exception raised for ISAM'''
class IsamNotOpen(IsamException):
  '''Exception raised when ISAM table not open'''
class IsamRecMutable(IsamException):
  '''Exception when given a non mutable buffer'''
class IsamFuncFailed(IsamException):
  '''ISAM function failed'''
  def __init__(self,errno):
    self.errno = errno
class IsamNoRecord(IsamFuncFailed):
  '''No record found'''

_def_ISAMmode = 'ISINPUT'                       # Default mode for isopen/isbuild
_def_ISAMlock = 'ISMANULOCK'                    # Default locking for isopen/isbuild

# Define some enumeration classes for the types of ISAM read and audit modes
class ISAMreadmode(enum.IntEnum):
  ISFIRST = 0
  ISLAST  = 1
  ISNEXT  = 2
  ISPREV  = 3
  ISCURR  = 4
  ISEQUAL = 5
  ISGREAT = 6
  ISGTEQ  = 7
  
class ISAMauditmode(enum.IntEnum):
  AUDSETNAME = 0
  AUDGETNAME = 1
  AUDSTART   = 2
  AUDSTOP    = 3
  AUDINFO    = 4
  AUDRECVR   = 5
  
class ISAMobject:
  '''This provides the interface to the underlying ISAM libraries.
     The underlying ISAM routines are loaded on demand with a
     prefix of an underscore, so isopen becomes _isopen.
  '''
  # The _const_ dictionary initially consists of the ctypes type
  # which will be mapped to the correct variable when accessed.
  _const_ = {
    'iserrno'      : c_int,    'iserrio'      : c_int,
    'isrecnum'     : c_long,   'isreclen'     : c_int,
    'isversnumber' : c_char_p, 'iscopyright'  : c_char_p,
    'isserial'     : c_char_p, 'issingleuser' : c_int,
    'is_nerr'      : c_int,    'is_errlist'   : None
  }
  # The _vldopn_ dictionary maps the various defines into numeric
  # equivalent values.
  _vldopn_ = {
    'ISINPUT'    : 0x000, 'ISOUTPUT'   : 0x001, 'ISINOUT'    : 0x002,
    'ISTRANS'    : 0x004, 'ISNOLOG'    : 0x008, 'ISVARLEN'   : 0x010,
    'ISSPECAUTH' : 0x020, 'ISFIXLEN'   : 0x000, 'ISNOCKLOG'  : 0x080,
    'ISAUTOLOCK' : 0x200, 'ISMANULOCK' : 0x400, 'ISEXCLLOCK' : 0x800
  }
  _setopn_ = set(_vldopn_.values())
  # The _vldmde_ dictionary maps the various read defines into numeric
  # equivalent values, it also permits reverse mapping to occur.
  _vldmde_ = {
    'ISFIRST' : 0, 'ISLAST'  : 1, 'ISNEXT'  : 2, 'ISPREV' : 3,
    'ISCURR'  : 4, 'ISEQUAL' : 5, 'ISGREAT' : 6, 'ISGTEQ' : 7,
  }
  _setmde_ = set(_vldmde_.values())
  # The _vldaud_ dictionary maps the various audit options, it also
  # permits reverse mapping to occur.
  _vldaud_ = {
    'AUDSETNAME' : 0, 'AUDGETNAME' : 1, 'AUDSTART' : 2,
    'AUDSTOP'    : 3, 'AUDINFO'    : 4, 'AUDRECVR' : 5
  }
  _setaud_ = set(_vldaud_.values())
  # Load the ISAM library once and share it in other instances
  # To make use of vbisam instead link the libpyisam.so accordingly
  _lib_ = cdll.LoadLibrary('./libpyisam.so') # Make 32/64-bit correct
  def __init__(self):
    self._isfd_ = self._fdmode_ = None
  def __getattr__(self,name):
    '''Lookup the given define and return the numeric equivalent'''
    if not isinstance(name,str):
      raise AttributeError(name)
    elif name.startswith('_is'):
      return getattr(self._lib_,name[1:])
    elif name.startswith('_'):
      raise AttributeError(name)
    elif name == 'is_errlist':
      val = self._const_['is_errlist']
      if val is None:
        errlist = c_char_p * (self.is_nerr - 100)
        val = self._const_['is_errlist'] = errlist.in_dll(self._lib_, 'is_errlist')
      return val
    val = self._const_.get(name)
    if val is None:
      val = self._vldopn_.get(name, \
            self._vldmde_.get(name, \
            self._vldaud_.get(name)))
      if val is None:
        raise AttributeError(name)
    elif not isinstance(val,_SimpleCData):
      val = val.in_dll(self._lib_, name)
      self._const_[name] = val
    return val.value if hasattr(val,'value') else val
  def _modelock(self,mode,lock):
    '''Method to return the correct mode for isbuild/isopen'''
    if isinstance(mode,int):
      if (mode & 0x0ff) not in self._setopn_:
        raise ValueError('Invalid mode provided')
    elif isinstance(mode,str):
      if mode not in self._vldopn_ or (self._vldopn_[mode] & 0xf00) != 0:
        raise ValueError('Invalid mode provided')
      mode = self._vldopn_[mode] & 0x0ff
    if isinstance(lock,int):
      if (lock & 0xf00) not in self._setopn_:
        raise ValueError('Invalid lock provided')
    elif isinstance(lock,str):
      if lock not in self._vldopn_ or (self._vldopn_[lock] & 0x0ff) != 0:
        raise ValueError('Invalid lock provided')
      lock = self._vldopn_[lock] & 0xf00
    return mode | lock
  def _convread(self,mode,start=False):
    if isinstance(mode,int):
      if mode not in self._setmde_:
        raise ValueError('Invalid read option provided')
    elif isinstance(mode,str):
      if mode not in self._vldmde_:
        raise ValueError('Invalid read option provided')
      mode = self._vldmde_[mode]
    if start and 2 <= mode <= 4:
      raise ValueError('Invalid read option provided')
    return mode
  def _convaud(self,mode):
    if isinstance(mode,int):
      if mode not in self._setaud_:
        raise ValueError('Invalid audit option provided')
    elif isinstance(mode,str):
      if mode not in self._valaud_:
        raise ValueError('Invalid audit option provided')
      mode = self._vldaud_[mode]
    return mode
  def _chkerror(self,result=None,func=None,args=None):
    '''Perform checks on the running of the underlying ISAM function by
       checking the iserrno provided by the ISAM library, if FUNCRES is
       given return that on successfull completion of this method'''
    if result is None:
      if self.iserrno == 101:
        raise IsamNotOpen
      elif self.iserrno == 111:
        raise IsamNoRecord
      elif self.iserrno != 0:
        raise IsamFuncFailed(self.iserrno)
    elif result < 0:
      if self.iserrno == 101:
        raise IsamNotOpen
      elif self.iserrno == 111:
        raise IsamNoRecord
      raise IsamFuncFailed(self.iserrno)
    return args
  def isamerror(self, errno=None):
    '''Return the error message related to the error number given'''
    if errno is None:
      errno = self.iserrno
    if 100 <= errno < self.is_nerr:
      return to_str(self.is_errlist[errno - 100])
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
    if mode == 'AUDSETNAME':
      return self._isaudit(self,_isfd_,audname,0)
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
  @ISAMfunc()
  def isbegin(self):
    '''Begin a transaction'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isbegin(self._isfd_)
  @ISAMfunc(c_char_p,c_int,POINTER(keydesc))
  def isbuild(self,tabname):
    '''Build a new table in exclusive mode'''
    mode = self._modelock('ISINOUT','ISEXCLMODE')
    self._isbuild(to_bytes(tabname),mode)
  @ISAMfunc()
  def iscleanup(self):
    '''Cleanup the ISAM library'''
    self._iscleanup()
  @ISAMfunc(c_int)
  def isclose(self):
    '''Close an open ISAM table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isclose(self._isfd_)
    self._isfd_ = self._fdmode_ = None
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
    '''Return whether GLS is in use with tabname'''
    return self._isglsversion(to_bytes(tabname))
  def isindexinfo(self,keynum):
    '''Backwards compatible method for version of ISAM < 7.26'''
    if keynum < 0:
      raise ValueError('Index must be a positive number or 0 for dictinfo')
    elif keynum > 0:
      return self.iskeyinfo(keynum)
    else:
      return self.isdictinfo()
  @ISAMfunc(c_int,POINTER(keydesc),c_int)
  def iskeyinfo(self,keynum):
    '''Return the keydesc for the specified key'''
    if self._isfd_ is None: raise IsamNotOpen
    ptr = keydesc()
    self._iskeyinfo(self._isfd_,ptr,keynum)
    return ptr
  @ISAMfunc(None)
  def islangchk(self):
    '''Switch on language checks'''
    self._islangchk()
  @ISAMfunc(c_char_p,restype=c_char_p)
  def islanginfo(self,tabname):
    '''Return the collation sequence if any in use'''
    res = to_str(self._islanginfo(to_bytes(tabname)))
    return res
  @ISAMfunc(c_int)
  def islock(self):
    '''Lock the entire table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._islock(self._isfd_)
  @ISAMfunc()
  def islogclose(self):
    '''Close the transaction logfile'''
    self._islogclose()
  @ISAMfunc(c_char_p)
  def islogopen(self,logname):
    '''Open a transaction logfile'''
    self._islogopen(logname)
  @ISAMfunc(c_char_p)
  def isnlsversion(self,tabname):
    self._isnlsversion(to_bytes(tabname))
  @ISAMfunc(None,restype=None)
  def isnolangchk(self):
    '''Switch off language checks'''
    self._isnolangchk()
  @ISAMfunc(c_char_p,c_int)
  def isopen(self,tabname,mode='ISINPUT',lock='ISMANULOCK'):
    '''Open an ISAM table'''
    self._fdmode_ = self._modelock(mode,lock)
    self._isfd_ = self._isopen(to_bytes(tabname),self._fdmode_)
  @ISAMfunc(c_int,c_char_p,c_int)
  def isread(self,recbuff,mode='ISNEXT'):
    '''Read a record from an open ISAM table'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    mode = self._convread(mode)
    self._isread(self._isfd_,recbuff,mode)
  @ISAMfunc()
  def isrecover(self):
    '''Recover a transaction'''
    self._isrecover()
  @ISAMfunc(c_int)
  def isrelease(self):
    '''Release locks on table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isrelease(self._isfd_)
  @ISAMfunc(c_char_p,c_char_p)
  def isrename(self,oldname,newname):
    '''Rename an ISAM table'''
    self._isrename(to_bytes(oldname),to_bytes(newname))
  @ISAMfunc(c_int,c_char_p)
  def isrewcurr(self,recbuff):
    '''Rewrite the current record on the table'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    self._isrewcurr(self._isfd_,recbuff)
  @ISAMfunc(c_int,c_long,c_char_p)
  def isrewrec(self,recnum,recbuff):
    '''Rewrite the specified record'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    self._isrewrec(self._isfd_,recnum,recbuff)
  @ISAMfunc(c_int,c_char_p)
  def isrewrite(self,recbuff):
    '''Rewrite the record on the table'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    self._isrewrite(self._isfd_,recbuff)
  @ISAMfunc()
  def isrollback(self):
    '''Rollback the current transaction'''
    self._isrollback()
  @ISAMfunc(c_int,c_long)
  def issetunique(self,uniqnum):
    '''Set the unique number'''
    if self._isfd_ is None: raise IsamNotOpen
    #if self._fdmode_ & 3 == 0: raise IsamNotOpen
    if not isinstance(uniqnum,c_long):
      uniqnum = c_long(uniqnum)
    self._issetunique(self._isfd_,uniqnum)
  @ISAMfunc(c_int,POINTER(keydesc),c_int,c_char_p,c_int)
  def isstart(self,kdesc,mode,recbuff=None,keylen=0):
    '''Start using a different index'''
    if self._isfd_ is None: raise IsamNotOpen
    mode = self._convread(mode,start=True)
    if recbuff is None: recbuff = self._row_.record
    self._isstart(self._isfd_,kdesc,keylen,recbuff,mode)
  @ISAMfunc(c_int,POINTER(c_long))
  def isuniqueid(self):
    '''Return the unique id for the table'''
    if self._isfd_ is None: raise IsamNotOpen
    if self._fdmode_ & 3 == 0: raise IsamNotOpen
    ptr = c_long(0)
    self._isuniqueid(self._isfd_,ptr)
    return ptr.value
  @ISAMfunc(c_int)
  def isunlock(self):
    '''Unlock the current table'''
    if self._isfd_ is None: raise IsamNotOpen
    self._isunlock(self._isfd_)
  @ISAMfunc(c_int,c_char_p)
  def iswrcurr(self,recbuff):
    '''Write the current record'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    return self._iswrcurr(self._isfd_,recbuff)
  @ISAMfunc(c_int,c_char_p)
  def iswrite(self,recbuff):
    '''Write a new record'''
    if self._isfd_ is None: raise IsamNotOpen
    if not isinstance(recbuff,Array): raise IsamRecMutable
    return self._iswrite(self._isfd_,recbuff)

"""The following is an attempt to make use of the standard functools features more...  
@functools.lru_cache
def StructDict(name,_struct):
  if isinstance(name,tuple):
    name,_struct = name
  return struct.Struct(_struct or name)
"""

# Provides a cache of the struct.Struct objects used to reduce need for multiple instances
class StructDict(dict):
  def __init__(self, **init):
    for item in list(init.items()):
      self.add(item)
  def add(self, name, _struct=None):
    if isinstance(name,tuple):
      name,_struct = name
    try:
      _ns = self[name]
    except KeyError:
      _ns = self[name] = struct.Struct(_struct or name)
    return _ns
  def __call__(self, name, _struct=None):
    return self.add(name, _struct)
_StructDict = StructDict()

"""Disabled until can determine how to use correctly...
class ColumnType(enum.Enum):
  '''Provide an enumeration of the valid CISAM column types'''
  char = 0
  short = 1
  long = 2
  double = 3
  float = 4

if __name__ == '__main__':
  print(ColumnType[2])
..."""

class ISAMcolumn:
  '''Base class providing the shared functionality for columns'''
  __slots__ = ('_name_','_offset_','_size_','_struct_','_type_')
  def __init__(self, name):
    self._name_ = name
    self._offset_ = -1          # Offset into record buffer
    if not hasattr(self, '_type_'):
      raise ValueError('No type provided for column')
    if not hasattr(self, '_struct_'):
      raise ValueError('No structure object provided for column')
    if not hasattr(self, '_size_'):
      if isinstance(self._struct_, struct.Struct):
        self._size_ = self._struct_.size
      else:
        raise ValueError('Must provide a size for column {}'.format(self._name_))
  def _unpack(self, recbuff):
    return self._struct_.unpack_from(recbuff, self._offset_)[0]
  def _pack(self, recbuff, value):
    if value is None and hasattr(self, '_convnull'):
      value = self._convnull()
    if isinstance(value,str):
      value = to_bytes(value).replace(b'\x00',b' ')
    self._struct_.pack_into(recbuff, self._offset_, value)
  def __str__(self):
    return '{0._name_} = ({0._offset_}, {0._size_}, {0._type_})'.format(self)

class CharColumn(ISAMcolumn):
  __slots__ = ('_appcode_',)
  _type_ = 0
  _size_ = 1
  _struct_ = _StructDict('c')
  def __init__(self, name, appcode=None):
    super().__init__(name)
    if appcode: self._appcode_ = appcode
  def _convnull(self):
    return ' '
  def _unpack(self, recbuff):
    return to_str(super()._unpack(recbuff).replace(b'\x00',b' ')).rstrip()

class TextColumn(ISAMcolumn):
  __slots__ = []
  _type_ = 0
  def __init__(self, name, length):
    if length < 0: raise ValueError("Must provide a positive length")
    self._size_ = length
    self._struct_ = _StructDict('%ds' % self._size_)
    super().__init__(name)
  def _pack(self, recbuff, value):
    if value is None:
      value = b' ' * self._size_
    else:
      value = to_bytes(value)
      if self._size_ < len(value):
        value = value[self._size_]
      elif len(value) < self._size_:
        value += b' ' * (self._size_ - len(value))
    super()._pack(recbuff, value.replace(b'\x00',b' '))
  def _unpack(self, recbuff):
    return to_str(super()._unpack(recbuff).replace(b'\x00',b' ')).rstrip()

class ShortColumn(ISAMcolumn):
  __slots__ = []
  _type_ = 1
  _struct_ = _StructDict('h', '>h')
  def _convnull(self):
    return 0

class LongColumn(ISAMcolumn):
  __slots__ = []
  _type_ = 2
  _struct_ = _StructDict('l', '>l')
  def _convnull(self):
    return 0

class FloatColumn(ISAMcolumn):
  __slots__ = []
  _type_ = 4
  _struct_ = _StructDict('f', '=f')
  def _convnull(self):
    return 0.0

class DoubleColumn(ISAMcolumn):
  __slots__ = []
  _type_ = 3
  _struct_ = _StructDict('d', '=d')
  def _convnull(self):
    return 0.0

class ISAMrecord(dict):
  '''Class providing access to the current record providing access to the columns
     as both attributes as well as dictionary access.'''
  def __init__(self, tabobj, idname=None, **kwd):
    if not isinstance(tabobj, ISAMtable):
      raise ValueError('Definition must be an instance of ISAMtable')
    self._tabobj_ = tabobj
    self._defn_ = defn = tabobj._defn_
    self._recbuff_ = create_string_buffer(defn._size_ + 1)
    colnames = [col._name_ for col in defn._columns_]
    fqtname = [defn._database_,defn._prefix_]
    if idname is not None:
      fqtname.append(idname)
    self._namedtuple_ = collections.namedtuple('_'.join(fqtname), colnames)
  @property
  def record(self):
    return self._recbuff_
  @property
  def rectuple(self):
    'Return an instance of the namedtuple for the current column values'
    col_value = [getattr(self, colinfo._name_) for colinfo in self._defn_._columns_]
    return self._namedtuple_._make(col_value)
  def fillindex(self, name, value):
    '''Fill the appropriate columns for the given index using the available values'''
    idxnum = self._defn_._keyname_.index(name)
    idxcol = self._defn_._indexes_[idxnum][2:]
    if isinstance(value,dict):
      for fld in idxcol:
        self[fld] = value.get(fld)
      return
    value = list(value)
    value += [None] * (len(idxcol) - len(value))
    for fld,val in zip(idxcol,value):
      self[fld] = val
  def __getattr__(self, name):
    if name in ('_columns_','_indexes_'):
      return getattr(self._defn_, name)
    elif name.startswith('_') and name.endswith('_'):
      raise AttributeError(name)
    else:
      return self._defn_._colinfo_[name]._unpack(self._recbuff_)
  def __setattr__(self, name, value):
    if name.startswith('_') and name.endswith('_'):
      object.__setattr__(self, name, value)
    elif name in self._defn_._colinfo_:
      self._defn_._colinfo_[name]._pack(self._recbuff_, value)
    else:
      self.fillindex(name,value)
  def __getitem__(self, name):
    if isinstance(name, (list, tuple)):
      return tuple(getattr(self, col) for col in name)
    else:
      return getattr(self, name)
  def __setitem__(self, name, value):
    if isinstance(name, tuple):
      if len(name) < len(value):
        raise ValueError('Must pass a value with the same number of columns')
      for n,v in zip(name, value):
        setattr(self, n, v)
    else:
      setattr(self, name, value)
  def __str__(self):
    return str(self.rectuple)

class ISAMtableDefn:
  '''Class providing the table definition for an instance of ISAMtable,
     this includes all the columns and the key definition which are then
     mapped to by the ISAMTable instance when the actual table is opened.
     This class calculates the struct() definition to retrieve the data
     from the record buffer without recourse to the ISAM ld*/st* routines
     which may be macros.

     The columns in the table are defined by the '_columns_' list which
     provides instances of ISAMcolumn giving the length and type of
     each column which are used to generate the associated mapping used
     to retreive the individual values from the record buffer.

     The indexes on the table are defined by the '_indexes_' list which
     are tuples of the form (NAME, DUPS[, {COL|(COL, LEN)|(COL, OFF, LEN)}]
     [, DESC]) where NAME is the name of the index, DUPS is either True
     if the index permits duplicates or False if the index is unique, the 
     optional list of columns defines the structure of the index, COL is
     one of the defined fields and is used in its entirety in the absense
     of OFF and LEN, OFF is an optional offset into the columns' value,
     LEN is the amount of the columns' value used in the index. DESC if
     present and True indicates that the index is meant to be used in 
     reverse order.
  '''
  __slots__ = ('_size_','_colinfo_','_colname_','_keyinfo_','_database_',
               '_prefix_')
  def __init__(self, **kwds):
    self._size_ = 0
    self._colinfo_ = {}
    self._colname_ = []
    self._keyinfo_ = {}
    if not hasattr(self,'_columns_') or not hasattr(self,'_indexes_'):
      raise IsamException('Must provide an attribute defining the columns and indexes')
    for col in self._columns_:
      self._add_column(col)
    for idx in self._indexes_:
      self._add_index(idx)
    if not hasattr(self, '_database_'):
      self._database_ = kwds['_database_']
    if not hasattr(self, '_prefix_'):
      self._prefix_ = kwds['_prefix_']
  def match_column(self, keypart):
    '''Attempt to match the given keypart into the equivalent column, returns
       the column information if found'''
    for colinfo in self._columns_:
      if colinfo._offset_ == keypart.start:
        if colinfo._type_ == keypart.type_:
          if colinfo._size_ == keypart.leng:
            return colinfo._name_
          elif colinfo._size_ > keypart.leng:
            return (colinfo._name_, keypart.leng)
          else:
            raise ValueError('Key part is larger than corresponding column: {}'.format(colinfo))
        else:
          raise ValueError('Key part is for different type of data: {}'.format(colinfo))
      elif colinfo._offset_ < keypart.start < colinfo._offset_ + colinfo._size_:
        if keypart.start + keypart.leng <= colinfo._start_ + colinfo._size_:
          return (colinfo._name_, colinfo._offset_ - keypart.start, keypart.leng)
  def match_index(self, kdesc):
    '''Attempt to match the given keydesc to a known index'''
    for idxname,idxdesc in self._keyinfo_.items():
      if idxdesc[0] == kdesc:
        return idxname
  def _add_column(self, col):
    if col._name_ in self._colinfo_:
      raise ValueError('Column of the same name ({}) already present in table'.format(col._name_))
    col._offset_ = self._size_
    self._size_ += col._size_
    self._colinfo_[col._name_] = col
    self._colname_.append(col._name_)
  def _add_index(self, idx):
    if idx[0] in self._keyinfo_:
      raise ValueError('Index of the same name ({}) already present in table'.format(idx[0]))
    def idxpart(idxcol):
      'Prepare a part of an index'
      colinfo = self._colinfo_[idxcol]
      kpart = keypart()
      if isinstance(idxcol,(tuple,list)):
        # Use part of column in index
        if idxcol[-1] > colinfo._size_:
          raise ValueError('Index part is larger than column')
        if len(idxcol) > 2:
          if idxcol[-2] + idxcol[-1] > colinfo._size_:
            raise ValueError('Index part too long for defintion')
          kpart.start = colinfo._offset_ + idxcol[1]
        else:
          kpart.start = colinfo._offset_
        kpart.leng  = idxcol[-1]
        kpart.type_ = colinfo._type_
        kdesc.leng += idxcol[-1]
      else:
        # Use whole column in index
        kpart.start = colinfo._offset_
        kpart.leng  = colinfo._size_
        kpart.type_ = colinfo._type_
        kdesc.leng += colinfo._size_
      return kpart
    kdesc = keydesc()
    kdesc.flags  = 1 if idx[1] else 0
    kdesc.nparts = max(1, len(idx) - 2)
    kdesc.leng   = 0
    if kdesc.nparts == 1:
      kdesc.part[0] = idxpart(idx[0])
    else:
      for curidx, idxcol in enumerate(idx[2:]):
        kdesc.part[curidx] = idxpart(idxcol)
    if kdesc.leng > 8:
      kdesc.flags |= 14  # COMPRESS within isam.h (octal 016)
    self._keyinfo_[idx[0]] = (kdesc,idx)

class ISAMrowset:
  '''Class that provides a result set that is sorted by the rowid'''
  def __init__(self, tabobj, size=None, cursor=None, descend=None):
    self._tabobj_ = tabobj
    self._rows_ = collections.OrderedDict()
    self._maxlen_ = size
    self._cursor_ = cursor
    self._descend_ = descend
    self._current_ = None
  def _first_(self):
    'Return the first row in the result set'
  def _next_(self):
    'Return the next row in the result set, extending if possible'
  def _prev_(self):
    'Return the prev row in the result set' 
  def _last_(self):
    'Return the last row in the result set'
  def _curr_(self):
    'Return the current row in the result set'
  def _rowid_(self):
    'Return the rowid for the current row in the result set'
  def _clear_(self):
    'Clear all rows from the result set'
  def _add_(self, recbuff, rowid=None):
    'Add the given record to the end of the current result set'
  def _del_(self, recbuff, rowid=None):
    'Remove the given record from the current result set'

class ISAMcursor:
  '''Class that provides a cursor like iterator for the underlying ISAM
     table, which provides a scrollable pointer into the current result
     set'''
  def __init__(self, tabobj, index, _descend=False, _cachesize=None, *colarg, **colkey):
    self._tabobj_ = tabobj
    if index not in tabobj._defn_._indexes_:
      raise ValueError('Non-existant index referenced')
    self._rowset_ = ISAMrowset(tabobj, cachesize=_cachesize, cursor=self, descend=_descend)
    # TODO Complete contents
  def __next__(self):
    # TODO Complete contents
    pass
  next = __next__

class ISAMtable(ISAMobject):
  '''Class that represents a table within ISAM, this holds all the column
     and index information and also provides the get/set functionality
     with dictionary like access. It will match up the index information
     with the information stored within the table file itself.'''
  def __init__(self, tabdefn, tabname=None, tabpath=None, **kwds):
    super(ISAMtable,self).__init__()
    self._defn_ = tabdefn if isinstance(tabdefn,ISAMtableDefn) else tabdefn()
    if tabname is None:
      tabname = self._defn_.__class__.__name__.lower()
      if tabname.endswith('defn'):
        tabname = tabname[:-4]
    self._name_ = tabname
    self._path_ = tabpath if tabpath else '.'
    self._row_  = ISAMrecord(self,kwds.get('idname'))
    self._mode_ = getattr(self, kwds.get('mode', 'ISINPUT'))
    self._lock_ = getattr(self, kwds.get('lock', 'ISMANULOCK'))
    self._curidx_ = None      # Current index being used
    pass # TODO Complete contents
  def __getitem__(self,name):
    'Retrieve the specified column having converted the value using O[name]'
    return self._row_[name]
  def __setitem__(self,name,value):
    'Set the specified column to the converted value using O[name] = value'
    self._row_[name] = value
  def __str__(self):
    return str(self._row_)
  def addindex(self, name, unique=False, *cols):
    pass
  @property
  def datbuff(self):
    'Return the default record buffer'
    return self._row_
  def open(self, mode=None, lock=None):
    'Open the underlying ISAM table with the specified modes'
    if self._isfd_ is None:
      if mode is None:
        mode = getattr(self,'_mode_','ISINPUT')
      if lock is None:
        lock = getattr(self,'_lock_','ISMANULOCK')
      self.isopen(os.path.join(self._path_,self._name_),mode,lock)
      self._prepare()
  def close(self):
    'Close the underlying ISAM table and cleanup resources'
    if self._isfd_ is not None:
      self.isclose()
  def read(self, mode='ISNEXT', *args, **kwd):
    self.open()
    if mode in ('ISNEXT','ISCURR','ISPREV'):
      self.isread(self._row_.record, mode)
      return self._row_.rectuple
    idxinfo = self._defn_._keyinfo_[args[0]]
    curarg = 1
    if len(idxinfo[1]) < 3:
      self[args[0]] = args[1]
    else:
      for col in idxinfo[1][2:]:
        if col in kwd:
          self[col] = kwd.get(col)
        elif curarg < len(args):
          self[col] = args[curarg]
          curarg += 1
        else:
          self[col] = None
    if self._curidx_ != args[0]:
      self.isstart(idxinfo[0],mode)
      self._curidx_ = args[0]
      mode = 'ISNEXT'
    self.isread(self._row_.record,mode)
    return self._row_.rectuple
  def _prepare(self):
    'Prepare the object for the actual table file provided'
    idxmap = [None] * len(self._defn_._indexes_)
    dinfo = self.isdictinfo()
    for curkey in range(dinfo.nkeys):
      kdesc = self.iskeyinfo(curkey+1)
      idxname = self._defn_.match_index(kdesc)
      if idxname is not None:
        idxmap[curkey] = idxname
    self._idxmap_ = idxmap

# Examples of the usage of ISAMtable and ISAMtableDefn
class DECOMPdefn(ISAMtableDefn):
  _columns_ = (TextColumn('comp',9),
               CharColumn('comptyp'),
               TextColumn('sys',9),
               TextColumn('prefix',5),
               TextColumn('user',4),
               TextColumn('database',6),
               TextColumn('release',5),
               LongColumn('timeup'),
               LongColumn('specup'))
  _indexes_ = (('comp',False),
               ('prefix',True),
               ('typkey',False,'comptyp','comp'),
               ('syskey',False,'sys','comptyp','comp'))
  _prefix_ = 'dec'
  _database_ = 'utool'

class DEITEMdefn(ISAMtableDefn):
  _columns_ = (TextColumn('item',9),
               ShortColumn('seq'),
               CharColumn('comptyp'),
               TextColumn('comp',9),
               CharColumn('spec'))
  _indexes_ = (('key',False,'item','seq','comptyp','comp'),
               ('usekey',False,'comp','item'),
               ('compkey',False,'item','comptyp','comp','seq'))
  _prefix_ = 'deit'
  _database_ = 'utool'

class DEFILEdefn(ISAMtableDefn):
  _columns_ = (TextColumn('filename',9),
               ShortColumn('seq'),
               TextColumn('field',9),
               TextColumn('refptr',9),
               CharColumn('type'),
               ShortColumn('size'),
               CharColumn('keytype'),
               ShortColumn('vseq'),
               ShortColumn('stype'),
               CharColumn('scode'),
               TextColumn('fgroup',10),
               CharColumn('idxflag'))
  _indexes_ = (('key',False,'filename','seq'),
               ('unikey',False,'filename','field'),
               ('vkey',False,'filename','vseq','field'))
  _prefix_ = 'def'
  _database_ = 'utool'

""" Example definition using clearer index classes
class DEFILEdefn2(ISAMtableDefn):
  _columns_ = (TextColumn('filename',9),
               ShortColumn('seq'),
               TextColumn('field',9),
               TextColumn('refptr',9),
               CharColumn('type'),
               ShortColumn('size'),
               CharColumn('keytype'),
               ShortColumn('vseq'),
               ShortColumn('stype'),
               CharColumn('scode'),
               TextColumn('fgroup',10),
               CharColumn('idxflag'))
  _indexes_ = (DupsIndex('key','filename','seq'),
               DupsIndex('unikey','filename','field'),
               DupsIndex('vkey','filename','vseq','field'))
  _prefix_ = 'def'
  _database_ = 'utool'

Other index classes are: DupsIndex, UniqIndex, RevDupsIndex, RevUniqIndex.
"""

class DEKEYSdefn(ISAMtableDefn):
  _columns_ = (TextColumn('filename',9),
               TextColumn('keyfield',9),
               TextColumn('key1',9),
               TextColumn('key2',9),
               TextColumn('key3',9),
               TextColumn('key4',9),
               TextColumn('key5',9),
               TextColumn('key6',9),
               TextColumn('key7',9),
               TextColumn('key8',9))
  _indexes_ = (('key',False,'filename','keyfield'),)
  _prefix_ = 'dek'
  _database_ = 'utool'

class DEBFILEdefn(ISAMtableDefn):
  _columns_ = (CharColumn('source'),
               LongColumn('license'),
               TextColumn('filename',9),
               TextColumn('dataset',4),
               TextColumn('field',9),
               CharColumn('action'),
               ShortColumn('stype'),
               ShortColumn('size'),
               CharColumn('scode'),
               CharColumn('keytype'),
               ShortColumn('vseq'),
               ShortColumn('seq'),
               TextColumn('group',10),
               TextColumn('refptr',9))
  _indexes_ = (('key',False,'source','license','filename','dataset','field'),
               ('fkey',True,'filename','dataset','field'))
  _prefix_ = 'debf'
  _database_ = 'utool'

class DEBKEYSdefn(ISAMtableDefn):
  _columns_ = (CharColumn('source'),
               LongColumn('license'),
               TextColumn('dataset',4),
               TextColumn('filename',9),
               TextColumn('keyfield',9),
               TextColumn('key1',9),
               TextColumn('key2',9),
               TextColumn('key3',9),
               TextColumn('key4',9),
               TextColumn('key5',9),
               TextColumn('key6',9),
               TextColumn('key7',9),
               TextColumn('key8',9))
  _indexes_ = (('key',False,'source','license','dataset','filename','keyfield'),)
  _prefix_ = 'debk'
  _database_ = 'utool'

class DEBCOMPdefn(ISAMtableDefn):
  _columns_ = (TextColumn('filename',9),
               CharColumn('stxbuild'),
               TextColumn('mask',5),
               TextColumn('dataset',5),
               TextColumn('location',128))
  _indexes_ = (('filename',False),
               ('mask',True))
  _prefix_ = 'debc'
  _database_ = 'utool'

if __name__ == '__main1__':
  DEFILE = ISAMtable(DEFILEdefn)
  tabname = 'debfile'
  def_rec = DEFILE.read('ISGREAT','key',tabname)
  while def_rec.filename == tabname:
    print(def_rec)
    def_rec = DEFILE.read()

if __name__ == '__main__':
  isamobj = ISAMobject()
  for errno in range(100,isamobj.is_nerr):
    print(isamobj.isamerror(errno))
