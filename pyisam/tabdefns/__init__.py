'''
This subpackage contains the classes to provide the definition of an ISAM
table including its columns and indexes, these are separate from the table
logic to permit alternative implementations to be provided and reduce the
complexity of the table module.
'''

import collections

__all__ = ('CharColumn', 'TextColumn', 'ShortColumn', 'LongColumn', 
           'FloatColumn', 'DoubleColumn', 'DuplicateIndex', 'UniqueIndex',
           'PrimaryIndex', 'AscDuplicateIndex', 'AscUniqueIndex',
           'AscPrimaryIndex', 'DescDuplicateIndex', 'DescUniqueIndex',
           'DescPrimaryIndex', 'TableDefnMeta')

class TableDefnMeta(type):
  'Metaclass for table definition classes that remembers the order of fields and indexes'
  @classmethod
  def __prepare__(metacls, name, bases, **kwds):
    return collections.OrderedDict()
  def __new__(cls, name, bases, namespace, **kwds):
    result = super().__new__(cls, name, bases, dict(namespace))
    fields, indexes = [], []
    if '_columns_' in namespace:
      # Fields given in _columns_ override the attribute variants
      for info in namespace._columns_:
        fields.append(info._name)
    if '_indexes_' in namespace:
      # Indexes given in _indexes_ override the attribute variants
      for info in namespace._indexes_:
        indexes.append(info._name)
    for name, info in namespace.items():
      # Check if a dunder ('__X__') name has been given
      if (name[:2] == '__' and name[-2:] == '__' and
          name[2:3] != '_' and name[-3:-2] != '_' and
          len(name) > 4):
        continue
      # Check if a sunder ('_X_') name has been given
      if (name[:1] == '_' and name[-1:] == '_' and
          name[1:2] != '_' and name[-2:-1] != '_' and
          len(name) > 2):
        continue
      # Check if an underscore name has been given
      if (name[:1] == '_' and name[1:2] != '_' and len(name) > 1):
        continue
      # Store as a field or indexes depending on the base type
      if issubclass(info, TableDefnColumn):
        col = info()
        if col.name in fields:
          raise NameError('Duplicate field {} defined'.format(col.name))
        fields.append(col.name)
      elif isinstance(info, TableDefnColumn):
        if name in fields:
          raise NameError('Duplicate field {} defined'.format(name))
        fields.append(name)
      elif issubclass(info, TableDefnIndex):
        idx = info()
        if idx.name in indexes:
          raise NameError('Duplicate index {} defined'.format(idx.name))
        indexes.append(idx.name)
      elif isinstance(info, TableDefnIndex):
        if name in indexes:
          raise NameError('Duplicate index {} defined'.format(name))
        indexes.append(name)
    result._fields_ = tuple(fields)
    result._indexes_ = tuple(indexes)
    for name in ('_tabname_', '_prefix_', '_database_'):
      value = getattr(namespace, name, None)
      if value is not None:
        setattr(result, name, value)
    return result

class TableDefnColumn:
  'Base class for all the column based classes used in definitions'
  def __init__(self, name, *args, **kwd):
    self.name = name
class CharColumn(TableDefnColumn):
  pass
class TextColumn(TableDefnColumn):
  def __init__(self, name, length, *args, **kwd):
    self.length = length
    super().__init__(name, *args, **kwd)
class ShortColumn(TableDefnColumn):
  pass
class LongColumn(TableDefnColumn):
  pass
class FloatColumn(TableDefnColumn):
  pass
class DoubleColumn(TableDefnColumn):
  pass

class TableIndexCol:
  'This class represents a column within an index definition'
  __slots__ = 'name', 'offset', 'length'
  def __init__(self, name, offset=None, length=None):
    self.name = name
    self.offset = offset
    self.length = length
  def __str__(self):
    out = ['TableIndexCol({0.name}']
    if self.length is not None:
      out.append(', 0' if self.offset is None else ', {0.offset}')
      out.append(', {0.length}')
    elif self.offset is not None:
      out.append(', {0.offset}')
    out.append(')')
    return ''.join(out).format(self)

class TableDefnIndex:
  '''This class provides the base implementation of an index for a table,
     an index is represented by a name, a flag whether duplicates are allowed,
     an optional list of TableindexCol instances which define the columns within
     the index, and an optional flag whether the index is descending instead of
     ascending.'''
  __slots__ = 'name', 'dups', 'desc', 'colinfo'
  def __init__(self, name, *colinfo, dups=True, desc=False):
    self.name = name
    self.dups = dups
    self.desc = desc
    self.colinfo = self._colinfo(colinfo)
  def _colinfo(self, colinfo):
    'Internal method to convert colinfo into a suitable class'
    if len(colinfo) < 1:
      return TableIndexCol(self.name)
    elif len(colinfo) == 1:
      if isinstance(colinfo[0], str):
        return TableIndexCol(colinfo[0])
      elif isinstance(colinfo[0], TableIndexCol):
        return colinfo[0]
      elif isinstance(colinfo[0], (list, tuple)):
        return self._colinfo(colinfo[0])
      else:
        raise ValueError('Column definition is not correctly formatted')
    else:
      newinfo = list()
      for col in colinfo:
        if isinstance(col, str):
          newinfo.append(TableIndexCol(col))
        elif isinstance(col, (tuple, list)):
          if len(col) < 2:
            newinfo.append(TableIndexCol(col[0]))
          elif len(col) < 3:
            newinfo.append(TableIndexCol(col[0], col[1]))
          elif len(col) < 4:
            newinfo.append(TableIndexCol(col[0], col[1], col[2]))
          else:
            raise ValueError('Column definition is not correctly formatted')
        elif isinstance(col, TableIndexCol):
          newinfo.append(col)
      return newinfo
  def __str__(self):
    'Provide a readable form of the information in the index'
    out = ['{}({}'.format(self.__class__.__name__, self.name)]
    if isinstance(self.colinfo, TableIndexCol):
      out.append('{}'.format(self.colinfo))
    else:
      for cinfo in self.colinfo:
        out.append('{}'.format(cinfo))
    out.append('dups={0.dups}, desc={0.desc})'.format(self))
    return ', '.join(out)

# Provide an easier to read way of identifying the various key types
class DuplicateIndex(TableDefnIndex):
  __slots__ = ()
  def __init__(self, name, *colinfo, desc=False):
    super().__init__(name, *colinfo, dups=True, desc=desc)
class UniqueIndex(TableDefnIndex):
  __slots__ = ()
  def __init__(self, name, *colinfo, desc=False):
    super().__init__(name, *colinfo, dups=False, desc=desc)
class PrimaryIndex(UniqueIndex):
  __slots__ = ()
  pass
class AscDuplicateIndex(DuplicateIndex):
  __slots__ = ()
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=False)
class AscUniqueIndex(UniqueIndex):
  __slots__ = ()
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=False)
class AscPrimaryIndex(AscUniqueIndex):
  __slots__ = ()
  pass
class DescDuplicateIndex(DuplicateIndex):
  __slots__ = ()
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=True)
class DescUniqueIndex(UniqueIndex):
  __slots__ = ()
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=True)
class DescPrimaryIndex(DescUniqueIndex):
  __slots__ = ()
  pass
