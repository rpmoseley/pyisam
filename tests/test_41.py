'''
Test 40: Check if the binary field parser is working correctly.
'''

import datetime
from pyisam.constants import ReadMode, OpenMode
from pyisam.error import IsamFunctionFailed
from pyisam.tabdefns import TextColumn, ShortColumn, DateColumn
from pyisam.tabdefns import PrimaryIndex
from pyisam.tabdefns.dynamic import DynamicTableDefn
from pyisam.table import ISAMtable

def test(opts):
  # Check if there is any test data available
  if not hasattr(opts, 'tstdata'):
    print('Test not supported without test data')
    return
  
  # Provide the definition of a simple table
  tabdefn = DynamicTableDefn(tabname='tstdata1')
  tabdefn.append(ShortColumn('seq'))
  tabdefn.append(TextColumn('name', 10))
  tabdefn.append(DateColumn('chg'))
  tabdefn.add_index(PrimaryIndex('order', 'seq', 'name'))
  
  # Create a table instance using the definition
  tabinst = ISAMtable(tabdefn, tabpath=opts.tstdata)
  
  # Check that the buffer is correctly defined
  recbuf = tabinst._record_(tabinst._name_)
  print(recbuf)

  # Build a new phyiscal table
  try:
    tabinst.build()
  except IsamFunctionFailed as exc:
    if exc.errno != 17:
      raise 
    tabinst.open() 

  # Create the appropriate TableIndex from the primary index
  #kd = tabinst.keyinfo(0)
  #print(kd)
  
  # Attempt to insert some data into the table, if the record is already present then error out
  tabinst.insert(seq=1, name='Abc', chg=datetime.datetime(2024, 1, 20))
  
  print(tabinst.read('key1', ReadMode.ISFIRST))
  print(tabinst.read())
  print(tabinst.read())
  print(tabinst._idxinfo_._idxmap.items())
