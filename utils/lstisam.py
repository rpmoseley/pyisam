'''
Python implementation of the C program lstisam
'''
from pyisam import ISAMobject
from pyisam.backend import use_conf, use_isamlib

isfd = ISAMobject()
isfd.isopen('data/decomp')
print(f'Using {use_conf} and {use_isamlib} library')
print(f'Library: {isfd.isversnumber}')
dictinfo = isfd.isdictinfo()
print('Num Keys :', dictinfo.nkeys)
print('Rec Size :', dictinfo.recsize)
print('Idx Size :', dictinfo.idxsize)
print('Num Rows :', dictinfo.nrecords)
for cidx in range(0, dictinfo.nkeys):
  keyinfo = isfd.iskeyinfo(cidx)
  print('Index', cidx, ':')
  print(' Flags   :', end=' ')
  if keyinfo.flags & 0x0e == 0x0e:
    print('COMPRESS')
  else:
    if keyinfo.flags & 0x08:
      print('TCOMPRESS', end=' ')
    if keyinfo.flags & 0x04:
      print(' LCOMPRESS', end=' ')
    if keyinfo.flags & 0x02:
      print(' DCOMPRESS')
    print('ISDUPS' if keyinfo.flags & 0x01 else 'ISNODUPS')
  print(' Num Part:', keyinfo.nparts)
  print(' Length  :', keyinfo.length)
  for cprt in range(keyinfo.nparts):
    keypart = keyinfo[cprt]
    print(' Part ', cprt, ' : (', keypart.start, ', ', keypart.leng, ', ', keypart.type, ')', sep='')
# Cause an error to occur
isfd.iskeyinfo(dictinfo.nkeys)
isfd.isclose()
