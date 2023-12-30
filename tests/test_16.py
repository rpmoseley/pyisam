'''
Test 16: Dump a specific error message
'''
from pyisam import ISAMobject

def test(opts):
  isobj = ISAMobject()
  print('ERRSTR:', isobj.strerror(109))
