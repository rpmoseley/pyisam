'''
This module provides all the various flags stored as enum objects.
'''

import enum

# Define the types that can be applied to the keydesc.flags field
class IndexFlags(enum.IntEnum):
  NO_DUPS      = 0x00
  DUPS         = 0x01
  DUP_COMPRESS = 0x02
  LDR_COMPRESS = 0x04
  TRL_COMPRESS = 0x08
  ALL_COMPRESS = 0x10
  CLUSTER      = 0x20
  DESCEND      = 0x80

# The OpenMode and LockMode enums provide the available modes used
# when using the isbuild or isopen methods.
class OpenMode(enum.Enum):
  ISINPUT    = 0x000
  ISOUTPUT   = 0x001
  ISINOUT    = 0x002
  ISTRANS    = 0x004
  ISNOLOG    = 0x008
  ISVARLEN   = 0x010
  ISSPECAUTH = 0x020
  ISFIXLEN   = 0x000
  ISNOCKLOG  = 0x080
class LockMode(enum.IntEnum):
  ISAUTOLOCK = 0x200
  ISMANULOCK = 0x400
  ISEXCLLOCK = 0x800

# The ReadMode enum provides the available modes when using the isread
# method.
class ReadMode(enum.IntEnum):
  ISFIRST    = 0
  ISLAST     = 1
  ISNEXT     = 2
  ISPREV     = 3
  ISCURR     = 4
  ISEQUAL    = 5
  ISGREAT    = 6
  ISGTEQ     = 7
  ISLOCK     = 0x100
  ISSKIPLOCK = 0x200
  ISWAIT     = 0x400
  ISLCKW     = 0x500
  ISKEEPLOCK = 0x800

# The StartMode enum provides the available modes when using the isstart
# method.
class StartMode(enum.IntEnum):
  ISFIRST    = 0
  ISLAST     = 1
  ISEQUAL    = 5
  ISGREAT    = 6
  ISGTEQ     = 7
  ISLOCK     = 0x100
  ISSKIPLOCK = 0x200
  ISWAIT     = 0x400
  ISLCKW     = 0x500
  ISKEEPLOCK = 0x800

# The types of column supported by the package
class ColumnType(enum.Enum):
  CHAR   = 0
  SHORT  = 1
  LONG   = 2
  DOUBLE = 3
  FLOAT  = 4
