'''
This module provides a structured way of accessing tables using ISAM by providing objects that
permit the table layout to be defined and referenced later.
'''

import collections
from ctypes import create_string_buffer
import os
import struct
from .isam import ReadMode,OpenMode,LockMode,ISAMobject,keydesc,keypart
from .utils import ISAM_bytes,ISAM_str

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
      value = ISAM_bytes(value).replace(b'\x00',b' ')
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
    if appcode: self._appcode_ = appcode   # FIXME: This was a special feature used by Strategix/OneOffice
  def _convnull(self):
    return ' '
  def _unpack(self, recbuff):
    return ISAM_str(super()._unpack(recbuff).replace(b'\x00',b' ')).rstrip()

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
      value = ISAM_bytes(value)
      if self._size_ < len(value):
        value = value[self._size_]
      elif len(value) < self._size_:
        value += b' ' * (self._size_ - len(value))
    super()._pack(recbuff, value.replace(b'\x00',b' '))
  def _unpack(self, recbuff):
    return ISAM_str(super()._unpack(recbuff).replace(b'\x00',b' ')).rstrip()

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
    self._row_  = ISAMrecord(self, kwds.get('idname'))
    self._mode_ = kwds.get('mode', self.def_openmode)
    self._lock_ = kwds.get('lock', self.def_lockmode)
    self._curidx_ = None      # Current index being used
    pass # TODO: Complete contents
  def __getitem__(self,name):
    'Retrieve the specified column having converted the value using O[name]'
    return self._row_[name]
  def __setitem__(self,name,value):
    'Set the specified column to the converted value using O[name] = value'
    self._row_[name] = value
  def __str__(self):
    return str(self._row_)
  def addindex(self, name, unique=False, *cols):
    pass # TODO: Implement this method
  @property
  def datbuff(self):
    'Return the default record buffer'
    return self._row_
  def open(self, mode=None, lock=None):
    'Open the underlying ISAM table with the specified modes'
    if self._isfd_ is None:
      if mode is None:
        mode = getattr(self,'_mode_', OpenMode.ISINPUT)
      if lock is None:
        lock = getattr(self,'_lock_', LockMode.ISMANULOCK)
      self.isopen(os.path.join(self._path_,self._name_),mode,lock)
      self._prepare()
  def close(self):
    'Close the underlying ISAM table and cleanup resources'
    if self._isfd_ is not None:
      self.isclose()
  def read(self, mode=None, *args, **kwd):
    self.open()
    if mode is None: mode = ReadMode.ISNEXT
    if mode in (ReadMode.ISNEXT, ReadMode.ISCURR, ReadMode.ISPREV):
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
      mode = ReadMode.ISNEXT
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

# NOTE: Place this in record.py
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
    fqtname = [defn._database_, defn._prefix_]
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

