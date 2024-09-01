'''
This module will load the backend that is currently configured for the underlying ISAM support.
'''

__all__ = 'ISAMobject', 'ISAMindex', 'ISAMdictinfo', 'ISAMkeydesc'

from .backend import _backend

# Stub for the actual ISAMobject which maintains existing usage
class ISAMobject(_backend.ISAMobjectMixin):
  'Class representing the low-level ISAM object interface'
  __slots__ = ('_fd', '_fdmode', '_fdlock', '_recsize')
  def __init__(self, *args, **kwds):
    _backend.ISAMobjectMixin.__init__(self)
    self._fd = self._fdmode = self._fdlock = None
    self._recsize = 0

# Stub for the actual ISAMindex which maintains existing usage
class ISAMindex(_backend.ISAMindexMixin):
  'Class representing the low-level ISAM index interface'
  pass

# Provide access to the keydesc, dictinfo and record classes
ISAMkeydesc = _backend.ISAMkeydesc
ISAMdictinfo = _backend.ISAMdictinfo
