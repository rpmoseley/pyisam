'''
This module provides a structured way of accessing tables using ISAM by providing objects that
permit the table layout to be defined and referenced later.
'''

import collections
import os
from .index import TableIndex, PrimaryIndex, DuplicateIndex, UniqueIndex
from .record import recordclass
from ..enums import ReadMode, StartMode
from ..error import IsamException, IsamOpened, IsamNoIndex
from ..isam import ISAMobject
from ..tabdefns import TableDefnIndex

# Define the objects that are available using 'from .table import *'
__all__ = ('ISAMtable',)

class ISAMtable:
  '''Class that represents a table within ISAM, the associated record object
     provides the column information that includes the underlying raw byte
     record area used by the ISAM library. -+- this holds all the column
     and index information and also provides the get/set functionality
     with dictionary like access. It will match up the index information
     with the information stored within the table file itself.'''
  # _slots_ = '_isobj_', '_name_', '_path_', '_mode_', '_lock_', '_database_',
  #           '_prefix_', '_curidx_', '_record_', '_recsize_', '_row_'
  def __init__(self, tabdefn, tabname=None, tabpath=None, isobj=None, **kwd):
    self._defn_ = tabdefn
    self._name_ = tabdefn._tabname_ if tabname is None else tabname
    self._path_ = tabpath if tabpath else '.'
    self._mode_ = kwd.get('mode', ISAMobject.def_openmode)
    self._lock_ = kwd.get('lock', ISAMobject.def_lockmode)
    self._isobj_ = isobj if isinstance(isobj, ISAMobject) else ISAMobject(**kwd)
    self._database_ = getattr(tabdefn, '_database_', None)
    self._prefix_ = getattr(tabdefn, '_prefix_', None)
    self._record_ = kwd.get('recordclass', recordclass(tabdefn))
    self._idxinfo_ = dict()          # Dictionary of indexes on this table's object
    self._curindex_ = None           # Current index being used in isread/isstart
    self._row_ = None                # Cannot allocate record until table is opened
    self._recsize_ = -1              # Length of record is given after table is opened

    # Define the indexes found in the definition object
    self.add_indexes(tabdefn)
  def add_indexes(self, tabdefn):
    'Add the indexes defined in the table definition'
    if isinstance(tabdefn._indexes_, dict):
      for index in tabdefn._indexes_.values():
        if isinstance(index, TableDefnIndex):
          # Convert a table definition index into a real table index
          colinfo = index.colinfo if isinstance(index.colinfo, (list, tuple)) else [index.colinfo]
          self._idxinfo_[index.name] = TableIndex(index.name, *colinfo, dups=index.dups, desc=index.desc)
        else:
          raise NotImplementedError('Index type not currently supported')
    else:
      indexes = tabdefn._indexes_ if isinstance(tabdefn._indexes_, (tuple, list)) else [tabdefn._indexes_]
      for index in indexes:
        colinfo = index.colinfo if isinstance(index.colinfo, (tuple, list)) else [index.colinfo]
        self._idxinfo_[index.name] = TableIndex(index.name, *colinfo, dups=index.dups, desc=index.desc)
  def __str__(self):
    return 'No data available' if self._row_ is None else str(self._row_)
  # TODO: This will be enabled when context manager features are implemented:
  # def __enter__(self):
  #   # Open the table using default mode, lock and paths
  #   self._isobj_.isopen(self._name_, None, None)
  #   return self
  # def __exit__(self, type_, value, traceback):
  #   # Close the table
  #   self._isobj_.isclose()
  def createrecord(self, fields=None, error=False):
    'Return a new instance of the record classs for this table'
    if self._recsize_ > 0:
      return self._record_(self._name_, self._recsize_, fields=fields)
    if error:
      raise IsamException('Record size not known to generate a record buffer')
  def addindex(self, index):
    'Add an instance of TableIndex to the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide an instance of TableIndex for the index to add')
    kd = index.as_keydesc(self)
    # Check if the index is already present on the table
    print('ADDIDX:',kd)  #DEBUG
    # TODO Complete
  def delindex(self, index):
    'Remove the instance of TableIndex from the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide an instance of TableIndex for the index to remove')
    kd = index.as_keydesc(self)
    print('DELIDX:',kd)
    # TODO Complete 
  def build(self, tabpath=None, **kwd):
    'Build a new ISAM table using the definition provided in the optional TABPATH'
    if self._isobj_._isfd_ is not None: raise IsamOpened
    # Locate the primary index in the definition
    index = self._LookupIndex(idxtype=PrimaryIndex)
    path = os.path.join(self._path_ if tabpath is None else tabpath, self._name_)
    varlen = kwd.get('varlen', None)
    if varlen is not None:
      self._isobj_.isreclen = int(varlen)
    self._isobj_.isbuild(path, reclen, index.as_keydesc(self))
  def open(self, tabpath=None, mode=None, lock=None, **kwd):
    'Open an existing ISAM table with the specified mode, lock and TABPATH'
    if self._isobj_._isfd_ is not None:
      print('ERR:', self._isobj_._isfd_)
      raise IsamOpened
    if mode is None: mode = self._mode_
    if lock is None: lock = self._lock_
    path = os.path.join(self._path_ if tabpath is None else tabpath, self._name_)
    self._isobj_.isopen(path, mode, lock)
    self._recsize_ = self._isobj_.isreclen  # Provided by the ISAM library
    self._match_indexes()
  def cluster(self, index):
    'Re-organize the ISAM table using the specified INDEX'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide an instance of ISAMindex for the index')
    self._isobj_.iscluster(index.as_keydesc(self))
  def close(self):
    'Close the underlying ISAM table'
    self._isobj_.isclose()
  def read(self, _index=None, _mode=ReadMode.ISNEXT, *args, **kwd):
    'Return the appropriate record into RECBUFF according to the MODE specified'
    # This function combines the isread/isstart functions as appropriate with the 
    # current index and the one that is specified by INDEX.
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    if self._isobj_._isfd_ is None: self.open(**kwd)
    if _index is None and self._curindex_ is not None:
      # Use the current index for the request
      _index = self._curindex_
    if _mode is None:
      # Assume that the next record is required
      _mode = ReadMode.ISNEXT
    _record = kwd.pop('recbuff', self._row_)
    if _record is None:
      # Use default record buffer for request
      if self._row_ is None:
        self._row_ = self._record_(self._name_, self._recsize_)
      _record = self._row_
    # Get the information about the index
    _index = self._LookupIndex(_index)
    if _index == self._curindex_:
      if _mode not in (ReadMode.ISNEXT, ReadMode.ISCURR, ReadMode.ISPREV):
        # Fill the columns that are part of the index 
        _index.fill_fields(_record, *args, **kwd)
      self._isobj_.isread(_record._buffer, _mode)
    else:
      # Convert the required mode into a restart mode
      smode = StartMode(_mode)

      # Fill the columns that are part of the index
      _index.fill_fields(_record, *args, **kwd)
    
      # Issue the restart followed by the read of the record itself
      self._isobj_.isstart(_index.as_keydesc(_record), smode, _record._buffer)
      self._isobj_.isread(_record._buffer, ReadMode.ISNEXT)

      # Make this the current index for the next invocation of the method
      self._curindex_ = _index
    # Return the record which can then be used as required
    return _record
  def insert(self, _record=None, *args, **kwd):
    'Insert a record'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    if self._isobj_._isfd_ is None: self.open(**kwd)
  def delete(self, keybuff=None, recbuff=None, *args, **kwd):
    'Delete a record from the underlying ISAM table'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    # If KEYBUFF is None then invoke isdelcurr
    # If KEYBUFF is not None then invoke isdelete
    if self._isobj_._isfd_ is None: self.open(**kwd)
  def update(self, keybuff=None, recbuff=None, *args, **kwd):
    'Update an existing record ...'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    # If KEYBUFF is None then invoke isdelcurr
    # If KEYBUFF is not None then invoke isdelete
  def lock(self, *args, **kwd):
    'Lock the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
  def unlock(self, *args, **kwd):
    'Unlock the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
  def release(self):
    'Release all the locks on the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
  def dictinfo(self):
    'Return an instance of dictinfo for the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    return self._isobj_.isdictinfo()
  def indexinfo(self, idxnum):
    'Return an instance of ISAMindexInfo for the given IDXNUM on the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    keydesc = self._isobj_.iskeyinfo(idxnum)
  @property
  def uniqueid(self):
    'Get the current uniqueid from the underlying ISAM table (this autoincrements)'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
  @uniqueid.setter
  def uniqueid(self, newval):
    'Set the uniqueid for the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
  def _LookupIndex(self, name=None, flags=None):
    """Return the information for the index NAME populating the index cache (_idxinfo_) if
       required."""
    if name is None:
      # Handle special case situations
      if (type(flags) == 'set' and 'NEED_PRIMARY' in flags) or flags == 'NEED_PRIMARY':
        for index in self._idxinfo_.values():
          if isinstance(index, PrimaryIndex):
            break
        else:
          raise AttributeError('No primary index defined for table')
    elif isinstance(name, TableIndex):
      return name
    elif isinstance(name, str):
      try:
        index = self._idxinfo_[name]
      except KeyError:
        # Lookup the index using the table definition as not in the cache
        index = self._defn_._indexes_.get(name)
        #TODO:raise IsamNoIndex(self._name_, name)
      return index
    else:
      raise ValueError('Unhandled type of index requested')
  def _match_indexes(self):
    '''Match the indexes found on the underlying table with those provided by the
       table definition object'''
    record = self.createrecord()
    for idxnum in range(self._isobj_.isdictinfo().nkeys):
      keydesc = self._isobj_.iskeyinfo(idxnum)
      for idxchk in self._idxinfo_.values():
        chkdesc = idxchk.as_keydesc(record, optimize=True)
        if keydesc == chkdesc:
          idxchk.keynum = idxnum
    # for value in self._idxinfo_.values():
    #   print('KEY: ({0.name}, {0.dups}, {0.desc}, {0.keynum}, {0._kdesc}, ['.format(value), end='')
    #   print(', '.join([cval.name for cval in value._colinfo]), end='')
    #   print('])')
  def _dump(self, ofd):
    # FIXME: This needs to be updated for the new version of the object
    ofd.write('Table: {}\n'.format(self.__class__.__name__))
    ofd.write('  Columns:\n')
    for col in self._defn_._colinfo.values():
      ofd.write('     {}\n'.format(col))
    ofd.write('  Indexes:\n')
    self._prepare_kdesc()
    for idxname,idxinfo in self._kdesc.items():
      ofd.write('     {}: {}\n'.format(idxname,idxinfo))
