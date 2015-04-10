add_custom_target(forwarding-headersEflForWebKitTestRunner
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT_TESTRUNNER_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include efl
)
set(ForwardingHeadersForWebKitTestRunner_NAME forwarding-headersEflForWebKitTestRunner)

add_custom_target(forwarding-headersSoupForWebKitTestRunner
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT_TESTRUNNER_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include soup
)
set(ForwardingNetworkHeadersForWebKitTestRunner_NAME forwarding-headersSoupForWebKitTestRunner)

list(APPEND WebKitTestRunner_SOURCES
    ${WEBKIT_TESTRUNNER_DIR}/cairo/TestInvocationCairo.cpp

    ${WEBKIT_TESTRUNNER_DIR}/efl/EventSenderProxyEfl.cpp
    ${WEBKIT_TESTRUNNER_DIR}/efl/PlatformWebViewEfl.cpp
    ${WEBKIT_TESTRUNNER_DIR}/efl/TestControllerEfl.cpp
    ${WEBKIT_TESTRUNNER_DIR}/efl/main.cpp
)

list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
    ${TOOLS_DIR}/DumpRenderTree/efl/
    ${WEBKIT2_DIR}/UIProcess/API/efl
    "${WTF_DIR}/wtf/gobject"
    ${CAIRO_INCLUDE_DIRS}
    ${ECORE_EVAS_INCLUDE_DIRS}
    ${ECORE_FILE_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${EINA_INCLUDE_DIRS}
    ${EO_INCLUDE_DIRS}
    ${EVAS_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

list(APPEND WebKitTestRunner_LIBRARIES
    ${CAIRO_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${EINA_LIBRARIES}
    ${EO_LIBRARIES}
    ${EVAS_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${OPENGL_LIBRARIES}
    WTF
)

if (ENABLE_ECORE_X)
    list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
        ${ECORE_X_INCLUDE_DIRS}
    )

    list(APPEND WebKitTestRunner_LIBRARIES
        ${ECORE_X_LIBRARIES}
        ${X11_Xext_LIB}
    )
endif ()

list(APPEND WebKitTestRunnerInjectedBundle_SOURCES
    ${TOOLS_DIR}/DumpRenderTree/efl/FontManagement.cpp

    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/efl/ActivateFontsEfl.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/efl/InjectedBundleEfl.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/efl/TestRunnerEfl.cpp
)

# FIXME: DOWNLOADED_FONTS_DIR should not hardcode the directory
# structure. See <https://bugs.webkit.org/show_bug.cgi?id=81475>.
add_definitions(-DFONTS_CONF_DIR="${TOOLS_DIR}/DumpRenderTree/gtk/fonts"
                -DDOWNLOADED_FONTS_DIR="${CMAKE_SOURCE_DIR}/WebKitBuild/Dependencies/Source/webkitgtk-test-fonts-0.0.3")

if (ENABLE_ACCESSIBILITY)
    list(APPEND WebKitTestRunnerInjectedBundle_SOURCES
        ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/atk/AccessibilityControllerAtk.cpp
        ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/atk/AccessibilityUIElementAtk.cpp
    )
    list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
        ${ATK_INCLUDE_DIRS}
    )
    list(APPEND WebKitTestRunner_LIBRARIES
        ${ATK_LIBRARIES}
    )
endif ()
