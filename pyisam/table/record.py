'''
This module provides an object representing the ISAM record where the fields can be
accessed as both attributes as well as by number, the underlying object is an instance
of collections.namedtuple.

Typical usage of a fixed definition using the ISAMrecordBase is as follows:

class DEFILErecord(ISAMrecordBase):
  _database = 'utool'
  _prefix = 'def'
  filename = TableText(9)
  seq      = TableShort()
  field    = TableText(9)
  refptr   = TableText(9)
  type     = TableChar()
  size     = TableShort()
  keytype  = TableChar()
  vseq     = TableShort()
  stype    = TableShort()
  scode    = TableChar()
  fgroup   = TableText(10)
  idxflag  = TableChar()
'''

import collections
import struct
import sys
from keyword import iskeyword
from ..backend import RecordBuffer
from ..enums import ColumnType

__all__ = 'CharColumn', 'TextColumn', 'ShortColumn', 'LongColumn', 'FloatColumn', 'DoubleColumn', 'recordclass'

class _BaseColumn:
  'Base class providing the shared functionality for columns'
  __slots__ = '_struct', '_size', '_offset', '_name', '_nullval'
  def __init__(self, size=None, offset=-1):
    if not hasattr(self, '_struct'):
      raise ValueError('No struct object provided for column')
    if not hasattr(self, '_size'):
      if hasattr(self._struct, 'size'):
        self._size = self._struct.size
      elif isinstance(size, int):
        self._size = size
      else:
        raise ValueError('Must provide a size for column')
    self._offset = offset          # Offset into record buffer

  def __get__(self, inst, objtype):
    if self._offset < 0:
      raise ValueError('Column offset not known')
    return self._struct.unpack_from(inst._buffer, self._offset)[0]

  def __set__(self, inst, value):
    if self._offset < 0:
      raise ValueError('Column offset not known')
    if value is None:
      if hasattr(self, '_convnull'):
        value = self._convnull()
      elif hasattr(self, '_nullval'):
        value = self._nullval
    self._struct.pack_into(inst._buffer, self._offset, value)

  @classmethod
  def _template(cls):
    return ''

  # Rich comparision methods
  def __eq__(self, other):
    print(self, '==', other)
    return super().__eq__(self, other)

  def __ne__(self, other):
    print(self, '!=', other)
    return super().__ne__(self, other)

  def __lt__(self, other):
    print(self, '<', other)
    return super().__lt__(self, other)

  def __gt__(self, other):
    print(self, '>', other)
    return super().__gt__(self, other)
  
