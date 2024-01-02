Overview
========
This package provides a python wrapper around the IBM Informix C-ISAM library or
alternatively the provided vbisam open source library that provides a drop-in
replacement of the C-ISAM library.

The primary reason for this package was to permit a virtual table implementation
to be written for the APSW SQLite wrapper package, without having to resort to
writing an extension module.

Although the original C-ISAM is owned and copyrighted by IBM, who now owns the
product, originally provided by Informix, both the 32-bit and 64-bit variants of
the libaries for Linux are in the git repository.

Also as part of the repository is a slightly modified version of VBISAM (v2.1.1)
at the time, which can be built using the meson build system, a copy of the library
is included built as a 64-bit library.

Required packages
-----------------
It is best to run the pyisam module from within a virtual environment, and ensure
that at least the packages in 'requirements.txt' are installed. To compile new
modules ensure that the packages in 'requirements-dev.txt' are also installed.

Generate support extension modules
----------------------------------
In order to generate the necessary lowlevel interface extensions, use the following
command:

$ python utils/bldlibisam.py

this will generate and copy all the necessary files that are required by the backend
support.

Selecting the backend and variant of ISAM to use
------------------------------------------------
The default setting of the backend is to use CFFI and the VBISAM library. To change
this modify the conf.py or set the environment variable 'PYISAM_BACKEND'.

The variable should have the format 'BACKEND.VARIANT' where BACKEND is one of
'ctypes' or 'cffi', VARIANT is one of 'ifisam', or 'vbisam'.

Running the tests
-----------------
A number of tests are provided which makes use of the sample data found in the 'data'
directory, this are also the tables defined in the 'pyisam/tabdefns/stxtables.py' module.
To run all the tests, run the following command:

$ python -m tests

To run a particular test, run the following command:

$ python -m tests -tNUM

To get help on the tests, run the following command:

$ python -m tests -h

Defining the row layout
-----------------------
As C-ISAM and VBISAM do not natively store the layout of a record, there is a need to
provide the definition manually using a similar way that the sample tables are defined
in the 'pyisam/tabdefns' subdirectory. As an alternative, the record can be constructed
at runtime using the DynamicTableDefn defined in 'pyisam/tabdefns/dynamic.py'.

Utilities
---------
A number of utility scripts can be found in the 'utils', including the 'bldlibisam.py'
program which compiles the extension modules.

Distribution
------------
Currently there is no installation method to install the package into place, as this still
a work in progress.


