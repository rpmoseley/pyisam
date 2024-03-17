'''
This module provides a structured way of accessing tables using ISAM by providing objects that
permit the table layout to be defined and referenced later.
'''

import os
from .index import TableIndex, create_TableIndex
from .record import create_record_class, ISAMrecordBase
from ..backend import _backend
from ..constants import ReadMode, StartMode, dflt_openmode, dflt_lockmode
from ..error import IsamIterError, IsamNotOpen, IsamError, IsamNoPrimaryIndex
from ..isam import ISAMobject
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
    if tabind is None or isinstance(tabind, TableIndex):
      self._tabind = tabind
    else:
      raise TypeError("Must provide an instance of 'TableIndex' for 'tabind'")

    # Set the underlying index information
    if keydesc is None or isinstance(keydesc, _backend.ISAMkeydesc):
      self._keydesc = keydesc
    else:
      raise TypeError("Must provide an instance of 'ISAMkeydesc' for 'keydesc'")

    # Set the application index name
    if idxname is None or isinstance(idxname, str):
      self._idxname = idxname
    else:
      raise TypeError("Must provide a string for 'idxname'")
      
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
    kd = self._keydesc
    if kd is None:
      if self._tabind is None:
        raise ValueError("Attempt to return keydesc of undefined 'TableIndex'")
      kd = self._keydesc = self._tabind.as_keydesc(isobj, record)
    return kd
    
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

  @property
  def nextnum(self):
    # Return the current value of _nextnum but increment for next time
    ret = self._nextnum
    self._nextnum += 1
    return ret
  
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
    if idxdesc is not None and not isinstance(idxdesc, _backend.ISAMkeydesc):
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
    except KeyError:
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
      raise ValueError(f'Index number has already been associated to: {prevname}')
    
    # Associate the index number to the actual name
    self._idxmap[idxnum] = self._idxmap[idxname]
    
    # Update the underlying ISAM keydesc information
    if keydesc is not None:
      # FIXME self._idxmap[idxnum]
      raise NotImplementedError('Fix the update of underlying keydesc information')

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
  # _slots_ = '_isobj', '_name', '_path', '_mode', '_lock', '_database',
  #           '_prefix', '_curidx', '_record', '_recsize', '_row'
  def __init__(self, tabdefn, tabname=None, tabpath=None, isobj=None, **kwds):
    self._defn = tabdefn
    self._name = tabdefn._tabname if tabname is None else tabname
    self._path = tabpath if tabpath else '.'
    self._mode = kwds.get('mode', dflt_openmode)
    self._lock = kwds.get('lock', dflt_lockmode)
    self._isobj = isobj if isinstance(isobj, ISAMobject) else ISAMobject(**kwds)
    self._database = getattr(tabdefn, '_database', None)
    self._prefix = getattr(tabdefn, '_prefix', None)
    self._record = kwds.get('recordclass', getattr(tabdefn, '_recinfo', None))
    if self._record is None:
      # NOTE: The following allocates a low-level buffer for the record, but
      # NOTE: then another instance is allocated for this object itself.
      self._record = create_record_class(tabdefn)
    self._idxinfo = TableIndexMapping(self)  # Mapping of indexes on this table's object
    self._primary = None             # Set to the primary index or first otherwise
    self._curindex = None            # Current index being used in isread/isstart
    self._row = None                 # Cannot allocate record until table is opened
    self._recsize = None             # Length of record is given after table is opened
    self._debug = bool(kwds.get('debug', False))
    self._varlen = kwds.get('varlen', None)
      
    # Define the indexes found in the definition object
    self._add_defn_indexes()

  def _add_defn_indexes(self):
    'Add the indexes defined in the table definition'
    def _addidxdefn(index, _debug=False):
      if not isinstance(index, TableDefnIndex):
        raise NotImplementedError('Unhandled index definition object')
      if _debug:
        print('CONV: Convert TableDefnIndex to TableIndex:', index.name)
      colinfo = index.colinfo if isinstance(index.colinfo, (tuple, list)) else [index.colinfo]
      self._add_idxinfo(TableIndex(index.name, *colinfo, dups=index.dups, desc=index.desc))

    idxdefn = self._defn._indexes
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
    if idxname in self._idxinfo:
      raise ValueError('Existing index of same name already present')
    self._idxinfo.add(idxname, tabind=idxinfo)
    if self._primary is None:
      self._primary = idxinfo

  def __str__(self):
    return 'No data available' if self._row is None else str(self._row)

  # TODO: This will be enabled when context manager features are implemented:
  # def __enter__(self):
  #   # Open the table using default mode, lock and paths
  #   self._isobj.isopen(self._name, None, None)
  #   return self
  # def __exit__(self, type_, value, traceback):
  #   # Close the table
  #   self._isobj.isclose()

  def _default_record(self):
    'Return the default record or initialise it if not already present'
    if self._row is None:
      self._row = self._record(self._name)
    return self._row  
    
  def _colinfo(self, colname):
    'Return the column information by name'
    return self._defn._columns.get(colname)
  
  @property
  def isclosed(self, error=False):
    'Check whether the underlying ISAM table has been opened'
    ret = self._isobj is None or self._isobj._isfd is None
    if error and ret:
      raise IsamNotOpen()
    return ret

  def addindex(self, index):
    'Add an instance of TableIndex to the underlying ISAM table'
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide an instance of TableIndex for the index to add')
    kd = index.as_keydesc(self)
    # Check if the index is already present on the table
    print('ADDIDX:',kd)  #DEBUG
    # TODO Complete
    raise NotImplementedError

  def delindex(self, index):
    'Remove the instance of TableIndex from the underlying ISAM table'
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide an instance of TableIndex for the index to remove')
    kd = index.as_keydesc(self)
    print('DELIDX:',kd)
    # TODO Complete 
    raise NotImplementedError

  def build(self, tabpath=None, varlen=None, **kwd):
    'Build a new ISAM table using the definition provided in the optional TABPATH'
    # Locate the primary index in the definition
    if not hasattr(self, '_primary') or self._primary is None:
      raise IsamError('Must provide a primary index when building table')
    index = self._LookupPrimaryIndex()
    path = os.path.join(self._path if tabpath is None else tabpath, self._name)
    reclen = self._record._recsize
    self._isobj_.isbuild(path, reclen, index.as_keydesc(self._isobj, self._record))

  def open(self, tabpath=None, mode=None, lock=None, **kwd):
    'Open an existing ISAM table with the specified mode, lock and TABPATH'
    if mode is None:
      mode = self._mode
    if lock is None:
      lock = self._lock
    path = os.path.join(self._path if tabpath is None else tabpath, self._name)
    self._isobj.isopen(path, mode, lock)
    self._recsize = self._isobj._recsize  # Provided by the ISAM library
    # self._update_indexes() # NOTE: Defer this to when required

  def cluster(self, index):
    'Re-organize the ISAM table using the specified INDEX'
    if not isinstance(index, TableIndex):
      raise ValueError('Must provide an instance of ISAMindex for the index')
    self._isobj.iscluster(index.as_keydesc(self))

  def close(self):
    'Close the underlying ISAM table'
    self._isobj.isclose()

  def read(self, index=None, mode=None, recnum=None, *args, **kwd):
    'Return the appropriate record into RECBUFF according to the MODE specified'
    # This function combines the isread/isstart functions as appropriate with the 
    # current index and the one that is specified by INDEX. If RECNUM is an integer
    # then perform a record order isstart() followed by a isread().
    # If RECBUFF is None then use ARGS and KWD to fill record in self._row
    # If RECBUFF is not None then use ARGS and KWD to fill record in RECBUFF
    
    # If a specified record number was given then switch the index
    if isinstance(index, int):
      kdesc = _backend.ISAMkeydesc()
      kdesc.k_nparts = 0
      
    if index is None and self._curindex is not None:
      # Use the current index for the request
      index = self._curindex
    if mode is None:
      # Assume that the next record is required
      mode = getattr(self, '_lastread', ReadMode.ISNEXT)
    record = kwd.pop('recbuff', None)
    if not isinstance(record, ISAMrecordBase):
      # Use default record buffer for request
      record = self._default_record()
    
    # Get the information about the index
    index = self._LookupPrimaryIndex()
    if index == self._curindex:
      # Using the same index so avoid changing the current index in use
      if mode not in (ReadMode.ISNEXT, ReadMode.ISCURR, ReadMode.ISPREV):
        # Fill the columns that are part of the index 
        index.fill_fields(record, *args, **kwd)
      self._isobj.isread(record._buffer, mode)
      self._recnum = self._isobj.isrecnum
      return record()

    # Convert the required mode into a restart mode
    try:
      smode = StartMode(mode)
    except ValueError:
      if mode == ReadMode.ISNEXT:
        smode = StartMode.ISFIRST
        mode = ReadMode.ISNEXT
      elif mode == ReadMode.ISPREV:
        smode = StartMode.ISLAST
        mode = ReadMode.ISPREV
      elif mode == ReadMode.ISCURR:
        smode = StartMode.ISFIRST
    
    # Fill the index information into the record buffer
    index.fill_fields(record, *args, **kwd)
    
    # Issue the restart followed by the read of the record itself
    self._isobj.isstart(index.as_keydesc(self._isobj, record), smode, record._buffer)
    self._isobj.isread(record._buffer, ReadMode.ISCURR)

    # Make this the current index for the next invocation of the method
    self._curindex = getattr(index, '_tabind', index)

    # Save the record number for use if required
    self._recnum = self._isobj.isrecnum
    
    # Save the mode used for next time if None given
    self._lastread = mode
    
    # Return the record which can then be used as required
    return record()

  def insert(self, recbuff=None, setcurr=False, *args, **kwd):
    'Insert a record'
    if recbuff is None:
      recbuff = self._default_record()
    recbuff._set_value(*args, **kwd)
    return self._isobj.iswrcurr(recbuff) if setcurr else self._isobj.iswrite(recbuff)
    
  def delete(self, keybuff=None, recbuff=None, *args, **kwd):
    'Delete a record from the underlying ISAM table'
    if keybuff is None:
      return self._isobj.isdelcurr()
    elif isinstance(keybuff, int):
      return self._isobj.isdelrec(keybuff)
    elif isinstance(keybuff, TableIndex):
      if recbuff is None:
        recbuff = self._row
      else:
        recbuff._set_value(*args, **kwd)
      return self._isobj.isdelete(recbuff)
    else:
      raise NotImplementedError

  def update(self, keybuff=None, recbuff=None, *args, **kwd):
    'Update an existing record ...'
    if recbuff is None:
      recbuff = self._row
    else:
      recbuff._set_value(*args, **kwd)
    if keybuff is None:
      return self._isobj.isrewcurr(recbuff)
    elif isinstance(keybuff, int):
      return self._isobj.isrewrec(keybuff, recbuff)
    elif isinstance(keybuff, TableIndex):
      return self._isobj.isrewrite(recbuff)
    else:
      raise NotImplementedError

  def lock(self, *args, **kwd):
    'Lock the underlying ISAM table'
    raise NotImplementedError

  def unlock(self, *args, **kwd):
    'Unlock the underlying ISAM table'
    raise NotImplementedError

  def release(self):
    'Release all the locks on the underlying ISAM table'
    raise NotImplementedError

  def dictinfo(self):
    'Return an instance of dictinfo for the underlying ISAM table'
    return self._isobj.isdictinfo()

  def keyinfo(self, idxnum):
    'Return an instance of ISAMindexInfo for the given IDXNUM on the underlying ISAM table'
    kd = self._isobj.iskeyinfo(idxnum)
    return create_TableIndex(kd, self._record, idxnum)

  @property
  def uniqueid(self):
    'Get the current uniqueid from the underlying ISAM table (this autoincrements)'
    raise NotImplementedError

  @uniqueid.setter
  def uniqueid(self, newval):
    'Set the uniqueid for the underlying ISAM table'
    raise NotImplementedError

  def _LookupPrimaryIndex(self):
    'Lookup up the primary index'
    if not hasattr(self, '_primary') or self._primary is None:
      self._autoload_indexes()
    if self._primary is None:
      raise IsamNoPrimaryIndex(self)
    return self._primary

  def _LookupIndex(self, name=None, flags=None):
    """Return the information for the index NAME populating the index cache (_idxinfo_) if
       required."""
    if name is None:
      raise ValueError('Must provide an index')
    elif isinstance(name, TableIndex):
      if self._debug:
        print('LOOKUP_IDX: EXIST:', name, eol='')
      index = name
    elif isinstance(name, TableIndexMapElem):
      if self._debug:
        print('LOOKUP_IDX: EXIST:', name._tabind, eol='')
      index = name._tabind
    elif isinstance(name, str):
      try:
        index = self._idxinfo[name]
        if index is None:
          # Attempt to match the indexes from the underlying ISAM file
          self._match_indexes()
          index = self._idxinfo[name]
        if self._debug:
          print('LOOKUP_IDX: OLDIDX:', index)
      except KeyError:
        # Lookup the index using the table definition as not in the cache
        index = self._defn._indexes.get(name)
        if index is None:
          # Attempt to generate the indexes directly from the underlying ISAM file
          self._autoload_indexes()
          self._match_indexes()         # Match indexes
          index = self._idxinfo[name]
        if self._debug:
          print('LOOKUP_IDX: NEWIDX:', index)
        if index is None:
          raise NotImplementedError
    else:
      raise ValueError(f'Unhandled type of index requested: {name}')
    return index
  
  def _fix_key_colinfo(self, index):
    '''
    Update the index information with the column offset and length if it was
    not provided by the table definition, since this was done before the
    related record object has been created.
    '''
    for colinfo in index._colinfo:
      if colinfo.length is None or colinfo.offset in (None, -1):
        print('WARN: no length/offset provided:', colinfo.name)
        if self._row is not None:
          # There is a default record instance so use that to complete the information
          for fldinfo in self._row._fields:
            if fldinfo.name == colinfo.name:
              print('   ==>', fldinfo)
              # Update the length if not known
              if colinfo.length is None:
                colinfo.length = fldinfo.size
              # Update the offset if not known
              if colinfo.offset in (None, -1):
                colinfo.offset = 0

  def _map_indexes(self, record=None, auto_add=False, match_partial=False, **kwds):
    '''Attempt to map the existing indexes in the underlying table against those in
       the index maptable and optionally add any that are not present'''
    raise NotImplementedError
  
  def _autoload_indexes(self, record=None):
    '''Load the index information from the underlying table matching single field
       indexes directly, whilst providing a unique name for multi-field indexes'''
    # Create a record instance to provide column information if a suitable one not given
    if not isinstance(record, ISAMrecordBase):
      record = self._default_record()
    for idxnum in range(self._isobj.isdictinfo().nkeys):
      keydesc = self._isobj.iskeyinfo(idxnum)
      idxinfo = create_TableIndex(keydesc, record, idxnum)
      self._add_idxinfo(idxinfo)

  def _match_indexes(self, record=None):
    '''Match the indexes found on the underlying table with those provided by the
       table definition object'''
    # FIXME: This needs to be reworked to correctly match the index information
    #        from the table definition against that of the underlying ISAM table,
    #        any multi-field indexes not matched will be left as INDEX*.
    record = self._default_record()
    for idxnum in range(self._isobj.isdictinfo().nkeys):
      keydesc = self._isobj.iskeyinfo(idxnum)
      #keyindex = create_TableIndex(keydesc, record, idxnum) # NOTE: Check how to reverse create a TableIndex instance
      for idxchk in self._idxinfo:
        if idxchk[2] is None:
          chkdesc = idxchk[1].as_keydesc(record, optimize=True)
          if keydesc == chkdesc:
            idxchk[2] = idxnum
            if self._debug:
              print('MATCHED:', idxchk[0], idxnum)
