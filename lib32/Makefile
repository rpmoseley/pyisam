#
# This makefile will create a libpyisam.so linked to run against the IBM/Informix C-ISAM runtime libraries,
# this version will create a 32-bit version using 32-bit libraries
libpyisam.so: libifisam.so libifisamx.so
	$(LD) -shared -soname=$@ -rpath='$${ORIGIN}' -o $@ -L. -lifisam -lifisamx
