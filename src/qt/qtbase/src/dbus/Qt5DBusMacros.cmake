#=============================================================================
# Copyright 2005-2011 Kitware, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

include(MacroAddFileDependencies)

include(CMakeParseArguments)

function(QT5_ADD_DBUS_INTERFACE _sources _interface _basename)
    get_filename_component(_infile ${_interface} ABSOLUTE)
    set(_header "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h")
    set(_impl   "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp")
    set(_moc    "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.moc")

    get_source_file_property(_nonamespace ${_interface} NO_NAMESPACE)
    if(_nonamespace)
        set(_params -N -m)
    else()
        set(_params -m)
    endif()

    get_source_file_property(_classname ${_interface} CLASSNAME)
    if(_classname)
        set(_params ${_params} -c ${_classname})
    endif()

    get_source_file_property(_include ${_interface} INCLUDE)
    if(_include)
        set(_params ${_params} -i ${_include})
    endif()

    add_custom_command(OUTPUT "${_impl}" "${_header}"
        COMMAND ${Qt5DBus_QDBUSXML2CPP_EXECUTABLE} ${_params} -p ${_basename} ${_infile}
        DEPENDS ${_infile} VERBATIM)

    set_source_files_properties("${_impl}" PROPERTIES SKIP_AUTOMOC TRUE)

    qt5_generate_moc("${_header}" "${_moc}")

    list(APPEND ${_sources} "${_impl}" "${_header}" "${_moc}")
    macro_add_file_dependencies("${_impl}" "${_moc}")
    set(${_sources} ${${_sources}} PARENT_SCOPE)
endfunction()


function(QT5_ADD_DBUS_INTERFACES _sources)
    foreach(_current_FILE ${ARGN})
        get_filename_component(_infile ${_current_FILE} ABSOLUTE)
        get_filename_component(_basename ${_current_FILE} NAME)
        # get the part before the ".xml" suffix
        string(TOLOWER ${_basename} _basename)
        string(REGEX REPLACE "(.*\\.)?([^\\.]+)\\.xml" "\\2" _basename ${_basename})
        qt5_add_dbus_interface(${_sources} ${_infile} ${_basename}interface)
    endforeach()
    set(${_sources} ${${_sources}} PARENT_SCOPE)
endfunction()


function(QT5_GENERATE_DBUS_INTERFACE _header) # _customName OPTIONS -some -options )
    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_DBUS_INTERFACE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_customName ${_DBUS_INTERFACE_UNPARSED_ARGUMENTS})
    set(_qt4_dbus_options ${_DBUS_INTERFACE_OPTIONS})

    get_filename_component(_in_file ${_header} ABSOLUTE)
    get_filename_component(_basename ${_header} NAME_WE)

    if(_customName)
        if(IS_ABSOLUTE ${_customName})
          get_filename_component(_containingDir ${_customName} PATH)
          if(NOT EXISTS ${_containingDir})
              file(MAKE_DIRECTORY "${_containingDir}")
          endif()
          set(_target ${_customName})
        else()
            set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_customName})
        endif()
    else()
        set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.xml)
    endif()

    add_custom_command(OUTPUT ${_target}
        COMMAND ${Qt5DBus_QDBUSCPP2XML_EXECUTABLE} ${_qt4_dbus_options} ${_in_file} -o ${_target}
        DEPENDS ${_in_file} VERBATIM
    )
endfunction()


function(QT5_ADD_DBUS_ADAPTOR _sources _xml_file _include _parentClass) # _optionalBasename _optionalClassName)
    get_filename_component(_infile ${_xml_file} ABSOLUTE)

    set(_optionalBasename "${ARGV4}")
    if(_optionalBasename)
        set(_basename ${_optionalBasename} )
    else()
        string(REGEX REPLACE "(.*[/\\.])?([^\\.]+)\\.xml" "\\2adaptor" _basename ${_infile})
        string(TOLOWER ${_basename} _basename)
    endif()

    set(_optionalClassName "${ARGV5}")
    set(_header "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h")
    set(_impl   "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp")
    set(_moc    "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.moc")

    if(_optionalClassName)
        add_custom_command(OUTPUT "${_impl}" "${_header}"
          COMMAND ${Qt5DBus_QDBUSXML2CPP_EXECUTABLE} -m -a ${_basename} -c ${_optionalClassName} -i ${_include} -l ${_parentClass} ${_infile}
          DEPENDS ${_infile} VERBATIM
        )
    else()
        add_custom_command(OUTPUT "${_impl}" "${_header}"
          COMMAND ${Qt5DBus_QDBUSXML2CPP_EXECUTABLE} -m -a ${_basename} -i ${_include} -l ${_parentClass} ${_infile}
          DEPENDS ${_infile} VERBATIM
        )
    endif()

    qt5_generate_moc("${_header}" "${_moc}")
    set_source_files_properties("${_impl}" PROPERTIES SKIP_AUTOMOC TRUE)
    macro_add_file_dependencies("${_impl}" "${_moc}")

    list(APPEND ${_sources} "${_impl}" "${_header}" "${_moc}")
    set(${_sources} ${${_sources}} PARENT_SCOPE)
endfunction()