class CharColumn(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('c')
  _nullval = b' '
  _type = ColumnType.CHAR
  def __get__(self, inst, objtype):
    return super().__get__(inst, objtype).replace(b'\x00', b' ').decode('utf-8').rstrip()

  def __set__(self, obj, value):
    super().__set__(obj, value.encode('utf-8').replace(b'\x00', b' '))
  
class TextColumn(_BaseColumn):
  __slots__ = ()
  _type = ColumnType.CHAR
  def __init__(self, size, offset=-1):
    if not isinstance(size, int) or size <= 0:
      raise ValueError('Must provide an positive integer size for column')
    self._struct = struct.Struct('{}s'.format(size))
    self._nullval = b' ' * size
    super().__init__(offset)

  def __get__(self, inst, objtype):
    return super().__get__(inst, objtype).replace(b'\x00', b' ').decode('utf-8').rstrip()

  def __set__(self, inst, value):
    if value is None:
      if hasattr(value, '_convnull'):
        value = self._convnull()
      else:
        value = self._nullval
    else:
      value = value.encode('utf-8')
      if self._size < len(value):
        value = value[:self._size]
      elif len(value) < self._size:
        value += self._nullval[:self._size - len(value)]
    super().__set__(inst, value.replace(b'\x00', b' '))

  @classmethod
  def _template(cls):
    return '{0.length}'   # NOTE This refers to the table definition class attributes
  
class ShortColumn(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('>h')
  _nullval = 0
  _type = ColumnType.SHORT

class LongColumn(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('>l')
  _nullval = 0
  _type = ColumnType.LONG

class FloatColumn(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('=f')
  _nullval = 0.0
  _type = ColumnType.FLOAT

class DoubleColumn(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('=d')
  _nullval = 0.0
  _type = ColumnType.DOUBLE

class _ISAMrecordBaseMixin:
  '''Mixin class providing access to the current record providing access to the
     columns as attributes where each column is implemented by a descriptor
     object which makes the conversion to/from the underlying raw buffer directly.'''
  # IGNORED since __dict__ still created: __slots__ = '_buffer', '_namedtuple', '_curoff'
  def __init__(self, recname, recsize=None, **kwds):
    # NOTE:
    # The passing of a 'fields' keyword permits the tuple that represents a record to an
    # application to have fewer fields, or have the fields in a different order, than the
    # actual underlying record in the ISAM table, but does not prevent the actual access
    # to all the fields available.
    if 'fields' in kwds and kwds['fields'] is not None:
      kwd_fields = kwds['fields']
      if isinstance(kwd_fields, str):
        kwd_fields = kwd_fields.replace(',', ' ').split()
      else:
        raise ValueError('Unhandled type of fields to be presented')
      tupfields = [fld for fld in kwd_fields if fld in self._fields]
      if not tupfields:
        raise ValueError('Provided fields produces no suitable columns to use')
    else:
      tupfields = [fld.name for fld in self._fields]
    self._namedtuple = collections.namedtuple(recname, tupfields)

    # Create a record buffer which will produce suitable instances for this table when
    # called, this provides a means of modifying the buffer implementation without
    # affecting the actual package usage.
    self._record = RecordBuffer(self._recsize if recsize is None else recsize)
    self._buffer = self._record()

  def __getitem__(self, fld):
    'Return the current value of the given item'
    if isinstance(fld, int):
      return getattr(self, self._namedtuple._fields[fld].name)
    elif isinstance(fld, ColumnInfo):
      return getattr(self, fld.name)
    elif fld in self._namedtuple._fields:
      return getattr(self, fld)
    else:
      raise ValueError("Unhandled field '{}'".format(fld))

  def __setitem__(self, fld, value):
    'Set the current value of a field to the given value'
    if isinstance(fld, int):
      setattr(self, self._namedtuple._fields[fld], value)
    elif isinstance(fld, ColumnInfo):
      setattr(self, fld.name, value)
    elif fld in self._namedtuple._fields:
      setattr(self, fld, value)
    else:
      raise ValueError("Unhandled field '{}'".format(fld))

  def __call__(self):
    'Return an instance of the namedtuple for the current column values'
    return self._namedtuple._make(getattr(self, name) for name in self._namedtuple._fields)

  def __contains__(self, name):
    'Return whether the record contains a field of the given NAME'
    return name in self._namedtuple._fields

  def __str__(self):
    'Return the current values as a string'
    fldval = []
    for fld in self._fields:
      if fld.type == ColumnType.CHAR:
        fldval.append("{}='{}'".format(fld.name, getattr(self, fld.name)))
      else:
        fldval.append('{}={}'.format(fld.name, getattr(self, fld.name)))
    return ''.join(('{}({})'.format(self.__class__.__name__, ', '.join(fldval))))

  @property
  def _cur_value(self):
    'Return the current values of all fields in the record'
    return [getattr(self, fld.name) for fld in self._fields]

  def _colinfo(self, colname):
    'Return the field information for the given COLNAME'
    for fld in self._fields:
      if fld.name == colname:
        return fld
    raise IndexError('{} not present in the record'.format(colname))

# Create a special tuple for storing the column information avoiding the
# descriptor lookup that would otherwise occur
ColumnInfo = collections.namedtuple('ColumnInfo', 'name offset size type')

# Function to check the suitablity of a column name by excluding those names that are actually
# used for hidden methods or special methods used in the underlying class
def _valid_name(name):
  'Return whether the given NAME is valid'
  if (name[:2] == '__' and name[-2:] == '__' and
      name[2:3] != '_' and name[-3:-2] != '_' and len(name) > 4):
    return False
  if name[:1] == '_' and name[1:2] != '_' and len(name) > 1:
    return False
  return True

if sys.version_info.minor < 6:
  # Versions before 3.6 require special handling to maintain the order of fields
  class _ISAMrecordMeta(type):
    '''Metaclass providing the special requirements of an ISAMrecord which includes
       producing a list of the fields to maintain their order, plus the ability to
       return the actual column information without retrieving the underlying data'''
    @classmethod
    def __prepare__(metacls, name, bases, **kwds):
      return collections.OrderedDict()

    def __new__(cls, name, bases, namespace, **kwds):
      result = type.__new__(cls, name, bases, dict(namespace))

      # Don't process any further if the object being created is the Mixin class itself
      if name.endswith('ISAMrecordBase'):
        return result

      # Process the field names to ensure they do not cause issues and also calculate the
      # offset for each field within the record buffer area.
      fields, flddict, curoff = [], {}, 0
      for name, info in namespace.items():
        if not _valid_name(name):
          continue
        info._offset = curoff
        curoff += info._size
        fields.append(ColumnInfo(name, info._offset, info._size, info._type))
        flddict[name] = fields[-1]
      result._fields = fields
      result._flddict = flddict
      result._recsize = curoff
      return result

  class ISAMrecordBase(_ISAMrecordBaseMixin, metaclass=_ISAMrecordMeta):
    '''Provide the mixin for the remainder of the package using the
       metaclass to maintain the field ordering'''
    def __getitem__(self, fld):
      'Return the current value of the given item'
      if isinstance(fld, int):
        return getattr(self, self._fields[fld].name)
      elif isinstance(fld, str) and fld in self._flddict:
        return getattr(self, fld)
      else:
        raise ValueError("Unhandled field '{}'".format(fld))

    def __setitem__(self, fld, value):
      'Set the current value of a field to the given value'
      if isinstance(fld, int):
        setattr(self, self._fields[fld].name, value)
      elif isinstance(fld, str) and fld in self._flddict:
        setattr(self, fld, value)
      else:
        raise ValueError("Unhandled field '{}'".format(fld))
else:
  # Versions after 3.5 maintain the order of class attributes
  class ISAMrecordBase(_ISAMrecordBaseMixin):
    def __init__(self, recname, recsize=None, **kwds):
      fields, flddict, curoff = [], {}, 0
      for name, info in self.__class__.__dict__.items():
        if not _valid_name(name) or info is None or \
           (isinstance(info, str) and info.startswith('Record')):
          continue
        info._offset = curoff
        curoff += info._size
        fldinfo = ColumnInfo(name, info._offset, info._size, info._type)
        fields.append(fldinfo)
        flddict[name] = fldinfo
      self._fields = fields
      self._flddict = flddict
      self._recsize = curoff
      super().__init__(recname, recsize=curoff, **kwds)

    def __getitem__(self, fld):
      'Return the current value of the given item'
      if isinstance(fld, int):
        return getattr(self, self._fields[fld].name)
      elif isinstance(fld, str) and fld in self:
        return getattr(self, fld)
      else:
        raise ValueError("Unhandled field '{}'".format(fld))

    def __setitem__(self, fld, value):
      'Set the given column to the given value'
      if isinstance(fld, int):
        setattr(self, self._fields[fld].name, value)
      elif isinstance(fld, str) and fld in self:
        setattr(self, fld, value)
      else:
        raise ValueError("Unhandled field '{}'".format(fld))

# Define the templates used to generate the record definition class at runtime
_record_class_template = """\
class {rec_name}(ISAMrecordBase):
{fld_defn}
"""
_record_field_template = """\
  {name} = {klassname}({defn})
"""

# Define the default namespace that is always used for new record instances
_record_namespace = {
    'CharColumn'      : CharColumn,
    'TextColumn'      : TextColumn,
    'ShortColumn'     : ShortColumn,
    'LongColumn'      : LongColumn,
    'FloatColumn'     : FloatColumn,
    'DoubleColumn'    : DoubleColumn
}

def recordclass(tabdefn, recname=None, *args, **kwd):
  'Create a new class for the given table definition'
  seen, fdefn = set(), []
  if isinstance(tabdefn._columns_, collections.OrderedDict):
    flds = list(tabdefn._columns_.values())
  elif isinstance(tabdefn._columns_, (list, tuple)):
    flds = tabdefn._columns_
  else:
    raise ValueError('Unhandled column information encountered')
  for fld in flds:
    fldname = fld.name
    klassname = fld.__class__.__name__

    # Validate the field name using a similar rule to that in collections.namedtuple
    if not fldname.isidentifier() or iskeyword(fldname) or fldname.startswith('_'):
      raise NameError("Field '{}' is not a valid identifier".format(fldname))
    elif fldname in seen:
      raise NameError("Field '{}' already present in definition".format(fldname))
    seen.add(fldname)

    # Generate the template for the particular type of field
    fld_template = _record_namespace[klassname]._template()
    if '{' in fld_template:
      fld_template = fld_template.format(fld)

    # Create the field template (the offset is calculated by the metaclass)
    fdefn.append(_record_field_template.format(name=fldname,
                                               klassname=klassname,
                                               defn=fld_template))

  # Provide the record template
  if recname is None:
    fqname = [getattr(tabdefn, '_database_', None),
              getattr(tabdefn, '_prefix_', None),
              kwd.get('idname', getattr(tabdefn, '_tabname_', None))]
    recname = '_'.join(filter(None, fqname))
  else:
    # TODO: Check the given record name is valid
    pass
  record_name = 'Record_{name}'.format(name=recname)
  record_definition = _record_class_template.format(rec_name=record_name,
                                                    fld_defn=''.join(fdefn))

  # Execute in a temporary namespace that also includes the ISAMrecordBase object, 
  # and column objects used in the new object with additional tracing utilities
  namespace = {
    '__name__'       : record_name,
    'ISAMrecordBase' : ISAMrecordBase,
  } 
  namespace.update(_record_namespace)
  exec(record_definition, namespace)
  result = namespace[record_name]
  result._source = record_definition
  return result
