'''
This module will load the backend that is currently configured for the underlying ISAM support.
'''

__all__ = 'ISAMobject', 'ISAMdictinfo', 'ISAMkeydesc', 'ISAMindexMixin'

from .backend import ISAMobjectMixin, ISAMindexMixin, ISAMdictinfo, ISAMkeydesc

# Stub for the actual ISAMobject which maintains existing usage
class ISAMobject(ISAMobjectMixin):
  __slots__ = ('_isfd_', '_fdmode_', '_fdlock_')
  def __init__(self, *args, **kwds):
    self._isfd_ = self._fdmode_ = self._fdlock_ = None
