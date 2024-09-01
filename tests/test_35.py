'''
Test 35: Check if record number is working as expected
'''
from pyisam.constants import OpenMode
from pyisam.table import ISAMtable, RecordOrderIndex
from pyisam.backend import _backend
from pyisam.tabdefns.stxtables import DECOMPdefn

def test(opts):
  DECOMP = ISAMtable(DECOMPdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  dec_rec = DECOMP.read(18328)    # Should be defile
  print(DECOMP._recnum, dec_rec())
