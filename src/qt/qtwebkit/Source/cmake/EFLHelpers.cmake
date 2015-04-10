# - Set of macros and functions that are useful for building the EFL port.
#
# The following functions are currently defined:
# FIND_EFL_LIBRARY(<name> HEADERS <header1> ... HEADER_PREFIXES <prefix1> ... LIBRARY <libname>)
#     Looks for the header files inside the given prefix directories, and for the library
#     passed to the LIBRARY parameter.
#     Two #defines in the form <UPPERCASED_NAME>_VERSION_MAJOR and <UPPERCASED_NAME>_VERSION_MINOR
#     are looked for in all the given headers, and the first occurrence is used to build the library's
#     version number.
#     This function defines the following variables:
#     - <UPPERCASED_NAME>_INCLUDE_DIRS: All the directories required by this library's headers.
#     - <UPPERCASED_NAME>_LIBRARIES:    All the libraries required to link against this library.
#     - <UPPERCASED_NAME>_VERSION:      The library's version in the format "major.minor".
#
# Copyright (C) 2012 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include(CMakeParseArguments)

function(FIND_EFL_LIBRARY _name)
    CMAKE_PARSE_ARGUMENTS(PARAM "" "LIBRARY" "HEADERS;HEADER_PREFIXES" ${ARGN})

    string(TOUPPER ${_name} _name_upper)
    set(_version_found FALSE)

    foreach (_current_header ${PARAM_HEADERS})
        find_path(${_current_header}_INCLUDE_DIR NAMES ${_current_header} PATH_SUFFIXES ${PARAM_HEADER_PREFIXES})
        list(APPEND ${_name}_INCLUDE_DIRS "${${_current_header}_INCLUDE_DIR}")

        if (NOT _version_found)
            set(_header_path "${${_current_header}_INCLUDE_DIR}/${_current_header}")
            if (EXISTS ${_header_path})
                file(READ "${_header_path}" _header_contents)

                string(REGEX MATCH "#define +${_name_upper}_VERSION_MAJOR +([0-9]+)" _dummy "${_header_contents}")
                set(_version_major "${CMAKE_MATCH_1}")
                string(REGEX MATCH "#define +${_name_upper}_VERSION_MINOR +([0-9]+)" _dummy "${_header_contents}")
                set(_version_minor "${CMAKE_MATCH_1}")

                if (_version_major AND _version_minor)
                    set(_version_found TRUE)
                endif ()
            endif ()
        endif ()
    endforeach ()

    find_library(${_name}_LIBRARIES NAMES ${PARAM_LIBRARY})

    set(${_name}_INCLUDE_DIRS ${${_name}_INCLUDE_DIRS} PARENT_SCOPE)
    set(${_name}_LIBRARIES ${${_name}_LIBRARIES} PARENT_SCOPE)
    set(${_name}_VERSION "${_version_major}.${_version_minor}" PARENT_SCOPE)
endfunction()
