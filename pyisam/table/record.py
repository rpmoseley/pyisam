'''
This module provides an object representing the ISAM record where the fields can be
accessed as both attributes as well as by number, the underlying object is an instance
of collections.namedtuple.

Typical usage of a fixed definition using the ISAMrecordBase is as follows:

class DEFILErecord(ISAMrecordBase):
  _database = 'utool'
  _prefix = 'def'
  filename = TextColumn(9)
  seq      = ShortColumn()
  field    = TextColumn(9)
  refptr   = TextColumn(9)
  type     = CharColumn()
  size     = ShortColumn()
  keytype  = CharColumn()
  vseq     = ShortColumn()
  stype    = ShortColumn()
  scode    = CharColumn()
  fgroup   = TextColumn(10)
  idxflag  = CharColumn()
'''

import collections
import dataclasses
import datetime
import struct
from keyword import iskeyword
from ..backend import RecordBuffer
from ..constants import ColumnType

__all__ = ('CharColumn', 'TextColumn', 'ShortColumn', 'LongColumn', 'FloatColumn',
           'DoubleColumn', 'DateColumn', 'SerialColumn', 'ColumnInfo', 'recordclass')

# Create a special tuple for storing the column information avoiding the
# descriptor lookup that would otherwise occur
@dataclasses.dataclass
class ColumnInfo:
  name: int
  offset: int
  size: int
  type: int

  def __eq__(self, other):
    if self.offset != other.offset:
      return False
    if self.size != other.size:
      return False
    return  self.type == other.type

class _BaseColumn:
  'Base class providing the shared functionality for columns'
  __slots__ = '_struct', '_size', '_offset', '_name', '_nullval', '_type'
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

  def __set_name__(self, owner, name):
    '''3.6+ method called during class instantiation to permit decorators
       to know their name and owner instance, this is used to maintain the
       list of known fields to permit information about columns to be
       returned without actually invoking the fetch to/from the underlying
       record buffer.'''
    if self._offset == -1:  
      try:
        self._offset = owner._recsize
        owner._recsize += self._size
      except AttributeError:      
        self._offset = 0
        owner._recsize = self._size
    colinfo = ColumnInfo(name, self._offset, self._size, self._type)
    if not hasattr(owner, '_flddict'):
      owner._flddict = dict()
    if not hasattr(owner, '_fields'):
      owner._fields = list()
    owner._flddict[name] = colinfo
    owner._fields.append(colinfo)
    # Mark record has having a serial field if one is present
    if hasattr(self, '_serial') and self._serial:
      owner._serial = colinfo

  def __get__(self, inst, objtype):
    if self._offset < 0:
      raise ValueError('Column offset not known')
    val = self._struct.unpack_from(inst._buffer, self._offset)[0]
    if hasattr(self, '_postprocess') and callable(self._postprocess):
      val = self._postprocess(val)
    return val

  def __set__(self, inst, value):
    if self._offset < 0:
      raise ValueError('Column offset not known')
    if hasattr(self, '_preprocess') and callable(self._preprocess):
      value = self._preprocess(value)
    if value is None and hasattr(self, '_nullval'):
      value = self._nullval() if callable(self._nullval) else self._nullval
    self._struct.pack_into(inst._buffer, self._offset, value)

  # Template for this column
  _template = ''

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

  def _postprocess(self, value):
    return value.decode('utf-8').replace('\x00', ' ').rstrip()

  def _preprocess(self, value):
    return value.encode('utf-8').replace(b'\x00', b' ') 
  
class TextColumn(_BaseColumn):
  __slots__ = ('_blankval', )
  _type = ColumnType.CHAR

  def __init__(self, size, offset=-1):
    if not isinstance(size, int) or size <= 0:
      raise ValueError('Must provide an positive integer size for column')
    self._struct = struct.Struct(f'{size}s')
    self._blankval = b' ' * self._struct.size
    super().__init__(offset)

  def _postprocess(self, value):
    return value.decode('utf-8').replace('\x00', ' ').rstrip()

  def _preprocess(self, value):
    if value is None:
      return self._blankval
    value = value.encode('utf-8')
    if self._size < len(value):
      value = value[:self._size]
    elif len(value) < self._size:
      value += self._blankval[-self._size:]
    return value

  # Template for fields of this column type
  _template = '{0.length}'
  
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

