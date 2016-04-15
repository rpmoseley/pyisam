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

Column Objects
--------------
These objects are used to both name and provide a type for the columns used within a table, all are based on TableDefnColumn, they are:

CharColumn(name)
    Name a column that will take a single character value, note that CISAM does not really have a single char type, this is
    provided to enable applications to process single character values without having to create a buffer each time.

TextColumn(name, length)
    Name a column that will take a string of character values upto a maximum length. Values are extended to the required length
    with blanks when writing to the ISAM table, and any trailing blanks are stripped before returning when reading.

ShortColumn(name)
    Name a column which takes a short integer, this is stored on the ISAM table as a 16-bit native value.

LongColumn(name)
    Name a column which takes a long integer, this is stored on the ISAM table as a 32-bit native value.

FloatColumn(name)
    Name a column which takes a float value, this is stored on the ISAM table as a native float value.

DoubleColumn(name)
    Name a column which takes a double value, this is stored on the ISAM table as a native double value.

Index Objects
~~~~~~~~~~~~~
These objects are used to repesent indexes on the ISAMtable object, each index stores the list of columns
that make up the index along with whether the index is descending and can accept duplicates, these are
based on the TableDefnIndex class:

PrimaryIndex
    ISAM expects the first index to be the primary index, that is the default index used when a table is
    first opened, it behaves like a 'UniqueIndex'.

DuplicateIndex
    This type of index permits the columns involved in the index to have values that result in a duplicate
    reference into the ISAM data, the overall index value can duplicate an existing index entry.

UniqueIndex
    This type of index does not permit the columns involved in the index to have values that result in a
    duplicate reference into the ISAM data, each entry in the index should consist of unique values when the
    columns involved are used to generate the index, in other words, individual column values may be duplicated,
    but the overall index value should be unique.

The following variants are provided to convey whether the index is ascending, the default, or descending in
order to improve readability:

AscPrimaryIndex
    This is used as an alias of PrimaryIndex to indicate that the index is specifically ascending.

AscDuplicateIndex
    This is used as an alias of DuplicateIndex to indicate that the index is specifically ascending.

AscUniqueIndex
    This is used as an alias of UniqueIndex to indicate that the index is specifically ascending.

DescPrimaryIndex
    This is used as an alias of PrimaryIndex to indicate that the index is specifically descending.

DescDuplicateIndex
    This is used as an alias of DuplicateIndex to indicate that the index is specifically descending.

DescUniqueIndex
    This is used as an alias of UniqueIndex to indicate that the index is specifically descending.
