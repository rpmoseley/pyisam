Iterator
========
This file provides an overview of the iterator object that can be used to access
data via a table object. It provides the mapping of the various arguments and key-
words to the .read() method of the ISAMtable object.

The purpose of the iterator is to enable an application to request a series of
rows of data using a simple 'for' loop and the iterator object will call the under-
lying methods .isstart() and .isread() appropriately, for instance, to get all the
records for a particular table within the 'defile' sample table.

.. code:: python
  DEFILE = ISAMtable(
  for row in ISAMiter(DEFILE, ReadMode.ISMATCH, filename='defile'):
    print(row)

DEFILE('defile', 10, 'filename', 'defilenam', 'C', 9, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 20, 'seq', 'defilseq', 'I', 0, 'N', 0, 1, ' ', ' ', ' ')
DEFILE('defile', 30, 'field', 'defield', 'C', 9, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 40, 'refptr', 'derefptr', 'C', 9, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 50, 'type', 'detype', 'C', 1, 'N', 0, 16, ' ' , ' ' , ' ')
DEFILE('defile', 60, 'size', 'desize', 'I', 0, 'N', 0, 1, ' ' ,  ' ', ' ')
DEFILE('defile', 70, 'keytype', 'dekeytype', 'C', 1, 'N', 0, 16, ' ' , ' ', ' ')
DEFILE('defile', 80, 'key', 'defilkey', 'K', 0, 'S', 0, 15, ' ', ' ', ' ')
DEFILE('defile', 90, 'unikey', 'defunikey', 'K', 0, 'S', 0, 15, ' ', ' ', ' ')
DEFILE('defile', 100, 'vseq', 'defvseq', 'I', 0, 'N', 0, 1, ' ', ' ', ' ')
DEFILE('defile', 110, 'stype', 'defstype', 'I', 0, 'N', 0, 1, ' ', ' ', ' ')
DEFILE('defile', 120, 'scode', 'defscode', 'C', 1, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 130, 'fgroup', 'defgroup', 'C', 10, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 140, 'vkey', 'defvkey', 'K', 0, 'S', 0, 15, ' ', ' ', ' ' )
DEFILE('defile', 150, 'idxflag', 'defidxfla', 'C', 1, 'N', 0, 16, ' ', ' ', ' ')

.. code:: python
  for row in ISAMiter(DEFILE, ReadMode.ISRMATCH, filename='defile'):
    print(row)

DEFILE('defile', 150, 'idxflag', 'defidxfla', 'C', 1, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 140, 'vkey', 'defvkey', 'K', 0, 'S', 0, 15, ' ', ' ', ' ' )
DEFILE('defile', 130, 'fgroup', 'defgroup', 'C', 10, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 120, 'scode', 'defscode', 'C', 1, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 110, 'stype', 'defstype', 'I', 0, 'N', 0, 1, ' ', ' ', ' ')
DEFILE('defile', 100, 'vseq', 'defvseq', 'I', 0, 'N', 0, 1, ' ', ' ', ' ')
DEFILE('defile', 90, 'unikey', 'defunikey', 'K', 0, 'S', 0, 15, ' ', ' ', ' ')
DEFILE('defile', 80, 'key', 'defilkey', 'K', 0, 'S', 0, 15, ' ', ' ', ' ')
DEFILE('defile', 70, 'keytype', 'dekeytype', 'C', 1, 'N', 0, 16, ' ' , ' ', ' ')
DEFILE('defile', 60, 'size', 'desize', 'I', 0, 'N', 0, 1, ' ' ,  ' ', ' ')
DEFILE('defile', 50, 'type', 'detype', 'C', 1, 'N', 0, 16, ' ' , ' ' , ' ')
DEFILE('defile', 40, 'refptr', 'derefptr', 'C', 9, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 30, 'field', 'defield', 'C', 9, 'N', 0, 16, ' ', ' ', ' ')
DEFILE('defile', 20, 'seq', 'defilseq', 'I', 0, 'N', 0, 1, ' ', ' ', ' ')
DEFILE('defile', 10, 'filename', 'defilenam', 'C', 9, 'N', 0, 16, ' ', ' ', ' ')

Calling sequence
----------------
To make use of the ISAM iterator object, you need to create an instance of
ISAMiter passing the underlying table object, the mode to restrict rows for the
iterator, and the field and values to be used for the restriction.

.. code:: python
  it = ISAMiter(tabobj, 

Iterator modes
--------------
ISFIRST
~~~~~~~
Return the first row in the table, ignoring any keyfield values, but setting the
current index. Subsequent calls will return the next row in turn.

ISLAST
~~~~~~
Return the last row in the table, ignoring any keyfield values, but setting the
current index. Subsequent calls will return the previous row in turn.

ISNEXT
~~~~~~
If a saved row is passed, saves this as the current keyfield values, using the
current index, then returns the next row. Subsequent calls will return the next
row in turn. If no previous initial mode was specified, then behaves as if the
initial mode was ISFIRST.

ISPREV
~~~~~~
If a saved row is passed, saves this as the current keyfield values, using the
current index, then returns the previous row. Subsequent calls will return the
previous row in turn. If no previous initial mode was specified, then behaves as
if the initial mode was ISLAST.

ISCURR
~~~~~~
If a saved row is passed, saves this as the current keyfield values, using the
current index, then returns the current contents of the row by issuing a ISEQUAL
.isread() call. If no current index, then the primary index is used.

ISEQUAL
~~~~~~~
Use any provided keyfield values, or default them to the type of field, and then
perform 

Iterator  | Initial | Record |
  Mode    |  Mode   |  Mode  | Notes
----------+---------+--------+------
 ISFIRST  | ISFIRST | ISNEXT |
 ISLAST   | ISLAST  | ISPREV |
 ISNEXT   | ------- | ISNEXT |
 ISPREV   | ------- | ISPREV |
 ISCURR   | ------- | ISCURR |
 ISEQUAL  | ISEQUAL | ISNEXT |
 ISGREAT  | ISGREAT | ISNEXT |
 ISGTEQ   | ISGTEQ  | ISNEXT |
 ISSMALL  | ------- | ISPREV |   1
 ISLTEQ   | ------- | ISPREV |   2
 ISFGTEQ  | ISGTEQ  | ------ |   3
 ISFLTEQ  | ISLTEQ  | ------ |   3
 ISMATCH  | ISGTEQ  | ISNEXT |   4
 ISRMATCH | ISLTEQ  | ISPREV |   5
 ISAGAIN  | ISEQUAL | ISCURR |   6

1 - This is not directly provided by the underlying ISAM library, this is
    implemented in the iterator by ensuring that the keyfield values are adjusted
    then the first row is fetched using ISGREAT, if this row does not match the
    keyfield value then no row is returned, otherwise an implicit ISPREV is used
    to retrieve the row immediately before the first row.
2 - This is not directly provided by the underlying ISAM library, this is
    implemented in the iterator by ensuring that the keyfield values are adjusted
    then the first row is fetched using ISGREAT