class SerialColumn(LongColumn):
  __slots__ = ()
  _serial = True

class DateColumn(LongColumn):
  __slots__ = ()
  _since_1900 = datetime.date(1899, 12, 31).toordinal()
  _nullval = -2147483648   # '\x80\x00\x00\x00' used for NUL dates

  def _postprocess(self, value):
    if value == self._nullval:
      return None
    else:
      return datetime.date.fromordinal(value + self._since_1900)

  def _preprocess(self, value):
    if value is None:
      return None
    else:
      return value.toordinal() - self._since_1900

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

class MoneyColumn(DoubleColumn):
  __slots__ = ()

class ISAMrecordBase:
  '''Base class providing access to the current record providing access to the
     columns as attributes where each column is implemented by a descriptor
     object which makes the conversion to/from the underlying raw buffer directly.'''
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

  def as_tuple(self):
    'Return an instance of the namedtuple for the current column values'
    return self._namedtuple._make(getattr(self, name) for name in self._namedtuple._fields)
  __call__ = as_tuple

  def __contains__(self, name):
    'Return whether the record contains a field of the given NAME'
    return name in self._namedtuple._fields

  @property
  def _cur_value(self):
    'Return the current values of all fields in the record'
    return [getattr(self, fld.name) for fld in self._namedtuple._fields]

  def __str__(self):
    'Return the current values as a string'
    fldval = []
    for fld in self._namedtuple._fields:
      finfo = self._flddict[fld]
      if finfo.type == ColumnType.CHAR:
        fldval.append(f"{fld}='{getattr(self, fld)}'")
      else:
        fldval.append(f'{fld}={getattr(self, fld)}')
    return ''.join(('{}({})'.format(self.__class__.__name__, ', '.join(fldval))))

# Define the templates used to generate the record definition class at runtime
_record_class_template = """\
class {rec_name}(ISAMrecordBase):
  __slots__ = ('_record',)
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
    'DoubleColumn'    : DoubleColumn,
    'DateColumn'      : DateColumn,
    'SerialColumn'    : SerialColumn,
    'MoneyColumn'     : MoneyColumn,
    'ISAMrecordBase'  : ISAMrecordBase,
}

def recordclass(tabdefn, recname=None, keepsrc=True, *args, **kwd):
  'Create a new class for the given table definition'
  seen, fdefn = set(), []
  if isinstance(tabdefn._columns_, (collections.OrderedDict, dict)):
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
      raise NameError(f"Field '{fldname}' is not a valid identifier")
    elif fldname in seen:
      raise NameError(f"Field '{fldname}' already present in definition")
    seen.add(fldname)

    # Generate the template for the particular type of field
    fld_template = _record_namespace[klassname]._template.format(fld)

    # Create the field template (the offset is calculated by the metaclass)
    fdefn.append(_record_field_template.format(name=fldname,
                                               klassname=klassname,
                                               defn=fld_template))

  # Provide the record template
  if recname is None:
    fqname = [getattr(tabdefn, '_database_', None),
              getattr(tabdefn, '_prefix_', None),
              kwd.get('idname', getattr(tabdefn, '_tabname_', None))]
    recname = '_'.join([str(x) for x in fqname if x is not None])
  if recname.isalnum() or recname.find('_') >= 0:
    record_name = f'Rec_{recname}'
  else:
    raise NameError(f"Record '{recname} is not a valid identifier")
  record_definition = _record_class_template.format(rec_name=record_name,
                                                    fld_defn=''.join(fdefn))

  # Execute in a temporary namespace that also includes the ISAMrecordBase object, 
  # and column objects used in the new object with additional tracing utilities
  namespace = {
    '__name__'       : record_name,
  } 
  namespace.update(_record_namespace)
  exec(record_definition, namespace)
  result = namespace[record_name]
  if keepsrc:
    result._source = record_definition
  return result
