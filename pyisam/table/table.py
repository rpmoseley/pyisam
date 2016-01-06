'''
This module provides a structured way of accessing tables using ISAM by providing objects that
permit the table layout to be defined and referenced later.
'''

import collections
import os
from .record import recordclass
from ..enums import ReadMode, StartMode
from ..error import IsamException, IsamOpened
from ..isam import ISAMobject, ISAMindexMixin

# Define the objects that are available using 'from .table import *'
__all__ = 'ISAMtable'

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
    self._name_ = tabdefn._tabname_ if tabname is None else tabname
    self._path_ = tabpath if tabpath else '.'
    self._mode_ = kwd.get('mode', ISAMobject.def_openmode)
    self._lock_ = kwd.get('lock', ISAMobject.def_lockmode)
    self._isobj_ = ISAMobject(**kwd) if isobj is None else isobj
    self._database_ = getattr(tabdefn, '_database_', None)
    self._prefix_ = getattr(tabdefn, '_prefix_', None)
    self._curindex_ = None           # Current index being used in isread/isstart
    self._record_ = kwd.get('recordclass', recordclass(tabdefn))
    self._row_ = None                # Cannot allocate record until table is opened
    self._recsize_ = -1              # Length of record is given after table is opened
  def __str__(self):
    return 'No data available' if self._row_ is None else str(self._row_)
  @property
  def _is_open(self):
    return self._isobj_._isfd_ is not None
  ''' TODO: This will be enabled when context manager features are implemented:
  def __enter__(self):
    # Open the table using default mode, lock and paths
    self._isobj_.isopen(self._name_, None, None)
    return self
  def __exit__(self, type_, value, traceback):
    # Close the table
    self._isobj_.isclose()
  '''
  def createrecord(self, fields=None, error=False):
    'Return a new instance of the record classs for this table'
    if self._recsize_ > 0:
      return self._record_(self._name_, self._recsize_, fields)
    if error:
      raise IsamException('Record size not known to generate a record buffer')
  def addindex(self, index):
    'Add an instance of ISAMindex to the underlying ISAM table'
    if not isinstance(index, ISAMindex):
      raise ValueError('Must provide an instance of ISAMindex for the index to add')
    kd = index.as_keydesc(self)
  def delindex(self, index):
    'Remove the instance of ISAMindex from the underlying ISAM table'
    if not isinstance(index, ISAMindex):
      raise ValueError('Must provide an instance of ISAMindex for the index to remove')
    kd = index.as_keydesc(self) 
  def build(self, tabpath=None, reclen=None, index=None, **kwd):
    'Build a new ISAM table using the definition provided in the optional TABPATH'
    if self._isobj._isfd_ is not None: raise IsamOpened
    if reclen is None:
      raise ValueError('Must provide the record length for the table being built')
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide a primary index which is an instance of TableIndex')
    path = os.path.join(self._path_ if tabpath is None else tabpath, self._name_)
    varlen = kwd.get('varlen', None)
    if varlen is not None:
      self._isobj.isreclen = int(varlen)
    self._isobj.isbuild(path, reclen, index.as_keydesc(self))
  def open(self, tabpath=None, mode=None, lock=None, **kwd):
    'Open an existing ISAM table with the specified mode, lock and TABPATH'
    if self._isobj_._isfd_ is not None: raise IsamOpened
    if mode is None:
      mode = getattr(self, '_mode_', None)
    if lock is None:
      lock = getattr(self, '_lock_', None)
    path = os.path.join(self._path_ if tabpath is None else tabpath, self._name_)
    self._isobj_.isopen(path, mode, lock)
    self._recsize_ = self._isobj_.isreclen  # Provided by the ISAM library
  def cluster(self, index):
    'Re-organize the ISAM table using the specified INDEX'
    if not isinstance(index, ISAMindex):
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
    if not self._is_open: self.open(**kwd)
    if _index is None and self._curindex is not None:
      # Use the current index for the request
      _index = self._curindex
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
    if isinstance(_index, str):
      _index = self._idxinfo_[_index]
    elif not isinstance(_index, TableIndex):
      raise ValueError('Unhandled instance for an index')
    if _index == self._curindex:
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
      self._isobj_.isstart(_index.as_keydesc(self), smode, _record._buffer)
      self._isobj_.isread(_record._buffer, ReadMode.ISNEXT)

      # Make this the current index for the next invocation of the method
      self._curindex_ = _index
    # Return the record which can then be used as required
    return _record
  def insert(self, _record=None, *args, **kwd):
    'Insert a record'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
  def delete(self, keybuff=None, recbuff=None, *args, **kwd):
    'Delete a record from the underlying ISAM table'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    # If KEYBUFF is None then invoke isdelcurr
    # If KEYBUFF is not None then invoke isdelete
  def update(self, keybuff=None, recbuff=None, *args, **kwd):
    'Update an existing record ...'
  def lock(self, *args, **kwd):
    'Lock the underlying ISAM table'
  def unlock(self, *args, **kwd):
    'Unlock the underlying ISAM table'
  def release(self):
    'Release all the locks on the underlying ISAM table'
  def dictinfo(self):
    'Return an instance of dictinfo for the underlying ISAM table'
  def indexinfo(self, idxnum):
    'Return an instance of ISAMindexInfo for the given IDXNUM on the underlying ISAM table'
  def _gt_uniqueid(self):
    'Get the current uniqueid from the underlying ISAM table (this autoincrements)'
  def _st_uniqueid(self, newval):
    'Set the uniqueid for the underlying ISAM table'
  uniqueid = property(_gt_uniqueid, _st_uniqueid, doc='Access the uniqueid for the table')
  def _dump(self,ofd):
    # FIXME: This needs to be updated for the new version of the object
    ofd.write('Table: {}\n'.format(self.__class__.__name__))
    ofd.write('  Columns:\n')
    for col in self._defn_._colinfo.items():
      ofd.write('     {}\n'.format(col))
    ofd.write('  Indexes:\n')
    self._prepare_kdesc()
    for idxname,idxinfo in self._kdesc.items():
      ofd.write('     {}: {}\n'.format(idxname,idxinfo))
