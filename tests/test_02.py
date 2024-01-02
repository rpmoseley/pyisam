'''
Test 01: Dump the list of error codes and meanings from the ISAM library
'''

from pyisam import ISAMobject

def test(opts):
  isobj = ISAMobject()
  for errno in range(100, isobj.is_nerr):
    print(errno, isobj.strerror(errno))
