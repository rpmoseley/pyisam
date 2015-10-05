'''
This module provides the cffi specific support for indexes
'''

from ._isam_cffi import ffi
from ..enums import IndexFlags

class ISAMindexMixin:
  'This class provides the cffi specific methods for ISAMindex'
  def create_keydesc(self, tabobj, optimize=False):
    'Create a new keydesc using the column information in tabobj'
    def _idxpart(idxno, idxcol):
      colinfo = tabobj._defn_._colinfo_[idxcol.name]
      if idxcol.length is None and idxcol.offset is None:
        # Use the whole column in the index
        kdesc.k_part[idxno].kp_start = colinfo._offset_
        kdesc.k_part[idxno].kp_leng = colinfo._size_
      elif idxcol.offset is None:
        # Use the prefix part of column in the index
        if idxcol.length > colinfo._size_:
          raise ValueError('Index part is larger than the specified column')
        kdesc.k_part[idxno].kp_start = colinfo._start_
        kdesc.k_part[idxno].kp_leng = idxcol.length
      else:
        # Use the length of column from the given offset in the index
        if idxcol.offset + idxcol.length > colinfo._size_:
          raise ValueError('Index part too long for the specified column')
        kdesc.k_part[idxno].kp_start = colinfo._offset_ + idxcol.offset
        kdesc.k_part[idxno].kp_leng = idxcol.length
      kdesc.k_part[idxno].kp_type = colinfo._type_
    kdesc = ffi.new('struct keydesc *')
    kdesc_leng = 0
    if isinstance(self.colinfo, (tuple, list)):
      # Multi-part index comprising the given columns
      kdesc.k_nparts = len(self.colinfo)
      for idxno, idxcol in enumerate(self.colinfo):
        _idxpart(idxno, idxcol)
        kdesc_leng += kdesc.k_part[idxno].kp_leng
    else:
      # Single part index comprising the given column
      kdesc.k_nparts = 1
      _idxpart(0, self.colinfo)
      kdesc_leng = kdesc.k_part[0].kp_leng
    if optimize and kdesc_leng > 8:
      kdesc.k_flags += IndexFlags.ALL_COMPRESS
    return kdesc
