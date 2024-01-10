'''
Test 40: Check if the binary field parser is working correctly.
'''

from pyisam.constants import ReadMode
from pyisam.tabdefns import fldfile
from pyisam.table import ISAMtable

def test(opts):
  # Check if there is any test data available
  if not hasattr(opts, 'tstdata'):
    print('Test not supported without test data')
    return
  tabdefn = fldfile.ParseFldInfo('cgtotst1', tabpath=opts.tstdata)
  tabinst = ISAMtable(tabdefn, tabpath=opts.tstdata)
  print(tabinst._idxinfo_._idxmap.items())
  print(tabinst._colinfo('fld004'))
  print(tabinst.read('key1', ReadMode.ISFIRST))
  print(tabinst.read())
  print(tabinst.read())
  print(tabinst._idxinfo_._idxmap.items())
