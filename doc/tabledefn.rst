Table Definition
================
As native CISAM has no means of defining the physical layout of a record (apart from within the application source
code), in order to provide a more object-orientated approach using attributes that are the field names requires the
underlying table to be separately defined purely for the pyisam package to provide individual field access. Native
CISAM does store the index information, mainly for its own use, but the package needs to perform a mapping from that
information to the indexes that are expected from an application.

The sub-package pyisam.tabdefns provides a number of example table definitions that can be added to according to
the purposes of the user of the package. It is also possible to define a table definition dynamically at run-time
when there is no static definition available, or the information is itself only available from ISAM tables. Of
course, the layout of those table definition tables needs to be known before the information can be accessed without
recourse to the raw offsets, sizes and related conversions.

Class Structure
---------------
The pyisam package expects that a table definition class shall define the following attributes (extra attributes can
be defined for the use of a particular application, but are ignored by the pyisam package):

* _tabname_
    This provides the default name of the file as stored in the OS filesystem, and can be overriden when calling the
    build() or open() methods of the ISAMtable object.

* _columns_
    This is a sequence of instances of CharColumn, TextColumn, ShortColumn, LongColumn, FloatColumn or DoubleColumn
    classes that define the layout of the record including the field name and for text fields the length of the field.

* _indexes_
    This is a sequence of instances of DuplicateIndex, UniqueIndex or PrimaryIndex classes that define the indexes that
    are known to be available to be used on the ISAM table. These include the name of the key (which may be the same as
    a field for applications requiring a single field index), and then the fields that make up that index.

* _prefix_
    Some applications expect that all fields will be prefixed by a given string followed by an underscore ('_') to avoid
    having to use aliases with SQL that is subsequently generated, if this is set then the pyisam package will return any
    field with the specified prefix applied.

* _database_
    Some applications expect certain ISAM tables to belong to a particular sub-directory under a central directory, if
    this is set then the pyisam package expects that the table will exist (or will be created) in this directory.
