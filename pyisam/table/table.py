'''
This module provides a structured way of accessing tables using ISAM by providing objects that
permit the table layout to be defined and referenced later.
'''

import os
from .index import TableIndex, PrimaryIndex, DuplicateIndex, UniqueIndex
from .index import create_TableIndex
from .record import recordclass, ISAMrecordBase
from ..constants import ReadMode, StartMode
from ..error import IsamException, IsamOpened, IsamNoIndex
from ..isam import ISAMobject
from ..tabdefns import TableDefnIndex

# Define the objects that are available using 'from .table import *'
__all__ = ('ISAMtable',)

class _IndexMapping:
  '''Class that provides the current known indexes on a given table, it is
     provided so that the correct underlying CISAM index number can be maintained
     whilst also enabling the indexes to be accessed by name, this enables a
     table which has some index(es) not provided in the table definition to be
     selected using a TABLE.read() call.'''
  def __init__(self):
    # Stores mapping from name to index number, TableIndex and CISAM keydesc;
    #             or from index number to name, TableIndex and CISAM keydesc.
    self._idxmap = {}
    # Stores the highest index number referenced
    self._idxmax = -1

  def __getitem__(self, key):
    if isinstance(key, int) and key < 0:
      raise ValueError('Indexes must be accessible using positive integers')
    try:
      return self._idxmap[key][-1]
    except KeyError:
      print('KEYDICT:', self._idxmap)
      raise

  def __setitem__(self, key, value):
    if isinstance(key, int):
      raise Warning('Use: X.add(idxnum, idxname=None, tabind=None, keydesc=None')
    else:
      raise Warning('Use: X.add(idxname, idxnum=None, tabind=None, keydesc=None')

  def __contains__(self, key):
    return key in self._idxmap

  @property
  def nextnum(self):
    self._idxmax += 1
    return self._idxmax

  def add(self, idxname=None, idxnum=None, tabind=None, idxdesc=None):
    'Add a new index or update an existing index'
    # Check if either an index number or name has been given
    if idxname is None and idxnum is None:
      raise ValueError('Either an index name or number needs to be given')
    elif idxname is None:
      idxname = 'Index{}'.format(self.nextnum)
    elif idxnum is None:
      idxnum = self.nextnum
    # Check if a negative index number has been given
    if idxnum < 0:
      raise ValueError('Must reference an index with a positive number')
    # Check if an TableIndex instance has been provided
    if tabind is not None and not isinstance(tabind, TableIndex):
      raise ValueError('Must pass an instance of TableIndex or None')
    # Check if a CISAM keydesc has been provided
    if idxdesc is not None and not isinstance(idxdesc, keydesc):
      raise ValueError('Must pass an instance of keydesc or None')
    # Update the mapping using the index name
    if idxname in self._idxmap:
      # Update existing index information
      # FIXME handle the case of the underlying information changing
      idxinfo = self_idxmap[idxname]
      if idxinfo[0] is None:
        idxinfo[0] = idxnum
      elif idxinfo[0] != idxnum:
        old_idxnum = idxinfo[0]   # Save old index number
        # TODO: Need to replace the associated index number with a new entry handling potential clashes
      if idxinfo[1] is None:
        idxinfo[1] = tabind
      if idxinfo[2] is None:
        idxinfo[2] = idxdesc
    else:
      # Add new index information
      self._idxmap[idxname] = [idxnum, tabind, idxdesc]
    # Update the mapping using the index number
    if idxnum in self._idxmap:
      # Update existing index information
      idxinfo = self._idxmap[idxnum]
      if idxinfo[0] is None:
        idxinfo[0] = idxname
      if idxinfo[1] is None:
        idxinfo[1] = tabind
      if idxinfo[2] is None:
        idxinfo[2] = idxdesc
    else:
      # Add new index information
      self._idxmap[idxnum] = [idxname, tabind, idxdesc]

  def remove(self, name_or_idxnum):
    'Remove an index from the list'
    if isinstance(name_or_idxnum, int):
      # Remove an index by its number, maintaining the number of index positions
      name = self._idxmap[name_or_idxnum][0]
      idxnum = name_or_idxnum
    elif name_or_idxnum in self._idxmap:
      # Remove an index by its name
      name = name_or_idxnum
      idxnum = self._idxmap[name_or_idxnum][0]
    else:
      raise ValueError('Unhandled type of object: {}'.format(name_or_idxnum))
    # Remove the elements by both index name and number
    del self._idxmap[name], self._idxmap[idxnum]      

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
    self._idxinfo_ = _IndexMapping()  # Mapping of indexes on this table's object
    self._primary_ = None             # Set to the primary index or first otherwise
    self._curindex_ = None            # Current index being used in isread/isstart
    self._row_ = None                 # Cannot allocate record until table is opened
    self._recsize_ = None             # Length of record is given after table is opened

    # Define the indexes found in the definition object
    self._add_defn_indexes()

  def _add_defn_indexes(self):
    'Add the indexes defined in the table definition'
    def _addidxdefn(index, _debug=False):
      if not isinstance(index, TableDefnIndex):
        raise NotImplementedError('Unhandled index definition object')
      if _debug: print('CONV: Convert TableDefnIndex to TableIndex:', index.name)
      colinfo = index.colinfo if isinstance(index.colinfo, (tuple, list)) else [index.colinfo]
      self._add_idxinfo(TableIndex(index.name, *colinfo, dups=index.dups, desc=index.desc))

    idxdefn = self._defn_._indexes_
    if isinstance(idxdefn, dict):
      for index in idxdefn.values():
        _addidxdefn(index)
    elif isinstance(idxdefn, (tuple, list)):
      for index in idxdefn:
        _addidxdefn(index)
    else:
      _addidxdefn(idxdefn)

  def _add_idxinfo(self, idxinfo, idxname=None):
    'Add an index to the internal dictionary of known indexes'
    if not isinstance(idxinfo, TableIndex):
      raise ValueError('Expecting an instance of TableIndex')
    if idxname is None:
      idxname = idxinfo.name
    if idxname in self._idxinfo_:
      raise ValueError('Existing index of same name already present')
    self._idxinfo_.add(idxname, tabind=idxinfo)
    if self._primary_ is None:
      self._primary_ = idxinfo

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

  def _createrecord(self, fields=None):
    'Return a new instance of the record classs for this table'
    return self._record_(self._name_, fields=fields)

  def _default_record(self):
    'Return the default record or initialise it not already present'
    if self._row_ is None:
      self._row_ = self._createrecord()
    return self._row_

  @property
  def isclosed(self):
    'Check whether the underlying ISAM table has been opened'
    return True if self._isobj_ is None else self._isobj_._isfd_ is None

  def addindex(self, index):
    'Add an instance of TableIndex to the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide an instance of TableIndex for the index to add')
    kd = index.as_keydesc(self)
    # Check if the index is already present on the table
    print('ADDIDX:',kd)  #DEBUG
    # TODO Complete
    raise NotImplementedError

  def delindex(self, index):
    'Remove the instance of TableIndex from the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide an instance of TableIndex for the index to remove')
    kd = index.as_keydesc(self)
    print('DELIDX:',kd)
    # TODO Complete 
    raise NotImplementedError

  def build(self, tabpath=None, **kwd):
    'Build a new ISAM table using the definition provided in the optional TABPATH'
    if self._isobj_._isfd_ is not None: raise IsamOpened
    # Locate the primary index in the definition
    index = self._LookupIndex(flags='NEED_PRIMARY')
    path = os.path.join(self._path_ if tabpath is None else tabpath, self._name_)
    varlen = kwd.get('varlen', None)
    if varlen is None:
      raise ValueError('Length of record not provided')
    self._isobj_.isreclen = int(varlen)
    self._isobj_.isbuild(path, reclen, index.as_keydesc(self))

  def open(self, tabpath=None, mode=None, lock=None, **kwd):
    'Open an existing ISAM table with the specified mode, lock and TABPATH'
    if self._isobj_._isfd_ is not None: raise IsamOpened
    if mode is None: mode = self._mode_
    if lock is None: lock = self._lock_
    path = os.path.join(self._path_ if tabpath is None else tabpath, self._name_)
    self._isobj_.isopen(path, mode, lock)
    self._recsize_ = self._isobj_.isreclen  # Provided by the ISAM library
    # self._update_indexes() # NOTE: Defer this to when required

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
      _record = self._default_record()
    # Get the information about the index
    _index = self._LookupIndex(_index, flags='NEED_PRIMARY')
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
    raise NotImplementedError

  def delete(self, keybuff=None, recbuff=None, *args, **kwd):
    'Delete a record from the underlying ISAM table'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    # If KEYBUFF is None then invoke isdelcurr
    # If KEYBUFF is not None then invoke isdelete
    if self._isobj_._isfd_ is None: self.open(**kwd)
    raise NotImplementedError

  def update(self, keybuff=None, recbuff=None, *args, **kwd):
    'Update an existing record ...'
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row_
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    # If KEYBUFF is None then invoke isdelcurr
    # If KEYBUFF is not None then invoke isdelete
    raise NotImplementedError

  def lock(self, *args, **kwd):
    'Lock the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    raise NotImplementedError

  def unlock(self, *args, **kwd):
    'Unlock the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    raise NotImplementedError

  def release(self):
    'Release all the locks on the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    raise NotImplementedError

  def dictinfo(self):
    'Return an instance of dictinfo for the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    return self._isobj_.isdictinfo()

  def indexinfo(self, idxnum):
    'Return an instance of ISAMindexInfo for the given IDXNUM on the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    keydesc = self._isobj_.iskeyinfo(idxnum)
    raise NotImplementedError

  @property
  def uniqueid(self):
    'Get the current uniqueid from the underlying ISAM table (this autoincrements)'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    raise NotImplementedError

  @uniqueid.setter
  def uniqueid(self, newval):
    'Set the uniqueid for the underlying ISAM table'
    if self._isobj_._isfd_ is None: raise IsamNotOpened
    raise NotImplementedError

  def _LookupIndex(self, name=None, flags=None):
    """Return the information for the index NAME populating the index cache (_idxinfo_) if
       required."""
    if name is None:
      # Handle special case situations
      if (isinstance(flags, set) and 'NEED_PRIMARY' in flags) or \
          flags == 'NEED_PRIMARY':
        if self._primary_ is None:
          self._autoload_indexes()
        print('ASSUME:', self._primary_)
        return self._primary_
      raise ValueError('No index name provided and flags are not supported')
    elif isinstance(name, TableIndex):
      index = name
    elif isinstance(name, str):
      try:
        index = self._idxinfo_[name]
      except KeyError:
        # Lookup the index using the table definition as not in the cache
        index = self._defn_._indexes_.get(name)
        if index is None:
          # Attempt to generate the indexes directly from the underlying ISAM file
          self._autoload_indexes()
          index = self._idxinfo_[name]
        print('NEWIDX:',index)
        if index is None:
          raise NotImplementedError
    else:
      raise ValueError('Unhandled type of index requested')
    """ TEMP remove logic
    # Update the index information with the column offset and length if it was
    # not provided by the table definition, since this was done before the
    # related record object has been created.
    for colinfo in index._colinfo:
      if colinfo.length is None or colinfo.offset in (None, -1):
        print('WARN: no length/offset provided:', colinfo.name)
        if self._row_ is not None:
          # There is a default record instance so use that to complete the information
          for fldinfo in self._row_._fields:
            if fldinfo.name == colinfo.name:
              print('   ==>', fldinfo)
              # Update the length if not known
              if colinfo.length is None:
                colinfo.length = fldinfo.size
              # Update the offset if not known
              if colinfo.offset in (None, -1):
                colinfo.offset = 0
    """
    return index

  def _autoload_indexes(self, record=None):
    '''Load the index information from the underlying table matching single field
       indexes directly, whilst providing a unique name for multi-field indexes'''
    # Create a record instance to provide column information if a suitable one not given
    if not isinstance(record, ISAMrecordBase):
      record = self._default_record()
    for idxnum in range(self._isobj_.isdictinfo().nkeys):
      keydesc = self._isobj_.iskeyinfo(idxnum)
      idxinfo = create_TableIndex(keydesc, record, idxnum)
      self._add_idxinfo(idxinfo)

  def _match_indexes(self):
    '''Match the indexes found on the underlying table with those provided by the
       table definition object'''
    # FIXME: This needs to be reworked to correctly match the index information
    #        from the table definition against that of the underlying ISAM table,
    #        any multi-field indexes not matched will be left as INDEX*.
    record = self._default_record()
    for idxnum in range(self._isobj_.isdictinfo().nkeys):
      keydesc = self._isobj_.iskeyinfo(idxnum)
      keyindex = create_TableIndex(keydesc, record, idxnum) # NOTE: Check how to reverse create a TableIndex instance
      for idxchk in self._idxinfo_.values():
        chkdesc = idxchk.as_keydesc(record, optimize=True)
        if keydesc == chkdesc:
          idxchk.keynum = idxnum
