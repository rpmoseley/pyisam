Overview
========
This file describes the feature of a dynamic table definition that removes the need to have a
pre-defined definition in the 'tabdefns' sub-package, such as the 'stxtables' that is provided
as an example of table definitions.

The table definition object is used to create the record object which permits the data stored in
the underlying ISAM data files to be accessed in a more Pythonic manner. ISAM has no means of
storing the record structure natively without external assistance, and has always relied on the
application to have the structure compiled within the binary.

Application Usage
-----------------
A typical application would make use of dynamic table definitions by creating an instance of the
'tabdefns.dynamic.DynamicTableDefn' class, and then add the required fields and indexes as necessary,
before creating an instance of the actual 'table.ISAMtable' class passing the previously established
definition object.

Fields are added by calling the 'append' method of the dynamic table definition object and should be
instances of 'tabdefns.CharColumn', 'tabdefns.TextColumn', 'tabdefns.ShortColumn', 'tabdefns.LongColumn',
'tabdefns.FloatColumn', 'tabdefns.DoubleColumn', 'tabdefns.DateColumn', 'tabdefns.MoneyColumn',
'tabdefns.SerialColumn' or derived classes.

To add a series of fields call the 'extend' method instead of 'append' passing a sequence of the same
instances as the 'append' method.

Indexes are added by calling the 'add_index' method of the dynamic table definition object and should be
instances of 'tabdefns.DuplicateIndex', 'tabdefns.UniqueIndex', 'tabdefns.PrimaryIndex',
'tabdefns.AscDuplicateIndex', 'tabdefns.AscUniqueIndex', 'tabdefns.AscPrimaryIndex',
'tabdefns.DescDuplicateIndex', 'tabdefns.DescUniqueIndex', 'tabdefns.DescPrimaryIndex', or derived classes.
