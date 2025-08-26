'''
Test 07: Check if the isrecnum, isreclen global variables can be modiifed
'''

from pyisam.isam import ISAMobject

def test(opts):
  # Print the current values which should be zeroes
  isobj = ISAMobject()
  try:
    print('RECNOM:', isobj.isrecnom)
  except AttributeError as exc:
    print(f"Misnamed attribute '{exc.name}' correct")
  print('RECNUM:', isobj.isrecnum)
  print('RECLEN:', isobj.isreclen)

  # Attempt to set the record number without an open file
  isobj.isrecnum = 1000
  print('RECNUM:', isobj.isrecnum)
  try:
    isobj.isrecnom = 2000
    print('RECNOM:', isobj.isrecnom)
  except AttributeError as exc:
    print(f"Misnamed attribute '{exc.name}' correct")
