Import data utility
===================

The import.py utility will build a new instance of the named table within the specified
database (unless the table can only exist within a certain database) and will then be
populated with data from the specified textual file.

Operational Notes
-----------------
A special varaint of the ISAMrecord object is used which takes each field from a line
within the textual file in the order of the _namedtuple._fields method and then places
the value suitably converted using the normal packing functionality before calling the
iswrite method on the underlying ISAMobject.
