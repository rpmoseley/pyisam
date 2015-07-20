'''
This module provides a structured way of accessing tables using ISAM by providing objects that
permit the table layout to be defined and referenced later.

TODO: Refactor how the column and index information is stored on the ISAMtable definition, to
      avoid having to iterate the list of columns and indexes each time.
'''

import collections
from ctypes import create_string_buffer
import os
import struct
from .isam import ReadMode,OpenMode,LockMode,IndexFlags,StartMode,ISAMobject,keydesc,keypart
from .utils import ISAM_bytes,ISAM_str

# Define the objects that are available using 'from .table import *'
__all__ = ('CharColumn','TextColumn','ShortColumn','LongColumn','FloatColumn',
           'DoubleColumn','PrimaryKey','UniqueKey','DuplicateKey','AscPrimaryKey',
           'AscUniqueKey','AscDuplicateKey','DescPrimaryKey','DescUniqueKey',
           'DescDuplicateKey','ISAMtableDefn','ISAMtable','ISAMrecord',
           'ISAMindex','ISAMindexCol')

class ISAMcolumn:
  'Base class providing the shared functionality for columns'
  __slots__ = ('_name_','_offset_','_size_','_struct_','_type_')
  def __init__(self, name, size=None):
    self._name_ = name
    self._offset_ = -1          # Offset into record buffer
    if not hasattr(self, '_type_'):
      raise ValueError('No type provided for column')
    if not hasattr(self, '_struct_'):
      raise ValueError('No structure object provided for column')
    if not hasattr(self, '_size_'):
      if isinstance(self._struct_, struct.Struct):
        self._size_ = self._struct_.size
      elif isinstance(size, long):
        self._size = size
      else:
        raise ValueError('Must provide a size for column {}'.format(self._name_))
  def _unpack(self, recbuff):
    try:
      return self._struct_.unpack_from(recbuff, self._offset_)[0]
    except AttributeError:
      pass
  def _pack(self, recbuff, value):
    if value is None and hasattr(self, '_convnull'):
      value = self._convnull()
    if isinstance(value,str):
      value = ISAM_bytes(value).replace(b'\x00',b' ')
    self._struct_.pack_into(recbuff, self._offset_, value)
  def __str__(self):
    return '{0._name_} = ({0._offset_}, {0._size_}, {0._type_})'.format(self)

class CharColumn(ISAMcolumn):
  __slots__ = ()
  _type_ = 0
  _struct_ = struct.Struct('c')
  def _convnull(self):
    return ' '
  def _unpack(self, recbuff):
    return ISAM_str(super()._unpack(recbuff).replace(b'\x00',b' ')).rstrip()

class TextColumn(ISAMcolumn):
  __slots__ = ()
  _type_ = 0
  def __init__(self, name, length):
    if length <= 0: raise ValueError("Must provide a positive length")
    self._struct_ = struct.Struct('{}s'.format(length))
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
  __slots__ = ()
  _type_ = 1
  _struct_ = struct.Struct('>h')
  def _convnull(self):
    return 0

class LongColumn(ISAMcolumn):
  __slots__ = ()
  _type_ = 2
  _struct_ = struct.Struct('>l')
  def _convnull(self):
    return 0

class FloatColumn(ISAMcolumn):
  __slots__ = ()
  _type_ = 4
  _struct_ = struct.Struct('=f')
  def _convnull(self):
    return 0.0

class DoubleColumn(ISAMcolumn):
  __slots__ = ()
  _type_ = 3
  _struct_ = struct.Struct('=d')
  def _convnull(self):
    return 0.0

class ISAMindexCol:
  'This class represents a column within an index definition'
  def __init__(self, name, offset=None, length=None):
    self._name = name
    self._offset = offset
    self._length = length
  def __getitem__(self,idx):
    if idx == 0:
      return self._name
    elif idx == 1:
      return self._offset
    elif idx == 2:
      return self._length
    else:
      raise IndexError('Invalid element specified: {}'.format(idx))
  @property
  def name(self):
    return self._name
  @property
  def offset(self):
    return self._offset
  @property
  def length(self):
    return self._length
  def __str__(self):
    if self._offset is None and self._length is None:
      return 'ISAMindexCol({0._name})'.format(self)
    elif self._length is None:
      return 'ISAMindexCol({0._name},{0._offset})'.format(self)
    elif self._offset is None:
      return 'ISAMindexCol({0._name},0,{0._length})'.format(self)
    else:
      return 'ISAMindexCol({0._name},{0._offset},{0._length})'.format(self)

