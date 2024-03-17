'''
This module provides the ability of configuring the backend support
'''

# Define the backend to be used by the package, valid options are:
#   'cffi'    - CFFI backend
#   'ctypes'  - CTYPE backend
#   'cython'  - CYTHON backend
# To select between the various ISAM variants suffix the value
# with one of:
#   '.ifisam' - C-ISAM variant (IBM/Informix)
#   '.vbisam' - VBISAM variant (Open Source)
#   '.disam'  - D-ISAM variant (Byte Design)
import os
backend = os.environ.get('PYISAM_BACKEND', 'cffi.vbisam')
