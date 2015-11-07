'''
This module provides a structured way of accessing tables using ISAM by providing objects that
permit the table layout to be defined and referenced later.
'''

import collections
import os
from . import *
from ..enums import ReadMode, StartMode
from ..isam import ISAMobject, ISAMindexMixin
from ..record import ISAMrecord
from ..utils import IsamException

# Define the objects that are available using 'from .table import *'
__all__ = ('ISAMtable', 'ISAMrecord')

def GenTableDefn(tabobj, defn, *args, **kwd):
  'Add the column and index information to TABOBJ using DEFN as the generic definition'
  if not isinstance(tabobj, ISAMtable):
    raise ValueError('Table object should be an instance of ISAMtable')
  if not hasattr(defn, '_columns_') or not hasattr(defn, '_indexes_'):
    raise ValueError('Definition should have _columns_ and _indexes_ attributes')

  # Determine the table name to be used
  try:
    tabobj._tabname_ = defn._tabname_
  except AttributeError:
    tabname = kwd.get('tabname', defn.__class__.__name__)
    tabobj._tabname_ = tabname[:-4] if tabname.endswith('defn') else tabname

  # Process the database and prefix for the table
  tabobj._database_ = getattr(defn, '_database_', kwd.get('database', None))
  tabobj._prefix_ = getattr(defn, '_prefix_', kwd.get('prefix', None))

  # Ensure that the indexes is a list
  idxs = [defn._indexes_] if not isinstance(defn._indexes_, (tuple, list)) else defn._indexes_

  # Check whether columns will clash with existing attributes on TABOBJ
  clashes = set()
  for info in defn._columns_:
    if hasattr(tabobj, info._name_):
      clashes.add(info._name_)

  # Check whether indexes will clash with existing attributes on TABOBJ
  for info in idxs:
    if hasattr(tabobj, info._name_) or hasattr(tabobj, 'i_' + info._name_):
      clashes.add(info._name_)

  # Fail if the names actually clash
  if clashes:
    raise IsamException('Table object has clashing attributes')

  # Process the list of columns in the definition and generate the tables' _colinfo
  allcol = list()
  curoff = 0
  for info in defn._columns_:
    if isinstance(info, CharColumnDef):
      newcol = CharColumn(curoff)
    elif isinstance(info, TextColumnDef):
      newcol = TextColumn(curoff, info._size_)
    elif isinstance(info, ShortColumnDef):
      newcol = ShortColumn(curoff)
    elif isinstance(info, LongColumnDef):
      newcol = LongColumn(curoff)
    elif isinstance(info, FloatColumnDef):
      newcol = FloatColumn(curoff)
    elif isinstance(info, DoubleColumnDef):
      newcol = DoubleColumn(curoff)
    else:
      raise IsamException('Column type not currently handled')
    curoff += newcol._size_
    setattr(tabobj, info._name_, newcol)
    allcol.append(info._name_)
  tabobj._colinfo_ = tuple(allcol)

  # Process the list of indexes and create the associated underlying descriptions
  allidx = collections.OrderedDict()
  for info in idxs:
    if isinstance(info, PrimaryIndexDef):
      newidx = PrimaryIndex(info._name_, info._colinfo_, info._dups_, info._desc_)
    elif isinstance(info, UniqueIndexDef):
      newidx = UniqueIndex(info._name_, info._colinfo_, info._dups_, info._desc_)
    elif isinstance(info, DuplicateIndexDef):
      newidx = DuplicateIndex(info._name_, info._colinfo_, info._dups_, info._desc_)
    else:
      raise IsamException('Index type not currently handled: {}'.format(info))
    allidx[info._name_] = newidx
  tabobj._idxinfo_ = allidx

