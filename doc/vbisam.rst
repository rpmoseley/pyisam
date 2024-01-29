Notes about the implementation of vbisam which is different from the ifisam library
===================================================================================
When operating using the VBISAM library, the index file, suffixed with '.idx' will
grow in size even though no data changes occur, (ie in read-only mode). This is due
to the lock structure being written onto the end of the index file, this should 
actually be a separate file, possibly suffixed with '.lck' in which all that
information can be appended. It should be noted that the log file as established by
a call to 'islogopen' is a separate file that is used to hold transactional details.

Compatibility concerns
----------------------
Splitting the locking information into a separate file would break existing situations
where the VBISAM library is used, and expect that only two files are related to an
individual table, these being the .dat and .idx files. Optionally, a log file would
exist, if the islogopen()/islogclose() functions are invoked, and as is expected by
the original C-ISAM implementation should be a pre-existing file on the filesystem,
otherwise no logging will actually be performed without error. Unlike the C-ISAM
library, the index file seems to grow whenever the VBISAM library is used, which
is adding information about every access even if no data is being updated.

The use of variable length tables as supported by C-ISAM is broken in VBISAM, although
the table can be opened, the fact that the table is using variable record length is
lost during the opening process. In C-ISAM the number of keys in the dictinfo structure
is negative if the table uses variable length records, and the range of the record size
is a combination of the *isreclen* global variable, which gives the fixed size, and the
dictinfo.recsize field, which gives the maximum length. In VBISAM, both the *isreclen*
and dictinfo.recsize give the fixed length, there is no indication of the maximum length.
