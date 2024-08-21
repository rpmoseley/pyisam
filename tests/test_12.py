'''
Test 12: Dump the contents of the defile table for itself.
'''
from pyisam.constants import OpenMode, ReadMode
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DEFILEdefn

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  def_rec = DEFILE.read('key', ReadMode.ISGREAT, 'defile')
  while def_rec.filename == 'defile':
    print('R:', def_rec)
    def_rec = DEFILE.read()
  print('A:', def_rec)
