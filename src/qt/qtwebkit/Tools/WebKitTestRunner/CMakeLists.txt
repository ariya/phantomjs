set(WEBKIT_TESTRUNNER_DIR "${TOOLS_DIR}/WebKitTestRunner")
set(WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR "${TOOLS_DIR}/WebKitTestRunner/InjectedBundle")

file(MAKE_DIRECTORY ${DERIVED_SOURCES_DIR}/InjectedBundle)

set(WebKitTestRunner_SOURCES
    ${WEBKIT_TESTRUNNER_DIR}/CyclicRedundancyCheck.cpp
    ${WEBKIT_TESTRUNNER_DIR}/GeolocationProviderMock.cpp
    ${WEBKIT_TESTRUNNER_DIR}/PixelDumpSupport.cpp
    ${WEBKIT_TESTRUNNER_DIR}/TestController.cpp
    ${WEBKIT_TESTRUNNER_DIR}/TestInvocation.cpp
    ${WEBKIT_TESTRUNNER_DIR}/WebNotificationProvider.cpp
    ${WEBKIT_TESTRUNNER_DIR}/WorkQueueManager.cpp
)

set(WebKitTestRunner_LIBRARIES
    JavaScriptCore
    WebCore
    WebCoreTestSupport
    WebKit2
)

set(WebKitTestRunner_INCLUDE_DIRECTORIES
    ${WEBKIT_TESTRUNNER_DIR}
    ${WEBKIT_TESTRUNNER_DIR}/InjectedBundle
    ${WEBKIT_TESTRUNNER_DIR}/InjectedBundle/Bindings
    ${JAVASCRIPTCORE_DIR}
    ${JAVASCRIPTCORE_DIR}/ForwardingHeaders
    ${WEBCORE_DIR}/editing
    ${WEBCORE_DIR}/platform
    ${WEBCORE_DIR}/platform/graphics
    ${WEBCORE_DIR}/platform/graphics/harfbuzz
    ${WEBCORE_DIR}/platform/graphics/harfbuzz/ng
    ${WEBCORE_DIR}/platform/network
    ${WEBCORE_DIR}/platform/text
    ${WEBCORE_DIR}/testing/js
    ${WEBKIT2_DIR}/Shared
    ${WEBKIT2_DIR}/Shared/API/c
    ${WEBKIT2_DIR}/Shared/Plugins
    ${WEBKIT2_DIR}/UIProcess
    ${WEBKIT2_DIR}/UIProcess/API/C/efl
    ${WEBKIT2_DIR}/UIProcess/API/C/soup
    ${WEBKIT2_DIR}/WebProcess/InjectedBundle
    ${WEBKIT2_DIR}/WebProcess/InjectedBundle/API/c
    ${WTF_DIR}
    ${DERIVED_SOURCES_WEBCORE_DIR}
    ${DERIVED_SOURCES_WEBCORE_DIR}/include
    ${DERIVED_SOURCES_WEBKIT2_DIR}/include
    ${DERIVED_SOURCES_DIR}/InjectedBundle
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${LIBSOUP_INCLUDE_DIRS}
)

set(WebKitTestRunnerInjectedBundle_SOURCES
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/AccessibilityController.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/AccessibilityTextMarker.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/AccessibilityTextMarkerRange.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/AccessibilityUIElement.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/EventSendingController.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/GCController.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/InjectedBundle.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/InjectedBundleMain.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/InjectedBundlePage.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/TestRunner.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/TextInputController.cpp

    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/JSWrapper.cpp
)

set(WebKitTestRunnerInjectedBundle_IDL_FILES
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/AccessibilityController.idl"
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/AccessibilityTextMarker.idl"
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/AccessibilityTextMarkerRange.idl"
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/AccessibilityUIElement.idl"
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/EventSendingController.idl"
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/GCController.idl"
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/TestRunner.idl"
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings/TextInputController.idl"
)

set(WebKitTestRunnerInjectedBundle_LIBRARIES
    ${WebKitTestRunner_LIBRARIES}
    ${ATK_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_FILE_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
)

GENERATE_BINDINGS(WebKitTestRunnerInjectedBundle_SOURCES
    "${WebKitTestRunnerInjectedBundle_IDL_FILES}"
    "${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings"
    "--include=${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/Bindings"
    "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR}"
    ${DERIVED_SOURCES_DIR}/InjectedBundle JS TestRunner
    ${WEBCORE_DIR}/bindings/scripts/IDLAttributes.txt)

INCLUDE_IF_EXISTS(${WEBKIT_TESTRUNNER_DIR}/Platform${PORT}.cmake)

include_directories(${WebKitTestRunner_INCLUDE_DIRECTORIES})

add_library(TestRunnerInjectedBundle SHARED ${WebKitTestRunnerInjectedBundle_SOURCES})
target_link_libraries(TestRunnerInjectedBundle ${WebKitTestRunnerInjectedBundle_LIBRARIES})

add_executable(WebKitTestRunner ${WebKitTestRunner_SOURCES})
target_link_libraries(WebKitTestRunner ${WebKitTestRunner_LIBRARIES})

add_dependencies(WebKit2 ${ForwardingHeadersForWebKitTestRunner_NAME})
add_dependencies(WebKit2 ${ForwardingNetworkHeadersForWebKitTestRunner_NAME})
