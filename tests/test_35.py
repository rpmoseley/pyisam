'''
Test 35: Check if record number is working as expected
'''
from pyisam.constants import OpenMode
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DECOMPdefn

def test(opts):
  DECOMP = ISAMtable(DECOMPdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  dec_rec = DECOMP.read(99)
  print(DECOMP._recnum, DECOMP._isobj.isrecnum, dec_rec)
