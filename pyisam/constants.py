'''
This module provides all the various flags stored as enum objects.
'''

from enum import IntFlag, IntEnum

# Define the types that can be applied to the keydesc.flags field
class IndexFlags(IntFlag):
  NO_DUPS      = 0x00        # No duplicates allowed in values
  DUPS         = 0x01        # Duplicates allowed in values
  DUP_COMPRESS = 0x02
  LDR_COMPRESS = 0x04
  TRL_COMPRESS = 0x08
  ALL_COMPRESS = 0x0E
  CLUSTER      = 0x20
  DESCEND      = 0x80

# The OpenMode and LockMode enums provide the available modes used
# when using the isbuild or isopen methods.
class OpenMode(IntFlag):
  ISINPUT    = 0x000         # Open for input only 
  ISOUTPUT   = 0x001         # Open for output only
  ISINOUT    = 0x002         # Open for input and output
  ISTRANS    = 0x004         # Open for transaction processing
  ISNOLOG    = 0x008         # No logging for this file
  ISVARLEN   = 0x010         # Use variable length records
  ISSPECAUTH = 0x020
  ISFIXLEN   = 0x000         # Use fixed length records
  ISNOCKLOG  = 0x080

class LockMode(IntFlag):
  ISAUTOLOCK = 0x200         # Automatic record locking
  ISMANULOCK = 0x400         # Manual record locking
  ISEXCLLOCK = 0x800         # Exclusive table lock

# The ReadMode enum provides the available modes when using the isread
# method.
class ReadMode(IntFlag):
  ISFIRST    = 0x000         # Position at first record
  ISLAST     = 0x001         # Position at last record
  ISNEXT     = 0x002         # Position at next record
  ISPREV     = 0x003         # Position at previous record
  ISCURR     = 0x004         # Position at current record
  ISEQUAL    = 0x005         # Position at == value
  ISGREAT    = 0x006         # Position at >  value
  ISGTEQ     = 0x007         # Position at >= value
  ISLOCK     = 0x100         # Lock record
  ISSKIPLOCK = 0x200         # Skip record even if locked
  ISWAIT     = 0x400         # Wait for record lock
  ISLCKW     = 0x500
  ISKEEPLOCK = 0x800         # Keep record lock in auto locking mode

# The ExtReadMode enum provides extra modes that are available when
# using an instance of ISAMtable.
class ExtReadMode(IntFlag):
  ISFIRST    = 0x000         # Position at first record
  ISLAST     = 0x001         # Position at last record
  ISNEXT     = 0x002         # Position at next record
  ISPREV     = 0x003         # Position at previous record
  ISCURR     = 0x004         # Position at current record
  ISEQUAL    = 0x005         # Position at == value
  ISGREAT    = 0x006         # Position at >  value
  ISGTEQ     = 0x007         # Position at >= value
  ISFGTEQ    = 0x008         # Position at >= value for one record only
  ISFLTEQ    = 0x009         # Position at <= value for one record only
  ISMATCH    = 0x00A         # Position at >= value (forward matching)
  ISRMATCH   = 0x00B         # Position at <= value (reverse matching)
  ISAGAIN    = 0x00C
  ISSMALL    = 0x00E         # Position at <  value
  ISLTEQ     = 0x00F         # Position at <= value
  ISLOCK     = 0x100         # Lock record
  ISSKIPLOCK = 0x200         # Skip record even if locked
  ISWAIT     = 0x400         # Wait for record lock
  ISLCKW     = 0x500
  ISKEEPLOCK = 0x800         # Keep record lock in auto locking mode
 
# The types of column supported by the package
class ColumnType(IntEnum):
  CHAR   = 0
  SHORT  = 1
  INT    = 1
  LONG   = 2
  DOUBLE = 3
  FLOAT  = 4

# Define the default open and lock mode
dflt_openmode = OpenMode.ISINPUT
dflt_lockmode = LockMode.ISMANULOCK
