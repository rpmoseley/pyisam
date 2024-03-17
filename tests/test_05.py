'''
Test 05: Check if the error is working correctly
'''
import os
from pyisam import ISAMobject
from pyisam.error import IsamFunctionFailed

def test(opts):
  isfd = ISAMobject()
  isfd.isopen(os.path.join(opts.tstdata, 'decomp'))
  print('DI:', isfd.isdictinfo())
  try:
    print('KI:', isfd.iskeyinfo(3))
  except IsamFunctionFailed as exc:
    print('ISKEYINFO triggered error')
    print(exc)
  isfd.isclose()
