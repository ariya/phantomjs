include(CMakeParseArguments)
# Sets extra compile flags for a target, depending on the compiler being used.
# Currently, only GCC is supported.
macro(WEBKIT_SET_EXTRA_COMPILER_FLAGS _target)
    set(options ENABLE_WERROR IGNORECXX_WARNINGS)
    CMAKE_PARSE_ARGUMENTS("OPTION" "${options}" "" "" ${ARGN})
    if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        get_target_property(OLD_COMPILE_FLAGS ${_target} COMPILE_FLAGS)
        if (${OLD_COMPILE_FLAGS} STREQUAL "OLD_COMPILE_FLAGS-NOTFOUND")
            set(OLD_COMPILE_FLAGS "")
        endif ()

        include(TestCXXAcceptsFlag)
        CHECK_CXX_ACCEPTS_FLAG("-dumpversion" CMAKE_CXX_ACCEPTS_DUMPVERSION)
        if (CMAKE_CXX_ACCEPTS_DUMPVERSION)
            EXEC_PROGRAM(${CMAKE_CXX_COMPILER} ARGS -dumpversion OUTPUT_VARIABLE COMPILER_VERSION)
        else ()
            EXEC_PROGRAM("${CMAKE_CXX_COMPILER} -E -Wp,-dM - < /dev/null | grep '#define __VERSION__' | grep -E -o '[0-9]+\\.[0-9]+\\.?[0-9]+?'" OUTPUT_VARIABLE COMPILER_VERSION)
        endif ()

        # Disable some optimizations on buggy compiler versions
        # GCC 4.5.1 does not implement -ftree-sra correctly
        if (${COMPILER_VERSION} STREQUAL "4.5.1")
            set(OLD_COMPILE_FLAGS "-fno-tree-sra ${OLD_COMPILE_FLAGS}")
        endif ()

        if (NOT SHARED_CORE)
            set(OLD_COMPILE_FLAGS "-fvisibility=hidden ${OLD_COMPILE_FLAGS}")
        endif ()

        get_target_property(TARGET_TYPE ${_target} TYPE)
        if (${TARGET_TYPE} STREQUAL "STATIC_LIBRARY") # -fPIC is automatically added to shared libraries
            set(OLD_COMPILE_FLAGS "-fPIC ${OLD_COMPILE_FLAGS}")
        endif ()

        set(OLD_COMPILE_FLAGS "-fno-exceptions -fno-strict-aliasing ${OLD_COMPILE_FLAGS}")

        # Enable warnings by default
        if (NOT ${OPTION_IGNORECXX_WARNINGS})
            set(OLD_COMPILE_FLAGS "-Wall -Wextra -Wcast-align -Wformat-security -Wmissing-format-attribute -Wpointer-arith -Wundef -Wwrite-strings ${OLD_COMPILE_FLAGS}")
        endif ()

        # Enable errors on warning
        if (OPTION_ENABLE_WERROR)
            set(OLD_COMPILE_FLAGS "-Werror -Wno-error=unused-parameter ${OLD_COMPILE_FLAGS}")
        endif ()

        # Disable C++0x compat warnings for GCC >= 4.6.0 until we build
        # cleanly with that.
        if (NOT ${OPTION_IGNORECXX_WARNINGS} AND NOT ${COMPILER_VERSION} VERSION_LESS "4.6.0")
            set(OLD_COMPILE_FLAGS "${OLD_COMPILE_FLAGS} -Wno-c++0x-compat")
        endif ()

        if ("${_target}" MATCHES "WebKit2")
            set(OLD_COMPILE_FLAGS "${OLD_COMPILE_FLAGS} -std=c++0x")
        endif ()

        set_target_properties(${_target} PROPERTIES
            COMPILE_FLAGS "${OLD_COMPILE_FLAGS}")

        unset(OLD_COMPILE_FLAGS)
    endif ()
endmacro()


# Append the given flag to the target property.
# Builds on top of get_target_property() and set_target_properties()
macro(ADD_TARGET_PROPERTIES _target _property _flags)
    get_target_property(_tmp ${_target} ${_property})
    if (NOT _tmp)
        set(_tmp "")
    endif (NOT _tmp)

    foreach (f ${_flags})
        set(_tmp "${_tmp} ${f}")
    endforeach (f ${_flags})

    set_target_properties(${_target} PROPERTIES ${_property} ${_tmp})
    unset(_tmp)
endmacro(ADD_TARGET_PROPERTIES _target _property _flags)


# Append the given dependencies to the source file
macro(ADD_SOURCE_DEPENDENCIES _source _deps)
    get_source_file_property(_tmp ${_source} OBJECT_DEPENDS)
    if (NOT _tmp)
        set(_tmp "")
    endif ()

    foreach (f ${_deps})
        list(APPEND _tmp "${f}")
    endforeach ()

    set_source_files_properties(${_source} PROPERTIES OBJECT_DEPENDS "${_tmp}")
    unset(_tmp)
endmacro()


# Append the given dependencies to the source file
# This one consider the given dependencies are in ${DERIVED_SOURCES_WEBCORE_DIR}
# and prepends this to every member of dependencies list
macro(ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES _source _deps)
    set(_tmp "")
    foreach (f ${_deps})
        list(APPEND _tmp "${DERIVED_SOURCES_WEBCORE_DIR}/${f}")
    endforeach ()

    ADD_SOURCE_DEPENDENCIES(${_source} ${_tmp})
    unset(_tmp)
endmacro()
