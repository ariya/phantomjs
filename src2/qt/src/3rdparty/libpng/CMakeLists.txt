cmake_minimum_required(VERSION 2.4.4)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

if(UNIX AND NOT DEFINED CMAKE_BUILD_TYPE)
  if(CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 4)
    # workaround CMake 2.4.x bug
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are:
           None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used)
           Debug
           Release
           RelWithDebInfo
           MinSizeRel.")
  else()
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are:
           None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used)
           Debug
           Release
           RelWithDebInfo
           MinSizeRel.")
  endif()
endif()

project(libpng C)
enable_testing()

# Copyright (C) 2007-2011 Glenn Randers-Pehrson

# This code is released under the libpng license.
# For conditions of distribution and use, see the disclaimer
# and license in png.h

set(PNGLIB_MAJOR 1)
set(PNGLIB_MINOR 5)
set(PNGLIB_RELEASE 4)
set(PNGLIB_NAME libpng${PNGLIB_MAJOR}${PNGLIB_MINOR})
set(PNGLIB_VERSION ${PNGLIB_MAJOR}.${PNGLIB_MINOR}.${PNGLIB_RELEASE})

# needed packages
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})

if(NOT WIN32)
  find_library(M_LIBRARY
    NAMES m
    PATHS /usr/lib /usr/local/lib
  )
  if(NOT M_LIBRARY)
    message(STATUS
      "math library 'libm' not found - floating point support disabled")
  endif()
else()
  # not needed on windows
  set(M_LIBRARY "")
endif()

# COMMAND LINE OPTIONS
if(DEFINED PNG_SHARED)
  option(PNG_SHARED "Build shared lib" ${PNG_SHARED})
else()
  option(PNG_SHARED "Build shared lib" ON)
endif()
if(DEFINED PNG_STATIC)
  option(PNG_STATIC "Build static lib" ${PNG_STATIC})
else()
  option(PNG_STATIC "Build static lib" ON)
endif()

if(MINGW)
  option(PNG_TESTS  "Build pngtest" NO)
else()
  option(PNG_TESTS  "Build pngtest" YES)
endif()

option(PNG_NO_CONSOLE_IO "FIXME" YES)
option(PNG_NO_STDIO      "FIXME" YES)
option(PNG_DEBUG         "Build with debug output" NO)
option(PNGARG            "FIXME" YES)
#TODO:
# PNG_CONSOLE_IO_SUPPORTED

# maybe needs improving, but currently I don't know when we can enable what :)
set(png_asm_tmp "OFF")
if(NOT WIN32)
  find_program(uname_executable NAMES uname PATHS /bin /usr/bin /usr/local/bin)
  if(uname_executable)
    exec_program(${uname_executable}
                 ARGS --machine OUTPUT_VARIABLE uname_output)
    if("uname_output" MATCHES "^.*i[1-9]86.*$")
      set(png_asm_tmp "ON")
    else("uname_output" MATCHES "^.*i[1-9]86.*$")
      set(png_asm_tmp "OFF")
    endif("uname_output" MATCHES "^.*i[1-9]86.*$")
  endif(uname_executable)
else()
  # this env var is normally only set on win64
  set(TEXT "ProgramFiles(x86)")
  if("$ENV{${TEXT}}" STREQUAL "")
    set(png_asm_tmp "ON")
  endif("$ENV{${TEXT}}" STREQUAL "")
endif()

# SET LIBNAME
set(PNG_LIB_NAME png${PNGLIB_MAJOR}${PNGLIB_MINOR})

# to distinguish between debug and release lib
set(CMAKE_DEBUG_POSTFIX "d")

# Use the prebuilt pnglibconf.h file from the scripts folder
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt
               ${CMAKE_CURRENT_BINARY_DIR}/pnglibconf.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# OUR SOURCES
set(libpng_public_hdrs
  png.h
  pngconf.h
  ${CMAKE_CURRENT_BINARY_DIR}/pnglibconf.h
)
set(libpng_sources
  ${libpng_public_hdrs}
  pngdebug.h
  pnginfo.h
  pngpriv.h
  pngstruct.h
  png.c
  pngerror.c
  pngget.c
  pngmem.c
  pngpread.c
  pngread.c
  pngrio.c
  pngrtran.c
  pngrutil.c
  pngset.c
  pngtrans.c
  pngwio.c
  pngwrite.c
  pngwtran.c
  pngwutil.c
)
set(pngtest_sources
  pngtest.c
)
# SOME NEEDED DEFINITIONS

add_definitions(-DPNG_CONFIGURE_LIBPNG)

