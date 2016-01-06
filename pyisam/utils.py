'''
This module provides the utility functions and exceptions for the pyisam package.
'''
__all__ = ('ISAM_bytes', 'ISAM_str')

# Convert the given value to a bytes value
def ISAM_bytes(value,default=None):
  if isinstance(value,str):
    value = bytes(value,'utf-8')
  elif value is None:
    value = b'' if default is None else ISAM_bytes(default)
  else:
    raise ValueError('Value is not convertable to a bytes value')
  return value

# Convert the given value to a str value
def ISAM_str(value,default=None):
  if isinstance(value,bytes):
    value = str(value,'utf-8')
  elif value is None:
    value = '' if default is None else ISAM_str(default)
  else:
    raise ValueError('Value is not convertable to a str value')
  return value
