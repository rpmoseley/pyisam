'''
This module contains the index for representing the index to an instance
of a table, included the underlying ISAM keydesc representation.
'''

__all__ = ('TableIndex', 'PrimaryIndex', 'DuplicateIndex', 'UniqueIndex',
           'AscPrimaryIndex', 'AscDuplicateIndex', 'AscUniqueIndex',
           'DescPrimaryIndex', 'DescDuplicateIndex', 'DescUniqueIndex')

from ..isam import ISAMindexMixin, keydesc

class TableIndexCol:
  'This class represents a column within an index definition'
  def __init__(self, name, offset=None, length=None,):
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

class TableIndex(ISAMindexMixin):
  '''Class used to store index information in an instance of ISAMtable'''
  __slots__ = 'name', 'dups', 'desc', 'keynum', '_kdesc', '_colinfo'
  def __init__(self, name, *colinfo, dups=True, desc=False, knum=-1, kdesc=None):
    self.name = name
    self.dups = dups
    self.desc = desc
    self.keynum = knum              # Stores the key number once matched
    self._kdesc = kdesc             # Stores the keydesc once prepared
    self._colinfo = []
    for col in colinfo:
      if hasattr(col, 'name'):
        self._colinfo.append(TableIndexCol(col.name,
                                           col.offset,
                                           col.length))
      elif isinstance(col, (tuple, list)):
        self._colinfo.append(TableIndexCol(col[0],
                                           col[1] if len(col) > 1 else None,
                                           col[2] if len(col) > 2 else None))
      elif isinstance(col, dict):
        self._colinfo.append(TableIndexCol(col['name'],
                                           col.get('offset'),
                                           col.get('length')))
      elif isinstance(col, str):
        self._colinfo.append(TableIndexCol(col))
      else:
        raise ValueError('Unhandled type of column information: {}'.format(type(col)))
  def as_keydesc(self, record, optimize=False):
    if self._kdesc is None:
      self._kdesc = self.create_keydesc(record, optimize=optimize)
    return self._kdesc
  def fill_fields(self, record, *args, **kwd):
    '''Fill the fields in the given RECORD with ARGS and KWD, if RECORD has an attribute
       _row_ it is assumed to be a table instance and the default record is used'''
    if hasattr(record, '_row_'):
      record = record._row_
    if not hasattr(record, '_namedtuple'):
      raise ValueError('Expecting an instance of ISAMrecord')

    # Process the fields using either keywords or arguments 
    fld_argn = 0
    for col in self._colinfo:
      try:
        record[col.name] = kwd[col.name]
      except KeyError:
        record[col.name] = args[fld_argn] if fld_argn < len(args) else None
        fld_argn += 1

'''
Provide aliases to make applications more readable and to check for particular
types of index when required.
'''
class UniqueIndex(TableIndex):
  def __init__(self, name, *colinfo, desc=False, knum=-1, kdesc=None):
    super().__init__(self, name, *colinfo, dups=False, desc=desc, knum=knum, kdesc=kdesc)

class PrimaryIndex(UniqueIndex):
  pass

class DuplicateIndex(TableIndex):
  def __init__(self, name, *colinfo, desc=False, knum=-1, kdesc=None):
    super().__init__(self, name, *colinfo, dups=True, desc=desc, knum=knum, kdesc=kdesc)

class AscUniqueIndex(UniqueIndex):
  def __init__(self, name, *colinfo, knum=-1, kdesc=None):
    super().__init__(self, name, *colinfo, desc=False, knum=knum, kdesc=kdesc)

class AscPrimaryIndex(AscUniqueIndex):
  pass

class AscDuplicateIndex(DuplicateIndex):
  def __init__(self, name, *colinfo, knum=-1, kdesc=None):
    super().__init__(self, name, *colinfo, desc=False, knum=knum, kdesc=kdesc)

class DescUniqueIndex(UniqueIndex):
  def __init__(self, name, *colinfo, knum=-1, kdesc=None):
    super().__init__(self, name, *colinfo, desc=True, knum=knum, kdesc=kdesc)

class DescPrimaryIndex(DescUniqueIndex):
  pass

class DescDuplicateIndex(DuplicateIndex):
  def __init__(self, name, *colinfo, knum=-1, kdesc=None):
    super().__init__(self, name, *colinfo, desc=True, knum=knum, kdesc=kdesc)
