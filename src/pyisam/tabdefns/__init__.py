'''
This subpackage contains the classes to provide the definition of an ISAM
table including its columns and indexes, these are separate from the table
logic to permit alternative implementations to be provided and reduce the
complexity of the table module.
'''

import dataclasses

__all__ = ('CharColumn', 'TextColumn', 'ShortColumn', 'LongColumn',
           'FloatColumn', 'DoubleColumn', 'DateColumn', 'SerialColumn',
           'DuplicateIndex', 'UniqueIndex', 'PrimaryIndex',
           'AscDuplicateIndex', 'AscUniqueIndex',
           'AscPrimaryIndex', 'DescDuplicateIndex', 'DescUniqueIndex',
           'DescPrimaryIndex', 'RecordOrderIndex')

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

class DateColumn(LongColumn):
  pass

class SerialColumn(LongColumn):
  pass 

class FloatColumn(TableDefnColumn):
  pass

class DoubleColumn(TableDefnColumn):
  pass

class MoneyColumn(DoubleColumn):
  pass

@dataclasses.dataclass
class TableDefnIndexCol:
  'This class represents a column within an index definition'
  name: str          # Name of column within index
  offset: int = None # Offset of column within index
  length: int = None # Length of column within index

  def __str__(self):
    out = ['TableDefnIndexCol({0.name}']
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
      return TableDefnIndexCol(self.name)
    elif len(colinfo) == 1:
      if isinstance(colinfo[0], str):
        return TableDefnIndexCol(colinfo[0])
      elif isinstance(colinfo[0], TableDefnIndexCol):
        return colinfo[0]
      elif isinstance(colinfo[0], (list, tuple)):
        return self._colinfo(colinfo[0])
      else:
        raise ValueError('Column definition is not correctly formatted')
    else:
      newinfo = list()
      for col in colinfo:
        if isinstance(col, str):
          newinfo.append(TableDefnIndexCol(col))
        elif isinstance(col, (tuple, list)):
          if len(col) < 2:
            newinfo.append(TableDefnIndexCol(col[0]))
          elif len(col) < 3:
            newinfo.append(TableDefnIndexCol(col[0], col[1]))
          elif len(col) < 4:
            newinfo.append(TableDefnIndexCol(col[0], col[1], col[2]))
          else:
            raise ValueError('Column definition is not correctly formatted')
        elif isinstance(col, TableDefnIndexCol):
          newinfo.append(col)
      return newinfo

  def __str__(self):
    'Provide a readable form of the information in the index'
    out = ['{}({}'.format(self.__class__.__name__, self.name)]
    if isinstance(self.colinfo, TableDefnIndexCol):
      out.append('{}'.format(self.colinfo))
    else:
      for cinfo in self.colinfo:
        out.append('{}'.format(cinfo))
    out.append('dups={0.dups}, desc={0.desc})'.format(self))
    return ', '.join(out)

# Provide a unique index that enables access by way of the record number
class RecordOrderIndex(TableDefnIndex):
  __slots__ = ()
  def __init__(self, name):
    self.name = name
    self.dups = False
    self.desc = False
    self.colinfo = list()

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
