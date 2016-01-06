'''
This is a python implementation of the lstisam.c program that was written to output the index information of
an existing CISAM table.
'''

import os
from pyisam.isam import ISAMobject

def fetch_isam_info(tabname, tabpath='.'):
  'List the index information for the given TABNAME'

  # Open the named table in order to access the information
  tabobj = ISAMobject()
  tabobj.isopen(os.path.join(tabpath, tabname))

  # Get the dictionary information to present the record length
  tabinfo = tabobj.isdictinfo()

  # Fetch the index information for all the keys on the table
  idxinfo = list()
  for idxnum in range(tabinfo.nkeys):
    idxinfo.append(tabobj.iskeyinfo(idxnum+1))

  # Close the ISAM table now all information has been retrieved
  tabobj.isclose()

  # Return the information for possible further processing
  return tabinfo,idxinfo

def lstisam(tabname, tabpath):
  ti, ii = fetch_isam_info(tabname, tabpath)
  print('TABNAME:', tabname)
  print('DINFO  :', ti)
  print('IINFO  : [{}]'.format(', '.join(str(i) for i in ii)))

if __name__ == '__main__':
  lstisam('defile', 'data')
  lstisam('dekeys', 'data')
  lstisam('decomp', 'data')
  lstisam('deitem', 'data')