class ISAMindex:
  '''This class provides the base implementation of an index for a table,
     an index is represented by a name, a flag whether duplicates are allowed,
     an optional list of ISAMindexCol instances which define the columns within
     the index, and an optional flag whether the index is descending instead of
     ascending.'''
  def __init__(self, name, *colinfo, dups=True, desc=False):
    self.name = name
    self.dups = dups
    self.desc = desc
    if len(colinfo) < 1:
      self.colinfo = ISAMindexCol(self.name)
    else:
      colinfo2 = list()
      for col in colinfo:
        if isinstance(col, str):
          colinfo2.append(ISAMindexCol(col))
        elif isinstance(col, (tuple, list)):
          if len(col) < 2:
            colinfo2.append(ISAMindexCol(col[0]))
          elif len(col) < 3:
            colinfo2.append(ISAMindexCol(col[0], col[1]))
          elif len(col) < 4:
            colinfo2.append(ISAMindexCol(col[0], col[1], col[2]))
          else:
            raise ValueError('Column definition is not correctly formatted: {}'.format(col))
      self.colinfo = colinfo2
  def prepare_kdesc(self, owntable, optimize=False):
    '''Return an instance of keydesc() using the column information from the
       given OWNTABLE instance'''
    if not isinstance(owntable, ISAMtable):
      raise ValueError('Expecting an instance of ISAMtable for `owntable`')
    def _idxpart(idxcol):
      '''Internal function to add column to current index definition.
         Also modifies the kdesc instance.'''
      if not isinstance(idxcol, ISAMindexCol):
        raise ValueError('Expecting an instance of ISAMindexCol for `idxcol`')
      colinfo = owntable._defn_._colinfo_[idxcol.name]
      kpart = keypart()
      if idxcol.length is None and idxcol.offset is None:
        # Use the whole column in the index
        kpart.start = colinfo._offset_
        kpart.leng  = colinfo._size_
        kpart.type_ = colinfo._type_
        kdesc.leng += colinfo._size_
      elif idxcol.offset is None:
        # Use the prefix part of column in the index
        if idxcol.length > colsize._size_:
          raise ValueError('Index part is larger than specified column')
        kpart.start = colinfo._offset_
        kpart.leng  = idxcol.length
        kpart.type_ = colinfo._type_
        kdesc.leng += colinfo._size_
      else:
        # Use the length of column from the given offset in the index
        if idxcol.offset + idxcol.length > colsize._size_:
          raise ValueError('Index part too long for specified column')
        kpart.start = colinfo._offset_ + idxcol.offset
        kpart.leng  = idxcol.length
        kpart.type_ = colinfo._type_
        kdesc.leng += colinfo._size_
      return kpart
    kdesc = keydesc()
    kdesc.flags = 1 if self.dups else 0
    kdesc.leng = 0
    if isinstance(self.colinfo, ISAMindexCol):
      # Single key comprising the given column
      kdesc.nparts = 1
      kdesc.part[0] = _idxpart(self.colinfo)
      kd
    elif isinstance(self.colinfo, (tuple, list)):
      # Multi-part key comprisning the given columns
      kdesc.nparts = len(self.colinfo)
      for curidx, idxcol in enumerate(self.colinfo):
        kdesc.part[curidx] = _idxpart(idxcol)
    if kdesc.leng > 8 and optimize:
      kdesc.flags |= IndexFlags.ALL_COMPRESS
    return kdesc
  def __str__(self):
    'Provide a readable form of the information in the index'
    out = ['{}({},'.format(self.__class__.__name__,self.name)]
    if isinstance(self.colinfo,ISAMindexCol):
      out.append('{},'.format(self.colinfo))
    else:
      for cinfo in self.colinfo:
        out.append('{},'.format(cinfo))
    out.append('dups={0.dups},desc={0.desc})'.format(self))
    return ''.join(out)

