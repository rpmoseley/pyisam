'''
This module contains the various objects and methods for representing the
index to an instance of a table, included the underlying ISAM keydesc
representation.
'''

__all__ = ('TableIndex', 'PrimaryIndex', 'DuplicateIndex', 'UniqueIndex',
           'AscPrimaryIndex', 'AscDuplicateIndex', 'AscUniqueIndex',
           'DescPrimaryIndex', 'DescDuplicateIndex', 'DescUniqueIndex',
           'create_TableIndex')

from .. import MaxKeyParts
from ..isam import ISAMindexMixin, keydesc
from ..enums import IndexFlags
from ..tabdefns import TableDefnIndexCol
from .record import ColumnInfo

class _TableIndexCol:
  '''Class representing a column within an index definition
     which means that the offset and length are relative to
     the column within the index only and not to the overall
     record as exposed by the key information stored on the
     underlying ISAM table.'''
  __slots__ = 'name', '_weight', 'offset', 'length'
  def __init__(self, name, offset=None, length=None):
    self.name = name
    self._weight = -1
    self.offset = offset
    self.length = length
  
  def _gt_weight(self):
    return 0 if self._weight < 0 else self._weight
  def _st_weight(self, idxcolnum):
    if 0 <= idxcolnum < MaxKeyParts:
      self._weight = 1 << (MaxKeyParts - idxcolnum - 1)
    elif idxcolnum < 0:
      self._weight = -1
    else:
      raise ValueError('Must specify a weight between 0 and {} or -1'.format(MaxKeyParts - 1))
  def _dl_weight(self):
    self._weight = -1
  weight = property(_gt_weight, _st_weight, _dl_weight, 'Weighting used in autoselecting an index')

  def __eq__(self, other):
    if not isinstance(other, _TableIndexCol):
      raise ValueError('Cannot compare against unhandled object')
    return self.name == other.name
    # TODO Add better comparisons for when length/offset is set
    
  def __str__(self):
    out = ["TableIndexCol(name='{0.name}'"]
    if self.length is not None:
      if self.offset is not None:
        out.append(', offset={0.offset}')
      out.append(', length={0.length}')
    elif self.offset is not None:
      out.append(', offset={0.offset}')
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
    if isinstance(col, _TableIndexCol):
      self._colinfo.append(col)
    elif isinstance(col, ColumnInfo):
      self._colinfo.append(_TableIndexCol(col.name))
    elif isinstance(col, str):
      self._colinfo.append(_TableIndexCol(col))
    elif isinstance(col, TableDefnIndexCol):
      self._colinfo.append(_TableIndexCol(col.name,
                                          col.offset,
                                          col.length))
    elif isinstance(col, (tuple, list)):
      if isinstance(col[0], (_TableIndexCol, TableDefnIndexCol, ColumnInfo)):
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
    else:
      raise ValueError('Unhandled type of column information')

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
    print('FILL_BEF:', record)

    # Process the fields using either keywords or arguments 
    fld_argn = 0
    for col in self._colinfo:
      try:
        record[col.name] = kwd[col.name]
      except KeyError:
        record[col.name] = args[fld_argn] if fld_argn < len(args) else None
        fld_argn += 1

  @staticmethod
  def keydesc_flags_as_set(keydesc):
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

  def __eq__(self, other):
    'Compare the two TableIndex instances for equality'
    if not isinstance(other, TableIndex):
      if other is None:
        # Always fails to compare equal against None
        return False
      print(type(other))
      raise ValueError('Unable to compare non TableIndex instance')
    # Check if the index is the same sort order
    if self.desc != other.desc:
      print('CHK: desc differs')
      return False
    # Check if the index both allow duplicates
    if self.dups != other.dups:
      print('CHK: dups differ')
      return False
    # Check if the number of key parts is the same
    if len(self._colinfo) != len(other._colinfo):
      print('CHK: len colinfo differs')
      return False
    # Check each key part for matching columns
    for self_colinfo, other_colinfo in zip(self._colinfo, other._colinfo):
      if self_colinfo != other_colinfo:
        print('CHK: colinfo differs')
        return False
    return True

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

def create_TableIndex(keydesc, record, idxnum):
  '''Function to create an instance of TableIndex given a KEYDESC in relation to
     the record object RECORD.'''
  # Determine whether the index is ascending or descending, then unique or not
  if keydesc.flags & IndexFlags.DESCEND:
    idxclass = DescDuplicateIndex if keydesc.flags & IndexFlags.DUPS else DescUniqueIndex
  else:
    idxclass = DuplicateIndex if keydesc.flags & IndexFlags.DUPS else UniqueIndex
  
  # Create a list of the columns within the index using the offset, size and type
  # information available from the underlying keydesc structure.
  idxcol = [None] * keydesc.nparts
  for npart in range(keydesc.nparts):
    for fld in record._fields:
      # Check if the field is the correct type
      if keydesc[npart].type == fld.type.value:
        # Check if part of the column
        if fld.offset <= keydesc[npart].start < (fld.offset + fld.size):
          # Check if the key oversteps the end of the column
          if keydesc[npart].leng <= fld.size:
            # Create the index column in relation to the column itself rather than
            # the whole record as keydesc stores it.
            if fld.offset == keydesc[npart].start:
              if fld.size == keydesc[npart].leng:
                # Offset and length matches a column exactly -
                #   create an index column by name only
                idxcol[npart] = _TableIndexCol(fld.name)
              else:
                # Offset matches a column offset exactly - 
                #   create an index column by name and length
                idxcol[npart] = _TableIndexCol(name=fld.name,
                                               length=keydesc[npart].leng)
            elif fld.size == keydesc[npart].leng:
              # Length matches a column exactly -
              #    create an index column by name and offset             
              idxcol[npart] = _TableIndexCol(name=fld.name,
                                             offset=keydesc[npart].start - fld.offset)
            else:
              # Create an index column by name, offset and length
              idxcol[npart] = _TableIndexCol(name=fld.name,
                                             offset=keydesc[npart].start - fld.offset,
                                             length=keydesc[npart].leng)
            break

  # Check if any of the index parts are still not resolved
  idxmiss = list(filter(None, idxcol))
  if len(idxmiss) != len(idxcol):
    # Attempt to deduce the missing fields preferring those that have the same
    # offset as a column even though the size maybe different
    raise ValueError('Some parts of index #{} do not match columns in the definition'.format(idxnum))

  # If the index consists of a single column then name the index with that column
  idxname = idxcol[0].name if keydesc.nparts == 1 else 'INDEX{}'.format(idxnum)

  # Create a new instance of TableIndex for the new index found
  return idxclass(idxname, idxcol, knum=idxnum, kdesc=keydesc)

# Provide aliases to make applications more readable and to check for particular
# types of index when required.
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
