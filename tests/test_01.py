'''
Test 02: Print the version string from the ISAM library
'''
from pyisam import ISAMobject
from pyisam.backend import use_conf, use_isamlib
from pyisam.utils import ISAM_str

def test(opts):
  print(f"Using '{use_conf}' backend and '{use_isamlib}' library")
  isobj = ISAMobject()
  print(isobj.isversnumber)
  print(isobj.iscopyright)
