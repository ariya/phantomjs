# This is an alternative CMakeLists.txt to the experimental one
# shipped with the original package. It builds only the essential
# for our tests. The WebKit version of gtest needs access to some
# headers from WTF and JSC for configuration.

include_directories(${THIRDPARTY_DIR}/gtest
    ${THIRDPARTY_DIR}/gtest/include
    ${JAVASCRIPTCORE_DIR}
    ${WTF_DIR}
)

set(GTEST_DIR "${THIRDPARTY_DIR}/gtest")

add_library(gtest SHARED
    ${GTEST_DIR}/src/gtest.cc
    ${GTEST_DIR}/src/gtest-death-test.cc
    ${GTEST_DIR}/src/gtest-filepath.cc
    ${GTEST_DIR}/src/gtest_main.cc
    ${GTEST_DIR}/src/gtest-port.cc
    ${GTEST_DIR}/src/gtest-test-part.cc
    ${GTEST_DIR}/src/gtest-typed-test.cc
)

add_definitions(-DGTEST_CREATE_SHARED_LIBRARY=1)

target_link_libraries(gtest
    WTF
)

if (CMAKE_USE_PTHREADS_INIT)
    target_link_libraries(gtest ${CMAKE_THREAD_LIBS_INIT})
    add_definitions(-DGTEST_HAS_PTHREAD=1)
endif ()
