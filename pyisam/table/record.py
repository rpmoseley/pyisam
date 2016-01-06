'''
This module provides an object representing the ISAM record where the fields can be
accessed as both attributes as well as by number, the underlying object is an instance
of collections.namedtuple.

Typical usage of a fixed definition using the ISAMrecordMixin is as follows:

class DEFILErecord(ISAMrecordMixin):
  __slots__ = ()
  _fields = ('filename', 'seq', 'field', 'refptr', 'type', 'size', 'keytype', 'vseq', 'stype', 'scode', 'fgroup', 'idxflag')
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
from keyword import iskeyword
from ..backend import RecordBuffer

__all__ = 'CharColumn', 'TextColumn', 'ShortColumn', 'LongColumn', 'FloatColumn', 'DoubleColumn', 'recordclass'

class ColumnBase:
  'Base class providing the shared functionality for columns'
  __slots__ = '_struct', '_size', '_offset', '_nullval'
  def __init__(self, size=None, offset=-1):
    if not hasattr(self, '_struct'):
      raise ValueError('No struct.struct object provided for column')
    if not hasattr(self, '_size'):
      if hasattr(self._struct, 'size'):
        self._size = self._struct.size
      elif isinstance(size, int):
        self._size = size
      else:
        raise ValueError('Must provide a size for column {}'.format(self._name_))
    self._offset = offset          # Offset into record buffer
  def __get__(self, inst, objtype):
    if self._offset < 0 and hasattr(inst, '_curoff'):
      self._offset = inst._curoff
      inst._curoff += self._size
    return self._struct.unpack_from(inst._buffer, self._offset)[0]
  def __set__(self, inst, value):
    if value is None:
      if hasattr(self, '_nullval'):
        value = self._nullval
      elif hasattr(self, '_convnull'):
        value = self._convnull()
    self._struct.pack_into(inst._buffer, self._offset, value)
  def __str__(self):
    return '({0._offset}, {0._size})'.format(self)

class CharColumn(ColumnBase):
  __slots__ = ()
  _struct = struct.Struct('c')
  _nullval = b' '
  def __get__(self, obj, objtype):
    return super().__get__(obj, objtype).replace(b'\x00', b' ').decode().rstrip()
  def __set__(self, obj, value):
    super().__set__(obj, value.encode().replace(b'\x00', b' '))

class TextColumn(ColumnBase):
  __slots__ = ()
  def __init__(self, length, size=None, offset=-1):
    if length <= 0: raise ValueError("Must provide a positive length")
    self._struct = struct.Struct('{}s'.format(length))
    super().__init__(size, offset)
    self._nullval = b' ' * self._size 
  def __get__(self, inst, objtype):
    return super().__get__(inst, objtype).replace(b'\x00', b' ').decode().rstrip()
  def __set__(self, inst, value):
    if value is not None:
      value = value.encode()
      if self._size < len(value):
        value = value[:self._size]
      elif len(value) < self._size_:
        value += self._nullvabl[:self._size_ - len(value)]
    super().__set__(inst, value.replace(b'\x00', b' '))

class ShortColumn(ColumnBase):
  __slots__ = ()
  _struct = struct.Struct('>h')
  _nullval = 0

class LongColumn(ColumnBase):
  __slots__ = ()
  _struct = struct.Struct('>l')
  _nullval = 0

class FloatColumn(ColumnBase):
  __slots__ = ()
  _struct = struct.Struct('=f')
  _nullval = 0.0

class DoubleColumn(ColumnBase):
  __slots__ = ()
  _struct = struct.Struct('=d')
  _nullval = 0.0

class ISAMrecordMixin:
  '''Mixin class providing access to the current record providing access to the
     columns as attributes where each column is implemented by a descriptor
     object which makes the conversion to/from the underlying raw buffer directly.'''
  # IGNORED since __dict__ still created: __slots__ = '_buffer', '_namedtuple', '_curoff'
  def __init__(self, recname, recsize, **kwd):
    # NOTE:
    # The passing of a 'fields' keyword permits the tuple that represents a record to an
    # application to have fewer fields than the actual underlying record in the ISAM table
    # but does not prevent the actual access to all the fields available.
    if 'fields' in kwd:
      kwd_fields = kwd['fields']
      if isinstance(kwd_fields, str):
        kwd_fields = kwd_fields.replace(',', ' ').split()
      tupfields = [fld for fld in kwd_fields if fld in self._fields]
      if len(tupfields) < 1:
        tupfields = self._fields
      print('TUP:',kwd_fields,'->',tupfields)
    else:
      tupfields = self._fields
    self._namedtuple = collections.namedtuple(recname, tupfields)
    self._buffer = RecordBuffer(recsize)

    # Calculate the offsets to the fields within the record
    self._curoff = 0
    for fld in self._fields:
      getattr(self, fld)
    del self._curoff
  def __getitem__(self, fld):
    'Return the current value of the given item'
    if isinstance(fld, int):
      return getattr(self, self._fields[fld])
    elif fld in self._fields:
      return getattr(self, fld)
    else:
      raise ValueError("Unhandled field '{}'".format(fld))
  def __setitem__(self, fld, value):
    'Set the current value of a field to the given value'
    if isinstance(fld, int):
      setattr(self, self._fields[fld], value)
    elif fld in self._fields:
      setattr(self, fld, value)
    else:
      raise ValueError("Unhandled field '{}'".format(fld))
  def __call__(self):
    'Return an instance of the namedtuple for the current column values'
    return self._namedtuple._make(getattr(self, name) for name in self._namedtuple._fields)
  def __bytes__(self):
    if hasattr(self._buffer, 'raw'):
      return self._buffer.raw  # NOTE: This assumes that the interface is meant for providing a raw view of record
    else:
      return self._buffer
  property
  def cur_value(self):
    return [getattr(self, name) for name in self._fields]

# Define the templates used to generate the record definition class at runtime
_record_class_template = """\
class Record_{name}(ISAMrecordMixin):
  #__slots__ = ()
  _fields = {fields!r}
{fld_defn}
"""
_record_field_template = """\
  {name} = {defn}
"""
def recordclass(tabdefn, recname=None, verbose=False, *args, **kwd):
  # Validate the field names, the rules are the same as for collections.namedtuple
  seen, names, defn = set(), [], []
  for fld in tabdefn._columns_:
    fldname = fld._name
    if (not fldname.isidentifier()
        or  iskeyword(fldname) 
        or  fldname.startswith('_')):
      raise ValueError("Field '{}' is not a valid identifier".format(fldname))
    elif fldname in seen:
      raise ValueError("Field '{}' already present in definition".format(fldname))
    seen.add(fldname)
    names.append(fldname)
    defn.append(_record_field_template.format(name = fldname, defn = fld._template()))

  # Provide the record template
  if recname is None:
    fqname = [tabdefn._database_, tabdefn._prefix_, kwd.get('idname', None)]
    recname = '_'.join(filter(None, fqname))
  klassname = 'Record_{name}'.format(name=recname)
  record_definition = _record_class_template.format(name = recname, fields = tuple(names), fld_defn = ''.join(defn))

  # Execute in a temporary namespace that also includes the ISAMrecordMixin object, 
  # and column objects used in the new object with additional tracing utilities
  namespace = {
    '__name__' : klassname,
    'ISAMrecordMixin' : ISAMrecordMixin,
    'CharColumn' : CharColumn,
    'TextColumn' : TextColumn,
    'ShortColumn' : ShortColumn,
    'LongColumn' : LongColumn,
    'FloatColumn' : FloatColumn,
    'DoubleColumn' : DoubleColumn
  }
  exec(record_definition, namespace)
  result = namespace[klassname]
  # TODO Be compatible with collections.namedtuple: result._source = record_definition
  if verbose:
    print(record_definition)
  return result
      
if __name__ == '__main__':
  class DECOMPrecM(ISAMrecordMixin):
    __slots__ = ()
    _fields = ('comp', 'comptyp', 'sys', 'prefix', 'user', 'database', 'release', 'timeup', 'specup')
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
  DR = DECOMPrec('decomp',49,fields='comptyp comp nonexistant')
  DRM = DECOMPrecM('decompM',49)
  print(DR.timeup, DR[7], DR['timeup'])
  print(bytes(DR))
  DR.specup = 100000
  print(DR.specup, DR[8], DR['specup'])
  print(bytes(DR))
  print(DR(),DR.timeup)
