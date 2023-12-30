'''
Test 18: Define a table pulling index information from the underlying
         table at runtime using the dynamic table support
'''
from pyisam.table import ISAMtable
from pyisam.tabdefns.dynamic import DynamicTableDefn
from pyisam.tabdefns import TextColumn, CharColumn, LongColumn
from .dumprec import dump_record_imp

def test(opts):
  DECOMPdefn = DynamicTableDefn(tabname='decomp',
                                fields=[TextColumn('comp',     9),
                                        CharColumn('comptyp'    ),
                                        TextColumn('sys',      9),
                                        TextColumn('prefix',   5),
                                        TextColumn('user',     4),
                                        TextColumn('database', 6),
                                        TextColumn('release',  5),
                                        LongColumn('timeup'     ),
                                        LongColumn('specup'     )],
                                error=opts.error_raise)
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  # Force the building of the indexes by using the ISAM table indirectly
  dump_record_imp(DECOMP, comp__eq='defile')
