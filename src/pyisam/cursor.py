'''
This module provides the cursor support utilising the concept of rowsets consisting of records
which are defined using the table module.
'''

import collections
from .table import ISAMtable

class ISAMrowset:
  '''Class that provides a result set that is sorted by the rowid'''
  def __init__(self, tabobj, size=None, cursor=None, descend=None):
    if not isinstance(tabobj, ISAMtable):
      raise ValueError('Must provide a ISAMtable object on which to base the rowset')
    self._tabobj_ = tabobj
    self._rows_ = collections.OrderedDict()
    self._maxlen_ = size
    self._cursor_ = cursor
    self._descend_ = descend
    self._current_ = None
  def _first_(self):
    'Return the first row in the result set'
    if len(self._rows_) < 1:
      self._next_()
    return self._rows_[0]
  def _next_(self):
    'Return the next row in the result set, extending if possible'
  def _prev_(self):
    'Return the prev row in the result set' 
  def _last_(self):
    'Return the last row in the result set'
  def _curr_(self):
    'Return the current row in the result set'
  def _rowid_(self):
    'Return the rowid for the current row in the result set'
  def _clear_(self):
    'Clear all rows from the result set'
  def _add_(self, recbuff, rowid=None):
    'Add the given record to the end of the current result set'
  def _del_(self, recbuff, rowid=None):
    'Remove the given record from the current result set'

class ISAMcursor:
  '''Class that provides a cursor like iterator for the underlying ISAM
     table, which provides a scrollable pointer into the current result
     set'''
  def __init__(self, tabobj, index, _descend=False, _cachesize=None, *colarg, **colkey):
    if not isinstance(tabobj, ISAMtable):
      raise ValueError('Must provide a ISAMtable object on which to base the cursor')
    self._tabobj_ = tabobj
    if index not in tabobj._defn_._indexes_:
      raise ValueError('Non-existant index referenced')
    self._rowset_ = ISAMrowset(tabobj, cachesize=_cachesize, cursor=self, descend=_descend)
    # TODO: Complete contents
  def __next__(self):
    # TODO: Complete contents
    pass
