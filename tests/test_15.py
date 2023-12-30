'''
Test 15: Dump the complete definition of a table using a function
'''
from pyisam import ReadMode
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DEFILEdefn, DEKEYSdefn, DECOMPdefn, DEITEMdefn
from .dumprec import dump_record_exp_eq

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  dump_record_exp_eq(DEFILE, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DEKEYS = ISAMtable(DEKEYSdefn, tabpath='data')
  dump_record_exp_eq(DEKEYS, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record_exp_eq(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'defile')
  DEITEM = ISAMtable(DEITEMdefn, tabpath='data')
  dump_record_exp_eq(DEITEM, 'usekey', ReadMode.ISGREAT, 'comp', 'defile')
