This package provides a python wrapper around the IBM Informix C-ISAM library or
alternatively the provided vbisam open source library that provides a drop-in
replacement of the C-ISAM library.

The primary reason for this package was to permit a virtual table implementation
to be written for the APSW SQLite wrapper package, without having to resort to
writing an extension module.

It should be noted that due to the fact that to legally get hold of the libraries
for the CISAM runtime from IBM requires a payement, that the actual shared libraries
are not included in the git repository, so in order to make use of the package it is
necessary to either already own a copy of the software or purchase a copy, unless
that is, you make use of the vbisam package which provides a virtual drop-in
replacement for much of the CISAM functionality except for the isaudit() function,
however, at the moment making use of this library causes unknown problems.
