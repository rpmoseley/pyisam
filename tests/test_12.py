'''
Test 12: Dump the contents of the defile table for itself.
'''
from pyisam import ReadMode
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DEFILEdefn

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  def_rec = DEFILE.read('key', ReadMode.ISGREAT, 'defile')
  while def_rec.filename == 'defile':
    print(def_rec)
    def_rec = DEFILE.read()
