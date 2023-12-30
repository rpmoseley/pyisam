'''
Test 21: 
'''
from pyisam import ReadMode
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DECOMPdefn
from .dumprec import dump_record_exp_eq

def test(opts):
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record_exp_eq(DECOMP, None, ReadMode.ISEQUAL, 'comp', 'defile')
