'''
This module contains the various objects and methods for representing the
index to an instance of a table, included the underlying ISAM keydesc
representation.
'''

__all__ = ('TableIndex', 'PrimaryIndex', 'DuplicateIndex', 'UniqueIndex',
           'AscPrimaryIndex', 'AscDuplicateIndex', 'AscUniqueIndex',
           'DescPrimaryIndex', 'DescDuplicateIndex', 'DescUniqueIndex',
           'create_tableindex')

from ..isam import ISAMindexMixin, keydesc
from ..enums import IndexFlags
from ..tabdefns import TableDefnIndexCol
from .record import ColumnInfo

class _TableIndexCol:
  'This class represents a column within an index definition'
  def __init__(self, name, offset=None, length=None,):
    self.name = name
    self.offset = offset
    self.length = length
  
  def __str__(self):
    out = ["TableIndexCol('{0.name}'"]
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
      self._add_colinfo(col)

  def _add_colinfo(self, col):
    '''Add a new column to the index definition'''
    if isinstance(col, ColumnInfo):
      self._colinfo.append(_TableIndexCol(col.name,
                                          col.offset,
                                          col.size))
    elif isinstance(col, TableDefnIndexCol):
      self._colinfo.append(_TableIndexCol(col.name,
                                          col.offset,
                                          col.length))
    elif isinstance(col, (tuple, list)):
      if isinstance(col[0], ColumnInfo):
        for subcol in col:
          self._add_colinfo(subcol)
      else:
        self._colinfo.append(_TableIndexCol(col[0],
                                            col[1] if len(col) > 1 else None,
                                            col[2] if len(col) > 2 else None))
    elif isinstance(col, dict):
      self._colinfo.append(_TableIndexCol(col['name'],
                                          col.get('offset'),
                                          col.get('size')))
    elif isinstance(col, str):
      self._colinfo.append(_TableIndexCol(col))
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

  @staticmethod
  def str_keydesc_flags(keydesc):
    'Return the flags on the keydesc as a set of flags'
    flagset, flags = set(), keydesc.flags
    flagset.add('DUPLICATES' if flags & IndexFlags.DUPS else 'UNIQUE')
    flagset.add('DESCEND' if flags & IndexFlags.DESCEND else 'ASCEND')
    if flags & IndexFlags.ALL_COMPRESS:
      flagset.add('ALL_COMPRESS')
    else:
      if flags & IndexFlags.DUP_COMPRESS:
        flagset.add('DUP_COMPRESS')
      if flags & IndexFlags.LDR_COMPRESS:
        flagset.add('LDR_COMPRESS')
      if flags & IndexFlags.TRL_COMPRESS:
        flagset.add('TRL_COMPRESS')
    if flags & IndexFlags.CLUSTER:
      flagset.add('CLUSTER')
    return flagset

  def __str__(self):
    'Return a printable version of the information in the index'
    out = ["{0.__class__.__name__}(name='{0.name}', dups={0.dups}, desc={0.desc}, "]
    if self.keynum >= 0:
      out.append('keynum={0.keynum}, ')
    out.append('colinfo=[')
    col = []
    for colinfo in self._colinfo:
      col.append(str(colinfo))
    out.append(', '.join(col))
    out.append('])')       
    return ''.join(out).format(self)

def create_tableindex(keydesc, record, idxnum):
  '''Function to create an instance of TableIndex given a KEYDESC in relation to
     the record object RECORD.'''
  # Determine whether the index is unique or not
  if keydesc.flags & IndexFlags.DESCEND:
    idxclass = DescDuplicateIndex if keydesc.flags & IndexFlags.DUPS else DescUniqueIndex
  else:
    idxclass = DuplicateIndex if keydesc.flags & IndexFlags.DUPS else UniqueIndex
  
  # Create a list of the columns within the index using the offset, size and type
  # information available from the underlying keydesc structure.
  idxcol = [None] * keydesc.nparts
  for fld in record._fields:
    for num in range(keydesc.nparts):
      # Check if part of the column
      if fld.offset <= keydesc[num].start < (fld.offset + fld.size):
        # Check if the key oversteps the end of the column
        if (keydesc[num].start + keydesc[num].leng) <= (fld.offset + fld.size):
          # Check if the field is the correct type
          if keydesc[num].type == fld.type.value:
            idxcol[num] = fld
            break

  # Check if any of the index parts still not resolved
  if not filter(None, idxcol):
    raise ValueError('Some parts of the index do match columns in the definition')

  # If the index consists of a single column then name the index with that column
  if keydesc.nparts == 1:
    idxname = idxcol[0].name
  else:
    idxname = 'INDEX{}'.format(idxnum)

  # Create a new instance of TableIndex for the new index found
  newidx = idxclass(idxname, idxcol, knum=idxnum, kdesc=keydesc)

  print('CR_KEYDESC:', keydesc)
  print('CR_NEWIDX :' ,newidx)

'''
Provide aliases to make applications more readable and to check for particular
types of index when required.
'''
class UniqueIndex(TableIndex):
  def __init__(self, name, *colinfo, desc=False, knum=-1, kdesc=None):
    super().__init__(name, *colinfo, dups=False, desc=desc, knum=knum, kdesc=kdesc)

class PrimaryIndex(UniqueIndex):
  pass

class DuplicateIndex(TableIndex):
  def __init__(self, name, *colinfo, desc=False, knum=-1, kdesc=None):
    super().__init__(name, *colinfo, dups=True, desc=desc, knum=knum, kdesc=kdesc)

class AscUniqueIndex(UniqueIndex):
  def __init__(self, name, *colinfo, knum=-1, kdesc=None):
    super().__init__(name, *colinfo, desc=False, knum=knum, kdesc=kdesc)

class AscPrimaryIndex(AscUniqueIndex):
  pass

class AscDuplicateIndex(DuplicateIndex):
  def __init__(self, name, *colinfo, knum=-1, kdesc=None):
    super().__init__(name, *colinfo, desc=False, knum=knum, kdesc=kdesc)

class DescUniqueIndex(UniqueIndex):
  def __init__(self, name, *colinfo, knum=-1, kdesc=None):
    super().__init__(name, *colinfo, desc=True, knum=knum, kdesc=kdesc)

class DescPrimaryIndex(DescUniqueIndex):
  pass

class DescDuplicateIndex(DuplicateIndex):
  def __init__(self, name, *colinfo, knum=-1, kdesc=None):
    super().__init__(name, *colinfo, desc=True, knum=knum, kdesc=kdesc)
