set(WinCELauncher_INCLUDE_DIRECTORIES
    "${WEBKIT_DIR}/wince"
    "${WEBCORE_DIR}"
    "${WEBCORE_DIR}/platform"
    "${WEBCORE_DIR}/platform/graphics"
    "${WEBCORE_DIR}/platform/network"
    "${WEBCORE_DIR}/platform/text"
    "${JAVASCRIPTCORE_DIR}"
    "${WTF_DIR}"
    "${DERIVED_SOURCES_DIR}"
    "${CMAKE_BINARY_DIR}"
)

set(WinCELauncher_SOURCES
    ${TOOLS_DIR}/WinCELauncher/main.cpp
)

set(WinCELauncher_LIBRARIES
    WebKit
)

include_directories(${WinCELauncher_INCLUDE_DIRECTORIES})
add_executable(WinCELauncher WIN32 ${WinCELauncher_SOURCES})
add_dependencies(WinCELauncher WebKit)
target_link_libraries(WinCELauncher ${WinCELauncher_LIBRARIES})
set_target_properties(WinCELauncher PROPERTIES FOLDER "Tools")
set_target_properties(WinCELauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
