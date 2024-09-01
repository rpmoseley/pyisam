'''
This module is a prototype into having a means of performing the retrieval of
records using a scheme similar to that used by django whereby the columns to be
checked are passed as keywords with a particular pattern along with the value to
be checked, that enables the correct ISAM read mode and index to be selected
without the application having to provide that information.
'''

import sys
from .record import ISAMrecordBase
from ..constants import ColumnType

# Define the following as TRUE to introduce the normal short-circuit operation
_shortcircuit = False

__all__ = ('dump_record_imp',)
_vld_ops = 'eq', 'ne', 'gt', 'ge', 'lt', 'le'
_sub_ops = {'lte' : 'le', 'gte' : 'ge', 'neq' : 'ne'} # Support django codes
_cmp_sym = {'EQ' : '==', 'NE' : '!=', 'GT' : '>', 'GE' : '>=', 'LT' : '<', 'LE' : '<='}

class _CompareMixin:
  _id = 'CM'
  def __init__(self, chkvalue):
    self.chkvalue = self._conv(chkvalue)

  def _print(self, comp, ownvalue, othvalue):
    print("{0._id}_{1}: {2} {3} {4}".format(self, comp, ownvalue, _cmp_sym.get(comp.upper()), othvalue))

  def __eq__(self, value):
    self._print('EQ', value, self.chkvalue)
    return value == self.chkvalue

  def __ne__(self, value):
    self._print('NE', value, self.chkvalue)
    return value != self.chkvalue

  def __lt__(self, value):
    self._print('LT', value, self.chkvalue)
    return value < self.chkvalue

  def __le__(self, value):
    self._print('LE', value, self.chkvalue)
    return value <= self.chkvalue

  def __gt__(self, value):
    self._print('GT', value, self.chkvalue)
    return value > self.chkvalue

  def __ge__(self, value):
    self._print('GE', value, self.chkvalue)
    return value >= self.chkvalue

class _TextCompare(_CompareMixin):
  'Perform textual comparisons'
  _id = 'TC'
  def _conv(self, value):
    return value if isinstance(value, str) else str(value)

  def _print(self, comp, ownvalue, othvalue):
    print("{0._id}_{1}: '{2}' {3} '{4}'".format(self, comp, ownvalue, _cmp_sym.get(comp.upper()), othvalue))

class _IntegerCompare(_CompareMixin):
  'Perform integer number comparisons'
  _id = 'IC'
  def _conv(self, value):
    return value if isinstance(value, int) else int(value)

class _FloatCompare(_CompareMixin):
  'Perform real number comparisons'
  _id = 'FC'
  def _conv(self, value):
    return value if isinstance(value, float) else float(value)

_conv_cmp = {
  ColumnType.CHAR   : _TextCompare,
  ColumnType.SHORT  : _IntegerCompare,
  ColumnType.LONG   : _IntegerCompare,
  ColumnType.DOUBLE : _FloatCompare,
  ColumnType.FLOAT  : _FloatCompare
}
  
def prepare_colcheck(record, **colcomp):
  '''Prepare the sequence of columns to be checked against the current record
     which must all succeed to return a success result, the order of checks
     matches the order of the fields in the record'''
  # Split the column check into a dictionary of the field, and check with value
  coldict = {}
  for colname, colvalue in colcomp.items():
    # Default to equality for column checks
    colcheck = colname.split('__') + ['eq']
    colname, colop = colcheck[:2]
    if colname not in record._flddict:
      #raise ValueError('Column not present in the table: {}'.format(colname))
      continue   # Ignore the column check 
    if colop in _sub_ops:
      colop = _sub_ops[colop]
    if colop not in _vld_ops:
      raise ValueError('Comparison not currently supported: {}'.format(colop))
    coldict[colname] = (colop, colvalue)

  # Now process the fields in record order adding those that need checking
  new_colcheck = []
  for col in record._fields:
    # Check if colname is one of those needed to compare against
    if col.name in coldict:
      colop, colvalue = coldict[col.name]
      # Add the appropriate call to make the correct comparison
      colcmp = '__{colop}__'.format(colop=colop)
      chkinst = _conv_cmp[col.type](colvalue)
      chkfunc = getattr(chkinst, colcmp)
      new_colcheck.append((col, chkfunc))
  return new_colcheck
     
def select_index(tabobj, colcheck, record=None):
  '''Attempt to select the correct index given the column checks'''
  if not isinstance(colcheck, (tuple, list)):
    raise ValueError('Expected a sequence of column check, got {}'.format(type(colcheck)))
  if not isinstance(record, ISAMrecordBase):
    record = tabobj._default_record()
  # Create a dictionary for each index defined for the record
  tabobj._autoload_indexes()
  print('RECIDX:', type(record), dir(record)) #DEBUG
  idxinfo = tabobj._idxinfo_[0]
  print('IDX0:', idxinfo)
  
if _shortcircuit:
  def perform_colcheck(record, colcheck):
    '''Run the prepared column check against the current record values'''
    if not isinstance(colcheck, (tuple, list)):
      raise ValueError('Expected a sequence of column check, got {}'.format(type(colcheck)))
    for col, chkfunc in colcheck:
      # Perform the comparison using the current value as self and the stored value
      # as the other (ie curvalue OP self.value)
      colvalue = getattr(record, col.name)
      cmpres = chkfunc(colvalue)
      print('CKRES:', cmpres)
      if cmpres == False:
        return False
    return True 
else:
  def perform_colcheck(record, colcheck):
    '''Run the prepared column check against the current record values'''
    if not isinstance(colcheck, (tuple, list)):
      raise ValueError('Expected a sequence of column check, got {}'.format(type(colcheck)))
    rslt = True
    for col, chkfunc in colcheck:
      # Perform the comparison using the current value as self and the stored value
      # as the other (ie curvalue OP self.value)
      colvalue = getattr(record, col.name)
      cmpres = chkfunc(colvalue)
      print('CKRES:', cmpres)
      if cmpres == False:
        rslt = False
    return rslt 
