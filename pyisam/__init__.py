'''
This package provides a python interface to the IBM Informix CISAM library (or alternatively the
open source VBISAM replacement library). Additional modules add the ability to add table layout
definitions and the idea of rowsets to the underlying library of choice.
'''

from .isam import ISAMobject

__all__ = ('ISAMobject',)
