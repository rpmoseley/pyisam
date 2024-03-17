
# Provide common restrictions on the underlying ISAM format
MaxKeyParts = 8               # Maximum number of parts per index
MaxKeyLength = 120            # Maximum number of bytes per index

# Provide common implementation of the keypart validation
def check_keypart(cls, part):
  if not isinstance(part, int):
    raise ValueError('Expecting an integer index part number')
  elif part >= MaxKeyLength:
    raise ValueError(f'Cannot refer beyond to index part {part}')
  elif part < -cls.nparts:
    raise ValueError('Cannot refer beyond the first index part')
  elif part < 0:
    part += cls.nparts
  elif cls.nparts < part:
    raise ValueError('Cannot refer beyond the last index part')
  return part
