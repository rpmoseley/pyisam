'''
This module provides a structured way of accessing tables using ISAM by providing objects that
permit the table layout to be defined and referenced later.
'''

import os
from .index import TableIndex
from .index import create_TableIndex
from .record import recordclass, ISAMrecordBase
from ..constants import ReadMode, StartMode, dflt_openmode, dflt_lockmode
from ..error import IsamIterError, IsamNotOpen, IsamOpen
from ..isam import ISAMobject, ISAMkeydesc
from ..tabdefns import TableDefnIndex

# Define the objects that are available using 'from .table import *'
__all__ = ('ISAMtable',)

class TableIndexMapElem:
  '''Class that provides an element in the table index mapping object.
     It is also used at application level to provide index information on a
     table by table basis. It exhibits the look and feel of a list with additional
     functionality such as the ability to fill in the fields that make up an index
     using the record associated with the table.
  '''
  def __init__(self, tabind=None, keydesc=None, idxnum=-1, idxname=None):
    # Set the table index information
    if tabind:
      if isinstance(tabind, TableIndex):
        self._tabind = tabind
      else:
        raise TypeError("Must provide an instance of 'TableIndex' for 'tabind'")
    else:
      self._tabind = None

    # Set the underlying index information
    if keydesc:
      if isinstance(keydesc, ISAMkeydesc):
        self._keydesc = keydesc
      else:
        raise TypeError("Must provide an instance of 'ISAMkeydesc' for 'keydesc'")
    else:
      self._keydesc = None

    # Set the application index name
    if idxname:
      if isinstance(idxname, str):
        self._idxname = idxname
      else:
        raise TypeError("Must provide a string for 'idxname'")
    else:
      self._idxname = None
      
    # Set the application index number
    self._idxnum = None if idxnum < 0 else idxnum

  def __getitem__(self, num):
    'Return the items as though we are a sequence'
    if isinstance(num, int) and 0 <= num < 3:
      if num == 0:
        return self._tabind
      elif num == 1:
        return self._keydesc
      else:
        return self._idxnum
    elif isinstance(num, str):
      if num == 'tabind':
        return self._tabind
      elif num == 'keydesc':
        return self._keydesc
      elif num == 'idxnum':
        return self._idxnum
    raise ValueError("Unhandled request: '{}'".format(num))

  def update(self, idxname=None, idxnum=-1, tabind=None, keydesc=None, info=None):
    if isinstance(info, TableIndexMapElem):
      if info._idxname is not None and self._idxname is None:
        self._idxname = info._idxname
      if info._idxnum >= 0 and self._idxnum < 0:
        self._idxnum = info._idxnum
      if info._tabind is not None and self._tabind is None:
        self._tabind = info._tabind
      if info._keydesc is not None and self._keydesc is None:
        self._keydesc = info._keydesc
    elif info is not None:
      raise ValueError('Unhandled type of info given')
    else:
      if idxname is not None and self._idxname is None:
        self._idxname = idxname
      if idxnum >= 0 and self._idxnum < 0:
        self._idxnum = idxnum
      if tabind is not None and self._tabind is None:
        self._tabind = tabind
      if keydesc is not None and self._keydesc is None:
        self._keydesc = keydesc
      
  def fill_fields(self, record, *args, **kwds):
    if self._tabind is None:
      raise ValueError("'Attempt to fill fields of undefined 'TableIndex'")
    return self._tabind.fill_fields(record, *args, **kwds)

  def as_keydesc(self, isobj, record):
    if self._keydesc:
      return self._keydesc
    if self._tabind is None:
      raise ValueError("Attempt to return keydesc of undefined 'TableIndex'")
    return self._tabind.as_keydesc(isobj, record)

    
