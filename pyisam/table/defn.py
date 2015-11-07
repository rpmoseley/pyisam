'''
This subpackage contains the classes to provide the definition of an ISAM
table including its columns and indexes, these are separate from the table
logic to permit alternative implementations to be provided and reduce the
complexity of the table module.
'''

__all__ = ('CharColumn', 'TextColumn', 'ShortColumn',
           'LongColumn', 'FloatColumn', 'DoubleColumn',
           'PrimaryIndex', 'UniqueIndex', 'DuplicateIndex', 'TableIndex')

import struct
from ..isam import ISAMindexMixin
from ..utils import IsamException

class TableColumn:
  'Base class providing the shared functionality for columns'
  __slots__ = ('_name_', '_offset_', '_size_', '_struct_', '_type_')
  def __init__(self, offset, size=None):
    self._offset_ = offset          # Offset into record buffer
    if not hasattr(self, '_type_'):
      raise ValueError('No type provided for column')
    if not hasattr(self, '_struct_'):
      raise ValueError('No structure object provided for column')
    if not hasattr(self, '_size_'):
      if isinstance(self._struct_, struct.Struct):
        self._size_ = self._struct_.size
      elif isinstance(size, int):
        self._size_ = size
      else:
        raise ValueError('Must provide a size for column {}'.format(self._name_))
  def _unpack(self, recbuff):
    if self._offset_ == -1:
      raise IsamException("No offset for column '{}' defined".format(self._name_))
    return self._struct_.unpack_from(recbuff, self._offset_)[0]
  def _pack(self, recbuff, value):
    if self._offset_ == -1:
      raise IsamException("No offset for column '{}' defined".format(self._name_))
    if value is None and hasattr(self, '_convnull'):
      value = self._convnull()
    self._struct_.pack_into(recbuff, self._offset_, value)
  def __str__(self):
    return '{0._name_} = ({0._offset_}, {0._size_}, {0._type_})'.format(self)

class CharColumn(TableColumn):
  __slots__ = ()
  _type_ = 0
  _struct_ = struct.Struct('c')
  def _convnull(self):
    return b' '
  def _pack(self, recbuff):
    super()._pack(recbuff, value.encode().replace(b'\x00',b' '))
  def _unpack(self, recbuff):
    return super()._unpack(recbuff).replace(b'\x00',b' ').decode().rstrip()

class TextColumn(TableColumn):
  __slots__ = ()
  _type_ = 0
  def __init__(self, offset, length):
    if length <= 0: raise ValueError("Must provide a positive length")
    self._struct_ = struct.Struct('{}s'.format(length))
    super().__init__(offset)
  def _pack(self, recbuff, value):
    if value is None:
      value = b' ' * self._size_
    else:
      value = value.encode()
      if self._size_ < len(value):
        value = value[:self._size_]
      elif len(value) < self._size_:
        value += b' ' * (self._size_ - len(value))
    super()._pack(recbuff, value.replace(b'\x00',b' '))
  def _unpack(self, recbuff):
    return super()._unpack(recbuff).replace(b'\x00',b' ').decode().rstrip()

class ShortColumn(TableColumn):
  __slots__ = ()
  _type_ = 1
  _struct_ = struct.Struct('>h')
  def _convnull(self):
    return 0

class LongColumn(TableColumn):
  __slots__ = ()
  _type_ = 2
  _struct_ = struct.Struct('>l')
  def _convnull(self):
    return 0

class FloatColumn(TableColumn):
  __slots__ = ()
  _type_ = 4
  _struct_ = struct.Struct('=f')
  def _convnull(self):
    return 0.0

class DoubleColumn(TableColumn):
  __slots__ = ()
  _type_ = 3
  _struct_ = struct.Struct('=d')
  def _convnull(self):
    return 0.0

class TableIndexCol:
  'This class represents a column within an index definition'
  def __init__(self, name, offset=None, length=None):
    self.name = name
    self.offset = offset
    self.length = length
  def __getitem__(self,idx):
    if idx == 0:
      return self.name
    elif idx == 1:
      return self.offset
    elif idx == 2:
      return self.length
    else:
      raise IndexError('Invalid element specified: {}'.format(idx))
  def __str__(self):
    if self.offset is None and self.length is None:
      return 'TableIndexCol({0.name})'.format(self)
    elif self.length is None:
      return 'TableIndexCol({0.name}, {0.offset})'.format(self)
    elif self.offset is None:
      return 'TableIndexCol({0.name}, 0, {0.length})'.format(self)
    else:
      return 'TableIndexCol({0.name}, {0.offset}, {0.length})'.format(self)

