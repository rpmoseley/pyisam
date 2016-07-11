from . import ISAMobject, ReadMode, OpenMode, LockMode
from .table import ISAMtable
from .utils import ISAM_str
from .tabdefns.stxtables import DEFILEdefn, DEKEYSdefn, DECOMPdefn, DEITEMdefn
import argparse
import os
import sys

MAX_TEST = 8
DEF_TEST = 8
parser = argparse.ArgumentParser(prog='pyisam',
                                 description='PyISAM command line interface',
                                 argument_default=False)
parser.add_argument('-n', '--dry-run',
                    dest='dry_run',
                    action='store_true',
                    help="Perform the tests that read data, don't run any that update data")
parser.add_argument('-V', '--version',
                    dest='version',
                    action='store_true',
                    help='Give the version of the program and library')
parser.add_argument('-i', '--interactive',
                    dest='interactive',
                    action='store_true',
                    help='Run an interactive shell like interface')
parser.add_argument('-t', '--test',
                    dest='run_mode',
                    type=int,
                    help='Run a specific test',
                    choices=range(1, MAX_TEST+1),
                    default=DEF_TEST)
parser.add_argument('-v', '--verbose',
                    dest='verbose',
                    action='store_true',
                    help='Produce verbose information about the progress')
parser.add_argument('-d', '--debug',
                    dest='debug',
                    type=int,
                    help='Provide debug information',
                    default=0)
parser.add_argument('-e', '--error', '-Werror',
                    dest='error_raise',
                    action='store_true',
                    help='Raise an error instead of continuing',
                    default=True)
opts = parser.parse_args()

# Give the version if requested and finish without further processing
if opts.version:
  print('pyisam early version 0.02')
  sys.exit(1)

# Switch off the testing if running in interactive mode
if opts.interactive:
  opts.run_mode = None

def dump_record(tabobj, idxkey, mode, colname, colval):
  record = tabobj.read(idxkey, mode, colval)
  while getattr(record, colname) == colval:
    print(record)
    record = tabobj.read()

def dump_record_v2(tabobj, idxkey, mode, colname, colval):
  record = tabobj.read(idxkey, mode, getattr(tabobj._row_, colname) == colval)
  while getattr(record, colname) == colval:
    print(record)
    record = tabobj.read()

if opts.run_mode in (1,5):
  # Test 01: Dump the contents of the defile table for itself.
  # Test 05: Dump the contents of the dekeys/decomp tables for defile in addition.
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
  # Test 02: Dump the list of error codes and meanings from the ISAM library
  isamobj = ISAMobject()
  for errno in range(100,isamobj.is_nerr):
    print(errno,isamobj.strerror(errno))

elif opts.run_mode == 3:
  # Test 03: Print the version string from the ISAM library
  isamobj = ISAMobject()
  print(ISAM_str(isamobj.isversnumber))

elif opts.run_mode == 4:
  # Test 04: Check how the new enums modules works with duplicate values
  isamobj = ISAMobject()
  print(OpenMode.ISFIXLEN.value,OpenMode.ISFIXLEN.name)

elif opts.run_mode == 6:
  # Test 06: Dump the complete definition of a table using a function
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  dump_record(DEFILE, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DEKEYS = ISAMtable(DEKEYSdefn, tabpath='data')
  dump_record(DEKEYS, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'defile')
  DEITEM = ISAMtable(DEITEMdefn, tabpath='data')
  dump_record(DEITEM, 'usekey', ReadMode.ISGREAT, 'comp', 'defile')

elif opts.run_mode == 7:
  # Test 07: Dump a specific error message
  isobj = ISAMobject()
  print('ERRSTR:', isobj.strerror(109))

elif opts.run_mode == 8:
  # Test 08: Define a table at runtime using the dynamic table support
  from .tabdefns.dynamic import DynamicTableDefn
  from .tabdefns import TextColumn, CharColumn, LongColumn
  from .tabdefns import PrimaryIndex, UniqueIndex, DuplicateIndex
  DECOMPdefn = DynamicTableDefn('decomp', error=opts.error_raise)
  DECOMPdefn.append(TextColumn('comp',     9))
  DECOMPdefn.append(CharColumn('comptyp'    ))
  DECOMPdefn.append(TextColumn('sys',      9))
  DECOMPdefn.append(TextColumn('prefix',   5))
  DECOMPdefn.append(TextColumn('user',     4))
  DECOMPdefn.extend([TextColumn('database', 6),
                     TextColumn('release',  5),
                     LongColumn('timeup'     ),
                     LongColumn('specup'     )])
  DECOMPdefn.add_index(PrimaryIndex('comp'))
  DECOMPdefn.add_index(DuplicateIndex('prefix'))
  DECOMPdefn.add_index(UniqueIndex('typkey', 'comptyp', 'comp'))
  DECOMPdefn.add_index(UniqueIndex('syskey', ['sys', 'comptyp', 'comp']))
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'adpara')

elif opts.run_mode == 9:
  # Test 09: 
  from .tabdefns.dynamic import DynamicTableDefn
  from .tabdefns import TextColumn, CharColumn, LongColumn