if(MSVC)
  add_definitions(-DPNG_NO_MODULEDEF -D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

if(PNG_SHARED OR  NOT MSVC)
  #if building msvc static this has NOT to be defined
  add_definitions(-DZLIB_DLL)
endif()


if(PNG_CONSOLE_IO_SUPPORTED)
  add_definitions(-DPNG_CONSOLE_IO_SUPPORTED)
endif()

if(PNG_NO_CONSOLE_IO)
  add_definitions(-DPNG_NO_CONSOLE_IO)
endif()

if(PNG_NO_STDIO)
  add_definitions(-DPNG_NO_STDIO)
endif()

if(PNG_DEBUG)
  add_definitions(-DPNG_DEBUG)
endif()

if(NOT M_LIBRARY AND NOT WIN32)
  add_definitions(-DPNG_NO_FLOATING_POINT_SUPPORTED)
endif()

# NOW BUILD OUR TARGET
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${ZLIB_INCLUDE_DIR})

if(PNG_SHARED)
  add_library(${PNG_LIB_NAME} SHARED ${libpng_sources})
  if(MSVC)
    # msvc does not append 'lib' - do it here to have consistent name
    set_target_properties(${PNG_LIB_NAME} PROPERTIES PREFIX "lib")
    set_target_properties(${PNG_LIB_NAME} PROPERTIES IMPORT_PREFIX "lib")
  endif()
  target_link_libraries(${PNG_LIB_NAME} ${ZLIB_LIBRARY} ${M_LIBRARY})
endif()

if(PNG_STATIC)
# does not work without changing name
  set(PNG_LIB_NAME_STATIC ${PNG_LIB_NAME}_static)
  add_library(${PNG_LIB_NAME_STATIC} STATIC ${libpng_sources})
  if(MSVC)
    # msvc does not append 'lib' - do it here to have consistent name
    set_target_properties(${PNG_LIB_NAME_STATIC} PROPERTIES PREFIX "lib")
  endif()
  target_link_libraries(${PNG_LIB_NAME_STATIC} ${ZLIB_LIBRARY} ${M_LIBRARY})
endif()


if(PNG_SHARED AND WIN32)
  set_target_properties(${PNG_LIB_NAME} PROPERTIES DEFINE_SYMBOL PNG_BUILD_DLL)
endif()

if(PNG_TESTS AND PNG_SHARED)
  # does not work with msvc due to png_lib_ver issue
  add_executable(pngtest ${pngtest_sources})
  target_link_libraries(pngtest ${PNG_LIB_NAME})
  add_test(pngtest ./pngtest ${CMAKE_CURRENT_SOURCE_DIR}/pngtest.png)
endif()

