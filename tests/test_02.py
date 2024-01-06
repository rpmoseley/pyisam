'''
Test 02: Dump the list of error codes and meanings from the ISAM library
'''

from pyisam import ISAMobject

def test(opts):
  isobj = ISAMobject()
  for errno in range(isobj._vld_errno[0], isobj._vld_errno[1]):
    print(errno, isobj.strerror(errno))
