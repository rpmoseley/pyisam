'''
This subpackage contains the classes to provide the definition of an ISAM
table including its columns and indexes, these are separate from the table
logic to permit alternative implementations to be provided and reduce the
complexity of the table module.
'''

__all__ = ('CharColumn', 'TextColumn', 'ShortColumn', 'LongColumn', 
           'FloatColumn', 'DoubleColumn', 'DuplicateIndex', 'UniqueIndex',
           'PrimaryIndex', 'AscDuplicateIndex', 'AscUniqueIndex',
           'AscPrimaryIndex', 'DescDuplicateIndex', 'DescUniqueIndex',
           'DescPrimaryIndex')

class TableColumn:
  'Base class for all the column based classes used in definitions'
  def __init__(self, name, *args, **kwd):
    self._name = name
  def _template(self, arg=None):
    return '{0.__class__.__name__}({1})'.format(self, '' if arg is None else arg)
class CharColumn(TableColumn):
  pass
class TextColumn(TableColumn):
  def __init__(self, name, length, *args, **kwd):
    self._length = length
    super().__init__(name, *args, **kwd)
  def _template(self):
    return super()._template(arg='{0._length}'.format(self))
class ShortColumn(TableColumn):
  pass
class LongColumn(TableColumn):
  pass
class FloatColumn(TableColumn):
  pass
class DoubleColumn(TableColumn):
  pass

class TableIndexCol:
  'This class represents a column within an index definition'
  __slots__ = 'name', 'offset', 'length'
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

class TableIndex:
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
    if len(colinfo) < 1:
      self.colinfo = TableIndexCol(name)
    elif len(colinfo) == 1:
      if isinstance(colinfo[0], str):
        self.colinfo = TableIndexCol(colinfo[0])
      elif isinstance(colinfo[0], TableIndexCol):
        self.colinfo = colinfo[0]
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
      self.colinfo = newinfo
  def __str__(self):
    'Provide a readable form of the information in the index'
    out = ['{}({},'.format(self.__class__.__name__, self.name)]
    if isinstance(self.colinfo, TableIndexCol):
      out.append('{},'.format(self.colinfo))
    else:
      for cinfo in self.colinfo_:
        out.append('{},'.format(cinfo))
    out.append('dups={0._dups}, desc={0._desc})'.format(self))
    return ' '.join(out)

# Provide an easier to read way of identifying the various key types
class DuplicateIndex(TableIndex):
  def __init__(self, name, *colinfo, desc=False):
    super().__init__(name, *colinfo, dups=True, desc=desc)
class UniqueIndex(TableIndex):
  def __init__(self, name, *colinfo, desc=False):
    super().__init__(name, *colinfo, dups=False, desc=desc)
class PrimaryIndex(UniqueIndex):
  pass
class AscDuplicateIndex(DuplicateIndex):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=False)
class AscUniqueIndex(UniqueIndex):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=False)
class AscPrimaryIndex(AscUniqueIndex):
  pass
class DescDuplicateIndex(DuplicateIndex):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=True)
class DescUniqueIndex(UniqueIndex):
  def __init__(self, name, *colinfo):
    super().__init__(name, *colinfo, desc=True)
class DescPrimaryIndex(DescUniqueIndex):
  pass
