'''
Python implementation of the C program lstisam
'''
import sys
from pyisam import ISAMobject
from pyisam.backend import use_conf, use_isamlib

isfd = ISAMobject()
isfd.isopen(sys.argv[1])
print(f'Using {use_conf} and {use_isamlib} library')
print(f'Library: {isfd.isversnumber}')
dictinfo = isfd.isdictinfo()
print('Num Keys :', dictinfo.nkeys)
print('Rec Size :', dictinfo.recsize)
print('Idx Size :', dictinfo.idxsize)
print('Num Rows :', dictinfo.nrecords)
for cidx in range(0, dictinfo.nkeys):
  keyinfo = isfd.iskeyinfo(cidx)
  keyflgs = []
  print('Index', cidx, ':')
  if keyinfo.flags & 0x0e == 0x0e:
    keyflgs.append('COMPRESS')
  else:
    if keyinfo.flags & 0x08:
      keyflgs.append('TCOMPRESS')
    if keyinfo.flags & 0x04:
      keyflgs.append('LCOMPRESS')
    if keyinfo.flags & 0x02:
      keyflgs.append('DCOMPRESS')
    keyflgs.append('ISDUPS' if keyinfo.flags & 0x01 else 'ISNODUPS')
  print(' Flags   :', keyflgs.join(', '))
  print(' Num Part:', keyinfo.nparts)
  print(' Length  :', keyinfo.length)
  for cprt in range(keyinfo.nparts):
    keypart = keyinfo[cprt]
    print(' Part ', cprt, ' : (', keypart.start, ', ', keypart.leng, ', ', keypart.type, ')', sep='')
isfd.isclose()