class TableIndexMapping:
  '''Class that provides the current known indexes on a given table, it is
     provided so that the correct underlying CISAM index number can be maintained
     whilst also enabling the indexes to be accessed by name, this enables a
     table which has some index(es) not provided in the table definition to be
     selected using a TABLE.read() call.'''
  def __init__(self, tabobj=None):
    # Store the owning table instance
    if tabobj is not None and not isinstance(tabobj, ISAMtable):
      raise TypeError('Table object needs to be an instance of pyisam.table.IsamTable')
    self._tabobj = tabobj

    # Stores mapping from name to index number, TableIndex and CISAM keydesc;
    #             or from index number to name, TableIndex and CISAM keydesc.
    self._idxmap = {}
    # Incrementing integer used to number indexes for this instance
    self._nextnum = 0

  def gt_nextnum(self):
    # Return the current value of _nextnum but increment for next time
    ret = self._nextnum
    self._nextnum += 1
    return ret
  nextnum = property(gt_nextnum)
  
  def __getitem__(self, key):
    if isinstance(key, int) and key < 0:
      raise ValueError('Indexes must be accessed using positive integers')
    try:
      # return self._idxmap[key][-1]
      return self._idxmap[key]
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
    if idxdesc is not None and not isinstance(idxdesc, ISAMkeydesc):
      raise ValueError('Must pass an instance of keydesc or None')
    # Create a new item element to share instances
    newinfo = TableIndexMapElem(tabind, idxdesc, idxnum, idxname)
    # Update the mapping using the index name
    try:
      self._idxmap[idxname].update(newinfo)
    except KeyError:
      self._idxmap[idxname] = newinfo
    # Update the mapping using the index number
    try:
      self._idxmap[idxnum].update(newinfo)
    except:
      self._idxmap[idxnum] = newinfo

  def remove(self, name_or_idxnum):
    'Remove an index from the list'
    if isinstance(name_or_idxnum, int):
      # Remove an index by its number, maintaining the number of index positions
      name = self._idxmap[name_or_idxnum].idxname
      idxnum = name_or_idxnum
    elif name_or_idxnum in self._idxmap:
      # Remove an index by its name
      name = name_or_idxnum
      idxnum = self._idxmap[name_or_idxnum].idxnum
    else:
      raise ValueError('Unhandled type of object: {}'.format(name_or_idxnum))
    # Remove the elements by both index name and number
    del self._idxmap[name], self._idxmap[idxnum]      

  def assoc_index_number(self, idxnum, idxname, keydesc=None):
    '''Associate the given index number as the specified name, unless
       it has already been associated.'''
    # Check if the index number has been used previously
    prevname = self._idxmap.get(idxnum, None)
    if prevname is not None:
      raise ValueError('Index number has already been associated to: {}'.format(prevname))
    
    # Associate the index number to the actual name
    self._idxmap[idxnum] = self._idxmap[idxname]
    
    # Update the underlying ISAM keydesc information
    if keydesc is not None:
      # FIXME self._idxmap[idxnum]
      pass

  # Provide an iterator over the current indexes
  def __iter__(self):
    if self._idxnext is not None:
      raise IsamIterError('Can only perform one iterator on index map at a time')
    self._idxnext = 0
    return self

  def __next__(self):
    try:
      value = self._idxmap[self._idxnext]
      self._idxnext += 1
      return value
    except KeyError:
      self._idxnext = None
      raise StopIteration


