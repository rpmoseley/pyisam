
# Provide common restrictions on the underlying ISAM format
MAXKPART = 8              # Maximum number of parts per index
MAXKLENG = 120            # Maximum number of bytes per index

# Provide common implementation of the keypart validation
def _checkpart(cls, part):
  if not isinstance(part, int):
    raise ValueError('Expecting an integer index part number')
  elif part >= MAXKLENG:
    raise ValueError(f'Cannot refer beyond to index part {part}')
  elif part < -cls.nparts:
    raise ValueError('Cannot refer beyond the first index part')
  elif part < 0:
    part += cls.nparts
  elif cls.nparts < part:
    raise ValueError('Cannot refer beyond the last index part')
  return part
