set(MINIBROWSER_DIR "${TOOLS_DIR}/MiniBrowser/gtk")
set(DERIVED_SOURCES_MINIBROWSER_DIR "${CMAKE_BINARY_DIR}/DerivedSources/MiniBrowser")

file(MAKE_DIRECTORY ${DERIVED_SOURCES_MINIBROWSER_DIR})

set(MiniBrowser_SOURCES
    ${DERIVED_SOURCES_MINIBROWSER_DIR}/BrowserMarshal.c
    ${MINIBROWSER_DIR}/BrowserCellRendererVariant.h
    ${MINIBROWSER_DIR}/BrowserCellRendererVariant.c
    ${MINIBROWSER_DIR}/BrowserDownloadsBar.h
    ${MINIBROWSER_DIR}/BrowserDownloadsBar.c
    ${MINIBROWSER_DIR}/BrowserSettingsDialog.h
    ${MINIBROWSER_DIR}/BrowserSettingsDialog.c
    ${MINIBROWSER_DIR}/BrowserWindow.h
    ${MINIBROWSER_DIR}/BrowserWindow.c
    ${MINIBROWSER_DIR}/main.c
)

set(MiniBrowser_INCLUDE_DIRECTORIES
    ${DERIVED_SOURCES_MINIBROWSER_DIR}
    ${DERIVED_SOURCES_DIR}/webkit2gtk
    ${DERIVED_SOURCES_DIR}/ForwardingHeaders/webkit2gtk
    ${DERIVED_SOURCES_DIR}/ForwardingHeaders
    ${CMAKE_SOURCE_DIR}/Source
    ${GTK3_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
)

set(MiniBrowser_LIBRARIES
    ${JavaScriptCore_LIBRARY_NAME}
    WebKit2
    ${GTK3_LIBRARIES}
    ${GLIB_LIBRARIES}
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_MINIBROWSER_DIR}/BrowserMarshal.c
           ${DERIVED_SOURCES_MINIBROWSER_DIR}/Browsermarshal.h
    MAIN_DEPENDENCY ${MINIBROWSER_DIR}/browser-marshal.list
    COMMAND glib-genmarshal --prefix=browser_marshal ${MINIBROWSER_DIR}/browser-marshal.list --body > ${DERIVED_SOURCES_MINIBROWSER_DIR}/BrowserMarshal.c
    COMMAND glib-genmarshal --prefix=browser_marshal ${MINIBROWSER_DIR}/browser-marshal.list --header > ${DERIVED_SOURCES_MINIBROWSER_DIR}/BrowserMarshal.h
    VERBATIM)

add_definitions(-DWEBKIT_EXEC_PATH="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
add_definitions(-DWEBKIT_INJECTED_BUNDLE_PATH="${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

include_directories(${MiniBrowser_INCLUDE_DIRECTORIES})
add_executable(MiniBrowser ${MiniBrowser_SOURCES})
target_link_libraries(MiniBrowser ${MiniBrowser_LIBRARIES})
set_target_properties(MiniBrowser PROPERTIES FOLDER "Tools")