class TableIndex(ISAMindexMixin):
  '''Class used to store index information in an instance of ISAMtable'''
  def __init__(self, name, colinfo, dups, desc, knum=-1, kdesc=None):
    self._name_ = name
    self._dups_ = dups
    self._desc_ = desc
    self._knum_ = knum                # Stores the key number once matched
    self._kdesc_ = kdesc              # Stores the keydesc once prepared
    if isinstance(colinfo, (tuple, list)):
      self._colinfo_ = list()
      for col in colinfo:
        self._colinfo_.append(TableIndexCol(col.name, col.offset, col.length))
    else:
      self._colinfo_ = TableIndexCol(colinfo.name, colinfo.offset, colinfo.length)
  def as_keydesc(self, tabobj, optimize=False):
    kdesc = self._kdesc_
    if kdesc is None:
      kdesc = self._kdesc_ = self.create_keydesc(tabobj, optimize=optimize)
    return kdesc
  def fill_fields(self, _record=None, *args, **kwd):
    '''Fill the fields in the given _RECBUFF with ARGS and KWD, if _RECBUFF is None
       then the default buffer _TABOBJ._row_ is used instead'''
    if _record is None:
      try:
        _tabobj = kwd['_tabobj']
        if not hasattr(_tabobj, '_isobj_'):
          raise ValueError("Expecting an instance of ISAMtable for '_tabobj'")
        _record = _tabobj._row_
      except KeyError:
        raise ValueError('Must provide a table instance if not giving a record buffer')
    elif hasattr(_record, '_isobj_'):
      _record = _record._row_
    elif not hasattr(_record, '_namedtuple_'):
      raise ValueError("Expecting an instance of ISAMrecord for '_recbuff'")
    else:
      _tabobj = _record._tabobj_

    # Process the fields using either keywords or arguments 
    _colinfo = [self._colinfo_] if not isinstance(self._colinfo_, (tuple, list)) else self._colinfo_
    _fld_argn = 0
    for _col in _colinfo:
      try:
        _record[_col] = kwd[_col.name]
      except KeyError:
        _record[_col] = args[_fld_argn] if _fld_argn < len(args) else None
        _fld_argn += 1

class PrimaryIndex(TableIndex):
  pass
class UniqueIndex(TableIndex):
  pass
class DuplicateIndex(TableIndex):
  pass

class TableDefn:
  '''Class providing the table definition for an instance of ISAMtable,
     this includes all the columns and the key definition which are then
     mapped to by the ISAMtable instance when the actual table is opened.
     This class calculates the struct() definition to retrieve the data
     from the record buffer without recourse to the ISAM ld*/st* routines
     which may be macros.

     The columns in the table are defined by the '_columns_' list which
     provides instances of TableColumn giving the length and type of
     each column which are used to generate the associated mapping used
     to retrieve the individual values from the record buffer.

     The indexes on the table are defined by the '_indexes_' list which
     provides instances of TableIndex giving the index information for the
     table.

     It is expected that an application adds additional logic to make use
     of the definition information, with the support of the low-level
     copying of data to/from a given record buffer provided by the columns
     themselves.'''
  __slots__ = ('_database_','_prefix_','_tabname_', '_columns_', '_indexes_')
  """
  def __call__(self, tabobj):
    'Calculate the column and index information updating the given TABOBJ'
    if not hasattr(tabobj, '_isobj_'): # Avoid cyclic-loop 
      raise ValueError('Expecting an instance of ISAMtable')
    size = 0
    allcol = collections.OrderedDict()
    allidx = collections.OrderedDict()
    # Create the column information
    for info in self._columns_:
      if info._name_ in allcol:
        raise ValueError('Column of the same name ({}) already present in table'.format(info._name_))
      allcol[info._name_] = ISAMcolumnInfo(info, size)
      size += info._size_
    # Create the index information
    for info in self._indexes_:
      if info._name_ in allidx:
        raise ValueError('Index of the same name ({}) already present in table'.format(info._name_))
      allidx[info._name_] = ISAMindexInfo(info)
    # Update the table instance
    tabobj._colinfo_ = allcol
    tabobj._idxinfo_ = allidx
  """
