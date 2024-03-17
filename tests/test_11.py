'''
Test 11: Dump the contents of the defile table using an iterator
'''
from pyisam.constants import OpenMode, ReadMode
from pyisam.error import IsamEndFile
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
    self._tabobj.open()
    return self

  def __next__(self):
    try:
      return self._tabobj.read(index=self._index, mode=self._mode, *self._args, **self._kwds)
    except IsamEndFile:
      raise StopIteration

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  for row in ISAMtableIter(DEFILE, mode=ReadMode.ISPREV, filename='defile'):
    print(row)
