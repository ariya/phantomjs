#
#  W A R N I N G
#  -------------
#
# This file is not part of the Qt API.  It exists purely as an
# implementation detail.  This file, and its contents may change from version to
# version without notice, or even be removed.
#
# We mean it.

message("CMAKE_VERSION: ${CMAKE_VERSION}")
message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message("CMAKE_MODULES_UNDER_TEST: ${CMAKE_MODULES_UNDER_TEST}")
foreach(_mod ${CMAKE_MODULES_UNDER_TEST})
    message("CMAKE_${_mod}_MODULE_MAJOR_VERSION: ${CMAKE_${_mod}_MODULE_MAJOR_VERSION}")
    message("CMAKE_${_mod}_MODULE_MINOR_VERSION: ${CMAKE_${_mod}_MODULE_MINOR_VERSION}")
    message("CMAKE_${_mod}_MODULE_PATCH_VERSION: ${CMAKE_${_mod}_MODULE_PATCH_VERSION}")
endforeach()

set(BUILD_OPTIONS_LIST)

if (CMAKE_C_COMPILER)
  list(APPEND BUILD_OPTIONS_LIST "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
endif()

if (CMAKE_CXX_COMPILER)
  list(APPEND BUILD_OPTIONS_LIST "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
endif()

if (CMAKE_BUILD_TYPE)
  list(APPEND BUILD_OPTIONS_LIST "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()

if (CMAKE_TOOLCHAIN_FILE)
  list(APPEND BUILD_OPTIONS_LIST "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

if (CMAKE_VERBOSE_MAKEFILE)
  list(APPEND BUILD_OPTIONS_LIST "-DCMAKE_VERBOSE_MAKEFILE=1")
endif()

if (NO_WIDGETS)
  list(APPEND BUILD_OPTIONS_LIST "-DNO_WIDGETS=True")
endif()
if (NO_DBUS)
  list(APPEND BUILD_OPTIONS_LIST "-DNO_DBUS=True")
endif()

foreach(module ${CMAKE_MODULES_UNDER_TEST})
    list(APPEND BUILD_OPTIONS_LIST
        "-DCMAKE_${module}_MODULE_MAJOR_VERSION=${CMAKE_${module}_MODULE_MAJOR_VERSION}"
        "-DCMAKE_${module}_MODULE_MINOR_VERSION=${CMAKE_${module}_MODULE_MINOR_VERSION}"
        "-DCMAKE_${module}_MODULE_PATCH_VERSION=${CMAKE_${module}_MODULE_PATCH_VERSION}"
    )
endforeach()

macro(expect_pass _dir)
  string(REPLACE "(" "_" testname "${_dir}")
  string(REPLACE ")" "_" testname "${testname}")
  add_test(${testname} ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_dir}"
    --build-config "${CMAKE_BUILD_TYPE}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-project ${_dir}
    --build-options "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}" ${BUILD_OPTIONS_LIST}
  )
endmacro()

macro(expect_fail _dir)
  string(REPLACE "(" "_" testname "${_dir}")
  string(REPLACE ")" "_" testname "${testname}")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}")
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}")

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}/${_dir}/FindPackageHints.cmake" "set(Qt5Tests_PREFIX_PATH \"${CMAKE_PREFIX_PATH}\")")

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}/CMakeLists.txt"
    "
      cmake_minimum_required(VERSION 2.8)
      project(${_dir})

      try_compile(Result \${CMAKE_CURRENT_BINARY_DIR}/${_dir}
          \${CMAKE_CURRENT_SOURCE_DIR}/${_dir}
          ${_dir}
          OUTPUT_VARIABLE Out
      )
      message(\"\${Out}\")
      if (Result)
        message(SEND_ERROR \"Succeeded build which should fail\")
      endif()
      "
  )
  add_test(${testname} ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}"
    "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}/build"
    --build-config "${CMAKE_BUILD_TYPE}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-project ${_dir}
    --build-options "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}" ${BUILD_OPTIONS_LIST}
  )
endmacro()

