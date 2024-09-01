'''
This module provides a way of defining a table dynamically at runtime without
needing to know the layout prior to running.

In operation, an application would create an instance of the DynamicTableDefn
object and then manually add the required fields and indexes before then
calling the table.ISAMtable object passing the dynamic table definition.
'''

from collections import OrderedDict
from . import TableDefnColumn, TableDefnIndex, TableDefnIndexCol
from ..error import TableDefnError

__all__ = ('DynamicTableDefn',)

class DynamicTableDefn:
  def __init__(self, tabname=None, fields=None, indexes=None, error=False):
    self._tabname = tabname
    self._columns = OrderedDict()
    self._indexes = dict()
    self._prefix = None
    self._error = error    # Store fact that errors raise an exception
    self.bad = ([], [])    # Bad elements: [0] = field, [1] = indexes
    if isinstance(fields, (list, tuple)):
      self.extend(fields)
    if isinstance(indexes, (list, tuple)):
      for idx in indexes:
        self.add_index(idx, error=error)
    elif isinstance(indexes, TableDefnIndex):
      self.add_index(idx, error=error)
    elif isinstance(indexes, dict):
      for idx in indexes:
        self.add_index(indexes[idx], error=error)
    elif indexes is not None:
      raise TableDefnError('Indexes must be derived from TableDefnIndex')

  def _raise_error(self, error, field, bad, excptclass):
    '''Check whether to an error should raise an Exception, if so then
       raised the EXCLASS, otherwise add to BAD'''
    # TODO: Need to remove ourselves from the traceback on exception raising
    if error or self._error:
      raise excptclass
    else:
      bad.append(field)

  def check(self):
    'Check if the table definition has any problems'
    return self.bad[0] or self.bad[1]

  def append(self, field, error=None):
    'Add a field to the table definition which should be an instance of TableDefnColumn'
    if not isinstance(field, TableDefnColumn):
      self._raise_error(error, field, self.bad[0],
                        TableDefnError('Field must be an instance of TableDefnColumn'))
    if field.name in self._columns:
      self._raise_error(error, field, self.bad[0],
                        TableDefnError('Field already present in the table definition'))
    self._columns[field.name] = field

  def extend(self, fields, error=None):
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
      raise TableDefnError('Fields must be retrievable in order on table, use an OrderedDict')
    elif fields is not None:
      raise TableDefnError('Only sequences and OrderedDict can be given to define fields')

  def add_index(self, index, error=None):
    'Add an index to the table definition which should be an instance of TableDefnIndex'
    if not isinstance(index, TableDefnIndex):
      self._raise_error(error, index, self.bad[1],
           TableDefnError('Index must be an instance of TableDefnIndex'))
    if index.name in self._indexes:
      if self._raise_error(error):
        raise TableDefnError('Index already present in the table definition')
    if not isinstance(index.colinfo, (list, tuple)):
      if not isinstance(index.colinfo, TableDefnIndexCol):
        raise TableDefnError('Index column should be instance of TableDefnIndexCol')
      if index.colinfo.name not in self._columns_:
        raise TableDefnError('Index contains a column not present in the table definition')
    else:
      for col in index.colinfo:
        if not isinstance(col, TableDefnIndexCol):
          raise TableDefnError('Index column should be an instance of TableDefnIndexCol')
        if col.name not in self._columns:
          raise TableDefnError('Index contains a column not present in the table definition')
    self._indexes[index.name] = index
