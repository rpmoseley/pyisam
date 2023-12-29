#! /usr/bin/env python3
from . import ISAMobject, ReadMode, OpenMode, LockMode
from .table import ISAMtable
from .utils import ISAM_str
from .tabdefns.stxtables import DEFILEdefn, DEKEYSdefn, DECOMPdefn, DEITEMdefn
from .dumprec import dump_record_imp, dump_record_exp_eq
import argparse
import os
import sys

if sys.version_info.major < 3 or sys.version_info.minor < 5:
  print('PyISAM is written to work with python 3.5+ only')
  sys.exit(1)

MAX_TEST = 12
DEF_TEST = 11
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
  print('pyisam early version 0.03')
  sys.exit(1)

# Switch off the testing if running in interactive mode
if opts.interactive:
  opts.run_mode = 0
else:
  print('TEST:', opts.run_mode)

if opts.run_mode == 1:
  # Test 01: Dump the list of error codes and meanings from the ISAM library
  isobj = ISAMobject()
  for errno in range(100, isobj.is_nerr):
    print(errno, isobj.strerror(errno))

elif opts.run_mode == 2:
  # Test 02: Print the version string from the ISAM library
  isobj = ISAMobject()
  print(ISAM_str(isobj.isversnumber))

elif opts.run_mode == 3:
  # Test 03: Check how the new enums modules works with duplicate values
  print(OpenMode.ISFIXLEN.value, OpenMode.ISFIXLEN.name)

elif opts.run_mode in (4, 5):
  # Test 04: Dump the contents of the defile table for itself.
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  def_rec = DEFILE.read('key', ReadMode.ISGREAT, 'defile')
  while def_rec.filename == 'defile':
    print(def_rec)
    def_rec = DEFILE.read()

  if opts.run_mode == 5:
    # Test 05: Dump the contents of the dekeys/decomp tables for defile in addition.
    DEKEYS = ISAMtable(DEKEYSdefn, tabpath='data')
    dek_rec = DEKEYS.read('key', ReadMode.ISGREAT, 'defile')
    while dek_rec.filename == 'defile':
      print(dek_rec)
      dek_rec = DEKEYS.read()
    DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
    dec_rec = DECOMP.read('comp', ReadMode.ISEQUAL, 'defile')
    print(dec_rec)

elif opts.run_mode == 6:
  # Test 06: Perform the test 05 using dynamic table definition in order to prove it
  #          is possible to base a definition on one pre-built definition and generate
  #          the remainder from that.
  pass

elif opts.run_mode == 7:
  # Test 07: Dump the complete definition of a table using a function
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  dump_record_exp_eq(DEFILE, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DEKEYS = ISAMtable(DEKEYSdefn, tabpath='data')
  dump_record_exp_eq(DEKEYS, 'key', ReadMode.ISGREAT, 'filename', 'defile')
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record_exp_eq(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'defile')
  DEITEM = ISAMtable(DEITEMdefn, tabpath='data')
  dump_record_exp_eq(DEITEM, 'usekey', ReadMode.ISGREAT, 'comp', 'defile')

elif opts.run_mode == 8:
  # Test 08: Dump a specific error message
  isobj = ISAMobject()
  print('ERRSTR:', isobj.strerror(109))

elif opts.run_mode == 9:
  # Test 09: Define a full table at runtime using the dynamic table support
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
  dump_record_exp_eq(DECOMP, 'comp', ReadMode.ISEQUAL, 'comp', 'adpara')

elif opts.run_mode == 10:
  # Test 10: Define a table pulling index information from the underlying
  #          table at runtime using the dynamic table support
  from .tabdefns.dynamic import DynamicTableDefn
  from .tabdefns import TextColumn, CharColumn, LongColumn
  DECOMPdefn = DynamicTableDefn(tabname='decomp',
                                fields=[TextColumn('comp',     9),
                                        CharColumn('comptyp'    ),
                                        TextColumn('sys',      9),
                                        TextColumn('prefix',   5),
                                        TextColumn('user',     4),
                                        TextColumn('database', 6),
                                        TextColumn('release',  5),
                                        LongColumn('timeup'     ),
                                        LongColumn('specup'     )],
                                error=opts.error_raise)
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  # Force the building of the indexes by using the ISAM table indirectly
  dump_record_imp(DECOMP, comp__eq='defile')

elif opts.run_mode == 11:
  # Test 11: Define a table pulling index information from the underlying
  #          table at runtime using the dynamic table support using multiple
  #          columns in the comparison
  from .tabdefns.dynamic import DynamicTableDefn
  from .tabdefns import TextColumn, CharColumn, ShortColumn
  DEFILEdefn = DynamicTableDefn(tabname='defile',
                                fields=[TextColumn ('filename',  9),
                                        ShortColumn('seq'         ),
                                        TextColumn ('field',     9),
                                        TextColumn ('refptr',    9),
                                        CharColumn ('type'        ),
                                        ShortColumn('size'        ),
                                        CharColumn ('keytype'     ),
                                        ShortColumn('vseq'        ),
                                        ShortColumn('stype'       ),
                                        CharColumn ('scode'       ),
                                        TextColumn ('fgroup',   10),
                                        CharColumn ('idxflag'     )],
                                error=opts.error_raise)
  DEFILE = ISAMtable(DEFILEdefn, tabpath='data')
  # Force the building of the indexes by using the ISAM table indirectly
  dump_record_imp(DEFILE, filename__eq='defile', seq__lte=100)

elif opts.run_mode == 12:
  # Test 12: 
  DECOMP = ISAMtable(DECOMPdefn, tabpath='data')
  dump_record_exp_eq(DECOMP, None, ReadMode.ISEQUAL, 'comp', 'defile')
