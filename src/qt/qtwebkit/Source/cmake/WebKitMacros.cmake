macro(INCLUDE_IF_EXISTS _file)
    if (EXISTS ${_file})
        message(STATUS "Using platform-specific CMakeLists: ${_file}")
        include(${_file})
    else ()
        message(STATUS "Platform-specific CMakeLists not found: ${_file}")
    endif ()
endmacro()


# Append the given dependencies to the source file
macro(ADD_SOURCE_DEPENDENCIES _source _deps)
    set(_tmp)
    get_source_file_property(_tmp ${_source} OBJECT_DEPENDS)
    if (NOT _tmp)
        set(_tmp "")
    endif ()

    foreach (f ${_deps})
        list(APPEND _tmp "${f}")
    endforeach ()

    set_source_files_properties(${_source} PROPERTIES OBJECT_DEPENDS "${_tmp}")
endmacro()


# Helper macro which wraps generate-bindings.pl script.
#   _output_source is a list name which will contain generated sources.(eg. WebCore_SOURCES)
#   _input_files are IDL files to generate.
#   _base_dir is base directory where script is called.
#   _idl_includes is value of --include argument. (eg. --include=${WEBCORE_DIR}/bindings/js)
#   _features is a value of --defines argument.
#   _destination is a value of --outputDir argument.
#   _prefix is a prefix of output files. (eg. JS - it makes JSXXX.cpp JSXXX.h from XXX.idl)
#   _generator is a value of --generator argument.
#   _supplemental_dependency_file is a value of --supplementalDependencyFile. (optional)
macro(GENERATE_BINDINGS _output_source _input_files _base_dir _idl_includes _features _destination _prefix _generator _idl_attributes_file)
    set(BINDING_GENERATOR ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl)
    set(_args ${ARGN})
    list(LENGTH _args _argCount)
    if (_argCount EQUAL 5)
        list(GET _args 0 _supplemental_dependency_file)
        if (_supplemental_dependency_file)
            set(_supplemental_dependency --supplementalDependencyFile ${_supplemental_dependency_file})
        endif ()
        list(GET _args 1 _window_constructors_file)
        list(GET _args 2 _workerglobalscope_constructors_file)
        list(GET _args 3 _sharedworkerglobalscope_constructors_file)
        list(GET _args 4 _dedicatedworkerglobalscope_constructors_file)
    endif ()

    foreach (_file ${_input_files})
        get_filename_component(_name ${_file} NAME_WE)

        add_custom_command(
            OUTPUT ${_destination}/${_prefix}${_name}.cpp ${_destination}/${_prefix}${_name}.h
            MAIN_DEPENDENCY ${_file}
            DEPENDS ${BINDING_GENERATOR} ${SCRIPTS_BINDINGS} ${_supplemental_dependency_file} ${_idl_attributes_file} ${_window_constructors_file} ${_workerglobalscope_constructors_file} ${_sharedworkerglobalscope_constructors_file} ${_dedicatedworkerglobalscope_constructors_file}
            COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${BINDING_GENERATOR} --defines "${_features}" --generator ${_generator} ${_idl_includes} --outputDir "${_destination}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" --idlAttributesFile ${_idl_attributes_file} ${_supplemental_dependency} ${_file}
            WORKING_DIRECTORY ${_base_dir}
            VERBATIM)

        list(APPEND ${_output_source} ${_destination}/${_prefix}${_name}.cpp)
    endforeach ()
endmacro()


macro(GENERATE_FONT_NAMES _infile)
    set(NAMES_GENERATOR ${WEBCORE_DIR}/dom/make_names.pl)
    set(_arguments  --fonts ${_infile})
    set(_outputfiles ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitFontFamilyNames.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitFontFamilyNames.h)

    add_custom_command(
        OUTPUT  ${_outputfiles}
        MAIN_DEPENDENCY ${_infile}
        DEPENDS ${NAMES_GENERATOR} ${SCRIPTS_BINDINGS}
        COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${NAMES_GENERATOR} --outputDir ${DERIVED_SOURCES_WEBCORE_DIR} ${_arguments}
        VERBATIM)
endmacro()


