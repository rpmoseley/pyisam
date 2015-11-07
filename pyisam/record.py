'''
This module provides an object representing the ISAM record where the fields can be
accessed as both attributes as well as by number, the underlying object is an instance
of collections.namedtuple
'''

import collections

class ISAMrecord:
  '''Class providing access to the current record providing access to the columns
     as both attributes as well as dictionary access.'''
  #__slots__ = ('_tabobj_', '_namedtuple_', '_recbuff_')
  def __init__(self, tabobj, idname=None, **kwd):
    if not hasattr(tabobj, '_isobj_'):
      raise ValueError('Definition must be an instance of ISAMtable')
    self._tabobj_ = tabobj
    fqtname = [tabobj._database_, tabobj._prefix_]
    if idname is not None:
      fqtname.append(idname)
    self._namedtuple_ = collections.namedtuple('_'.join(fqtname), tabobj._colinfo_)
    self._recbuff_ = tabobj._isobj_.Record(tabobj._recsize_)
  @property
  def as_buffer(self):
    'Return a record buffer for the associated table object'
    return self._recbuff_
  @property
  def as_tuple(self):
    'Return an instance of the namedtuple for the current column values'
    col_value = [getattr(self, name) for name in self._namedtuple_._fields]
    return self._namedtuple_._make(col_value)
  def __getattr__(self, name):
    if name in ('_colinfo_','_idxinfo_'):
      return getattr(self._tabobj_, name)
    elif name.startswith('_') and name.endswith('_'):
      raise AttributeError(name)
    else:
      return getattr(self._tabobj_, name)._unpack(self._recbuff_)
  def __setattr__(self, name, value):
    if name.startswith('_') and name.endswith('_'):
      object.__setattr__(self, name, value)
    elif name in self._colinfo_:
      getattr(self._tabobj_, name)._pack(self._recbuff_, value)
  def __getitem__(self, name):
    if isinstance(name, list):
      return list(getattr(self, col) for col in name)
    elif isinstance(name, tuple):
      return tuple(getattr(self, col) for col in name)
    else:
      return getattr(self, name)
  def __setitem__(self, name, value):
    if isinstance(name, tuple):
      if len(name) < len(value):
        raise ValueError('Must pass a value with the same number of columns as underlying table')
      for n,v in zip(name, value):
        setattr(self, getattr(n, 'name', n), value)
    else:
      setattr(self, getattr(name, 'name', name), value)
  def __str__(self):
    return str(self.rectuple)
