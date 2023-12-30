'''
Test 04: Check if the ISAMfunc decorator is correctly behaving on some made up calls then
         checking that the .restype and .argtype on the function are correct.
         
         NOTE: The checking of arguments and return type are not enforced by this test, only
         that the correct path through the ISAMfunc decorator has been taken.

For reference with the checks below here is a copy of the ISAMfunc decorator marked:

def ISAMfunc(*orig_args, **orig_kwd):
  def call_wrapper(func):
    def wrapper(self, *args, **kwd):
      lib_func = getattr(self._lib_, func.__name__)
      if not (hasattr(lib_func, 'restype') and hasattr(lib_func, 'argtypes')):
        if 'restype' in orig_kwd:                                   # T1
          lib_func.restype = orig_kwd['restype']
          if 'argtypes' in orig_kwd:                                # T2
            lib_func.argtypes = orig_kwd['argtypes']
          else:
            lib_func.argtypes = orig_args if orig_args else None    # T3, T4
        elif not orig_args:                                         # T5
          lib_func.restype = lib_func.argtypes = None
        else:                          
          lib_func.argtypes = orig_args if orig_args[0] else None   # T6, T7
      return func(self, *args, **kwd)
    return functools.wraps(func)(wrapper)
  return call_wrapper
'''

from argparse import Namespace
from pyisam.backend import conf
if conf.backend == 'ctypes':
  from pyisam.backend.ctypes import ISAMfunc

  class TestFunc:
    def __init__(self, func):
      self._func = func
      self.restype = int
      
    def __call__(self, *args, **kwds):
      self._func(*args, **kwds)
    
  class TestObject:
    _lib_ = Namespace()           # Mock the underlying library by adding functions as they are called
  
    def __init__(self):
      self._lib_._chkerror = self._chkerror
      self._lib_.tst01 = TestFunc(self.tst01)
      self._lib_.tst02 = TestFunc(self.tst02)
      self._lib_.tst03 = TestFunc(self.tst03)
      self._lib_.tst04 = TestFunc(self.tst04)
      self._lib_.tst05 = TestFunc(self.tst05)
      self._lib_.tst06 = TestFunc(self.tst06)
        
    def __getattr__(self, name):
      if name in self._lib_:
        return self._lib_[name]
      return object.__getattribute__(self, name)
  
    def _chkerror(self):
      pass
  
    @ISAMfunc(restype='hello', argtypes='world')   # T1, T2
    def tst01(self):
      pass

    @ISAMfunc('world', restype='hello')            # T1, T3
    def tst02(self):
      pass

    @ISAMfunc(restype='hello')                     # T1, T4
    def tst03(self):
      pass

    @ISAMfunc()                                    # T5
    def tst04(self):
      pass

    @ISAMfunc(None)                                # T6
    def tst05(self):
      pass

    @ISAMfunc('world')                             # T7
    def tst06(self):
      pass

    @ISAMfunc('world', 'now')
    def tst07(self):
      pass

  def test(opts):
    TO = TestObject()
    TO.tst01()                      # ISAMfunc(restype='hello', argtypes='world'): tst01('world') => hello
    TO.tst02()                      # ISAMfunc('world', restype='hello')         : tst02(('world',)) => hello
    TO.tst03()                      # ISAMfunc(restype='hello')                  : tst03(None) => hello
    TO.tst04()                      # ISAMfunc()                                 : tst04(None) => None
    TO.tst05()                      # ISAMfunc(None)                             : tst05(None) => <class 'int'>
    TO.tst06()                      # ISAMfunc('world')                          : tst06(('world',)) => <class 'int'>
    for tst in 'tst01 tst02 tst03 tst04 tst05 tst06'.split(None):
      tfunc = getattr(TO._lib_, tst)
      if not hasattr(tfunc, 'restype'):
        print(tst.upper(), 'NO RESTYPE')
      elif not hasattr(tfunc, 'argtypes'):
        print(tst.upper(), 'NO ARGTYPES')
      else:
        print(tst.upper(), 'RESTYPE:', tfunc.restype, 'ARGTYPES:', tfunc.argtypes)
else:
  def test(opts):
    print('Test not valid for backend selected')