macro(GENERATE_EVENT_FACTORY _infile _outfile)
    set(NAMES_GENERATOR ${WEBCORE_DIR}/dom/make_event_factory.pl)

    add_custom_command(
        OUTPUT  ${DERIVED_SOURCES_WEBCORE_DIR}/${_outfile}
        MAIN_DEPENDENCY ${_infile}
        DEPENDS ${NAMES_GENERATOR} ${SCRIPTS_BINDINGS}
        COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${NAMES_GENERATOR} --input ${_infile} --outputDir ${DERIVED_SOURCES_WEBCORE_DIR}
        VERBATIM)
endmacro()


macro(GENERATE_EXCEPTION_CODE_DESCRIPTION _infile _outfile)
    set(NAMES_GENERATOR ${WEBCORE_DIR}/dom/make_dom_exceptions.pl)

    add_custom_command(
        OUTPUT  ${DERIVED_SOURCES_WEBCORE_DIR}/${_outfile}
        MAIN_DEPENDENCY ${_infile}
        DEPENDS ${NAMES_GENERATOR} ${SCRIPTS_BINDINGS}
        COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${NAMES_GENERATOR} --input ${_infile} --outputDir ${DERIVED_SOURCES_WEBCORE_DIR}
        VERBATIM)
endmacro()


macro(GENERATE_SETTINGS_MACROS _infile _outfile)
    set(NAMES_GENERATOR ${WEBCORE_DIR}/page/make_settings.pl)

    add_custom_command(
        OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/${_outfile} ${DERIVED_SOURCES_WEBCORE_DIR}/InternalSettingsGenerated.h ${DERIVED_SOURCES_WEBCORE_DIR}/InternalSettingsGenerated.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InternalSettingsGenerated.idl
        MAIN_DEPENDENCY ${_infile}
        DEPENDS ${NAMES_GENERATOR} ${SCRIPTS_BINDINGS}
        COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${NAMES_GENERATOR} --input ${_infile} --outputDir ${DERIVED_SOURCES_WEBCORE_DIR}
        VERBATIM)
endmacro()


macro(GENERATE_DOM_NAMES _namespace _attrs)
    set(NAMES_GENERATOR ${WEBCORE_DIR}/dom/make_names.pl)
    set(_arguments  --attrs ${_attrs})
    set(_outputfiles ${DERIVED_SOURCES_WEBCORE_DIR}/${_namespace}Names.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/${_namespace}Names.h)
    set(_extradef)
    set(_tags)

    foreach (f ${ARGN})
        if (_tags)
            set(_extradef "${_extradef} ${f}")
        else ()
            set(_tags ${f})
        endif ()
    endforeach ()

    if (_tags)
        set(_arguments "${_arguments}" --tags ${_tags} --factory --wrapperFactory)
        set(_outputfiles "${_outputfiles}" ${DERIVED_SOURCES_WEBCORE_DIR}/${_namespace}ElementFactory.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/${_namespace}ElementFactory.h ${DERIVED_SOURCES_WEBCORE_DIR}/JS${_namespace}ElementWrapperFactory.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/JS${_namespace}ElementWrapperFactory.h)
    endif ()

    if (_extradef)
        set(_additionArguments "${_additionArguments}" --extraDefines=${_extradef})
    endif ()

    add_custom_command(
        OUTPUT  ${_outputfiles}
        DEPENDS ${NAMES_GENERATOR} ${SCRIPTS_BINDINGS} ${_attrs} ${_tags}
        COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${NAMES_GENERATOR} --preprocessor "${CODE_GENERATOR_PREPROCESSOR_WITH_LINEMARKERS}" --outputDir ${DERIVED_SOURCES_WEBCORE_DIR} ${_arguments} ${_additionArguments}
        VERBATIM)
endmacro()


