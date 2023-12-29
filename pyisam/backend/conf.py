'''
This module provides the ability of configuring the backend support
'''

# Define the backend to be used by the package, valid options are:
#   'cffi'    - CFFI backend
#   'ctypes'  - CTYPE backend
#   'cython'  - CYTHON backend
import os
backend = os.environ.get('PYISAM_BACKEND', 'cffi')
#backend = 'ctypes'               # Was ctypes then cffi
