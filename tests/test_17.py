'''
Test 17: Define a full table at runtime using the dynamic table support
'''
from pyisam import ReadMode
from pyisam.table import ISAMtable
from pyisam.tabdefns.dynamic import DynamicTableDefn
from pyisam.tabdefns import TextColumn, CharColumn, LongColumn
from pyisam.tabdefns import PrimaryIndex, UniqueIndex, DuplicateIndex
from .dumprec import dump_record_exp_eq

def test(opts):
  DECOMPdefn = DynamicTableDefn('decomp', error=opts.error_raise)
  DECOMPdefn.append(TextColumn('comp',     9))
  DECOMPdefn.append(CharColumn('comptyp'    ))
  DECOMPdefn.append(TextColumn('sys',      9))
  DECOMPdefn.append(TextColumn('prefix',   5))
  DECOMPdefn.append(TextColumn('user',     4))
  DECOMPdefn.extend([TextColumn('database', 6),
                     TextColumn('release',  5),
                     LongColumn('timeup'     ),
                     LongColumn('specup'     )])
  DECOMPdefn.add_index(PrimaryIndex('comp'))
  DECOMPdefn.add_index(DuplicateIndex('prefix'))
  DECOMPdefn.add_index(UniqueIndex('typkey', 'comptyp', 'comp'))
  DECOMPdefn.add_index(UniqueIndex('syskey', ['sys', 'comptyp', 'comp']))
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record_exp_eq(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'adpara')
