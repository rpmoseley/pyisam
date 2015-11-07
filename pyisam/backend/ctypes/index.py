'''
This module provides the ctypes specific support for indexes
'''

from .isam import keydesc,keypart
from ...enums import IndexFlags

class ISAMindexMixin:
  'This class provides the ctypes specific methods for ISAMindex'
  def create_keydesc(self, tabobj, optimize=False):
    'Create a new keydesc using the column information in tabobj'
    def _idxpart(idxcol):
      colinfo = getattr(tabobj, idxcol.name)
      kpart = keypart()
      if idxcol.length is None and idxcol.offset is None:
        # Use the whole column in the index
        kpart.start = colinfo._offset_
        kpart.leng = colinfo._size_
      elif idxcol.offset is None:
        # Use the prefix part of column in the index
        if idxcol.length > colinfo._size_:
          raise ValueError('Index part is larger than specified column')
        kpart.start = colinfo._start_
        kpart.leng = idxcol.length
      else:
        # Use the length of column from the given offset in the index
        if idxcol.offset + idxcol.length > colinfo._size:
          raise ValueError('Index part too long for the specified column')
        kpart.start = colinfo._offset_ + idxcol.offset
        kpart.leng = idxcol.length
      kpart.type_ = colinfo._type_
      return kpart
    kdesc = keydesc()
    kdesc.flags = IndexFlags.DUPS if self._dups_ else IndexFlags.NO_DUPS
    if self._desc_: kdesc.flags += IndexFlags.DESCEND
    kdesc_leng = 0
    if isinstance(self._colinfo_, (tuple, list)):
      # Multi-part index comprising the given columns
      kdesc.nparts = len(self._colinfo_)
      for idxno, idxcol in enumerate(self._colinfo_):
        kpart = kdesc.part[idxno] = _idxpart(idxcol)
        kdesc_leng += kpart.leng
    else:
      # Single part index comprising the given column
      kdesc.nparts = 1
      kpart = kdesc.part[0] = _idxpart(self._info_._colinfo_)
      kdesc_leng = kpart.leng
    if optimize and kdesc_leng > 8:
      kdesc.flags += IndexFlags.ALL_COMPRESS
    return kdesc