#! /usr/bin/env python3
import argparse
import os
import sys

if sys.version_info.major < 3 or sys.version_info.minor < 6:
  print('PyISAM is written to work with python 3.6+ only')
  sys.exit(1)

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
  from . import __version__
  from .backend import use_conf
  print(f'pyisam early version {__version__} using {use_conf}')
  sys.exit(1)
