set(EWebLauncher_SOURCES
    ${TOOLS_DIR}/EWebLauncher/url_bar.c
    ${TOOLS_DIR}/EWebLauncher/url_utils.c
    ${TOOLS_DIR}/EWebLauncher/main.c
)

set(EWebLauncher_LIBRARIES
    JavaScriptCore
    WebCore
    WebKit
    ${CAIRO_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${ECORE_FILE_LIBRARIES}
    ${ECORE_X_LIBRARIES}
    ${EDJE_LIBRARIES}
    ${EINA_LIBRARIES}
    ${EO_LIBRARIES}
    ${EVAS_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES} -lm
    ${SQLITE_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
)

set(EWebLauncher_INCLUDE_DIRECTORIES
    "${WEBKIT_DIR}/efl/ewk"
    ${CAIRO_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${ECORE_EVAS_INCLUDE_DIRS}
    ${ECORE_FILE_INCLUDE_DIRS}
    ${ECORE_X_INCLUDE_DIRS}
    ${EDJE_INCLUDE_DIRS}
    ${EINA_INCLUDE_DIRS}
    ${EO_INCLUDE_DIRS}
    ${EVAS_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
)

include_directories(${EWebLauncher_INCLUDE_DIRECTORIES})
add_executable(EWebLauncher ${EWebLauncher_SOURCES})
target_link_libraries(EWebLauncher ${EWebLauncher_LIBRARIES})
set_target_properties(EWebLauncher PROPERTIES FOLDER "Tools")

add_dependencies(EWebLauncher ControlTheme)