macro(GENERATE_GRAMMAR _prefix _input _output_header _output_source _features)
    # This is a workaround for winflexbison, which does not work corretly when
    # run in a different working directory than the installation directory.
    get_filename_component(_working_directory ${BISON_EXECUTABLE} PATH)

    add_custom_command(
        OUTPUT ${_output_header} ${_output_source}
        MAIN_DEPENDENCY ${_input}
        DEPENDS ${_input}
        COMMAND ${PERL_EXECUTABLE} -I ${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/css/makegrammar.pl --outputDir ${DERIVED_SOURCES_WEBCORE_DIR} --extraDefines "${_features}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" --bison "${BISON_EXECUTABLE}" --symbolsPrefix ${_prefix} ${_input}
        WORKING_DIRECTORY ${_working_directory}
        VERBATIM)
endmacro()

macro(MAKE_HASH_TOOLS _source)
    get_filename_component(_name ${_source} NAME_WE)

    if (${_source} STREQUAL "DocTypeStrings")
        set(_hash_tools_h "${DERIVED_SOURCES_WEBCORE_DIR}/HashTools.h")
    else ()
        set(_hash_tools_h "")
    endif ()

    add_custom_command(
        OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/${_name}.cpp ${_hash_tools_h}
        MAIN_DEPENDENCY ${_source}.gperf
        COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/make-hash-tools.pl ${DERIVED_SOURCES_WEBCORE_DIR} ${_source}.gperf
        VERBATIM)

    unset(_name)
    unset(_hash_tools_h)
endmacro()

macro(WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS)
    INCLUDE_IF_EXISTS(${CMAKE_CURRENT_SOURCE_DIR}/Platform${PORT}.cmake)
endmacro()

macro(WEBKIT_WRAP_SOURCELIST)
    foreach (_file ${ARGN})
        get_filename_component(_basename ${_file} NAME_WE)
        get_filename_component(_path ${_file} PATH)

        if (NOT _file MATCHES "${DERIVED_SOURCES_WEBCORE_DIR}")
            string(REGEX REPLACE "/" "\\\\\\\\" _sourcegroup "${_path}")
            source_group("${_sourcegroup}" FILES ${_file})
        endif ()

        if (WTF_PLATFORM_QT)
            set(_moc_filename ${DERIVED_SOURCES_WEBCORE_DIR}/${_basename}.moc)

            file(READ ${_file} _contents)

            string(REGEX MATCHALL "#include[ ]+\"${_basename}\\.moc\"" _match "${_contents}")
            if (_match)
                QT4_GENERATE_MOC(${_file} ${_moc_filename})
                ADD_SOURCE_DEPENDENCIES(${_file} ${_moc_filename})
            endif ()
        endif ()
    endforeach ()

    source_group("DerivedSources" REGULAR_EXPRESSION "${DERIVED_SOURCES_WEBCORE_DIR}")
endmacro()


macro(WEBKIT_CREATE_FORWARDING_HEADER _target_directory _file)
    get_filename_component(_absolute "${_file}" ABSOLUTE)
    get_filename_component(_name "${_file}" NAME)
    set(_content "#include \"${_absolute}\"\n")
    set(_filename "${_target_directory}/${_name}")

    if (EXISTS "${_filename}")
        file(READ "${_filename}" _old_content)
    endif ()

    if (NOT _old_content STREQUAL _content)
        file(WRITE "${_filename}" "${_content}")
    endif ()
endmacro()

macro(WEBKIT_CREATE_FORWARDING_HEADERS _framework)
    set(_processing_directories 0)
    set(_processing_files 0)
    set(_target_directory "${DERIVED_SOURCES_DIR}/ForwardingHeaders/${_framework}")

    file(GLOB _files "${_target_directory}/*.h")
    foreach (_file ${_files})
        file(READ "${_file}" _content)
        string(REGEX MATCH "^#include \"([^\"]*)\"" _matched ${_content})
        if (_matched AND NOT EXISTS "${CMAKE_MATCH_1}")
           file(REMOVE "${_file}")
        endif ()
    endforeach ()

    foreach (_currentArg ${ARGN})
        if ("${_currentArg}" STREQUAL "DIRECTORIES")
            set(_processing_directories 1)
            set(_processing_files 0)
        elseif ("${_currentArg}" STREQUAL "FILES")
            set(_processing_directories 0)
            set(_processing_files 1)
        elseif (_processing_directories)
            file(GLOB _files "${_currentArg}/*.h")
            foreach (_file ${_files})
                WEBKIT_CREATE_FORWARDING_HEADER(${_target_directory} ${_file})
            endforeach ()
        elseif (_processing_files)
            WEBKIT_CREATE_FORWARDING_HEADER(${_target_directory} ${_currentArg})
        endif ()
    endforeach ()
endmacro()
