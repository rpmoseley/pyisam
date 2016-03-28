ISAMtable structure
===================

The ISAMtable object (provided by the pyisam.table module), provides an object-orientated
view of an ISAM table replacing the need for accessing the individual column information
directly from the current record area into accesses using the x[fld] notation.  It also
maps the underlying key descriptions that define the columns in each of the indexes on the
table so that selecting an index becomes a matter of providing the index name as known by
the application instead of needing to pass the actual structure of the key as expected by
the underlying ISAM library (exposed by the ISAMobject provided by the pyisam.isam module).

The ISAMtable has a number of attributes which permit it to provide the necessary mapping
to the application, these are in general of the form _attr_ to avoid possible clashes
with actual column names which enable them to be accessed using the usual x.attr syntax.

  The _name_ attribute stores the actual name of the ISAM file as stored on disk.

  The _path_ attribute stores the pathname to the ISAM file named by the _name_ attribute.

  The _defn_ attribute stores a reference to the ISAMtableDefn object (or other type of
  object that provides the necessary attributes) which provides information about the columns
  and indexes on the table which are otherwise not directly available within the underlying
  ISAM tables. [NOTE *This was the original intent* This information provides the offsets
  to the various column values within a record and also enable the correct key description
  to be generated to enable the use of indexes other than the primary one to be used during
  queries of the ISAM table.]

  The _colinfo_ attribute provides the list of columns available on the actual underlying
  ISAM table and adds additional information information to that provided in the _defn_
  attribute, such as offset and size of the column within the record.

  The _row_ attribute provides the last record that has been read for the associated table,
  and can also be used to build up a new record that will be written to the table.

  The _mode_ attribute stores the access mode in which the ISAM table has been opened and
  is an instance of the OpenMode enum object which limits the allowable values that can be
  used.

  The _lock_ attribute stores the lock mode in which the ISAM table has been opened and
  is an instance of the LockMode enum object which limits the allowable values that can be
  used.

  The _curidx_ attribute stores the current index being used to retrieve rows from the ISAM
  table, and is used to reduce the number of times that a new index is selected using the
  underlying 'isstart' function. It is updated by methods that initate a new row result set.

  The _idxmap_ attribute provides a mapping between the actual indexes available on the
  underlying ISAM table to those provided in the _defn_ attribute, and consists of a list
  of index names or 'None' for each of the indexes within the ISAM table. There may be more
  (or less) entries in the list according to the number of indexes provided in the _defn_
  object, trying to use an index that is not present in the list will result in an exception
  being raised and is expected to be handled by the calling application.

  The _kdesc_ attribute provides a mapping between the index names as provided by the _defn_
  attribute and the calculated key description that would be returned by the underlying ISAM
  library functions.

[NOTE: The _idxmap_ and _kdesc_ attributes should be reviewed in order to enable a better
way of accessing the relevant index information without excessive re-accessing of attributes
in order to work out which of the underlying ISAM key descriptions is related to the _defn_
object.

A possible re-work would replace the mapping on _idxmap_ to be between the index name and
and a new object ISAMindexMap which would enable the access of an index both by using its name
and also its number within the ISAM table itself (the primary key is key number 0).]

Records
-------
A record in pyisam knows how to retrieve and store the data for each column in a table, so when 
an instance of ISAMtable returns a record, it is the record that knows the values of the columns
and not the ISAMtable instance.