class ISAMtable:
  '''Class that represents a table within ISAM, the associated record object
     provides the column information that includes the underlying raw byte
     record area used by the ISAM library. -+- this holds all the column
     and index information and also provides the get/set functionality
     with dictionary like access. It will match up the index information
     with the information stored within the table file itself.'''
  # _slots_ = '_isobj_', '_name_', '_path_', '_mode_', '_lock_', '_database_',
  #           '_prefix_', '_curidx_', '_record_', '_recsize_', '_row_'
  def __init__(self, tabdefn, tabname=None, tabpath=None, isobj=None, **kwds):
    self._defn_ = tabdefn
    self._name_ = tabdefn._tabname_ if tabname is None else tabname
    self._path_ = tabpath if tabpath else '.'
    self._mode_ = kwds.get('mode', dflt_openmode)
    self._lock_ = kwds.get('lock', dflt_lockmode)
    self._isobj_ = isobj if isinstance(isobj, ISAMobject) else ISAMobject(**kwds)
    self._database_ = getattr(tabdefn, '_database_', None)
    self._prefix_ = getattr(tabdefn, '_prefix_', None)
    self._record_ = kwds.get('recordclass', recordclass(tabdefn))
    self._idxinfo_ = TableIndexMapping(self)  # Mapping of indexes on this table's object
    self._primary_ = None             # Set to the primary index or first otherwise
    self._curindex_ = None            # Current index being used in isread/isstart
    self._row_ = None                 # Cannot allocate record until table is opened
    self._recsize_ = None             # Length of record is given after table is opened
    self._debug = bool(kwds.get('debug', False))
    self._varlen_ = kwds.get('varlen', None)
      
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

  def _colinfo(self, colname):
    'Return the column information by name'
    return self._defn_._columns_.get(colname)
  
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
      reclen = self._record_._recsize
    else:
      self._isobj_.isreclen, reclen = varlen
    self._isobj_.isbuild(path, reclen, index.as_keydesc(self._isobj_, self._record_))

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
      self._isobj_.isstart(_index.as_keydesc(self._isobj_, _record), smode, _record._buffer)
      self._isobj_.isread(_record._buffer, ReadMode.ISNEXT)

      # Make this the current index for the next invocation of the method
      try:
        self._curindex_ = _index._tabind
      except AttributeError:
        self._curindex_ = _index

    # Return the record which can then be used as required
    return _record()

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
      if (isinstance(flags, (list, tuple, set)) and 'NEED_PRIMARY' in flags) or \
         (isinstance(flags, str) and flags.find('NEED_PRIMARY') >= 0):
        if self._primary_ is None:
          self._autoload_indexes()
        if self._debug: print('LOOKUP_IDX: ASSUME:', self._primary_)
        return self._primary_
      raise ValueError('No index name provided and flags are not supported')
    elif isinstance(name, TableIndex):
      if self._debug: print('LOOKUP_IDX: EXIST:', name, eol='')
      index = name
    elif isinstance(name, TableIndexMapElem):
      if self._debug: print('LOOKUP_IDX: EXIST:', name._tabind, eol='')
      index = name._tabind
    elif isinstance(name, str):
      try:
        index = self._idxinfo_[name]
        if index is None:
          # Attempt to match the indexes from the underlying ISAM file
          self._match_indexes()
          index = self._idxinfo_[name]
        if self._debug: print('LOOKUP_IDX: OLDIDX:', index)
      except KeyError:
        # Lookup the index using the table definition as not in the cache
        index = self._defn_._indexes_.get(name)
        if index is None:
          # Attempt to generate the indexes directly from the underlying ISAM file
          self._autoload_indexes()
          index = self._idxinfo_[name]
        if self._debug: print('LOOKUP_IDX: NEWIDX:', index)
        if index is None:
          raise NotImplementedError
    else:
      raise ValueError(f'Unhandled type of index requested: {name}')
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

  def _map_indexes(self, record=None, auto_add=False, match_partial=False, **kwds):
    '''Attempt to map the existing indexes in the underlying table against those in
       the index maptable and optionally add any that are not present'''

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

  def _match_indexes(self, record=None):
    '''Match the indexes found on the underlying table with those provided by the
       table definition object'''
    # FIXME: This needs to be reworked to correctly match the index information
    #        from the table definition against that of the underlying ISAM table,
    #        any multi-field indexes not matched will be left as INDEX*.
    record = self._default_record()
    for idxnum in range(self._isobj_.isdictinfo().nkeys):
      keydesc = self._isobj_.iskeyinfo(idxnum)
      keyindex = create_TableIndex(keydesc, record, idxnum) # NOTE: Check how to reverse create a TableIndex instance
      for idxchk in self._idxinfo_:
        if idxchk[2] is None:
          chkdesc = idxchk[1].as_keydesc(record, optimize=True)
          if keydesc == chkdesc:
            idxchk[2] = idxnum
            if self._debug: print('MATCHED:', idxchk[0], idxnum)
