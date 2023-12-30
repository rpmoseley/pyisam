'''
Test 30: Build a new table using a fixed record size and a dynamic table definition
'''
from pyisam import ReadMode
from pyisam.table import ISAMtable
from pyisam.tabdefns.dynamic import DynamicTableDefn
from pyisam.tabdefns import TextColumn, CharColumn, LongColumn, ShortColumn
from pyisam.tabdefns import PrimaryIndex

def test(opts):
  NEWTABdefn = DynamicTableDefn('newtab', error=opts.error_raise)
  NEWTABdefn.append(TextColumn('file', 9))
  NEWTABdefn.append(LongColumn('seq'))
  NEWTABdefn.append(TextColumn('fldname', 9))
  NEWTABdefn.append(ShortColumn('fldtype'))
  NEWTABdefn.append(LongColumn('fldsize'))
  NEWTABdefn.add_index(PrimaryIndex('key', 'file', 'seq'))
  NEWTAB = ISAMtable(NEWTABdefn)
  NEWTAB.build(tabpath='data')
