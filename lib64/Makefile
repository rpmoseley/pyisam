#
# This makefile will create a libpyisam.so linked to run against the IBM/Informix C-ISAM runtime libraries,
# this version will create a 64-bit version using 64-bit libraries
libpyisam.so: libifisam.so libifisamx.so
	$(LD) -melf_x86_64 -shared -soname=$@ -rpath='$$ORIGIN/../lib' -o $@ -L. -lifisam -lifisamx
