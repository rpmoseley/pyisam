'''
Test 03: Check how the new enums modules works with duplicate values
'''
from pyisam import OpenMode  

def test(opts):
  print(OpenMode.ISFIXLEN.value, OpenMode.ISFIXLEN.name)
