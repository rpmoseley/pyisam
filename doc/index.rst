Overview
========
The pyisam.table.index module provides the means to access indexes which are defined purely by their name and composite column(s)
and which when required will produce the required underlying ISAM keydesc structure given the particular record object which
defines the column information such as offset, size and type required. The table object is not stored within the index thus
allowing a single index object to be shared with different table objects.
