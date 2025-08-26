'''
Test 11: Dump the contents of the defile table using an iterator
'''
from pyisam.constants import OpenMode, ReadMode
from pyisam.error import IsamEndFile
from pyisam.table import ISAMtable
from pyisam.tabdefns.stxtables import DEFILEdefn

class ISAMtableIter:
  def __init__(self, tabobj, *args, **kwds):
    self._tabobj = tabobj
    self._args = args
    self._kwds = kwds

  def _parseargs(self):
    'Parse the arguments the same as the table.read() method'
    if len(self._args) < 1:
      # Assume that this is a complete table read and start from the first record
      return ReadMode.ISFIRST

    # Convert arguments into a list to permit popping values
    args, narg = list(self._args), list()

    # Drop the index argument if present
    if isinstance(args[0], (ReadMode, ISAMrecordBase)):
      pass
    elif args[0] is None or isinstance(args[0], (str, int, TableIndex, TableIndexMapElem)):
      narg.append(args.pop(0))
    else:
      raise ValueError('Invalid calling sequence')

    # Determine the value of the mode argument
    if len(args) > 0 and (args[0] is None or isinstance(args[0], ReadMode)):
      mode = args.pop(0)
    else:
      mode = None

    # Convert the mode into the one expected by the underlying isstart
    if mode == ReadMode.ISNEXT:
      mode = ReadMode.ISFIRST
    elif mode == ReadMode.ISPREV:
      mode = ReadMode.ISLAST
    narg.append(mode)

    # Add the remaining unprocessed arguments and replace the original
    narg.extend(args)
    self._first = tuple(narg)
    return mode

  def __iter__(self):
    self._parseargs()
    return self

  def __next__(self):
    if self._first:
      args = self._first
      self._first = None
      return self._tabobj.read(*args, **self._kwds)
    return self._tabobj.read()

def test(opts):
  DEFILE = ISAMtable(DEFILEdefn, tabpath=opts.tstdata, mode=OpenMode.ISINPUT)
  for row in ISAMtableIter(DEFILE, ReadMode.ISPREV, filename='defile'):
    print(row)
