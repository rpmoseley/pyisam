'''
This module provides the cursor support utilising the concept of rowsets consisting of records
which are defined using the table module.
'''

import collections

class ISAMrowset:
  '''Class that provides a result set that is sorted by the rowid'''
  def __init__(self, tabobj, size=None, cursor=None, descend=None):
    self._tabobj_ = tabobj
    self._rows_ = collections.OrderedDict()
    self._maxlen_ = size
    self._cursor_ = cursor
    self._descend_ = descend
    self._current_ = None
  def _first_(self):
    'Return the first row in the result set'
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
    self._tabobj_ = tabobj
    if index not in tabobj._defn_._indexes_:
      raise ValueError('Non-existant index referenced')
    self._rowset_ = ISAMrowset(tabobj, cachesize=_cachesize, cursor=self, descend=_descend)
    # TODO: Complete contents
  def __next__(self):
    # TODO: Complete contents
    pass
