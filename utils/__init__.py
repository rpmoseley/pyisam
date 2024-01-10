'''
This subpackage contains utilities classes and methods that are used to build the pyisam package,
the subpackage itself is not part of the actual package and is not distributed.
'''

from .dirstack import PathStack
from .reposdiff import gen_changes

__all__ = 'PathStack', 'gen_changes'
