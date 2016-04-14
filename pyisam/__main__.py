from . import ISAMobject, ReadMode, OpenMode, LockMode
from .table import ISAMtable
from .utils import ISAM_str
from .tabdefns.stxtables import DEFILEdefn, DEKEYSdefn, DECOMPdefn, DEITEMdefn
import argparse
import os
import sys

MAX_TEST = 10
DEF_TEST = 1
parser = argparse.ArgumentParser(prog='pyisam', description='PyISAM command line interface')
parser.add_argument('--test', '-t',
                    dest='run_mode',
                    type=int,
                    help='Run a specific test instead of interactive mode',
                    choices=range(1, MAX_TEST+1),
                    default=DEF_TEST)
opts = parser.parse_args()

def dump_record(tabobj, idxkey, mode, colname, colval):
  record = tabobj.read(idxkey, mode, colval)
  while getattr(record, colname) == colval:
    print(record)
    record = tabobj.read()

if opts.run_mode in (1,5):
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  def_rec = DEFILE.read('key', ReadMode.ISGREAT, 'defile')
  while def_rec.filename == 'defile':
    print(def_rec)
    def_rec = DEFILE.read()
  if opts.run_mode == 5:
    DEKEYS = ISAMtable(DEKEYSdefn, tabpath='data')
    dek_rec = DEKEYS.read('key', ReadMode.ISGREAT, 'defile')
    while dek_rec.filename == 'defile':
      print(dek_rec)
      dek_rec = DEKEYS.read()
    DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
    dec_rec = DECOMP.read('comp', ReadMode.ISEQUAL, 'defile')
    print(dec_rec)

elif opts.run_mode == 2:
  isamobj = ISAMobject()
  for errno in range(100,isamobj.is_nerr):
    print(errno,isamobj.strerror(errno))

elif opts.run_mode == 3:
  isamobj = ISAMobject()
  print(ISAM_str(isamobj.isversnumber))

elif opts.run_mode == 4:
  isamobj = ISAMobject()
  print(OpenMode.ISFIXLEN.value,OpenMode.ISFIXLEN.name)

elif opts.run_mode == 6:
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  dump_record(DEFILE, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DEKEYS = ISAMtable(DEKEYSdefn, tabpath='data')
  dump_record(DEKEYS, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'defile')
  DEITEM = ISAMtable(DEITEMdefn, tabpath='data')
  dump_record(DEITEM, 'usekey', ReadMode.ISGREAT, 'comp', 'defile')

elif opts.run_mode == 7:
  from .table import recordclass
  DECOMPrec = recordclass(DECOMPdefn)
  DR1 = DECOMPrec('DECOMP')
  print('DR1:', DR1)
  from .table.record import ISAMrecordMixin,TextColumn,CharColumn,LongColumn
  class RecordDECOMPM(ISAMrecordMixin):
    comp = TextColumn(9)
    comptyp = CharColumn()
    sys = TextColumn(9)
    prefix = TextColumn(5)
    user = TextColumn(4)
    database = TextColumn(6)
    release = TextColumn(5)
    timeup = LongColumn()
    specup = LongColumn()
  DR2 = RecordDECOMPM('DECOMPM')
  print('DR2:', DR2)

elif opts.run_mode == 8:
  # Test keydesc creation
  from .table.index import TableIndex,TableIndexCol
  from .table.record import ISAMrecordMixin,TextColumn,LongColumn
  class Record(ISAMrecordMixin):
    __slots__ = ()
    col1 = TextColumn(9)
    col2 = LongColumn()
  recobj = Record('tab1', 16)
  idx = TableIndex('primkey',
                   TableIndexCol('col1'),
                   'col2',
                   dups=False)
  print(idx.as_keydesc(recobj))

elif opts.run_mode == 9:
  isobj = ISAMobject()
  print('ERRSTR:', isobj.strerror(109))

elif opts.run_mode == 10:
  from .tabdefns.dynamic import DynamicTableDefn
  from .tabdefns import TextColumn, CharColumn, LongColumn
  from .tabdefns import PrimaryIndex, UniqueIndex, DuplicateIndex
  DECOMPdefn = DynamicTableDefn('decomp', error=True)
  DECOMPdefn.append(TextColumn('comp',     9))
  DECOMPdefn.append(CharColumn('comptyp'    ))
  DECOMPdefn.append(TextColumn('sys',      9))
  DECOMPdefn.append(TextColumn('prefix',   5))
  DECOMPdefn.append(TextColumn('user',     4))
  DECOMPdefn.extend([TextColumn('database', 6),
                    TextColumn('release',  5),
                    LongColumn('timeup'     ),
                    LongColumn('specup'     )])
  DECOMPdefn.add(PrimaryIndex('comp'))
  DECOMPdefn.add(DuplicateIndex('prefix'))
  DECOMPdefn.add(UniqueIndex('typkey', 'comptyp', 'comp'))
  DECOMPdefn.add(UniqueIndex('syskey', 'sys', 'comptyp', 'comp'))
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'adpara')