function(test_module_includes)

  set(all_args ${ARGN})
  set(packages_string "")
  set(libraries_string "")

  foreach(_package ${Qt5_MODULE_TEST_DEPENDS})
    set(packages_string
      "
      ${packages_string}
      find_package(Qt5${_package} 5.0.0 REQUIRED)
      "
    )
  endforeach()

  while(all_args)
    list(GET all_args 0 qtmodule)
    list(REMOVE_AT all_args 0 1)

    set(CMAKE_MODULE_VERSION ${CMAKE_${qtmodule}_MODULE_MAJOR_VERSION}.${CMAKE_${qtmodule}_MODULE_MINOR_VERSION}.${CMAKE_${qtmodule}_MODULE_PATCH_VERSION} )

    set(packages_string
      "${packages_string}
      find_package(Qt5${qtmodule} 5.0.0 REQUIRED)
      include_directories(\${Qt5${qtmodule}_INCLUDE_DIRS})
      add_definitions(\${Qt5${qtmodule}_DEFINITIONS})\n")

    list(FIND CMAKE_MODULES_UNDER_TEST ${qtmodule} _findIndex)
    if (NOT _findIndex STREQUAL -1)
        set(packages_string
          "${packages_string}
          if(NOT \"\${Qt5${qtmodule}_VERSION}\" VERSION_EQUAL ${CMAKE_MODULE_VERSION})
            message(SEND_ERROR \"Qt5${qtmodule}_VERSION variable was not ${CMAKE_MODULE_VERSION}. Got \${Qt5${qtmodule}_VERSION} instead.\")
          endif()
          if(NOT \"\${Qt5${qtmodule}_VERSION_MAJOR}\" VERSION_EQUAL ${CMAKE_${qtmodule}_MODULE_MAJOR_VERSION})
            message(SEND_ERROR \"Qt5${qtmodule}_VERSION_MAJOR variable was not ${CMAKE_${qtmodule}_MODULE_MAJOR_VERSION}. Got \${Qt5${qtmodule}_VERSION_MAJOR} instead.\")
          endif()
          if(NOT \"\${Qt5${qtmodule}_VERSION_MINOR}\" VERSION_EQUAL ${CMAKE_${qtmodule}_MODULE_MINOR_VERSION})
            message(SEND_ERROR \"Qt5${qtmodule}_VERSION_MINOR variable was not ${CMAKE_${qtmodule}_MODULE_MINOR_VERSION}. Got \${Qt5${qtmodule}_VERSION_MINOR} instead.\")
          endif()
          if(NOT \"\${Qt5${qtmodule}_VERSION_PATCH}\" VERSION_EQUAL ${CMAKE_${qtmodule}_MODULE_PATCH_VERSION})
            message(SEND_ERROR \"Qt5${qtmodule}_VERSION_PATCH variable was not ${CMAKE_${qtmodule}_MODULE_PATCH_VERSION}. Got \${Qt5${qtmodule}_VERSION_PATCH} instead.\")
          endif()
          if(NOT \"\${Qt5${qtmodule}_VERSION_STRING}\" VERSION_EQUAL ${CMAKE_MODULE_VERSION})
            message(SEND_ERROR \"Qt5${qtmodule}_VERSION_STRING variable was not ${CMAKE_MODULE_VERSION}. Got \${Qt5${qtmodule}_VERSION_STRING} instead.\")
          endif()\n"
        )
    endif()
    set(libraries_string "${libraries_string} Qt5::${qtmodule}")
  endwhile()

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/module_includes/CMakeLists.txt"
    "
      cmake_minimum_required(VERSION 2.8)
      project(module_includes)

      ${packages_string}

      set(CMAKE_CXX_FLAGS \"\${CMAKE_CXX_FLAGS} \${Qt5Core_EXECUTABLE_COMPILE_FLAGS}\")

      add_executable(module_includes_exe \"\${CMAKE_CURRENT_SOURCE_DIR}/main.cpp\")
      target_link_libraries(module_includes_exe ${libraries_string})\n"
  )

  set(all_args ${ARGN})
  set(includes_string "")
  set(instances_string "")
  while(all_args)
    list(GET all_args 0 qtmodule)
    list(GET all_args 1 qtclass)
    if (${qtclass}_NAMESPACE)
      set(qtinstancetype ${${qtclass}_NAMESPACE}::${qtclass})
    else()
      set(qtinstancetype ${qtclass})
    endif()
    list(REMOVE_AT all_args 0 1)
    set(includes_string
      "${includes_string}
      #include <${qtclass}>
      #include <Qt${qtmodule}/${qtclass}>
      #include <Qt${qtmodule}>
      #include <Qt${qtmodule}/Qt${qtmodule}>"
    )
    set(instances_string
    "${instances_string}
    ${qtinstancetype} local${qtclass};
    ")
  endwhile()

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/module_includes/main.cpp"
    "

    ${includes_string}

    int main(int, char **) { ${instances_string} return 0; }\n"
  )

  add_test(module_includes ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_BINARY_DIR}/module_includes/"
    "${CMAKE_CURRENT_BINARY_DIR}/module_includes/build"
    --build-config "${CMAKE_BUILD_TYPE}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-project module_includes
    --build-options "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}" ${BUILD_OPTIONS_LIST}
  )
endfunction()
