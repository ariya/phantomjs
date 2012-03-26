#!gmake
#
# License Applicability. Except to the extent portions of this file are
# made subject to an alternative license as permitted in the SGI Free
# Software License B, Version 1.1 (the "License"), the contents of this
# file are subject only to the provisions of the License. You may not use
# this file except in compliance with the License. You may obtain a copy
# of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
# Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
#
# http://oss.sgi.com/projects/FreeB
#
# Note that, as provided in the License, the Software is distributed on an
# "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
# DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
# CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
# PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
#
# Original Code. The Original Code is: OpenGL Sample Implementation,
# Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
# Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
# Copyright in any portions created by third parties is as indicated
# elsewhere herein. All Rights Reserved.
#
# Additional Notice Provisions: The application programming interfaces
# established by SGI in conjunction with the Original Code are The
# OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
# April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
# 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
# Window System(R) (Version 1.3), released October 19, 1998. This software
# was created using the OpenGL(R) version 1.2.1 Sample Implementation
# published by SGI, but has not been independently verified as being
# compliant with the OpenGL(R) version 1.2.1 Specification.
#
# $Date$ $Revision$
# $Header: //depot/main/gfx/lib/glu/libtess/GNUmakefile#9 $

OBJECT_STYLE = N32

include $(ROOT)/usr/include/make/commondefs

TARGET = libtess.a

TARGETS = $(TARGET)

LCINCS = -I../include

# Compilation flags:
#
# -DNDEBUG is for the production code; it removes all assertion checks
#    (note that <assert.h> looks at this symbol).
#
# -DNO_MALLOPT uses regular malloc instead of the mallopt() version.
#   ***** Unless you use this flag, you must use "-lmalloc" to link
#   ***** your application!
#
# -DMEMORY_DEBUG turns on the M_DEBUG option of mallopt; this can
#    increase the running time a LOT.
#
# -DGLU_TESS_API_FLOAT compiles a single-precision version of the library.
#
# -float prevents automatic promotion to double precision; this will produce
#    faster code when compiled with -DGLU_TESS_API_FLOAT.
#
# -DNO_BRANCH_CONDITIONS uses & and | instead of && and || on a couple
#    of heavily-used tests (VertEq and VertLeq); some compilers can generate
#    better code with these (use special instructions to avoid branching).
#
# -DFOR_TRITE_TEST_PROGRAM is *only* for use with the test program called
#    "trite".  It uses some variables which are defined by the test program,
#    so you won't be able to link it with anything else.

HFILES = \
	dict.h \
	dict-list.h \
	geom.h \
	memalloc.h \
	mesh.h \
	normal.h \
	priorityq-heap.h \
	priorityq-heap.c \
	priorityq-sort.h \
	priorityq.h \
	render.h \
	sweep.h \
	tess.h \
	tessmono.h \
	$(NULL)

CFILES = \
	dict.c \
	geom.c \
	memalloc.c \
	mesh.c \
	normal.c \
	priorityq.c \
	render.c \
	sweep.c \
	tess.c \
	tessmono.c \
	$(NULL)

default libs libs_install install: $(TARGET)

headers headers_install apps:

$(TARGET): $(OBJECTS)
	$(AR) crl $@ $(OBJECTS);

include $(COMMONRULES)
