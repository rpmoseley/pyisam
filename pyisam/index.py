'''
This module provides the index related objects
'''

class ISAMindexMap:
  '''Class providing the mapping between index name and underlying ISAM
     key description information encapsulating the information'''
  def __init__(self, tabobj, intern_index):
    if not isinstance(tabobj, ISAMtable):
      raise ValueError('Expecting TABOBJ to be an instance of ISAMtable')
    if not isinstance(intern_index, ISAMindex):
      raise ValueError('Expecting INTERN_INDEX to be an instance of ISAMindex')
    self._tabobj = tabobj
    self.name = intern_index.name
    self.intern_defn = intern_index
    self.isam_desc = None
    self.isam_num = -1
  def is_match(self, isam_desc, isam_knum):
    '''Checks if the ISAM_DESC matches the internal version and updates the
       ISAM_DESC and NUMBER attributes appropriately'''
