'''
Common functions used by various tests
'''

from pyisam.autoselect import prepare_colcheck, select_index, perform_colcheck

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
  if record is None:
    record = tabobj._default_record()
  if idxkey is None and mode is None:
    # Auto-select the appropriate mode and index to use
    check = prepare_colcheck(record, **colcheck)
    print('AS:', check)
    idxname = select_index(tabobj, check, record)
    print('IDX:', idxname)
    result = perform_colcheck(record, check)
    print('RSLT:', result)
  elif idxkey is None:
    # Auto-select the appropriate index to use, but fix the mode
    print('IDXKEY: Unimplemented')
  else:
    # Use the given index and mode to use
    print('USEKEY: Unimplemented')
  """
  record = tabobj.read(idxkey, mode, getattr(tabobj._row_, colname) == colval)
  while getattr(record, colname) == colval:
    print(record)
    record = tabobj.read()
  """

