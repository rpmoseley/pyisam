'''
This module will take the __main__.py module and perform a simple conversion to
make a run_wing.py module for use under the WingIDE product.

It looks for the lines starting with 'from .X import' and transform them into
'from pyisam.X import' unless X is '' when it is 'from pyisam import'
'''

# This will only create the file 'run_wing.py' from 'tests/__main__.py'
i_name = 'tests/__main__.py'
o_name = 'run_wing.py'
with open(i_name) as fd, open(o_name, 'w') as ofd:
  for line in fd:
    from_off = line.find('from .')
    if from_off >= 0:
      impoff = line.find(' import ', from_off + len('from .'))
      ofd.write(line.replace(' .', ' pyisam.' if impoff > from_off + len('from . ') else ' pyisam'))
    else:
      ofd.write(line)
