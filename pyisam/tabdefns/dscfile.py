'''
This module will take a .dsc file as used by Strategix/OneOffice and produce
a DynamicTableDefn based definition suitable for use within the package.
'''

import pathlib
from . import CharColumn, TextColumn, ShortColumn, LongColumn, FloatColumn
from . import DoubleColumn, SerialColumn, DateColumn, MoneyColumn
from . import CurrencyColumn, AppcodeColumn, TimeColumn
from . import PrimaryIndex, UniqueIndex, DuplicateIndex
from .dynamic import DynamicTableDefn
"""NOT USED:
from . import AppdescColumn
"""

# Define the mapping of field types to definition types, these correspond to
# stxtype field on the defile table which provides a wider range of types than
# C-ISAM natively supports.
_fldtyp_map = {
  1: ShortColumn,
  3: LongColumn,
  4: FloatColumn,
  5: DoubleColumn,
  8: TextColumn,
  11: SerialColumn,
  13: MoneyColumn,
  16: TextColumn,
  19: DateColumn,
  21: CurrencyColumn,
  24: AppcodeColumn,
  27: TimeColumn,
  32: CharColumn,
  72: AppcodeColumn,
  88: AppcodeColumn,
  96: AppcodeColumn,
}

def _convtype(oldtype):
  'Provide the newer type information for a field without one'
  _convtype_map = {
    '514': 19, '770': 19, '1026': 19,
    '258': 11, '1283': 13,
    '1': 2, '2': 3, '3': 5, '4': 4,
  }
  return _convtype_map.get(oldtype, 16)
  
def ParseDSCFile(tabname, tabpath=None):
  'Read the contents of the .dsc for the given tabname and create a definition'

  # Check the tabname and create a full path to the data files
  if isinstance(tabname, (str, bytes)):
    tabname = pathlib.Path(tabname)
  elif not isinstance(tabname, pathlib.Path):
    raise ValueError('Unhandled type of tabname')
  if not tabname.is_absolute() and tabpath:
    tabpath = pathlib.Path(tabpath) / tabname
  else:
    tabpath = tabname

  # Prepare the definition object to work with
  defn = DynamicTableDefn(str(tabname))

  # Store column names for index lookup
  colinfo = list()
  
  # Process the .dsc contents
  with tabpath.with_suffix('.dsc').open('rt') as dscfd:
    num = int(dscfd.readline().rstrip())
    while num > 0:
      # Split the line according to type
      lfld = dscfd.readline().rstrip().split(None)
      nfld = len(lfld)
      if nfld < 5:
        #UNUSED:otyp, ocod = _convtype(lfld[3]), None
        otyp = _convtype(lfld[3])
      else:
        otyp = int(lfld[4])
        #UNUSED:ocod = lfld[5] if nfld > 5 and otyp == 96 else None

      # Default the table's prefix
      if defn._prefix_ is None:
        defn._prefix_ = lfld[0].split('_')[0] if lfld[0].find('_') > 0 else ''
      fldname = lfld[0].split('_')[1] if defn._prefix_ else lfld[0]

      # Add to the list of known columns
      colinfo.append(fldname)
      
      # Determine the field type
      klass = _fldtyp_map[otyp]
      if issubclass(klass, TextColumn):
        defn.append(klass(fldname, int(lfld[2])))
      else:
        defn.append(klass(fldname))
        
      # Done with this field
      num -= 1
    
    # Process the indexes
    num = int(dscfd.readline().rstrip())
    while num > 0:
      # Split the line to determine number of keyparts
      lfld = dscfd.readline().rstrip().split(None)
      fldname = lfld[0].split('_')[1] if defn._prefix_ else lfld[0]
      npart = int(lfld[2])
      
      # Determine which index definition to create
      if lfld[1] == 'P':
        klass = PrimaryIndex
      elif lfld[1] == 'D':
        klass = DuplicateIndex
      elif lfld[1] == 'U':
        klass = UniqueIndex
      else:
        raise ValueError('Unhandled index type')
        
      # Check if this field is already defined as a column
      if fldname in defn._columns_:
        idx = klass(fldname, fldname)
      else:
        cols = [colinfo[lfld[n+3]] for n in range(npart)]
        idx = klass(fldname, cols) 
      defn.add_index(idx)
      
      # Processed this index
      num -= 1
