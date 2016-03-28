from . import ISAMobject, ReadMode, OpenMode, LockMode
from .table import ISAMtable
from .utils import ISAM_str
from .tabdefns.stxtables import DEFILEdefn, DEKEYSdefn, DECOMPdefn, DEITEMdefn
import os
import sys

if len(sys.argv) > 1 and sys.argv[1] in '1234567':
  run_name = '__main{}__'.format(sys.argv[1])
elif __name__ == '__main__':
  run_name = '__main1__'

def dump_record(tabobj, idxkey, mode, colname, colval):
  record = getattr(tabobj, 'read')(idxkey, mode, colval)
  while getattr(record, colname) == colval:
    print(record)
    record = getattr(tabobj, '_read')()

if run_name in ('__main1__','__main5__'):
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  def_rec = DEFILE.read('key', ReadMode.ISGREAT, 'defile')
  while def_rec.filename == 'defile':
    print(def_rec)
    def_rec = DEFILE.read()
  if run_name == '__main5__':
    DEKEYS = ISAMtable(DEKEYSdefn, tabpath='data')
    dek_rec = DEKEYS.read('key', ReadMode.ISGREAT, 'defile')
    while dek_rec.filename == 'defile':
      print(dek_rec)
      dek_rec = DEKEYS.read()
    DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
    dec_rec = DECOMP.read('comp', ReadMode.ISEQUAL, 'defile')
    print(dec_rec)

elif run_name == '__main2__':
  isamobj = ISAMobject()
  for errno in range(100,isamobj.is_nerr):
    print(errno,isamobj.strerror(errno))

elif run_name == '__main3__':
  isamobj = ISAMobject()
  print(ISAM_str(isamobj.isversnumber))

elif run_name == '__main4__':
  isamobj = ISAMobject()
  print(OpenMode.ISFIXLEN.value,OpenMode.ISFIXLEN.name)

elif run_name == '__main6__':
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  dump_record(DEFILE, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DEKEYS = ISAMtable(DEKEYSdefn, tabpath='data')
  dump_record(DEKEYS, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'defile')
  DEITEM = ISAMtable(DEITEMdefn, tabpath='data')
  dump_record(DEITEM, 'usekey', ReadMode.ISGREAT, 'comp', 'defile')

elif run_name == '__main7__':
  from .table import recordclass
  DECOMPrec = recordclass(DECOMPdefn)
  DR1 = DECOMPrec('DECOMP')
  print(dir(DR1),DR1._curoff,DR1.__dir__)
  from .record import ISAMrecordMixin,TextColumn,CharColumn,LongColumn
  class RecordDECOMPM(ISAMrecordMixin):
    __slots__ = ()
    _fields = ('comp', 'comptyp', 'sys', 'prefix', 'user', 'database', 'release', 'timeup', 'specup')
    comp = TextColumn(9)
    comptyp = CharColumn()
    sys = TextColumn(9)
    prefix = TextColumn(5)
    user = TextColumn(4)
    database = TextColumn(6)
    release = TextColumn(5)
    timeup = LongColumn()
    specup = LongColumn()
  DR2 = RecordDECOMPM()
  print(dir(DR2),DR2._curoff,DR2.__dir__)
