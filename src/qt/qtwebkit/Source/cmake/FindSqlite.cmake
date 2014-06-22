# - Try to find Sqlite
# Once done this will define
#
#  SQLITE_FOUND - system has Sqlite
#  SQLITE_INCLUDE_DIR - the Sqlite include directory
#  SQLITE_LIBRARIES - Link these to use Sqlite
#  SQLITE_DEFINITIONS - Compiler switches required for using Sqlite
#
# Copyright (c) 2008, Gilles Caulier, <caulier.gilles@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)
    # in cache already
    set(Sqlite_FIND_QUIETLY TRUE)
endif (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
if (NOT WIN32)
    find_package(PkgConfig)

    pkg_check_modules(PC_SQLITE sqlite3)

    set(SQLITE_DEFINITIONS ${PC_SQLITE_CFLAGS_OTHER})
endif (NOT WIN32)

find_path(SQLITE_INCLUDE_DIR NAMES sqlite3.h
    PATHS
    ${PC_SQLITE_INCLUDEDIR}
    ${PC_SQLITE_INCLUDE_DIRS}
)

find_library(SQLITE_LIBRARIES NAMES sqlite3
    PATHS
    ${PC_SQLITE_LIBDIR}
    ${PC_SQLITE_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Sqlite DEFAULT_MSG SQLITE_INCLUDE_DIR SQLITE_LIBRARIES)

# show the SQLITE_INCLUDE_DIR and SQLITE_LIBRARIES variables only in the advanced view
mark_as_advanced(SQLITE_INCLUDE_DIR SQLITE_LIBRARIES)
