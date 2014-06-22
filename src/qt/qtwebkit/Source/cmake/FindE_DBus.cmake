# - Try to find E_DBus
# Once done, this will define
#
#  E_DBUS_FOUND - system has E_DBus installed.
#  E_DBUS_INCLUDE_DIRS - directories which contain the E_DBus headers.
#  E_DBUS_LIBRARIES - libraries required to link against E_DBus.
#
# Optionally, the COMPONENTS keyword can be passed to find_package()
# and additional E_DBus libraries can be looked for. Currently, the
# following libraries can be searched, and they define the following
# variables if found:
#
#  EUKIT - E_DBUS_EUKIT_INCLUDE_DIRS and E_DBUS_EUKIT_LIBRARIES
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

include(EFLHelpers)

FIND_EFL_LIBRARY(E_DBUS
    HEADERS E_DBus.h
    HEADER_PREFIXES e_dbus-1
    LIBRARY edbus
)

# Components.
FIND_EFL_LIBRARY(E_DBUS_EUKIT
    HEADERS E_Ukit.h
    HEADER_PREFIXES e_dbus-1
    LIBRARY eukit
)

foreach (_component ${E_DBus_FIND_COMPONENTS})
    set(_e_dbus_component "E_DBUS_${_component}")
    string(TOUPPER ${_e_dbus_component} _UPPER_NAME)

    list(APPEND _E_DBUS_REQUIRED_COMPONENT_VARS ${_UPPER_NAME}_INCLUDE_DIRS ${_UPPER_NAME}_LIBRARIES)
endforeach ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(E_DBus REQUIRED_VARS E_DBUS_INCLUDE_DIRS E_DBUS_LIBRARIES ${_E_DBUS_REQUIRED_COMPONENT_VARS}
                                         VERSION_VAR   E_DBUS_VERSION)
