list(APPEND WebKit2_SOURCES
    Platform/efl/LoggingEfl.cpp
    Platform/efl/ModuleEfl.cpp
    Platform/efl/WorkQueueEfl.cpp
    Platform/unix/SharedMemoryUnix.cpp

    Platform/CoreIPC/unix/ConnectionUnix.cpp
    Platform/CoreIPC/unix/AttachmentUnix.cpp

    PluginProcess/unix/PluginControllerProxyUnix.cpp
    PluginProcess/unix/PluginProcessMainUnix.cpp
    PluginProcess/unix/PluginProcessUnix.cpp

    Shared/API/c/cairo/WKImageCairo.cpp
    Shared/API/c/efl/WKArrayEfl.cpp

    Shared/Downloads/efl/DownloadSoupErrorsEfl.cpp
    Shared/Downloads/soup/DownloadSoup.cpp

    Shared/cairo/ShareableBitmapCairo.cpp

    Shared/efl/LayerTreeContextEfl.cpp
    Shared/efl/NativeWebKeyboardEventEfl.cpp
    Shared/efl/NativeWebWheelEventEfl.cpp
    Shared/efl/NativeWebMouseEventEfl.cpp
    Shared/efl/NativeWebTouchEventEfl.cpp
    Shared/efl/ProcessExecutablePathEfl.cpp
    Shared/efl/WebEventFactory.cpp

    Shared/linux/SeccompFilters/OpenSyscall.cpp
    Shared/linux/SeccompFilters/SigactionSyscall.cpp
    Shared/linux/SeccompFilters/SigprocmaskSyscall.cpp
    Shared/linux/SeccompFilters/SeccompBroker.cpp
    Shared/linux/SeccompFilters/SeccompFilters.cpp
    Shared/linux/SeccompFilters/Syscall.cpp
    Shared/linux/SeccompFilters/SyscallPolicy.cpp

    Shared/linux/WebMemorySamplerLinux.cpp

    Shared/soup/PlatformCertificateInfo.cpp
    Shared/soup/WebCoreArgumentCodersSoup.cpp

    UIProcess/DefaultUndoController.cpp
    UIProcess/PageViewportController.cpp

    Shared/Plugins/Netscape/x11/NetscapePluginModuleX11.cpp

    UIProcess/API/C/cairo/WKIconDatabaseCairo.cpp

    UIProcess/API/C/CoordinatedGraphics/WKView.cpp

    UIProcess/API/C/efl/WKEventEfl.cpp
    UIProcess/API/C/efl/WKPageEfl.cpp
    UIProcess/API/C/efl/WKPopupItem.cpp
    UIProcess/API/C/efl/WKPopupMenuListener.cpp
    UIProcess/API/C/efl/WKViewEfl.cpp

    UIProcess/API/cpp/efl/WKEinaSharedString.cpp

    UIProcess/API/C/soup/WKContextSoup.cpp
    UIProcess/API/C/soup/WKCookieManagerSoup.cpp
    UIProcess/API/C/soup/WKSoupRequestManager.cpp

    UIProcess/API/efl/EwkView.cpp
    UIProcess/API/efl/EvasGLContext.cpp
    UIProcess/API/efl/EvasGLSurface.cpp
    UIProcess/API/efl/SnapshotImageGL.cpp
    UIProcess/API/efl/ewk_auth_request.cpp
    UIProcess/API/efl/ewk_back_forward_list.cpp
    UIProcess/API/efl/ewk_back_forward_list_item.cpp
    UIProcess/API/efl/ewk_color_picker.cpp
    UIProcess/API/efl/ewk_context.cpp
    UIProcess/API/efl/ewk_context_menu.cpp
    UIProcess/API/efl/ewk_context_menu_item.cpp
    UIProcess/API/efl/ewk_cookie_manager.cpp
    UIProcess/API/efl/ewk_database_manager.cpp
    UIProcess/API/efl/ewk_download_job.cpp
    UIProcess/API/efl/ewk_error.cpp
    UIProcess/API/efl/ewk_favicon_database.cpp
    UIProcess/API/efl/ewk_file_chooser_request.cpp
    UIProcess/API/efl/ewk_form_submission_request.cpp
    UIProcess/API/efl/ewk_main.cpp
    UIProcess/API/efl/ewk_navigation_data.cpp
    UIProcess/API/efl/ewk_navigation_policy_decision.cpp
    UIProcess/API/efl/ewk_object.cpp
    UIProcess/API/efl/ewk_page_group.cpp
    UIProcess/API/efl/ewk_popup_menu.cpp
    UIProcess/API/efl/ewk_popup_menu_item.cpp
    UIProcess/API/efl/ewk_security_origin.cpp
    UIProcess/API/efl/ewk_settings.cpp
    UIProcess/API/efl/ewk_storage_manager.cpp
    UIProcess/API/efl/ewk_text_checker.cpp
    UIProcess/API/efl/ewk_url_request.cpp
    UIProcess/API/efl/ewk_url_response.cpp
    UIProcess/API/efl/ewk_url_scheme_request.cpp
    UIProcess/API/efl/ewk_view.cpp
    UIProcess/API/efl/ewk_window_features.cpp

    UIProcess/cairo/BackingStoreCairo.cpp

    UIProcess/CoordinatedGraphics/WebView.cpp
    UIProcess/CoordinatedGraphics/WebViewClient.cpp

    UIProcess/efl/BatteryProvider.cpp
    UIProcess/efl/ContextHistoryClientEfl.cpp
    UIProcess/efl/ContextMenuClientEfl.cpp
    UIProcess/efl/DownloadManagerEfl.cpp
    UIProcess/efl/EwkTouchEvent.cpp
    UIProcess/efl/EwkTouchPoint.cpp
    UIProcess/efl/FindClientEfl.cpp
    UIProcess/efl/FormClientEfl.cpp
    UIProcess/efl/InputMethodContextEfl.cpp
    UIProcess/efl/NetworkInfoProvider.cpp
    UIProcess/efl/PageLoadClientEfl.cpp
    UIProcess/efl/PagePolicyClientEfl.cpp
    UIProcess/efl/PageUIClientEfl.cpp
    UIProcess/efl/PageViewportControllerClientEfl.cpp
    UIProcess/efl/RequestManagerClientEfl.cpp
    UIProcess/efl/TextCheckerClientEfl.cpp
    UIProcess/efl/TextCheckerEfl.cpp
    UIProcess/efl/VibrationClientEfl.cpp
    UIProcess/efl/ViewClientEfl.cpp
    UIProcess/efl/WebViewEfl.cpp
    UIProcess/efl/WebContextEfl.cpp
    UIProcess/efl/WebContextMenuProxyEfl.cpp
    UIProcess/efl/WebFullScreenManagerProxyEfl.cpp
    UIProcess/efl/WebInspectorProxyEfl.cpp
    UIProcess/efl/WebPageProxyEfl.cpp
    UIProcess/efl/WebPopupItemEfl.cpp
    UIProcess/efl/WebPopupMenuListenerEfl.cpp
    UIProcess/efl/WebPreferencesEfl.cpp
    UIProcess/efl/WebProcessProxyEfl.cpp
    UIProcess/efl/WebUIPopupMenuClient.cpp

    UIProcess/InspectorServer/efl/WebInspectorServerEfl.cpp
    UIProcess/InspectorServer/soup/WebSocketServerSoup.cpp

    UIProcess/soup/WebCookieManagerProxySoup.cpp
    UIProcess/soup/WebSoupRequestManagerClient.cpp
    UIProcess/soup/WebSoupRequestManagerProxy.cpp

    UIProcess/Launcher/efl/ProcessLauncherEfl.cpp

    UIProcess/Plugins/unix/PluginInfoStoreUnix.cpp
    UIProcess/Plugins/unix/PluginProcessProxyUnix.cpp

    UIProcess/Storage/StorageManager.cpp

    WebProcess/Cookies/soup/WebCookieManagerSoup.cpp
    WebProcess/Cookies/soup/WebKitSoupCookieJarSqlite.cpp

    WebProcess/InjectedBundle/efl/InjectedBundleEfl.cpp

    WebProcess/Plugins/Netscape/unix/PluginProxyUnix.cpp

    WebProcess/Plugins/Netscape/x11/NetscapePluginX11.cpp

    WebProcess/WebCoreSupport/efl/WebContextMenuClientEfl.cpp
    WebProcess/WebCoreSupport/efl/WebEditorClientEfl.cpp
    WebProcess/WebCoreSupport/efl/WebErrorsEfl.cpp
    WebProcess/WebCoreSupport/efl/WebPopupMenuEfl.cpp

    WebProcess/WebCoreSupport/soup/WebFrameNetworkingContext.cpp

    WebProcess/WebPage/atk/WebPageAccessibilityObjectAtk.cpp
    WebProcess/WebPage/efl/WebInspectorEfl.cpp
    WebProcess/WebPage/efl/WebPageEfl.cpp

    WebProcess/efl/SeccompFiltersWebProcessEfl.cpp
    WebProcess/efl/WebProcessMainEfl.cpp

    WebProcess/soup/WebProcessSoup.cpp
    WebProcess/soup/WebSoupRequestManager.cpp
    WebProcess/soup/WebKitSoupRequestGeneric.cpp
    WebProcess/soup/WebKitSoupRequestInputStream.cpp
)

