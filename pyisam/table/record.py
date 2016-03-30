'''
This module provides an object representing the ISAM record where the fields can be
accessed as both attributes as well as by number, the underlying object is an instance
of collections.namedtuple.

Typical usage of a fixed definition using the ISAMrecordMixin is as follows:

class DEFILErecord(ISAMrecordMixin):
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
  __slots__ = '_struct', '_size', '_offset', '_nullval'
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
  def _template(self):
    return '{}()'.format(self.__class__.__name__)
  def __str__(self):
    return '({0._offset}, {0._size})'.format(self)
  def __get__(self, inst, objtype):
    if self._offset < 0:
      raise ValueError('Column offset not known')
    return self._struct.unpack_from(inst._buffer, self._offset)[0]
  def __set__(self, inst, value):
    if value is None:
      if hasattr(self, '_nullval'):
        value = self._nullval
      elif hasattr(self, '_convnull'):
        value = self._convnull()
    self._struct.pack_into(inst._buffer, self._offset, value)

class CharColumn(_BaseColumn):
  __slots__ = ()
  _struct = struct.Struct('c')
  _nullval = b' '
  _type = ColumnType.CHAR
  def __get__(self, obj, objtype):
    return super().__get__(obj, objtype).replace(b'\x00', b' ').decode('utf-8').rstrip()
  def __set__(self, obj, value):
    super().__set__(obj, value.encode('utf-8').replace(b'\x00', b' '))

class TextColumn(_BaseColumn):
  __slots__ = ()
  _type = ColumnType.CHAR
  def __init__(self, length, size=None, offset=-1):
    if length <= 0:
      raise ValueError("Must provide a positive length")
    self._struct = struct.Struct('{}s'.format(length))
    super().__init__(size, offset)
    self._nullval = b' ' * self._size 
  def _template(self):
    return '{0.__class__.__name__}({0._size})'.format(self)
  def __get__(self, inst, objtype):
    return super().__get__(inst, objtype).replace(b'\x00', b' ').decode('utf-8').rstrip()
  def __set__(self, inst, value):
    if value is None:
      value = self._nullval
    else:
      value = value.encode('utf-8')
      if self._size < len(value):
        value = value[:self._size]
      elif len(value) < self._size:
        value += self._nullval[:self._size - len(value)]
    super().__set__(inst, value.replace(b'\x00', b' '))

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

# Create a special tuple for storing the column information avoiding the
# descriptor lookup that would otherwise occur
ColumnInfo = collections.namedtuple('ColumnInfo', 'name offset size type')

# Create a variation of a tuple for the list of fields to permit comparing
# using the field name alone without regard to the fact the elements of the
# sequence are instances of ColumnInfo
class _FieldTuple(tuple):
  def __contains__(self, value):
    for info in self:
      if info.name == value:
        return True
    return False

class _ISAMrecordMeta(type):
  '''Metaclass providing the special requirements of an ISAMrecord which includes
     producing a list of the fields to maintian their order, plus the ability to
     return the actual column information without retrieving the underlying data'''
  @classmethod
  def __prepare__(metacls, name, bases, **kwds):
    return collections.OrderedDict()
  def __new__(cls, name, bases, namespace, **kwds):
    result = super().__new__(cls, name, bases, dict(namespace))

    # Don't process any further if the object being created is the Mixin class itself
    if name == 'ISAMrecordMixin':
      return result

    # Process the field names to ensure they do not cause issues and also calculate the
    # offset for each field within the record buffer area.
    fields, curoff = [], 0
    for name, info in namespace.items():
      # Check if a dunder ('__X__') function
      if (name[:2] == '__' and name[-2:] == '__' and
          name[2:3] != '_' and name[-3:-2] != '_' and
          len(name) > 4):
        continue
      # Check if the name begins with an underscore
      if (name[:1] == '_' and name[1:2] != '_' and len(name) > 1):
        continue
      info._offset = curoff
      curoff += info._size
      fields.append(ColumnInfo(name, info._offset, info._size, info._type))
    result._fields = _FieldTuple(fields)
    result._recsize = curoff
    return result

class ISAMrecordMixin(metaclass=_ISAMrecordMeta):
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
      tupfields = [fld for fld in kwd_fields if fld in self._fields]
      if len(tupfields) < 1:
        tupfields = self._fields[0].name
      #D#print('TUP:', kwd_fields, '->', tupfields) # DEBUG
    else:
      tupfields = [fld.name for fld in self._fields]
      #D#print('TUP:', tupfields) # DEBUG
    self._namedtuple = collections.namedtuple(recname, tupfields)

    # Create a record buffer which will produce suitable instances for this table when
    # called, this provides a means of modifying the buffer implementation without
    # affecting the actual package usage.
    self.record = RecordBuffer(self._recsize if recsize is None else recsize)
    self._buffer = self.record()
  def __getitem__(self, fld):
    'Return the current value of the given item'
    if isinstance(fld, int):
      return getattr(self, self._fields[fld].name)
    elif isinstance(fld, ColumnInfo):
      return getattr(self, fld.name)
    elif fld in self._fields:
      return getattr(self, fld)
    else:
      raise ValueError("Unhandled field '{}'".format(fld))
  def __setitem__(self, fld, value):
    'Set the current value of a field to the given value'
    if isinstance(fld, int):
      setattr(self, self._fields[fld], value)
    elif isinstance(fld, ColumnInfo):
      setattr(self, fld.name, value)
    elif fld in self._fields:
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
    for fld in self._namedtuple._fields:
      if hasattr(fld, 'name'):
        fldval.append('{}={}'.format(fld.name, getattr(self, fld.name)))
      else:
        fldval.append('{}={}'.format(fld, getattr(self, fld)))
    return ''.join(('{}({})'.format(self.__class__.__name__, ', '.join(fldval))))
  #1#def __bytes__(self):
  #1#  if hasattr(self._buffer, 'raw'):
  #1#    return self._buffer.raw  # NOTE: This assumes that the interface is meant for providing a raw view of record
  #1#  else:
  #1#    print('BUF:', str(self._buffer))
  #1#    return self._buffer.__str__()
  @property
  def cur_value(self):
    'Return the current values of all fields in the record'
    return [getattr(self, fld.name) for fld in self._fields]
  def colinfo(self, colname):
    'Return the field information for the given COLNAME'
    for info in self._fields:
      if info.name == colname:
        return info
    return None

# Define the templates used to generate the record definition class at runtime
_record_class_template = """\
class {rec_name}(ISAMrecordMixin):
{fld_defn}
"""
_record_field_template = """\
  {name} = {defn}
"""

# Define the mapping from column name to descriptor objects
_column_mapping = {
  'CharColumn'   : CharColumn,
  'TextColumn'   : TextColumn,
  'ShortColumn'  : ShortColumn,
  'LongColumn'   : LongColumn,
  'FloatColumn'  : FloatColumn,
  'DoubleColumn' : DoubleColumn
}

def recordclass(tabdefn, recname=None, verbose=False, *args, **kwd):
  'Create a new class for the given table definition'
  seen, fname, fdefn = set(), [], []
  for fld in tabdefn._columns_:
    fldname = fld.name

    # Validate the field name using a similar rule to that in collections.namedtuple
    if not fldname.isidentifier() or iskeyword(fldname) or fldname.startswith('_'):
      raise NameError("Field '{}' is not a valid identifier".format(fldname))
    elif fldname in seen:
      raise NameError("Field '{}' already present in definition".format(fldname))
    seen.add(fldname)
    fname.append(fldname)

    # Calculate the field offset in the record buffer
    klassobj = _column_mapping[fld.__class__.__name__]
    klass = klassobj(length=fld.length) if hasattr(fld, 'length') else klassobj()

    # Create the field template with the calculated offset
    fdefn.append(_record_field_template.format(name=fldname, defn=klass._template()))

  # Provide the record template
  if recname is None:
    fqname = [tabdefn._database_, tabdefn._prefix_, kwd.get('idname', None)]
    recname = '_'.join(filter(None, fqname))
  else:
    # TODO: Check the given record name is valid
    pass
  record_name = 'Record_{name}'.format(name=recname)
  record_definition = _record_class_template.format(rec_name=record_name,
                                                    fld_defn=''.join(fdefn))

  # Execute in a temporary namespace that also includes the ISAMrecordMixin object, 
  # and column objects used in the new object with additional tracing utilities
  namespace = {
    '__name__'        : record_name,
    'ISAMrecordMixin' : ISAMrecordMixin,
    'CharColumn'      : CharColumn,
    'TextColumn'      : TextColumn,
    'ShortColumn'     : ShortColumn,
    'LongColumn'      : LongColumn,
    'FloatColumn'     : FloatColumn,
    'DoubleColumn'    : DoubleColumn
  } 
  exec(record_definition, namespace)
  result = namespace[record_name]
  # TODO Be compatible with collections.namedtuple: result._source = record_definition
  if verbose:
    print(record_definition)
  return result
      
if __name__ == '__main__':
  class DECOMPrecM(ISAMrecordMixin):
    # NOTE The following line commented out is now populated by the metaclass
    #_fields = ('comp', 'comptyp', 'sys', 'prefix', 'user', 'database', 'release', 'timeup', 'specup')
    comp = TextColumn(9)
    comptyp = CharColumn()
    sys = TextColumn(9)
    prefix = TextColumn(5)
    user = TextColumn(4)
    database = TextColumn(6)
    release = TextColumn(5)
    timeup = LongColumn()
    specup = LongColumn()

  from ..tabdefns.stxtables import DECOMPdefn
  DECOMPrec = recordclass(DECOMPdefn)
  DR = DECOMPrec('decomp',DECOMPrec._recsize,fields='comptyp comp nonexistant')
  DRM = DECOMPrecM('decompM',47)
  print(DRM, DRM._fields, sep='\n')
  print(dir(DRM))
  print(DR.timeup, DR[7], DR['timeup'])
  print(DR)
  DR.specup = 100000
  print(DR.specup, DR[8], DR['specup'])
  print(DR, DR.timeup)
