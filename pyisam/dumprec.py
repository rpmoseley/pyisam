'''
Common functions used by various tests
'''

def dump_record_exp_eq(tabobj, idxkey, mode, colname, colval):
  'Dump records using explicit read MODE and check COLNAME=COLVAL'
  record = tabobj.read(idxkey, mode, colval)
  while getattr(record, colname) == colval:
    print(record)
    record = tabobj.read()

def dump_record_imp(tabobj, *, record=None, idxkey=None, mode=None, **colcheck):
  '''Dump records using implicit mode and check from COLCHECK and auto-selecting
     a suitable index based on the columns involved in the check'''
  if tabobj.isclosed:
    tabobj.open()
  record = tabobj.read(idxkey, mode, getattr(tabobj._row_, colname) == colval)
  while getattr(record, colname) == colval:
    print(record)
    record = tabobj.read()
