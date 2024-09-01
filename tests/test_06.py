'''
Test 06: Low-level access to data without using a Table instance
'''

import os
import struct
from pyisam.constants import ReadMode, OpenMode
from pyisam.isam import ISAMobject

defile_defn = struct.Struct('>9sh9s9schchhc10sc')

def dump_defile(recbuff):
  fld = defile_defn.unpack_from(recbuff, 0)
  return fld

def test(opts):
  tabpath = os.path.join(opts.tstdata, 'defile')
  isobj = ISAMobject()
  isobj.isopen(tabpath, mode=OpenMode.ISINPUT)
  buff = isobj.create_record(isobj._recsize)
  struct.pack_into('>9sh', buff, 0, b'defile', 0)
  isobj.isread(buff, ReadMode.ISGTEQ)
  print(dump_defile(buff))
