'''
Test 11: Dump the contents of the defile table for itself.
'''
from pyisam.constants import ReadMode
from pyisam.error import IsamFunctionFailed, IsamNotOpen, IsamEndFile
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DEFILEdefn

class ISAMtableIter:
  def __init__(self, tabobj, index=None, mode=None, *args, **kwds):
    self._tabobj = tabobj
    self._index = index
    self._mode = ReadMode(mode)
    self._args = args
    self._kwds = kwds

  def __iter__(self):
    return self

  def __next__(self):
    try:
      return self._tabobj.read(index=self._index, mode=self._mode, *self._args, **self._kwds)
    except IsamEndFile:
      raise StopIteration

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  for row in ISAMtableIter(DEFILE, mode=ReadMode.ISPREV, ):
    print(row)
