'''
This module provides a means of taking the .fld binary file that is produced by
MicroFocus verastream host integrator (former known as SuperNova), it will take
the binary file and retrieve the column type information from it, since there
are no names to associate the fields against, each field is added as 'fldNNN'.
Additionally the index structure has to be retrieved directly from the .idx
file associated with the table.
'''

import pathlib
import struct
from . import CharColumn, TextColumn, ShortColumn, LongColumn, DateColumn
from . import SerialColumn, FloatColumn, DoubleColumn
from . import RecordOrderIndex, UniqueIndex, DuplicateIndex
from .dynamic import DynamicTableDefn
""" NOT USED :
from .. import ISAMobject
from ..constants import IndexFlags
from ..table.record import recordclass
END NOT USED """

# Define the default prefix for auto-gererated field names
_dflt_prefix = 'fld'

# Provide the mapping of field type to definition types
# TODO: Fill in the missing types of fields
_fldtyp_map = {
  2 : ShortColumn,
  4 : DoubleColumn,
  6 : TextColumn,
  8 : DateColumn,
}

# Define the structure of each element within the .fld file
_fldent = struct.Struct('<l')

def ParseFldInfo(tabname, tabprefix=None, fldnames=None, padding=3, tabpath=None):
  'Read the binary .fld file and generate table definition'

  # Check the tabname and create a full path to data files
  if isinstance(tabname, (str, bytes)):
    tabname = pathlib.Path(tabname)
  elif not isinstance(tabname, pathlib.Path):
    raise ValueError('Unhandled type of tabname')
  if not tabname.is_absolute() and tabpath:
    tabpath = pathlib.Path(tabpath) / tabname
  else:
    tabpath = tabname

  # Check if any sequence of field names has been provided
  if fldnames and not isinstance(fldnames, (tuple, list)):
    raise ValueError('Must provide a sequence of field names or None')

  # Use a default prefix if none provided
  fldprefix = _dflt_prefix if tabprefix is None else tabprefix

  # Select the appropriate template to use for field names
  if fldnames is None:
    template = f'{fldprefix}{{fldnum:0{padding}d}}'
  else:
    template = f'{fldprefix}{{fldnames[fldnum]}}'

  # Prepare the definition object to work with
  defn = DynamicTableDefn(str(tabname))

  # Process the .fld contents
  with tabpath.with_suffix('.fld').open('rb') as fldfd:
    for fldnum in range(_fldent.unpack(fldfd.read(_fldent.size))[0]):
      fldtyp = _fldent.unpack(fldfd.read(_fldent.size))[0]
      fldsiz = _fldent.unpack(fldfd.read(_fldent.size))[0]
      fldnam = template.format(fldnum=fldnum if fldnames else fldnum+1)
      klass = _fldtyp_map.get(fldtyp, None)
      if klass is None:
        raise ValueError(f'Unknown field type: {fldtyp}, {fldsiz}')
      elif issubclass(klass, TextColumn):
        defn.append(klass(fldnam, fldsiz))
      else:
        defn.append(klass(fldnam))
  # Return the definition object with columns added, index information will be
  # filled in when the ISAMtable object is created.
  return defn
  
""" NOT USED :
def parse_idx(self, tabname, prefix):
  'Attempt to extract the index information from ISAM files'
  def _match(kdesc):
    kcol = []
    for kprt in range(kdesc.nparts):
      kfld = kdesc[kprt]
      for cfld in self._recinfo_._fields:
        if cfld.offset == kfld.start and cfld.size == kfld.leng and cfld.type == kfld.type:
          kcol.append(cfld.name)
          break
    return kcol

  isfd = ISAMobject()
  isfd.isopen(tabname)
  di = isfd.isdictinfo()
  for kidx in range(di.nkeys):
    kname = f'key{kidx}'
    ki = isfd.iskeyinfo(kidx)
    if ki.nparts == 0:
      # Handle the special index of 0 parts
      self._indexes_[kname] = RecordOrderIndex(kname)
      continue
    kcol = _match(ki)
    if ki.flags & IndexFlags.DUPS:
      newidx = DuplicateIndex(kname, kcol)
    else:
      newidx = UniqueIndex(kname, kcol)
    self._indexes_[kname] = newidx
END NOT USED """
