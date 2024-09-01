'''
Test 11: Dump the contents of the defile table using an iterator
'''
from pyisam.constants import OpenMode, ReadMode
from pyisam.error import IsamEndFile
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DEFILEdefn

class ISAMtableIter:
  def __init__(self, tabobj, *args, **kwds):
    self._tabobj = tabobj
    self._args = args
    self._kwds = kwds

  def __iter__(self):
    self._first = True
    return self

  def __next__(self):
    if self._first:
      self._first = False
      return self._tabobj.read(*self._args, **self._kwds)
    return self._tabobj.read()

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  for row in ISAMtableIter(DEFILE, ReadMode.ISPREV, filename='defile'):
    print(row)
