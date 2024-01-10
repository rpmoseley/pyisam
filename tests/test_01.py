'''
Test 01: Print the version string from the ISAM library
'''
from pyisam import ISAMobject
from pyisam.backend import use_conf, use_variant
from pyisam.utils import ISAM_str

def test(opts):
  print(f"Using '{use_conf}' backend and '{use_variant}' library\n")
  isobj = ISAMobject()
  print(isobj.isversnumber)
  print(isobj.iscopyright)
