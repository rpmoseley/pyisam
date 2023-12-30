'''
Test 02: Print the version string from the ISAM library
'''
from pyisam import ISAMobject
from pyisam.utils import ISAM_str

def test(opts):
  isobj = ISAMobject()
  print(ISAM_str(isobj.isversnumber))
