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
To avoid the situation where the index file seems to grow whilst using the library,
an application should ensure that it calls the vb_get_rtd() function to correctly
initialise the library. Otherwise, the locking file handle is 0, which then matches
the first actual file opened with the library.

The use of variable length tables as supported by C-ISAM is broken in VBISAM, although
the table can be opened, the fact that the table is using variable record length is
lost during the opening process. In C-ISAM the number of keys in the dictinfo structure
is negative if the table uses variable length records, and the range of the record size
is a combination of the *isreclen* global variable, which gives the fixed size, and the
dictinfo.recsize field, which gives the maximum length. In VBISAM, both the *isreclen*
and dictinfo.recsize give the fixed length, there is no indication of the maximum length.

Unlike the libifisam library the vbisam library *DOES NOT* update the global variable
isreclen when a table is opened, it is only updated when the application makes a call
to the isindexinfo() function. This has been fixed in the version of the library that
is distributed with the pyisam package when the macro ISOPEN_SET_ISRECLEN is set during
compilation.

The default source code for the vbisam library *DOES NOT* handle the storage of a NUL
float or double correctly, under libifisam, this is represented by a sequence of 0xFF
bytes to the length of either a float or double depending on the type of value. This
means that the use of the stfltnull/stdblnull functions always returns that a value is
not NUL even if the actual value is the aforementioned sequence (this leads a -Nan
being given if the value is later output using the printf family of functions).
