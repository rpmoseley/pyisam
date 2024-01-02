#! /usr/bin/env python3
# This is the entry point for WingIDE to avoid problems with relative
# imports which occur as the package is not installed.
import argparse
import importlib
import os
import re
import sys

if sys.hexversion < 0x3060000:
  print('PyISAM is written to work with python 3.6+ only')
  sys.exit(1)

class AvailableTest:
  fre = re.compile(r'test_([0-9]{1,3})\.py$', re.IGNORECASE)

  def __init__(self, testdir='.'):
    self._dir = testdir
    self._populate()

  def _populate(self):
    _lst, _map = list(), dict()
    for cur in os.scandir(self._dir):
      mtch = self.fre.match(cur.name)
      if mtch:
        mtchnum = int(mtch.group(1))
        _lst.append(mtchnum)
        _map[mtchnum] = cur
    _lst.sort()
    self._ltst = _lst
    self._lmap = _map

  def __iter__(self):
    self._ctst = 0
    return self

  def __next__(self):
    if self._ctst >= len(self._ltst):
      raise StopIteration
    rst = self._ltst[self._ctst]
    self._ctst += 1
    return rst

  def __getitem__(self, key):
    if isinstance(key, int):
      return self._lmap[key]
    elif isinstance(key, str):
      keynum = int(key)
      if keynum in self._ltst:
        return keynum
      raise IndexError(keynum)
    raise ValueError(key)

  def __str__(self):
    return '<NotScanned>' if self._ltst is None else str(self._ltst) 

  def _all_tests(self):
    return self._ltst
  all_tests = property(_all_tests)

  def _max_test(self):
    return self._ltst[-1]
  max_test = property(_max_test)

  def run_test(self, testnum, opts):
    def _do_test(testnum):
      mod = importlib.import_module(f'.{self._lmap[testnum].name[:-3]}', self._dir)
      if opts.dry_run and hasattr(mod, 'attr') and 'write' in mod.attr:
        if opts.debug:
          print(f'Test {testnum} skipped, no updates allowed')
        return None
      if opts.debug:
        print(f'Test {testnum}:')
      tfunc = getattr(mod, 'test', None)
      if tfunc:
        return tfunc(opts)

    if testnum in (None, 0):
      return [_do_test(testnum) for testnum in self._ltst]
    else:
      return _do_test(testnum)


avail_tests = AvailableTest('tests')

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
parser.add_argument('-t', '--test',
                    dest='run_mode',
                    action='store',
                    type=int,
                    help='Run a specific test',
                    choices=avail_tests.all_tests,
                    default=0)
parser.add_argument('-v', '--verbose',
                    dest='verbose',
                    action='store_true',
                    help='Produce verbose information about the progress')
parser.add_argument('-g', '-d', '--debug',
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

def run_test(testdir, testnum, opts, avail_tests=avail_tests):
  'Run a particular test within a particular test directory'
  mod = importlib.import_module(f'.{avail_tests[testnum].name[:-3]}', testdir)
  if opts.dry_run and hasattr(mod, 'attr') and 'write' in mod.attr:
    if opts.debug:
      print(f'Test {testnum} skipped, no updates allowed')
    return None
  try:
    if opts.debug:
      print(f'Test {testnum}:')
    return getattr(mod, 'test')(opts)
  except AttributeError:
    return None
  
  
# Give the version if requested and finish without further processing
if opts.version:
  from pyisam import __version__
  from pyisam.backend import use_conf
  print(f'pyisam early version {__version__} using {use_conf}')
  sys.exit(1)

# Make use of the routine provided by the avail_tests object instead.
avail_tests.run_test(opts.run_mode, opts)
