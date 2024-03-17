'''
Test 45: Check if the DSC file parser works
'''

from pyisam.constants import OpenMode
from pyisam.tabdefns import dscfile
from pyisam.table import ISAMtable

def test(opts):
  # Check if there is any test data available
  if not hasattr(opts, 'tstdata'):
    print('Test not supported without test data')
    return
  tabdefn = dscfile.ParseDSCFile('decomp', tabpath=opts.tstdata)
  tabinst = ISAMtable(tabdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  #print(tabinst._idxinfo_._idxmap.items())
  #print(tabinst._colinfo('fld004'))
  #print(tabinst.read('key1', ReadMode.ISFIRST))
  #print(tabinst.read())
  #print(tabinst.read())
  #print(tabinst._idxinfo_._idxmap.items())
