'''
Test 13: Dump the contents of the dekeys/decomp tables for defile in addition.
'''
from pyisam.constants import ReadMode
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DEFILEdefn, DEKEYSdefn, DECOMPdefn, DEITEMdefn

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  def_rec = DEFILE.read('key', ReadMode.ISGREAT, 'defile')
  while def_rec.filename == 'defile':
    print(def_rec)
    def_rec = DEFILE.read()

  DEKEYS = ISAMtable(DEKEYSdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  dek_rec = DEKEYS.read('key', ReadMode.ISGREAT, 'defile')
  while dek_rec.filename == 'defile':
    print(dek_rec)
    dek_rec = DEKEYS.read()

  DECOMP = ISAMtable(DECOMPdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  dec_rec = DECOMP.read('comp', ReadMode.ISEQUAL, 'defile')
  print(dec_rec)
