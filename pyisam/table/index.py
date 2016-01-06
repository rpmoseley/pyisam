'''
This module contains the index for representing the index to an instance
of a table, included the underlying ISAM keydesc representation.
'''

__all__ = 'TableIndex'

from ..isam import ISAMindexMixin

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

class TableIndex(ISAMindexMixin):
  '''Class used to store index information in an instance of ISAMtable'''
  __slots__ = 'name', 'dups', 'desc', 'knum', 'kdesc', 'colinfo'
  def __init__(self, name, colinfo, dups=True, desc=False, knum=-1, kdesc=None): # TODO Check the colinfo argument
    self.name = name
    self.dups = dups
    self.desc = desc
    self.knum = knum                # Stores the key number once matched
    self.kdesc = kdesc              # Stores the keydesc once prepared
    self.colinfo = tuple(TableIndexCol(col.name, col.offset, col.length) for col in colinfo)
  def as_keydesc(self, tabobj, optimize=False):
    kdesc = self._kdesc
    if kdesc is None:
      kdesc = self._kdesc = self.create_keydesc(tabobj, optimize=optimize)
    return kdesc
  def fill_fields(self, _record=None, *args, **kwd):
    '''Fill the fields in the given _RECORD with ARGS and KWD, if _RECORD is None
       then the default buffer _TABOBJ._row is used instead'''
    if _record is None:
      try:
        _tabobj = kwd['_tabobj']
        if not hasattr(_tabobj, '_isobj_'):
          raise ValueError("Expecting an instance of ISAMtable for '_tabobj'")
        _record = _tabobj._row_
      except KeyError:
        raise ValueError('Must provide a table instance if not giving a record buffer')
    elif hasattr(_record, '_isobj_'):
      _record = _record._row_
    elif not hasattr(_record, '_namedtuple'):
      raise ValueError("Expecting an instance of ISAMrecord for '_record'")
    else:
      _tabobj = _record._tabobj

    # Process the fields using either keywords or arguments 
    _fld_argn = 0
    for _col in self._colinfo:
      try:
        _record[_col] = kwd[_col.name]
      except KeyError:
        _record[_col] = args[_fld_argn] if _fld_argn < len(args) else None
        _fld_argn += 1
