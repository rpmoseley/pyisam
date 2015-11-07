'''
This subpackage contains the classes to provide the definition of an ISAM
table including its columns and indexes, these are separate from the table
logic to permit alternative implementations to be provided and reduce the
complexity of the table module.
'''

__all__ = ('CharColumnDef', 'TextColumnDef', 'ShortColumnDef',
           'LongColumnDef', 'FloatColumnDef', 'DoubleColumnDef',
           'DuplicateIndexDef', 'UniqueIndexDef', 'PrimaryIndexDef',
           'AscDuplicateIndexDef', 'AscUniqueIndexDef', 'AscPrimaryIndexDef',
           'DescDuplicateIndexDef', 'DescUniqueIndexDef', 'DescPrimaryIndexDef')

import enum

class ColumnType(enum.Enum):
  CHAR = 0
  SHORT = 1
  LONG = 2
  DOUBLE = 3
  FLOAT = 4

class Column:
  'Base class providing the shared functionality for columns'
  __slots__ = ('_name_', '_type_')
  def __init__(self, name):
    self._name_ = name
    if not hasattr(self, '_type_'):
      raise ValueError('No type provided for column')

class CharColumnDef(Column):
  __slots__ = ()
  _type_ = ColumnType.CHAR

class TextColumnDef(Column):
  __slots__ = ('_size_',)
  _type_ = ColumnType.CHAR
  def __init__(self, name, length):
    if length <= 0: raise ValueError("Must provide a positive length")
    self._name_ = name
    self._size_ = length

class ShortColumnDef(Column):
  __slots__ = ()
  _type_ = ColumnType.SHORT

class LongColumnDef(Column):
  __slots__ = ()
  _type_ = ColumnType.LONG

class FloatColumnDef(Column):
  __slots__ = ()
  _type_ = ColumnType.FLOAT

class DoubleColumnDef(Column):
  __slots__ = ()
  _type_ = ColumnType.DOUBLE

class TableIndexCol:
  'This class represents a column within an index definition'
  def __init__(self, name, offset=None, length=None):
    self.name = name
    self.offset = offset
    self.length = length
  def __str__(self):
    if self.offset is None and self.length is None:
      return 'TableIndexCol({0.name})'.format(self)
    elif self.length is None:
      return 'TableIndexCol({0.name}, {0.offset})'.format(self)
    elif self.offset is None:
      return 'TableIndexCol({0.name}, 0, {0.length})'.format(self)
    else:
      return 'TableIndexCol({0.name}, {0.offset}, {0.length})'.format(self)

class TableIndexDef:
  '''This class provides the base implementation of an index for a table,
     an index is represented by a name, a flag whether duplicates are allowed,
     an optional list of ISAMindexCol instances which define the columns within
     the index, and an optional flag whether the index is descending instead of
     ascending.'''
  __slots__ = ('_name_', '_dups_', '_desc_', '_colinfo_')
  def __init__(self, name, *colinfo, dups=True, desc=False):
    self._name_ = name
    self._dups_ = dups
    self._desc_ = desc
    if len(colinfo) < 1:
      self._colinfo_ = TableIndexCol(name)
    elif len(colinfo) == 1:
      if isinstance(colinfo[0], str):
        self._colinfo_ = TableIndexCol(colinfo[0])
      elif isinstance(colinfo[0], TableIndexCol):
        self._colinfo_ = colinfo[0]
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
      self._colinfo_ = newinfo
  def __str__(self):
    'Provide a readable form of the information in the index'
    out = ['{}({},'.format(self.__class__.__name__, self.name)]
    if isinstance(self._colinfo_, TableIndexCol):
      out.append('{},'.format(self._colinfo_))
    else:
      for cinfo in self._colinfo_:
        out.append('{},'.format(cinfo))
    out.append('dups={0._dups_}, desc={0._desc_})'.format(self))
    return ' '.join(out)

# Provide an easier to read way of identifying the various key types
class DuplicateIndexDef(TableIndexDef):
  def __init__(self, name, *colinfo, desc=False):
    super().__init__(name, *colinfo, dups=True, desc=desc)
class UniqueIndexDef(TableIndexDef):
  def __init__(self, name, *colinfo, desc=False):
    super().__init__(name, *colinfo, dups=False, desc=desc)
class PrimaryIndexDef(UniqueIndexDef):
  pass
class AscDuplicateIndexDef(DuplicateIndexDef):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=False)
class AscUniqueIndexDef(UniqueIndexDef):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=False)
class AscPrimaryIndexDef(AscUniqueIndexDef):
  pass
class DescDuplicateIndexDef(DuplicateIndexDef):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=True)
class DescUniqueIndexDef(UniqueIndexDef):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=True)
class DescPrimaryIndexDef(DescUniqueIndexDef):
  pass
