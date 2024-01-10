'''
Test 22: 
'''
from pyisam.table import ISAMtable
from pyisam.table.index import TableIndex
from pyisam.tabdefns.stxtables import DECOMPdefn

def test(opts):
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  DECrec = DECOMP._default_record()
  tabidx = DECOMP._idxinfo_._idxmap['comp'].as_keydesc(DECrec)
  print(tabidx)