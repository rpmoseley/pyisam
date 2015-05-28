# Title:	Makefile
# Copyright:	(C) 2003 Trevor van Bremen
# Author:	Trevor van Bremen
# Created:	11Dec2003
# Description:
#	This is the main makefile that BUILDS all this stuff (I hope)
# Version:
#	$Id: Makefile,v 1.12 2004/06/16 10:53:55 trev_vb Exp $
# Modification History:
#	$Log: Makefile,v $
#	Revision 1.12  2004/06/16 10:53:55  trev_vb
#	16June2004 TvB With about 150 lines of CHANGELOG entries, I am NOT gonna repeat
#	16June2004 TvB them all HERE!  Go look yaself at the 1.03 CHANGELOG
#	
#	Revision 1.11  2004/06/13 07:52:17  trev_vb
#	TvB 13June2004
#	Implemented sharing of open files.
#	Changed the locking strategy slightly to allow table-level locking granularity
#	(i.e. A process opening the same table more than once can now lock itself!)
#	
#	Revision 1.10  2004/06/13 06:32:33  trev_vb
#	TvB 12June2004 See CHANGELOG 1.03 (Too lazy to enumerate)
#	
#	Revision 1.9  2004/06/11 22:16:16  trev_vb
#	11Jun2004 TvB As always, see the CHANGELOG for details. This is an interim
#	checkin that will not be immediately made into a release.
#	
#	Revision 1.8  2004/06/06 20:52:21  trev_vb
#	06Jun2004 TvB Lots of changes! Performance, stability, bugfixes.  See CHANGELOG
#	
#	Revision 1.7  2004/03/23 21:55:55  trev_vb
#	TvB 23Mar2004 Endian on SPARC (Phase I).  Makefile changes for SPARC.
#	
#	Revision 1.6  2004/03/23 15:13:19  trev_vb
#	TvB 23Mar2004 Changes made to fix bugs highlighted by JvN's test suite.  Many thanks go out to JvN for highlighting my obvious mistakes.
#	
#	Revision 1.5  2004/01/06 14:31:59  trev_vb
#	TvB 06Jan2004 Added in VARLEN processing (In a fairly unstable sorta way)
#	
#	Revision 1.4  2004/01/03 02:28:48  trev_vb
#	TvB 02Jan2004 WAY too many changes to enumerate!
#	TvB 02Jan2003 Transaction processing done (excluding iscluster)
#	
#	Revision 1.3  2003/12/23 03:24:43  trev_vb
#	TvB 22Dec2002 Added in -fPIC to CFLAGS for portability + changed default libdir
#	
#	Revision 1.2  2003/12/22 04:42:50  trev_vb
#	TvB 21Dec2003 Added in a cvs header section
#	
#
SHELL	= /bin/sh
DEPS	= isinternal.h vbisam.h Makefile
LIB	= vbisam
ALB	= /usr/lib/lib$(LIB).a
SLB	= /usr/lib/lib$(LIB).so
# ****************************************************************************
# CFLAGS info
# ===========
# -fPIC		Seems to be needed to build libraries on MANY systems
# -DEBUG:	Includes some (useful?) debugging functions
# -O3 -s:	Optimizes code and strips symbols (Better / Faster code?)
# -g:		Produces code with which gdb can be used
# -pg:		Generate gprof-able code
# -D_FILE_OFFSET_BITS=64:
#		Needed to break the 2GB barrier on many 32-bit CPU systems
# -DISAMMODE=1	Files use 64-bit node and row numbers and thus can't work with
#		any C-ISAM linked programs
# -DISAMMODE=0	Files are (supposedly) 100% C-ISAM compatible
# -DCISAMLOCKS	If this is set, then the improved table granular locks revert
#		to the old process wide locks
# ****************************************************************************
CC	= gcc
# CFLAGS	= -fPIC -Wall -pg -g -D_FILE_OFFSET_BITS=64 -DDEBUG -DISAMMODE=1
# CFLAGS	= -fPIC -Wall -O3 -s -D_FILE_OFFSET_BITS=64 -DDEBUG -DISAMMODE=1
# CFLAGS	= -fPIC -Wall -pg -g -D_FILE_OFFSET_BITS=64 -DDEBUG -DISAMMODE=0
# CFLAGS	= -fPIC -Wall -O3 -s -D_FILE_OFFSET_BITS=64 -DDEBUG -DISAMMODE=0
# CFLAGS	= -fPIC -Wall -pg -g -D_FILE_OFFSET_BITS=64 -DISAMMODE=1
# CFLAGS	= -fPIC -Wall -O3 -s -D_FILE_OFFSET_BITS=64 -DISAMMODE=1
# CFLAGS	= -fPIC -Wall -pg -g -D_FILE_OFFSET_BITS=64 -DISAMMODE=0
CFLAGS	= -fPIC -Wall -O3 -s -D_FILE_OFFSET_BITS=64 -DISAMMODE=0

