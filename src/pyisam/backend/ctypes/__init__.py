'''
This is the CTYPES specific implementation of the package

This module provides a ctypes based interface to either the IBM C-ISAM or
the open source VBISAM library without requiring an explicit extension to
be compiled.

The particular library is chosen depending on the setting of the use_isamlib
variable that is set when the backend system is initialised, appropriate
replacement methods are provided for the VBISAM backend to match the C-ISAM one.
'''
