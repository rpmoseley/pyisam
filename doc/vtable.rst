Virtual table implementation
============================

Overview
--------
This file describes how the virtual table interface works between the pyisam package and SQLite (via the APSW package).

The tabdefns sub-package definitions are used to generate the necessary classes as outlined in the APSW documentation for virtual tables,
in order for the xIndex method to return more reasonable results.
