'''
This module provides the ability of configuring the backend support
'''

# Define the backend to be used by the package, valid options are:
#   'cffi'    - CFFI backend
#   'ctypes'  - CTYPE backend
#   'cython'  - CYTHON backend
# To select between the C-ISAM and VBISAM variant suffix the value
# with either:
#   '.ifisam' - C-ISAM variant
#   '.vbisam' - VBISAM variant
import os
backend = os.environ.get('PYISAM_BACKEND', 'cffi.vbisam')