# Provide an easier to read way of identifying the various key types
class DuplicateKey(ISAMindex):
  def __init__(self, name, *colinfo, desc=False):
    super().__init__(name, *colinfo, dups=True, desc=desc)
class UniqueKey(ISAMindex):
  def __init__(self, name, *colinfo, desc=False):
    super().__init__(name, *colinfo, dups=False, desc=desc)
class PrimaryKey(UniqueKey):
  pass
class AscDuplicateKey(DuplicateKey):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=False)
class AscUniqueKey(UniqueKey):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=False)
class AscPrimaryKey(PrimaryKey):
  pass
class DescDuplicateKey(DuplicateKey):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=True)
class DescUniqueKey(UniqueKey):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=True)
class DescPrimaryKey(AscUniqueKey):
  pass

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
     provides instances of ISAMindex giving the index information for the
     table.
  '''
  __slots__ = ('_size_','_colinfo_','_idxinfo_','_database_','_prefix_')
  _columns_ = None               # Will be overidden by predefined definitions
  _indexes_ = None               # Will be overidden by predefined definitions
  def __init__(self, **kwds):
    self._size_ = 0
    self._colinfo_ = collections.OrderedDict()
    self._idxinfo_ = {}
    if self._columns_ is not None:
      if not isinstance(self._columns_,(tuple,list)):
        raise ValueError('`_columns_` should be a sequence object')
      for col in self._columns_:
        self._add_column(col)
    if self._indexes_ is not None:
      if not isinstance(self._indexes_,(tuple,list)):
        raise ValueError('`_indexes_` should be a sequence object')
      for idx in self._indexes_:
        self._add_index(idx)
    if not hasattr(self, '_database_'):
      self._database_ = kwds.pop('_database_',None)
    if not hasattr(self, '_prefix_'):
      self._prefix_ = kwds.pop('_prefix_',None)
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
  def _add_column(self, col):
    if col._name_ in self._colinfo_:
      raise ValueError('Column of the same name ({}) already present in table'.format(col._name_))
    col._offset_ = self._size_
    self._size_ += col._size_
    self._colinfo_[col._name_] = col
  def _add_index(self, idx):
    if not isinstance(idx, ISAMindex):
      raise ValueError('Index is expected to be an instance of ISAMindex')
    if idx.name in self._idxinfo_:
      raise ValueError('Index of the same name ({}) already present in table'.format(idx.name))
    self._idxinfo_[idx.name] = idx
  @property
  def _colname_(self):
    'Provide a backwards compatible list of the column names on a table'
    return list(self._colinfo_.keys())

class ISAMtableDefnDyn(ISAMtableDefn):
  '''Class providing the table definition for an instance of ISAMtable, which adds
     the ability to add columns and indexes after the object is created. This can
     be used to dynamically define a table at runtime.
  '''
  def __init__(self, **kwds):
    super().__init__(**kwds)

class ISAMtable(ISAMobject):
  '''Class that represents a table within ISAM, this holds all the column
     and index information and also provides the get/set functionality
     with dictionary like access. It will match up the index information
     with the information stored within the table file itself.'''
  __slots__ = ('_name_','_path_','_row_','_mode_','_lock_','_curidx_','_kdesc_','_defn_','_idxmap_')
  def __init__(self, tabdefn, tabname=None, tabpath=None, **kwds):
    super(ISAMtable,self).__init__()
    self._defn_ = tabdefn if isinstance(tabdefn, ISAMtableDefn) else tabdefn()
    if tabname is None:
      tabname = self._defn_.__class__.__name__.lower()
      if not tabname.endswith('defn'):
        raise ValueError('Table definition class is expected to be of the form <table>defn')
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
  def match_index(self, kdesc):
    '''Attempt to match the given keydesc to a known index'''
    if not hasattr(self,'_kdesc_'):
      self._prepare_kdesc()
    for idxname,idxdesc in self._kdesc_.items():
      if idxdesc == kdesc:
        return idxname
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
  def read(self, idxname=None, mode=None, *args, **kwd):
    self.open()
    if mode is None: mode = ReadMode.ISNEXT
    if mode in (ReadMode.ISNEXT, ReadMode.ISCURR, ReadMode.ISPREV):
      self.isread(self._row_.record, mode)
      return self._row_.rectuple
    if idxname is None:
      self.isread(self._row_.record, mode)
      return self._row_.rectuple
    idxinfo = self._defn_._idxinfo_[idxname]
    if idxinfo.colinfo[0] in kwd:
      # Use keywords to fill in index information
      for col in idxinfo.colinfo:
        if col not in kwd:
          raise ValueError('Use only keywords, do not mix with arguments')
        self[col] = kwd.get(col)
    else:
      # Use arguments to fill in index information
      for argn,col in enumerate(idxinfo.colinfo):
        if argn < len(args):
          self[col] = args[argn]
        else:
          self[col] = None
    if self._curidx_ != idxname:
      smode = StartMode(mode)           # Convert to a StartMode value
      self.isstart(self._kdesc_[idxname],smode)
      self._curidx_ = idxname
      mode = ReadMode.ISNEXT
    self.isread(self._row_.record,mode)
    return self._row_.rectuple
  def _prepare(self):
    'Prepare the object for the actual table file provided'
    self._prepare_kdesc()
    idxmap = [None] * len(self._kdesc_)
    dinfo = self.isdictinfo()
    for curkey in range(dinfo.nkeys):
      kdesc = self.iskeyinfo(curkey+1)
      idxname = self.match_index(kdesc)
      if idxname is not None:
        idxmap[curkey] = idxname
    self._idxmap_ = idxmap
  def _prepare_kdesc(self):
    'Prepare the kdesc information for each of the indexes defined for this table'
    if not hasattr(self,'_kdesc_'):
      newkdesc = dict()
      for idx in self._defn_._idxinfo_.values():
        newkdesc[idx.name] = idx.prepare_kdesc(self)
      self._kdesc_ = newkdesc
  def dump(self,ofd):
    ofd.write('Table: {}\n'.format(self.__class__.__name__))
    ofd.write('  Columns:\n')
    for col in self._defn_._colinfo_.items():
      ofd.write('     {}\n'.format(col))
    ofd.write('  Indexes:\n')
    self._prepare_kdesc()
    for idxname,idxinfo in self._kdesc_.items():
      ofd.write('     {}: {}\n'.format(idxname,idxinfo))

class ISAMrecord(dict):
  '''Class providing access to the current record providing access to the columns
     as both attributes as well as dictionary access.'''
  def __init__(self, tabobj, idname=None, **kwd):
    if not isinstance(tabobj, ISAMtable):
      raise ValueError('Definition must be an instance of ISAMtable')
    self._tabobj_ = tabobj
    self._defn_ = defn = tabobj._defn_
    self._recbuff_ = create_string_buffer(defn._size_ + 1)
    fqtname = [defn._database_, defn._prefix_]
    if idname is not None:
      fqtname.append(idname)
    self._namedtuple_ = collections.namedtuple('_'.join(fqtname), defn._colname_)
  @property
  def record(self):
    return self._recbuff_
  @property
  def rectuple(self):
    'Return an instance of the namedtuple for the current column values'
    col_value = [getattr(self, name) for name in self._defn_._colname_]
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
    if isinstance(name, list):
      return list(getattr(self, col) for col in name)
    elif isinstance(name, tuple):
      return tuple(getattr(self, col) for col in name)
    else:
      return getattr(self, name)
  def __setitem__(self, name, value):
    if isinstance(name, tuple):
      if len(name) < len(value):
        raise ValueError('Must pass a value with the same number of columns')
      for n,v in zip(name, value):
        setattr(self, n.name if isinstance(n, ISAMindexCol) else n, value)
    else:
      setattr(self, name.name if isinstance(name, ISAMindexCol) else name, value)
  def __str__(self):
    return str(self.rectuple)

if __name__ == '__main__':
  iic = ISAMindexCol('test')
  print(iic.name,   iic[0])
  print(iic.offset, iic[1])
  print(iic.length, iic[2])
  print(iic[3])
