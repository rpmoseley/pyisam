/* config.h.  Win32/64 VS2005/2008/2010 specific.  */


/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if you have `alloca', as a function or macro. */
/* #define HAVE_ALLOCA 1 */

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
/* #define HAVE_ALLOCA_H 1 */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #define HAVE_DLFCN_H 1 */

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the `fcntl' function. */
/* #define HAVE_FCNTL 1 */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
/* #define HAVE_INTTYPES_H 1 */

/* Define if you have the 'long long' type. */
/* #define HAVE_LONG_LONG 1 */

/* Define to 1 if you have the <malloc.h> header file. */
/* #define HAVE_MALLOC_H 1 */

/* Define to 1 if you have the `memmove' function. */
/* #define HAVE_MEMMOVE 1 */

/* Define to 1 if you have the <memory.h> header file. */
/* #define HAVE_MEMORY_H 1 */

/* Define to 1 if you have the `memset' function. */
/* #define HAVE_MEMSET 1 */

/* Define to 1 if you have the <stddef.h> header file. */
/* #define HAVE_STDDEF_H 1 */

/* Define to 1 if you have the <stdint.h> header file. */
/* #define HAVE_STDINT_H 1 */

/* Define to 1 if you have the <stdlib.h> header file. */
/* #define HAVE_STDLIB_H 1 */

/* Define to 1 if you have the `strcasecmp' function. */
/* #define HAVE_STRCASECMP 1 */

/* Define to 1 if you have the `strchr' function. */
/* #define HAVE_STRCHR 1 */

/* Define to 1 if you have the `strdup' function. */
/* #define HAVE_STRDUP 1 */

/* Define to 1 if you have the <strings.h> header file. */
/* #define HAVE_STRINGS_H 1 */

/* Define to 1 if you have the <string.h> header file. */
/* #define HAVE_STRING_H 1 */

/* Define to 1 if you have the `strrchr' function. */
/* #define HAVE_STRRCHR 1 */

/* Define to 1 if you have the `strstr' function. */
/* #define HAVE_STRSTR 1 */

/* Define to 1 if you have the `strtol' function. */
/* #define HAVE_STRTOL 1 */

/* Define to 1 if you have the <sys/stat.h> header file. */
/* #define HAVE_SYS_STAT_H 1 */

/* Define to 1 if you have the <sys/types.h> header file. */
/* #define HAVE_SYS_TYPES_H 1 */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if you have the `vprintf' function. */
/* #define HAVE_VPRINTF 1 */

/* Define to 1 if you have the <wchar.h> header file. */
/* #define HAVE_WCHAR_H 1 */

/* ISAM mode 0=C-ISAM compatible, 1=Extended */
#define ISAMMODE 1

/* Name of package */
#define PACKAGE "vbisam"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "vbisam-list@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "VBISAM"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "VBISAM 2.0.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "vbisam"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.0.1"

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
    STACK_DIRECTION > 0 => grows toward higher addresses
    STACK_DIRECTION < 0 => grows toward lower addresses
    STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
/* #define STDC_HEADERS 1 */

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Turn on debugging mode */
/* #define VBDEBUG 1 */

/* Version number of package */
#define VERSION "2.0.1"

/*
NOTE about 'WITH_LFS64' with Visual studio 2010:

If not defined, gives the following warnings:

..\..\libvbisam\vblowlevel.c(239): warning C4293: '>>' : shift count negative or too big, undefined behavior
..\..\libvbisam\vblowlevel.c(243): warning C4293: '>>' : shift count negative or too big, undefined behavior
..\..\libvbisam\vblowlevel.c(248): warning C4293: '>>' : shift count negative or too big, undefined behavior

So it is expected the left operand (i >> 32) to be allways positive, otherwise 'not sure!!!'...

Microsoft says:

The value of E1 >> E2 is E1 right-shifted E2 bit positions.
If E1 has an unsigned type or if E1 has a signed type and a non-negative value,
the value of the result is the integral part of the quotient of E1/2^E2.
If E1 has a signed type and a negative value, the resulting value is implementation-defined.

*/

/* Compile with large file system I/O */
#define WITH_LFS64 1

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to 1 if on HPUX.  */
/* #undef _XOPEN_SOURCE_EXTENDED */

/* Compiler optimization */
/* #define __USE_STRING_INLINES 1 */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

/*
* NEW
*
*/

/* see vbisam.h */
#define VBISAM_USE_LONG_LONG 1