class ISAMtable:
  '''Class that represents a table within ISAM, this holds all the column
     and index information and also provides the get/set functionality
     with dictionary like access. It will match up the index information
     with the information stored within the table file itself.'''
  # _slots_ = ('_isobj_','_name_','_path_','_mode_','_lock_','_database_',
  #            '_prefix_','_curidx_','_row_')
  def __init__(self, tabdefn, tabname=None, tabpath=None, isobj=None, **kwd):
    GenTableDefn(self, tabdefn, **kwd)
    self._isobj_ = ISAMobject(tabpath, **kwd) if isobj is None else isobj
    self._name_ = tabdefn._tabname_ if tabname is None else tabname
    self._path_ = tabpath if tabpath else '.'
    self._mode_ = kwd.get('mode', self._isobj_.def_openmode)
    self._lock_ = kwd.get('lock', self._isobj_.def_lockmode)
    self._database_ = getattr(tabdefn, '_database_', None)
    self._prefix_ = getattr(tabdefn, '_prefix_', None)
    self._curindex_ = None           # Current index being used in isread/isstart
    self._row_ = None                # Cannot allocate record until table is opened
  def __getitem__(self, colname):
    'Retrieve the specified COLNAME having converted the value using O[name]'
    if self._row_ is None: raise AttributeError(colname)
    return self._row_[name]
  def __setitem__(self, colname, value):
    'Set the specified column to the converted value using O[name] = value'
    if self._row_ is None: raise AttributeError(colname)
    self._row_[name] = value
  def __str__(self):
    return 'No data available' if self._row_ is None else str(self._row)
  @property
  def _is_open(self):
    return self._isobj_._isfd_ is not None
  def _addindex(self, index):
    'Add an instance of ISAMindex to the underlying ISAM table'
    if not isinstance(index, ISAMindex):
      raise ValueError('Must provide an instance of ISAMindex for the index to add')
    kd = index.as_keydesc(self)
  def _delindex(self, index):
    'Remove the instance of ISAMindex from the underlying ISAM table'
    if not isinstance(index, ISAMindex):
      raise ValueError('Must provide an instance of ISAMindex for the index to remove')
    kd = index.as_keydesc(self) 
  def _build(self, tabpath=None, reclen=None, index=None, **kwd):
    'Build a new ISAM table using the definition provided in the optional TABPATH'
    if self._is_open:
      raise IsamException('Table is already open, close first')
    if reclen is None:
      raise ValueError('Must provide the record length for the table being built')
    if not isinstance(index, ISAMindex):
      raise ValueError('Must provide a primary index which is an instance of ISAMindex')
    path = os.path.join(self._path_ if tabpath is None else tabpath, self._name_)
    varlen = kwd.get('varlen', None)
    if varlen is not None:
      self._isobj_.isreclen = int(varlen)
    self._isobj_.isbuild(path, reclen, index.as_keydesc(self))
  def _open(self, tabpath=None, mode=None, lock=None, **kwd):
    'Open an existing ISAM table with the specified mode, lock and TABPATH'
    if self._is_open:
      raise IsamException('Table is already open, close first')
    if mode is None:
      mode = getattr(self, '_mode_', None)
    if lock is None:
      lock = getattr(self, '_lock_', None)
    path = os.path.join(self._path_ if tabpath is None else tabpath, self._name_)
    self._isobj_.isopen(path, mode, lock)
    self._recsize_ = self._isobj_.isreclen  # Provided by the ISAM library
    ## self._row_ = ISAMrecord(self, **kwd) # Leave to ISAMrecord.record to create
  def _cluster(self, index):
    'Re-organize the ISAM table using the specified INDEX'
    if not isinstance(index, ISAMindex):
      raise ValueError('Must provide an instance of ISAMindex for the index')
    self._isobj_.iscluster(index.as_keydesc(self))
  def _close(self):
    'Close the underlying ISAM table'
    self._isobj_.isclose()
  def _read(self, _index=None, _mode=ReadMode.ISNEXT, *args, **kwd):
    'Return the appropriate record into RECBUFF according to the MODE specified'
    # This function will combine the isread/isstart functions as appropriate with the 
    # current index and the one that is specified by INDEX.
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    if not self._is_open: self._open(**kwd)
    if _index is None and self._curindex_ is not None:
      # Use the current index for the request
      _index = self._curindex_
    if _mode is None:
      # Assume that the next record is required
      _mode = ReadMode.ISNEXT
    _recbuff = kwd.pop('_recbuff', None)
    if _recbuff is None:
      # Use default record buffer for request
      if self._row_ is None:
        self._row_ = ISAMrecord(self)
      _recbuff = self._row_
    # Get the information about the index
    if isinstance(_index, str):
      _index = self._idxinfo_[_index]
    elif not isinstance(_index, TableIndex):
      raise ValueError('Unhandled instance for an index')
    if _index == self._curindex_:
      # Using same index so can return if the mode is a simple continuation
      if _mode in (ReadMode.ISNEXT, ReadMode.ISCURR, ReadMode.ISPREV):
        self._isobj_.isread(_recbuff._recbuff_, _mode)
      else:
        # Fill the columns that are part of the index 
        _index.fill_fields(_recbuff, *args, **kwd)
        self._isobj_.isread(_recbuff._recbuff_, _mode)
    else:
      # Convert the required mode into a resatart mode
      smode = StartMode(_mode)

      # Fill the columns that are part of the index 
      _index.fill_fields(_recbuff, *args, **kwd)
    
      # Issue the restart followed by the read of the record itself
      self._isobj_.isstart(_index.as_keydesc(self), smode, _recbuff._recbuff_)
      
      # Return the next record
      self._isobj_.isread(_recbuff._recbuff_, ReadMode.ISNEXT)

      # Make this the current index for the next instance of the method
      self._curindex_ = _index
    # Return the record as an instance of the table row tuple
    return self._row_.as_tuple
  def _insert(self, _recbuff=None, *args, **kwd):
    'Insert a record'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
  def _delete(self, keybuff=None, recbuff=None, *args, **kwd):
    'Delete a record from the underlying ISAM table'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    # If KEYBUFF is None then invoke isdelcurr
    # If KEYBUFF is not None then invoke isdelete
  def _update(self, keybuff=None, recbuff=None, *args, **kwd):
    'Update an existing record ...'
  def _lock(self, *args, **kwd):
    'Lock the underlying ISAM table'
  def _unlock(self, *args, **kwd):
    'Unlock the underlying ISAM table'
  def _release(self):
    'Release all the locks on the underlying ISAM table'
  def _dictinfo(self):
    'Return an instance of dictinfo for the underlying ISAM table'
  def _indexinfo(self, idxnum):
    'Return an instance of ISAMindexInfo for the given IDXNUM on the underlying ISAM table'
  def _gt_uniqueid(self):
    'Get the current uniqueid from the underlying ISAM table (this autoincrements)'
  def _st_uniqueid(self, newval):
    'Set the uniqueid for the underlying ISAM table'
  uniqueid = property(_gt_uniqueid, _st_uniqueid, doc='Obtain the uniqueid for the table')
  def _dump(self,ofd):
    # FIXME: This needs to be updated for the new version of the object
    ofd.write('Table: {}\n'.format(self.__class__.__name__))
    ofd.write('  Columns:\n')
    for col in self._defn_._colinfo_.items():
      ofd.write('     {}\n'.format(col))
    ofd.write('  Indexes:\n')
    self._prepare_kdesc()
    for idxname,idxinfo in self._kdesc_.items():
      ofd.write('     {}: {}\n'.format(idxname,idxinfo))
