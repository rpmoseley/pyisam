ISAMindexMap structure
======================

The ISAMindexMap object provides the information for a particular index within the pyisam
package, the object encapsulates the ISAM index number, the ISAM key description and also
the ISAMindex definition which are then accessed using an OrderedDict like an ordinary
dictionary but which maintains the ISAM index number ordering.

Each instance of the ISAMindexMap object contains the following attributes:

  The tabobj attribute stores the table on which the remaining information is based, this
  also permits access to the table's definition provided by the ISAMtableDefn object.

  The number attribute stores the index number for the index which was used when calling
  the underlying ISAM function iskeyinfo(), if the index is not present in the underlying
  ISAM table then the attribute is set to None.

  The kdesc attribute stores the key description as calculated by the pyisam package, this
  is used to match the indexes defined in the table to those actually present on the ISAM
  table, this attribute will be set regardless of whether the index exists on the underlying
  ISAM table.

  The name attribute stores the name of the index as defined in pyisam package, this 
  permits the ISAM index to be referenced by number or name.

  The idx_defn attribute stores the definition of the index as defined by the ISAMindex object.

  The isam_desc attribute stores the key description of the index as read from the underlying
  ISAM table, if no index actually exists then None is stored instead.
