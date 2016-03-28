libisam
=======

This file provides details about how to prepare the CISAM libraries provided by IBM for use by the pyisam package.

ctypes backend
--------------
The builtin ctypes support requires a library to have an SONAME defined with the library in order to be able to be loaded, 
the libraries as distributed are expected to be linked directly in the application which wishes to make access to an ISAM
database. Thus the pyisam package loads a library named libpyisam which links with either the 32- or 64-bit variants of the
underlying library and also provides the necessary information to be used.

cffi backend
------------
Since the cffi support requires an extra build step that creates a compiled extension that directly refers to the underlying
libraries, all that is required is to ensure that the correct variant (32- or 64-bit) is placed into the 'lib' directory in 
order for the system loader to locate it correctly.
