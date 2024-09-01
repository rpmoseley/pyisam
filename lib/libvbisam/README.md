Overview
========

This directory contains the source code for the VBISAM which is
an open source replacement for the Informix (IBM) C-ISAM library,
whose original readme is available at the end of the file.

Extra functions added to provide consistency to IFISAM
------------------------------------------------------
The current version of libifisam has split the confusing isindexinfo()
function into two separate functions, isdictinfo() and iskeyinfo(),
removing the void pointer required by isindexinfo(). The original 
function is preserved but now calls the other two internally, if the
given key is 0 then isdictinfo() is called, otherwise iskeyinfo() is
called, as keys are numbered from 1 onwards in IFISAM.

Original README file contents
-----------------------------
			       VBISAM
	       http://sourceforge.net/projects/vbisam

VBISAM is a replacement for IBM's C-ISAM.

This package contains the following subdirectories:

    bin		VBISAM utility programs
    libvbisam   The VBISAM library
    tests       Test programs
    docs	Documentation

All programs are distributed under either the GNU General Public
License or the GNU Lesser General Public License.
See COPYING and COPYING.LIB for details.

See AUTHORS for the author of each file.

Requirements
============

VBISAM only requires a working C development system.

Installation
============

See INSTALL for general installation instruction.  Typically,
this is done by the following commands:

    ./configure
    make
    make install

The default target for installed files is "/usr/local".

Other than the usual configure options (./configure --help)
there are the following specific VBISAM configure options:

  --with-cisamcompat	use VBISAM C-ISAM comatibility mode
  --with-lfs64		use large file system for file I/O (default)
  --with-debug		Enable debugging mode

To squeeze extra performance out of the code, you may want to
do for the install eg :
make install-strip


Development
===========

You need to install the following extra packages with specified
minumum version before hacking VBISAM configure/makefile files:

  o Autoconf 2.59
  o Automake 1.9.6
  o Libtool 1.5.24
  o m4 1.4

Run "autoreconf -ifv -I m4" to regenerate configure/makefile scripts.
You need to run autoreconf whenever you modify configure.ac
or Makefile.am.
