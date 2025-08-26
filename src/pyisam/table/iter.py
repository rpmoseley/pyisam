'''
This module provides an iterator that will process all the rows that match the
conditions that were given when the iterator was created. This will make the
appropriate low-level method calls.
'''

from .index import TableIndex
from .record import ISAMrecordBase
from .table import ISAMtable
from ..constants import ReadMode

class ISAMiter:
  def __init__(self, tabobj, *args, **kwds):
    # Ensure that we have a correct table object to operate on
    if not isinstance(tabobj, ISAMtable):
      raise ValueError('Must pass an ISAMtable instance to operate on')
    self.tabobj = tabobj

    # Parse the arguments and keywords
    self._parseargs(*args, **kwds)

  def _parseargs(self, *args, **kwds):
    'Parse the arguments appropriately'

    # Calling sequence:
    #   ()                             -> (CURINDEX, CURMODE, RECBUFF) {}
    #   (INDEX)                        ->
    #   (MODE)                         ->
    #   (BUFF)                         ->
    #   (KEYCOL...)                    ->
    #   (INDEX, MODE)                  ->
    #   (INDEX, MODE, KEYCOL...)       ->
    #   (INDEX, BUFF)                  ->
    #   (INDEX, BUFF, KEYCOL...)       ->
    #   (INDEX, MODE, BUFF)            ->
    #   (INDEX, MODE, BUFF, KEYCOL...) ->
    #   (MODE, KEYCOL...)              ->
    #   (MODE, BUFF)                   ->
    #   (MODE, BUFF, KEYCOL...)        ->
    if len(args) + len(kwds) < 1:
      # Assume that the entire table will be processed
      self._initmode = ReadMode.ISFIRST
      self._recmode = ReadMode.ISNEXT
      self._useindex = self.tabobj._PrimaryIndex
      self._condflds = None
      return

    elif len(args) < 1:
      # Assume that the entire table will be processed filtered by keywords
      self._initmode = ReadMode.ISGTEQ
      self._recmode = ReadMode.ISNEXT
      self._useindex = self.tabobj._PrimaryIndex
      self._condflds = kwds
      return

    # Convert the arguments to a list to permit popping values
    largs = list(args)

    # Determine the index to be used
    if isinstance(olargs[0], (ReadMode, ISAMrecordBase)):
      index = None
    elif args[0] is None or isinstance(args[0], (str, int, TableIndex)):
      index = largs.pop(0)
    else:
      raise ValueError('Invalid calling sequence')

    # Determine the initial mode, and calculate the record mode
    if largs and (largs[0] is None or isinstance(largs[0], ReadMode)):
      mode = largs.pop(0)
    else:
      mode = None

    # Determine the record buffer to use for the iterator
    if largs and (largs[0] is None or isinstance(largs[0], ISAMrecordBase)):
      recbuff = largs.pop(0)
    else:
      recbuff = None

    # Prepare new *args and **kwds for the .read() method
    nargs = (useindex, mode, recbuff)
    nkwds = kwds.copy() if kwds else dict()

    # Copy each of the remaining arguments as new entries in the keywords
    # using the column name from the underlying table in order of appearance
    for num, carg in enumerate(largs):
      nkwds[self.tabobj._colinfo(    # TODO: Lookup how columns are defined
      # TODO: in the table definition object: _columns.