list(APPEND WebKit2_MESSAGES_IN_FILES
    UIProcess/soup/WebSoupRequestManagerProxy.messages.in
    WebProcess/soup/WebSoupRequestManager.messages.in
)

list(APPEND WebKit2_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/efl"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBCORE_DIR}/platform/text/enchant"
    "${WEBKIT2_DIR}/Shared/API/c/efl"
    "${WEBKIT2_DIR}/Shared/Downloads/soup"
    "${WEBKIT2_DIR}/Shared/efl"
    "${WEBKIT2_DIR}/Shared/soup"
    "${WEBKIT2_DIR}/UIProcess/API/C/cairo"
    "${WEBKIT2_DIR}/UIProcess/API/C/CoordinatedGraphics"
    "${WEBKIT2_DIR}/UIProcess/API/C/efl"
    "${WEBKIT2_DIR}/UIProcess/API/C/soup"
    "${WEBKIT2_DIR}/UIProcess/API/cpp/efl"
    "${WEBKIT2_DIR}/UIProcess/API/efl"
    "${WEBKIT2_DIR}/UIProcess/efl"
    "${WEBKIT2_DIR}/UIProcess/soup"
    "${WEBKIT2_DIR}/WebProcess/efl"
    "${WEBKIT2_DIR}/WebProcess/soup"
    "${WEBKIT2_DIR}/WebProcess/WebCoreSupport/efl"
    "${WEBKIT2_DIR}/WebProcess/WebCoreSupport/soup"
    "${WTF_DIR}/wtf/efl/"
    "${WTF_DIR}/wtf/gobject"
    ${CAIRO_INCLUDE_DIRS}
    ${ECORE_EVAS_INCLUDE_DIRS}
    ${ECORE_IMF_EVAS_INCLUDE_DIRS}
    ${ECORE_IMF_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${ECORE_X_INCLUDE_DIRS}
    ${EDJE_INCLUDE_DIRS}
    ${EFREET_INCLUDE_DIRS}
    ${EINA_INCLUDE_DIRS}
    ${EO_INCLUDE_DIRS}
    ${EVAS_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${WTF_DIR}
)

list(APPEND WebKit2_LIBRARIES
    ${CAIRO_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${ECORE_IMF_EVAS_LIBRARIES}
    ${EDJE_LIBRARIES}
    ${EFREET_LIBRARIES}
    ${EINA_LIBRARIES}
    ${EO_LIBRARIES}
    ${EVAS_LIBRARIES}
    ${Freetype_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${GLIB_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
)

list(APPEND WebProcess_SOURCES
    efl/MainEfl.cpp
)

list(APPEND WebProcess_LIBRARIES
    ${CAIRO_LIBRARIES}
    ${ECORE_IMF_LIBRARIES}
    ${ECORE_IMF_EVAS_LIBRARIES}
    ${EDJE_LIBRARIES}
    ${EFLDEPS_LIBRARIES}
    ${EVAS_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${SQLITE_LIBRARIES}
)

if (ENABLE_SECCOMP_FILTERS)
    list(APPEND WebKit2_LIBRARIES
        ${LIBSECCOMP_LIBRARIES}
    )
    list(APPEND WebKit2_INCLUDE_DIRECTORIES
        ${LIBSECCOMP_INCLUDE_DIRS}
    )

    # If building with jhbuild, add the root build directory to the
    # filesystem access policy.
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/WebKitBuild/Dependencies)
        add_definitions(-DSOURCE_DIR=\"${CMAKE_SOURCE_DIR}\")
    endif ()
endif ()

if (ENABLE_ECORE_X)
    list(APPEND WebProcess_LIBRARIES
        ${ECORE_X_LIBRARIES}
        ${X11_Xext_LIB}
    )
    list(APPEND WebKit2_LIBRARIES
        ${ECORE_X_LIBRARIES}
    )
endif ()

add_custom_target(forwarding-headerEfl
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT2_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include efl
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT2_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include CoordinatedGraphics
)

add_custom_target(forwarding-headerSoup
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT2_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include soup
)

set(WEBKIT2_EXTRA_DEPENDENCIES
     forwarding-headerEfl
     forwarding-headerSoup
)

configure_file(efl/ewebkit2.pc.in ${CMAKE_BINARY_DIR}/WebKit2/efl/ewebkit2.pc @ONLY)
set(EWebKit2_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/EWebKit2.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_auth_request.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_back_forward_list.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_back_forward_list_item.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_color_picker.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_context.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_context_menu.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_context_menu_item.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_cookie_manager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_database_manager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_defines.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_download_job.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_error.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_favicon_database.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_file_chooser_request.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_form_submission_request.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_page_group.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_main.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_navigation_data.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_navigation_policy_decision.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_object.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_popup_menu.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_popup_menu_item.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_security_origin.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_settings.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_storage_manager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_text_checker.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_touch.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_url_request.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_url_response.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_url_scheme_request.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_view.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/ewk_window_features.h"
)

install(FILES ${CMAKE_BINARY_DIR}/WebKit2/efl/ewebkit2.pc DESTINATION lib/pkgconfig)
install(FILES ${EWebKit2_HEADERS} DESTINATION include/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR})

if (ENABLE_PLUGIN_PROCESS)
    add_definitions(-DENABLE_PLUGIN_PROCESS=1)

    list(APPEND PluginProcess_INCLUDE_DIRECTORIES
        "${WEBKIT2_DIR}/PluginProcess/unix"
    )

    include_directories(${PluginProcess_INCLUDE_DIRECTORIES})

    list(APPEND PluginProcess_SOURCES
        ${WEBKIT2_DIR}/unix/PluginMainUnix.cpp
    )

    set(PluginProcess_LIBRARIES
        WebKit2
    )

    if (ENABLE_ECORE_X)
        list(APPEND PluginProcess_LIBRARIES
            ${ECORE_X_LIBRARIES}
        )
    endif ()

    add_executable(PluginProcess ${PluginProcess_SOURCES})
    target_link_libraries(PluginProcess ${PluginProcess_LIBRARIES})
    install(TARGETS PluginProcess DESTINATION "${EXEC_INSTALL_DIR}")
endif () # ENABLE_PLUGIN_PROCESS

include_directories(${THIRDPARTY_DIR}/gtest/include)

set(EWK2UnitTests_LIBRARIES
    WTF
    JavaScriptCore
    WebCore
    WebKit2
    ${CAIRO_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${EVAS_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_GTHREAD_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
    gtest
)

set(WEBKIT2_EFL_TEST_DIR "${WEBKIT2_DIR}/UIProcess/API/efl/tests")
set(TEST_RESOURCES_DIR ${WEBKIT2_EFL_TEST_DIR}/resources)
set(TEST_INJECTED_BUNDLE_DIR ${WEBKIT2_EFL_TEST_DIR}/InjectedBundle)

add_definitions(-DTEST_RESOURCES_DIR=\"${TEST_RESOURCES_DIR}\"
    -DTEST_LIB_DIR=\"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}\"
    -DGTEST_LINKED_AS_SHARED_LIBRARY=1
    -DLIBEXECDIR=\"${CMAKE_INSTALL_PREFIX}/${EXEC_INSTALL_DIR}\"
    -DWEBPROCESSNAME=\"WebProcess\"
    -DPLUGINPROCESSNAME=\"PluginProcess\"
)

add_library(ewk2UnitTestUtils
    ${WEBKIT2_EFL_TEST_DIR}/UnitTestUtils/EWK2UnitTestBase.cpp
    ${WEBKIT2_EFL_TEST_DIR}/UnitTestUtils/EWK2UnitTestEnvironment.cpp
    ${WEBKIT2_EFL_TEST_DIR}/UnitTestUtils/EWK2UnitTestMain.cpp
    ${WEBKIT2_EFL_TEST_DIR}/UnitTestUtils/EWK2UnitTestServer.cpp
)

target_link_libraries(ewk2UnitTestUtils ${EWK2UnitTests_LIBRARIES})

# The "ewk" on the test name needs to be suffixed with "2", otherwise it
# will clash with tests from the WebKit 1 test suite.
set(EWK2UnitTests_BINARIES
    test_ewk2_auth_request
    test_ewk2_back_forward_list
    test_ewk2_color_picker
    test_ewk2_context
    test_ewk2_context_history_callbacks
    test_ewk2_context_menu
    test_ewk2_cookie_manager
    test_ewk2_database_manager
    test_ewk2_download_job
    test_ewk2_eina_shared_string
    test_ewk2_favicon_database
    test_ewk2_file_chooser_request
    test_ewk2_object
    test_ewk2_page_group
    test_ewk2_popup_menu
    test_ewk2_refptr_evas_object
    test_ewk2_settings
    test_ewk2_storage_manager
    test_ewk2_text_checker
    test_ewk2_view
    test_ewk2_window_features
)

# Skipped unit tests list:
#
# webkit.org/b/107422: test_ewk2_auth_request
#

if (ENABLE_API_TESTS)
    foreach (testName ${EWK2UnitTests_BINARIES})
        add_executable(${testName} ${WEBKIT2_EFL_TEST_DIR}/${testName}.cpp)
        add_test(${testName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${testName})
        set_tests_properties(${testName} PROPERTIES TIMEOUT 60)
        target_link_libraries(${testName} ${EWK2UnitTests_LIBRARIES} ewk2UnitTestUtils)
    endforeach ()

    add_library(ewk2UnitTestInjectedBundleSample SHARED ${TEST_INJECTED_BUNDLE_DIR}/injected_bundle_sample.cpp)
    target_link_libraries(ewk2UnitTestInjectedBundleSample WebKit2)
endif ()

if (ENABLE_SPELLCHECK)
    list(APPEND WebKit2_INCLUDE_DIRECTORIES
        ${ENCHANT_INCLUDE_DIRS}
    )
    list(APPEND WebKit2_LIBRARIES
        ${ENCHANT_LIBRARIES}
    )
endif ()

if (ENABLE_ACCESSIBILITY)
    list(APPEND WebKit2_INCLUDE_DIRECTORIES
        "${WEBKIT2_DIR}/WebProcess/WebPage/atk"
        ${ATK_INCLUDE_DIRS}
    )
    list(APPEND WebKit2_LIBRARIES
        ${ATK_LIBRARIES}
    )
endif ()
