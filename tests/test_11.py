'''
Test 11: Dump the contents of the defile table for itself.
'''
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DEFILEdefn

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  print(dir(DEFILE))
  print(DEFILE)
