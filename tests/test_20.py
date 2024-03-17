'''
Test 20: Define a table pulling index information from the underlying
         table at runtime using the dynamic table support using multiple
         columns in the comparison
'''
from pyisam.table import ISAMtable
from pyisam.tabdefns.dynamic import DynamicTableDefn
from pyisam.tabdefns import TextColumn, CharColumn, ShortColumn
from .dumprec import dump_record_imp

def test(opts):
  DEFILEdefn = DynamicTableDefn(tabname='defile',
                                fields=[TextColumn ('filename',  9),
                                        ShortColumn('seq'         ),
                                        TextColumn ('field',     9),
                                        TextColumn ('refptr',    9),
                                        CharColumn ('type'        ),
                                        ShortColumn('size'        ),
                                        CharColumn ('keytype'     ),
                                        ShortColumn('vseq'        ),
                                        ShortColumn('stype'       ),
                                        CharColumn ('scode'       ),
                                        TextColumn ('fgroup',   10),
                                        CharColumn ('idxflag'     )],
                                error=opts.error_raise)
  print('COL:', DEFILEdefn._columns_[0])   ##DEBUG
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  # Force the building of the indexes by using the ISAM table indirectly
  dump_record_imp(DEFILE, filename__eq='defile', seq__lte=100)
