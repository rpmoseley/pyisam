'''
Test 05: Check if the error is working correctly
'''
from pyisam import ISAMobject
from pyisam.error import IsamFunctionFailed

def test(opts):
  isfd = ISAMobject()
  isfd.isopen('data/decomp')
  try:
    isfd.iskeyinfo(4)
  except IsamFunctionFailed:
    print('ISKEYINFO triggered error')
  isfd.isclose()