#
# Set a variable with CMake code which:
# Creates a symlink from src to dest (if possible) or alternatively
# copies if different.
macro(_png_generate_symlink_code CODE SRC DEST)
  if(WIN32 AND NOT CYGWIN)
    set(_png_gsc_message "Copying ${SRC} to ${DEST} if needed")
    set(_png_gsc_operation "copy_if_different")
  else()
    set(_png_gsc_message "Symlinking ${SRC} to ${DEST}")
    set(_png_gsc_operation "create_symlink")
  endif()

  set(${CODE} "
    message(STATUS \"${_png_gsc_message}\")
    execute_process(COMMAND \${CMAKE_COMMAND} -E ${_png_gsc_operation}
      \"${SRC}\" \"${DEST}\")
    ")
endmacro()

# CREATE PKGCONFIG FILES
# we use the same files like ./configure, so we have to set its vars
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(LIBS        "-lz -lm")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc @ONLY)
_png_generate_symlink_code(PNG_PC_INSTALL_CODE
  ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc
  ${CMAKE_CURRENT_BINARY_DIR}/libpng.pc)
install(CODE ${PNG_PC_INSTALL_CODE})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng-config.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config @ONLY)
_png_generate_symlink_code(PNG_CONFIG_INSTALL_CODE
  ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
  ${CMAKE_CURRENT_BINARY_DIR}/libpng-config)
install(CODE ${PNG_CONFIG_INSTALL_CODE})

# SET UP LINKS
if(PNG_SHARED)
  set_target_properties(${PNG_LIB_NAME} PROPERTIES
#   VERSION 15.${PNGLIB_RELEASE}.1.5.4
    VERSION 15.${PNGLIB_RELEASE}.0
    SOVERSION 15
    CLEAN_DIRECT_OUTPUT 1)
endif()
if(PNG_STATIC)
  # MSVC doesn't use a different file extension for shared vs. static
  # libs.  We are able to change OUTPUT_NAME to remove the _static
  # for all other platforms.
  if(NOT MSVC)
    set_target_properties(${PNG_LIB_NAME_STATIC} PROPERTIES
      OUTPUT_NAME ${PNG_LIB_NAME}
      CLEAN_DIRECT_OUTPUT 1)
  endif()
endif()

# If CMake > 2.4.x, we set a variable used below to export
# targets to an export file.
# TODO: Use VERSION_GREATER after our cmake_minimum_required >= 2.6.2
if(CMAKE_MAJOR_VERSION GREATER 1 AND CMAKE_MINOR_VERSION GREATER 4)
  set(PNG_EXPORT_RULE EXPORT libpng)
elseif(CMAKE_MAJOR_VERSION GREATER 2) # future proof
  set(PNG_EXPORT_RULE EXPORT libpng)
endif()

# INSTALL
if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
  if(PNG_SHARED)
    install(TARGETS ${PNG_LIB_NAME}
        ${PNG_EXPORT_RULE}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    # Create a symlink for libpng.dll.a => libpng15.dll.a on Cygwin
    if(CYGWIN)
      _png_generate_symlink_code(PNG_SHARED_IMPLIB_INSTALL_CODE
        ${PNGLIB_NAME}${CMAKE_IMPORT_LIBRARY_SUFFIX}
        libpng${CMAKE_IMPORT_LIBRARY_SUFFIX})
      install(CODE ${PNG_SHARED_IMPLIB_INSTALL_CODE})
      install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/libpng${CMAKE_IMPORT_LIBRARY_SUFFIX}
        DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()

    if(NOT WIN32)
      IF(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        _png_generate_symlink_code(PNG_SHARED_INSTALL_CODE
 ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PNGLIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
 ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpng${CMAKE_SHARED_LIBRARY_SUFFIX})
                install(CODE ${PNG_SHARED_INSTALL_CODE})
                install(FILES
 ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpng${CMAKE_SHARED_LIBRARY_SUFFIX}
                DESTINATION ${CMAKE_INSTALL_LIBDIR})
      ELSE(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        _png_generate_symlink_code(PNG_SHARED_INSTALL_CODE
           ${PNGLIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
           libpng${CMAKE_SHARED_LIBRARY_SUFFIX})
        install(CODE ${PNG_SHARED_INSTALL_CODE})
        install(FILES
           ${CMAKE_CURRENT_BINARY_DIR}/libpng${CMAKE_SHARED_LIBRARY_SUFFIX}
                DESTINATION ${CMAKE_INSTALL_LIBDIR})
      ENDIF(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    endif()
  endif()

  if(PNG_STATIC)
    install(TARGETS ${PNG_LIB_NAME_STATIC}
        ${PNG_EXPORT_RULE}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    if(NOT WIN32 OR CYGWIN)
      IF(CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
         _png_generate_symlink_code(PNG_STATIC_INSTALL_CODE
${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${PNGLIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/libpng${CMAKE_STATIC_LIBRARY_SUFFIX})
        install(CODE ${PNG_STATIC_INSTALL_CODE})
        install(FILES
          ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/libpng${CMAKE_STATIC_LIBRARY_SUFFIX}
             DESTINATION ${CMAKE_INSTALL_LIBDIR})
      ELSE(CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
        _png_generate_symlink_code(PNG_STATIC_INSTALL_CODE
           ${PNGLIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
           libpng${CMAKE_STATIC_LIBRARY_SUFFIX})
        install(CODE ${PNG_STATIC_INSTALL_CODE})
        install(FILES
          ${CMAKE_CURRENT_BINARY_DIR}/libpng${CMAKE_STATIC_LIBRARY_SUFFIX}
                     DESTINATION ${CMAKE_INSTALL_LIBDIR})
      ENDIF(CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    endif()
  endif()
endif()

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL )
  install(FILES ${libpng_public_hdrs}   DESTINATION include)
  install(FILES ${libpng_public_hdrs}   DESTINATION include/${PNGLIB_NAME})
endif()
if(NOT SKIP_INSTALL_EXECUTABLES AND NOT SKIP_INSTALL_ALL )
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/libpng-config DESTINATION bin)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
          DESTINATION bin)
endif()
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
  # Install man pages
  if(NOT PNG_MAN_DIR)
    set(PNG_MAN_DIR "share/man")
  endif()
  install(FILES libpng.3 libpngpf.3      DESTINATION ${PNG_MAN_DIR}/man3)
  install(FILES png.5                    DESTINATION ${PNG_MAN_DIR}/man5)
  # Install pkg-config files
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpng.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/libpng-config
          DESTINATION bin)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
          DESTINATION bin)
endif()

# On versions of CMake that support it, create an export file CMake
# users can include() to import our targets
if(PNG_EXPORT_RULE AND NOT SKIP_INSTALL_EXPORT AND NOT SKIP_INSTALL_ALL )
  install(EXPORT libpng DESTINATION lib/libpng FILE lib${PNG_LIB_NAME}.cmake)
endif()

# what's with libpng-$VER%.txt and all the extra files?


# UNINSTALL
# do we need this?


# DIST
# do we need this?

# to create msvc import lib for mingw compiled shared lib
# pexports libpng.dll > libpng.def
# lib /def:libpng.def /machine:x86

