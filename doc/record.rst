ISAM Record Handling
====================

Overview
--------
Traditionally an ISAM record layout was only known by offsets into an area of memory that
had the size as reported by the 'isrecsize' global variable available from the ISAM library,
this is known as the record buffer within the 'pyisam' package.

The 'pyisam.table.record' module provides a means to create classes that define that record buffer
in a more "Pythonic" way, in that the individual fields can be accessed through the use of their
name or index within the record buffer.

This is achieved by creating at runtime, a class definition that makes use of a mixin class to provide
the shared functionality, and then a series of getset descriptors for each of the field in the record that
update the underlying ISAM record buffer, which is based as an array of bytes to the underlying ISAM library.

Examples
--------
