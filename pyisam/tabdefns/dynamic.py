'''
This module provides a way of defining a table dynamically at runtime without
needing to know the layout prior to running.

In operation, an application would create an instance of the DynamicTableDefn
object and then manually add the required fields and indexes before then
calling the table.ISAMtable object passing the dynamic table definition.
'''

from . import TableDefnColumn, TableDefnIndex, TableIndexCol
from collections import OrderedDict
from ..error import TableDefnError

class DynamicTableDefn:
  def __init__(self, tabname=None, fields=None, indexes=None, error=False):
    self._tabname_ = tabname
    self._columns_ = OrderedDict()
    self._indexes_ = dict()
    self.bad = ([], [])    # Bad elements: [0] = field, [1] = indexes
    if fields is not None:
      self.extend(fields)
    if isinstance(indexes, (list, tuple)):
      for idx in indexes:
        self.add(idx, error=error)
    elif isinstance(indexes, TableDefnIndex):
      self.add(idx, error=error)
    elif isinstance(indexes, dict):
      for idx in indexes:
        self.add(indexes[idx], error=error)
    elif indexes is not None:
      raise TableDefnError('Indexes must be derived from TableDefnIndex')
  def check(self):
    'Check if the table definition has any problems'
    return self.bad[0] or self.bad[1]
  def append(self, field, error=False):
    'Add a field to the table definition which should be an instance of TableDefnColumn'
    if not isinstance(field, TableDefnColumn):
      if error:
        raise TableDefnError('Field must be an instance of TableDefnColumn')
      self._bad[0].append(fld)
      return
    if field.name in self._columns_:
      raise TableDefnError('Field already present in the table definition')
    self._columns_[field.name] = field
  def extend(self, fields, error=False):
    'Add a sequence of fields to the table definition'
    if isinstance(fields, (list, tuple)):
      for fld in fields:
        self.append(fld, error=error)
    elif isinstance(fields, OrderedDict):
      for defn in fields.values():
        self.append(defn)
    elif isinstance(fields, TableDefnColumn):
      self.append(fields)
    elif isinstance(fields, dict):
      raise TableDefnError('Fields must be retrivable in order on table, us an OrderedDict')
    elif fields is not None:
      raise TableDefnError('Only sequences and OrderedDict can be given to define fields')
  def add(self, index, error=False):
    'Add an index to the table definition which should be an instance of TableDefnIndex'
    if not isinstance(index, TableDefnIndex):
      if error:
        raise TableDefnError('Index must be an instance of TableDefnIndex')
      self._bad[1].append(idx)
      return
    if index.name in self._indexes_:
      raise TableDefnError('Index already present in the table definition')
    if not isinstance(index.colinfo, (list, tuple)):
      if not isinstance(index.colinfo, TableIndexCol):
        raise TableDefnError('Index column should be instance of TableIndexCol')
      if index.colinfo.name not in self._columns_:
        raise TableDefnError('Index contains a column not present in the table definition')
    else:
      for col in index.colinfo:
        if not isinstance(col, TableIndexCol):
          raise TableDefn('Index column should be an instance of TableIndexCol')
        if col.name not in self._columns_:
          raise TableDefnError('Index contains a column not present in the table definition')
    self._indexes_[index.name] = index
