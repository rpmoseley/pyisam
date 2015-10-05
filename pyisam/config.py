'''
This module provides configuration for the pyisam package
'''

# Set the following variable to True to enable the use of the CFFI interface, otherwise
# leave as False to use the ctypes interface.
use_cffi = True

# Set the following variable to use the original version ISAMindex which does not use
# mixin to choose between the ctypes or cffi way of preparing the key information
use_orig_ISAMindex = False