SRCS	= \
	isDecimal.c \
	isHelper.c \
	isaudit.c \
	isbuild.c \
	isdelete.c \
	isopen.c \
	isread.c \
	isrecover.c \
	isrewrite.c \
	istrans.c \
	iswrite.c \
	vbBlockIO.c \
	vbDataIO.c \
	vbIndexIO.c \
	vbKeysIO.c \
	vbLocking.c \
	vbLowLevel.c \
	vbMemIO.c \
	vbNodeMemIO.c \
	vbVarLenIO.c
OBJS	= ${SRCS:.c=.o}

all:	${ALB} ${SLB} vbCheck vbIndexEdit vbRecover MVTest

.o:
	$(CC) $(CFLAGS) -o $@ $< -l$(LIB)

${ALB}:	${OBJS} Makefile
	${AR} srv $@ $?

${SLB}:	${OBJS} Makefile
	${CC} -shared -o $@ ${OBJS}

# Weird, but the following line caused me problems when I moved from using
# access(2) to using stat(2) instead!
#	${LD} -shared -o $@ ${OBJS}

clean:
	rm -f *.o

vbCheck:	vbCheck.o ${SLB}

vbCheck.o:	vbCheck.c ${DEPS}

vbIndexEdit:	vbIndexEdit.o ${SLB}

vbIndexEdit.o:	vbIndexEdit.c ${DEPS}

vbRecover:	vbRecover.o ${SLB}

vbRecover.o:	vbRecover.c ${DEPS}

MVTest:		MVTest.o ${SLB}

MVTest.o:	MVTest.c ${DEPS}

CvtTo64:	CvtTo64.o $(SLB)

CvtTo64.o:	CvtTo64.c $(DEPS)

IsamTest:	IsamTest.o $(SLB)

IsamTest.o:	IsamTest.c $(DEPS)

isDecimal.o:	isDecimal.c $(DEPS) decimal.h

isHelper.o:	isHelper.c $(DEPS)

isaudit.o:	isaudit.c $(DEPS)

isbuild.o:	isbuild.c $(DEPS)

isdelete.o:	isdelete.c $(DEPS)

isopen.o:	isopen.c $(DEPS)

isread.o:	isread.c $(DEPS)

isrecover.o:	isrecover.c $(DEPS)

isrewrite.o:	isrewrite.c $(DEPS)

istrans.o:	istrans.c $(DEPS)

iswrite.o:	iswrite.c $(DEPS)

vbBlockIO.o:	vbBlockIO.c $(DEPS)

vbDataIO.o:	vbDataIO.c $(DEPS)

vbIndexIO.o:	vbIndexIO.c $(DEPS)

vbKeysIO.o:	vbKeysIO.c $(DEPS)

vbLocking.o:	vbLocking.c $(DEPS)

vbLowLevel.o:	vbLowLevel.c $(DEPS)

vbMemIO.o:	vbMemIO.c $(DEPS)

vbNodeMemIO.o:	vbNodeMemIO.c $(DEPS)

vbVarlenIO.o:	vbVarlenIO.c $(DEPS)

