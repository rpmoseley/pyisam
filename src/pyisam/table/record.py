'''
This module provides an object representing the ISAM record where the fields can
be accessed as both attributes as well as by number, the underlying object is an
instance of collections.namedtuple.

Typical usage of a fixed definition using the ISAMrecordBase is as follows:

class DEFILErecord(ISAMrecordBase):
  _database = "utool"
  _prefix = "def"
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
import functools
import struct
import types
from collections.abc import Callable
from keyword import iskeyword
from ..backend import _backend
from ..constants import ColumnType

__all__ = ('CharColumn', 'TextColumn', 'ShortColumn', 'LongColumn', 'FloatColumn',
           'DoubleColumn', 'DateColumn', 'SerialColumn', 'ColumnInfo')

# Create a special tuple for storing the column information avoiding the
# descriptor lookup that would otherwise occur
@dataclasses.dataclass
class ColumnInfo:
  name: str
  offset: int
  size: int
  type: int

  def __eq__(self, other):
    return self.offset == other.offset and self.size == other.size and self.type == other.type

  def __ne__(self, other):
    return self.offset != other.offset or self.size != other.size or self.type != other.type

class _BaseColumn:
  'Base class providing the shared functionality for columns'
  __slots__ = '_serial', '_struct', '_size', '_offset', '_name', '_nullval', '_type'
  _size: int
  _struct: struct.Struct
  _type: int
  _nullval: bytes|int|float
  _template: str|None = None
  _postprocess: Callable|None = None
  _preprocess: Callable|None = None

  def __init__(self, size=None):
    if not hasattr(self, '_struct'):
      raise TypeError('No struct object provided for column')
    if hasattr(self, '_size'):
      if not isinstace(size, int) or size < 1:
        raise TypeError('Size of column must be a positive integer value')
    else:
      if hasattr(self._struct, 'size'):
        self._size = self._struct.size
      elif isinstance(size, int) and size > 0:
        self._size = size
      else:
        raise TypeError('Must provide a positive intger for the size of column')
    if not hasattr(self, '_nullval'):
      raise TypeError('No default value for NUL provided (_nullval)')

  def __set_name__(self, owner, name):
    '''3.6+ method called during class instantiation to permit decorators
       to know their name and owner instance, this is used to maintain the
       list of known fields to permit information about columns to be
       returned without actually invoking the fetch to/from the underlying
       record buffer.'''
    if hasattr(owner, '_recsize'):
      self._offset = owner._recsize
      owner._recsize += self._size
    else:
      self._offset = 0
      owner._recsize = self._size
    # Create column information instance
    colinfo = ColumnInfo(name, self._offset, self._size, self._type)
    if not hasattr(owner, '_fields'):
      owner._fields = dict()
    elif name in owner._fields:
      raise ValueError(f'Duplicate column name: {name}')
    else:
      owner._fields[name] = colinfo
    # Mark record has having a serial field if one is present
    if hasattr(self, '_serial') and self._serial:
      owner._serial = colinfo

  def __get__(self, inst, objtype):
    'Fetch the current value from the underlying record buffer'
    assert self._offset >= 0, 'Column offset not known'
    value = self._struct.unpack_from(inst._buffer, self._offset)[0]
    proc = getattr(self, '_postprocess', None)
    return proc(value) if callable(proc) else value

  def __set__(self, inst, value):
    'Set the underlying record buffer value'
    assert self._offset >= 0, 'Column offset not known'
    proc = getattr(self, '_preprocess', None)
    self._struct.pack_into(inst._buffer, self._offset, proc(value) if callable(proc) else value)

  def _postprocess(self, value):
    'Default post processing method'
    return None if value == self._nullval else value

  def _preprocess(self, value):
    'Default pre processing method'
    return self._null if value is None else value

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
  
class CharColum(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('c')
  _type = ColumnType.CHAR
  _template = 'char {0.name};'

  def _postprocess(self, value):
    return value.decode('iso8859-1').replace('\x00', ' ').rstrip()

  def _preprocess(self, value):
    return value.encode('iso8859-1').replace(b'\x00', b' ') 
  
class TextColumn(_BaseColumn):
  __slots__ = ()
  _type = ColumnType.CHAR
  _template = 'char {0.name}[{0.size}];'

  def __init__(self, size, offset=-1):
    if not isinstance(size, int) or size <= 0:
      raise ValueError('Must provide a positive integer size for column')
    self._struct = struct.Struct(f'{size}s')
    self._nullval = b' '.ljust(size)
    super().__init__()

  def _postprocess(self, value):
    return value.decode('iso8859-1').replace('\x00', ' ').rstrip()

  def _preprocess(self, value):
    if value is None:
      return self._nullval
    value = value.encode('iso8859-1')
    if self._size < len(value):
      return value[:self._size]
    elif len(value) < self._size:
      return value.ljust(self._size)
    else:
      return value

class ShortColumn(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('>h')
  _nullval = 0
  _type = ColumnType.SHORT
  _template = 'long {0.name};'

class LongColumn(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('>l')
  _nullval = 0
  _type = ColumnType.LONG
  _template = 'long {0.name};'

class SerialColumn(LongColumn):
  __slots__ = ()
  _serial = True

class DateColumn(LongColumn):
  __slots__ = ()
  _1900 = datetime.date(1899, 12, 31).toordinal()    # Days upto 1900
  _nullval = -2147483648   # '\x80\x00\x00\x00' used for NUL dates

  def _postprocess(self, value):
    if value == self._nullval:
      return None
    else:
      return datetime.date.fromordinal(value + self._1900)

  def _preprocess(self, value):
    if value is None:
      return self._nullval
    elif isinstance(value, datetime.date):
      return value.toordinal() - self._1900
    else:
      raise ValueError("Provided value not a 'datetime.date' instance")

class _RealMixin(_BaseColumn):
  __slots__ = ()
  def __get__(self, inst, objtype):
    'Fetch the current value checking for the special NUL value'
    assert self._offset >= 0, 'Column offset not known'
    for off in range(self._offset, self._offset + self._size):
      if inst._buffer[off] != 0xFF:
        value = self._struct.unpack_from(inst._buffer, self._offset)[0]
        if hasattr(self, '_postprocess') and callable(self._postprocess):
          return self._postprocess(value)
        return value
    else:
      return None

  def __set__(self, inst, value):
    'Set the value handling the special NUL value'
    assert self._offset >= 0, 'Collumn offset not known'
    if hasattr(self, '_preprocess') and callable(self._preprocess):
      value = self._preprocess(value)
    if value is None:
      for off in range(self._offset, self_offset + self._size):
        inst._buffer[off] = 0xFF;
    else:
      self._struct.pack_into(inst._buffer, self._offset, value)

class FloatColumn(_RealMixin):
  __slots__ = ()
  _struct = struct.Struct('=f')
  _type = ColumnType.FLOAT
  _template = 'float {0.name};'

class DoubleColumn(_RealMixin):
  __slots__ = ()
  _struct = struct.Struct('=d')
  _type = ColumnType.DOUBLE
  _template = 'double {0.name};'

class MoneyColumn(DoubleColumn):
  __slots__ = ()

class ISAMrecordBase:
  '''Base class providing access to the current record providing access to the
     columns as attributes where each column is implemented by a descriptor
     object which makes the conversion to/from the underlying raw buffer directly.'''
  # Provide information for static type analysis
  _fields: dict[str, ColumnInfo]
  _recsize: int
  
  def __init__(self, recname, fields=None):
    # NOTE:
    # The passing of a 'fields' keyword permits the tuple that represents a record to an
    # application to have fewer fields, or have the fields in a different order, than the
    # actual underlying record in the ISAM table, but does not prevent the actual access
    # to all the fields available.
    if fields is None:
      tupfields = self._fields.keys()
    elif isinstance(fields, (str, list, tuple)):
      tupfields = fields
    else:
      raise TypeError('Unhandled type of fields presented')
    if not tupfields:
      raise TypeError('Provided fields produces no suitable columns to use')
    self._namedtuple = collections.namedtuple(recname, tupfields)
    self._buffer = None
  
  def __getitem__(self, fld):
    'Return the current value of the given item'
    if isinstance(fld, int):
      return getattr(self, self._namedtuple._fields[fld].name)
    elif isinstance(fld, ColumnInfo):
      return getattr(self, fld.name)
    elif fld in self._namedtuple._fields:
      return getattr(self, fld)
    else:
      raise ValueError(f"Unhandled field '{fld}'")

  def __setitem__(self, fld, value):
    'Set the current value of a field to the given value'
    if isinstance(fld, int):
      setattr(self, self._namedtuple._fields[fld], value)
    elif isinstance(fld, ColumnInfo):
      setattr(self, fld.name, value)
    elif fld in self._namedtuple._fields:
      setattr(self, fld, value)
    else:
      raise ValueError(f"Unhandled field '{fld}'")

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
    return [getattr(self, fld) for fld in self._namedtuple._fields]

  def _set_value(self, *args, **kwd):
    'Set the record area to the given KWD or ARGS'
    if kwd:
      for fld in self._namedtuple._fields:
        setattr(self, fld, kwd[fld])
    else:
      for num, fld in enumerate(self._namedtuple._fields):
        setattr(self, fld, args[num])

  def __str__(self):
    'Return the current values as a string'
    fldval = []
    for fld in self._namedtuple._fields:
      if self._fields[fld].type == ColumnType.CHAR:
        fldval.append(f"{fld}='{getattr(self, fld)}'")
      else:
        fldval.append(f'{fld}={getattr(self, fld)}')
    return f'{self.__class__.__name__}({", ".join(fldval)})'

# Define the templates used to generate the record definition class at runtime,
# these will be passed through the 'format' function.
_record_class = 'class {rec_name}(ISAMrecordBase):\n  __slots__ = ()\n{fld_defn}\n'
_record_field = '  {name} = {klassname}({defn})'

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

@functools.cache
def create_record_class(tabdefn, recname, keepsrc, **kwd):
  'Create a new class for the given table definition'
  # Produce a record name if none provided
  if recname is None:
    fqname = [getattr(tabdefn, '_database', None),
              getattr(tabdefn, '_prefix',   None),
              kwd.get('idname', getattr(tabdefn, '_tabname', None))]
    recname = '_'.join([x for x in fqname if x is not None])

  # Retrieve the field information from the table definition
  seen, fdefn = set(), []
  if isinstance(tabdefn._columns, (collections.OrderedDict, dict)):
    flds = list(tabdefn._columns.values())
  elif isinstance(tabdefn._columns, (list, tuple)):
    flds = tabdefn._columns
  else:
    raise ValueError('Unhandled column information encountered')

  # Check the fields defined on the table definition
  for fld in flds:
    fldname = fld.name
    klassname = fld.__class__.__name__

    # Validate the field name using a similar rule to that in collections.namedtuple
    if fldname in seen:
      raise NameError(f"Duplicate '{fldname}'")
    elif not fldname.isidentifier() or iskeyword(fldname) or fldname.startswith('_'):
      raise NameError(f"Field '{fldname}'")
    seen.add(fldname)

    # Generate the template for the particular type of field
    fld_template = _record_namespace[klassname]._template.format(fld)

    # Create the field template (the offset is calculated by the metaclass)
    fdefn.append(_record_field.format(name=fldname, klassname=klassname, defn=fld_template))

  # Provide the record template
  record_definition = _record_class.format(rec_name=recname, fld_defn='\n'.join(fdefn))

  # Execute in a temporary namespace that also includes the ISAMrecordBase object, 
  # and column objects used in the new object with additional tracing utilities
  namespace = {
    '__name__' : recname,
  } 
  namespace.update(_record_namespace)
  exec(record_definition, namespace)
  result = namespace[recname]
  if keepsrc:
    result._source = record_definition
  return result

def _recordclass33(tabdefn, recname, keepsrc, **kwd):
  'Create a new class for the given table definition using types.new_class'
  seen, fdefn = set(), list()
  if isinstance(tabdefn._columns, (collections.OrderedDict, dict)):
    flds = list(tabdefn._columns.values())
  elif isinstance(tabdefn._columns, (list,tuple)):
    flds = tabdefn._columns
  else:
    raise ValueError('Unhandled column information encountered')
  for fld in flds:
    if fld.name in seen:
      raise NameError(f"Duplicate '{fld.name}'")
    elif not fld.name.isidentifier() or iskeyword(fld.name) or fld.name.startwith('_'):
      raise NameError(f"Field '{fld.name}' is not a valid name")
    seen.add(fld.name)
  
  def _recordclass_cb(ns):
    'Callback function to fill in the namespace of the newly created class'

