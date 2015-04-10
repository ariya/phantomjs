set(DERIVED_SOURCES_WEBKIT2GTK_DIR ${DERIVED_SOURCES_DIR}/webkit2gtk)
set(WEBKIT2_BUILT_API_DIR ${DERIVED_SOURCES_WEBKIT2GTK_DIR}/webkit2)
set(WEBKIT2_FORWARDING_HEADERS_DIR ${DERIVED_SOURCES_DIR}/ForwardingHeaders/webkit2gtk)

file(MAKE_DIRECTORY ${DERIVED_SOURCES_WEBKIT2_DIR})
file(MAKE_DIRECTORY ${WEBKIT2_BUILT_API_DIR})
file(MAKE_DIRECTORY ${WEBKIT2_FORWARDING_HEADERS_DIR})

configure_file(UIProcess/API/gtk/WebKitVersion.h.in ${WEBKIT2_BUILT_API_DIR}/WebKitVersion.h)

add_definitions(-DWEBKIT2_COMPILATION)
add_definitions(-DLIBEXECDIR="${CMAKE_INSTALL_FULL_LIBEXECDIR}")
add_definitions(-DPACKAGE_LOCALE_DIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")
add_definitions(-DLIBDIR="${CMAKE_INSTALL_FULL_LIBDIR}")

list(APPEND WebKit2_SOURCES
    ${WEBKIT2_BUILT_API_DIR}/WebKitMarshal.cpp
    ${WEBKIT2_BUILT_API_DIR}/WebKitEnumTypes.cpp
    Platform/gtk/LoggingGtk.cpp
    Platform/gtk/ModuleGtk.cpp
    Platform/gtk/WorkQueueGtk.cpp
    Platform/unix/SharedMemoryUnix.cpp
    WebProcess/WebPage/gtk/WebPrintOperationGtk.cpp
    WebProcess/WebPage/gtk/WebPageGtk.cpp
    WebProcess/WebPage/gtk/LayerTreeHostGtk.cpp
    WebProcess/WebPage/gtk/WebInspectorGtk.cpp
    WebProcess/WebCoreSupport/gtk/WebErrorsGtk.cpp
    WebProcess/WebCoreSupport/gtk/WebPopupMenuGtk.cpp
    WebProcess/WebCoreSupport/gtk/WebDragClientGtk.cpp
    WebProcess/WebCoreSupport/gtk/WebContextMenuClientGtk.cpp
    WebProcess/WebCoreSupport/gtk/WebEditorClientGtk.cpp
    WebProcess/InjectedBundle/gtk/InjectedBundleGtk.cpp
    UIProcess/API/C/gtk/WKFullScreenClientGtk.cpp
    UIProcess/API/C/gtk/WKInspectorClientGtk.cpp
    UIProcess/gtk/ExperimentalFeatures.cpp
    UIProcess/gtk/WebContextMenuProxyGtk.cpp
    UIProcess/gtk/WebContextGtk.cpp
    UIProcess/gtk/WebPageProxyGtk.cpp
    UIProcess/gtk/WebPreferencesGtk.cpp
    UIProcess/gtk/WebFullScreenClientGtk.cpp
    UIProcess/gtk/WebFullScreenManagerProxyGtk.cpp
    UIProcess/gtk/WebInspectorProxyGtk.cpp
    UIProcess/gtk/TextCheckerGtk.cpp
    UIProcess/gtk/WebPopupMenuProxyGtk.cpp
    UIProcess/gtk/WebInspectorClientGtk.cpp
    UIProcess/gtk/WebProcessProxyGtk.cpp
    UIProcess/Launcher/gtk/ProcessLauncherGtk.cpp
    UIProcess/InspectorServer/gtk/WebInspectorServerGtk.cpp

    Shared/API/c/gtk/WKGraphicsContextGtk.cpp
    Shared/Downloads/gtk/DownloadSoupErrorsGtk.cpp
    Shared/gtk/ArgumentCodersGtk.cpp
    Shared/gtk/LayerTreeContextGtk.cpp
    Shared/gtk/NativeWebKeyboardEventGtk.cpp
    Shared/gtk/NativeWebMouseEventGtk.cpp
    Shared/gtk/NativeWebWheelEventGtk.cpp
    Shared/gtk/PrintInfoGtk.cpp
    Shared/gtk/ProcessExecutablePathGtk.cpp
    Shared/gtk/WebEventFactory.cpp

    Platform/CoreIPC/unix/ConnectionUnix.cpp
    Platform/CoreIPC/unix/AttachmentUnix.cpp
    PluginProcess/unix/PluginControllerProxyUnix.cpp
    PluginProcess/unix/PluginProcessMainUnix.cpp
    PluginProcess/unix/PluginProcessUnix.cpp
    Shared/API/c/cairo/WKImageCairo.cpp
    Shared/Downloads/soup/DownloadSoup.cpp
    Shared/cairo/ShareableBitmapCairo.cpp
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
    Shared/Plugins/Netscape/x11/NetscapePluginModuleX11.cpp
    UIProcess/API/C/cairo/WKIconDatabaseCairo.cpp
    UIProcess/API/C/CoordinatedGraphics/WKView.cpp
    UIProcess/API/C/soup/WKContextSoup.cpp
    UIProcess/API/C/soup/WKCookieManagerSoup.cpp
    UIProcess/API/C/soup/WKSoupRequestManager.cpp
    UIProcess/cairo/BackingStoreCairo.cpp
    UIProcess/CoordinatedGraphics/WebView.cpp
    UIProcess/CoordinatedGraphics/WebViewClient.cpp
    UIProcess/InspectorServer/soup/WebSocketServerSoup.cpp
    UIProcess/soup/WebCookieManagerProxySoup.cpp
    UIProcess/soup/WebSoupRequestManagerClient.cpp
    UIProcess/soup/WebSoupRequestManagerProxy.cpp
    UIProcess/Plugins/unix/PluginInfoStoreUnix.cpp
    UIProcess/Plugins/unix/PluginProcessProxyUnix.cpp
    UIProcess/Storage/StorageManager.cpp
    WebProcess/Cookies/soup/WebCookieManagerSoup.cpp
    WebProcess/Cookies/soup/WebKitSoupCookieJarSqlite.cpp
    WebProcess/Plugins/Netscape/unix/PluginProxyUnix.cpp
    WebProcess/Plugins/Netscape/x11/NetscapePluginX11.cpp
    WebProcess/WebCoreSupport/soup/WebFrameNetworkingContext.cpp
    WebProcess/WebPage/atk/WebPageAccessibilityObjectAtk.cpp
    WebProcess/soup/WebProcessSoup.cpp
    WebProcess/soup/WebSoupRequestManager.cpp
    WebProcess/soup/WebKitSoupRequestGeneric.cpp
    WebProcess/soup/WebKitSoupRequestInputStream.cpp

    UIProcess/API/gtk/PageClientImpl.cpp
    UIProcess/API/gtk/PageClientImpl.h
    UIProcess/API/gtk/webkit2.h
    UIProcess/API/gtk/WebKitAuthenticationDialog.cpp
    UIProcess/API/gtk/WebKitAuthenticationDialog.h
    UIProcess/API/gtk/WebKitBackForwardList.cpp
    UIProcess/API/gtk/WebKitBackForwardList.h
    UIProcess/API/gtk/WebKitBackForwardListItem.cpp
    UIProcess/API/gtk/WebKitBackForwardListItem.h
    UIProcess/API/gtk/WebKitBackForwardListPrivate.h
    UIProcess/API/gtk/WebKitContextMenuActions.cpp
    UIProcess/API/gtk/WebKitContextMenuActions.h
    UIProcess/API/gtk/WebKitContextMenuActionsPrivate.h
    UIProcess/API/gtk/WebKitContextMenuClient.cpp
    UIProcess/API/gtk/WebKitContextMenuClient.h
    UIProcess/API/gtk/WebKitContextMenu.cpp
    UIProcess/API/gtk/WebKitContextMenu.h
    UIProcess/API/gtk/WebKitContextMenuItem.cpp
    UIProcess/API/gtk/WebKitContextMenuItem.h
    UIProcess/API/gtk/WebKitContextMenuItemPrivate.h
    UIProcess/API/gtk/WebKitContextMenuPrivate.h
    UIProcess/API/gtk/WebKitCookieManager.cpp
    UIProcess/API/gtk/WebKitCookieManager.h
    UIProcess/API/gtk/WebKitCookieManagerPrivate.h
    UIProcess/API/gtk/WebKitDefines.h
    UIProcess/API/gtk/WebKitDownloadClient.cpp
    UIProcess/API/gtk/WebKitDownloadClient.h
    UIProcess/API/gtk/WebKitDownload.cpp
    UIProcess/API/gtk/WebKitDownload.h
    UIProcess/API/gtk/WebKitDownloadPrivate.h
    UIProcess/API/gtk/WebKitEditingCommands.h
    UIProcess/API/gtk/WebKitError.cpp
    UIProcess/API/gtk/WebKitError.h
    UIProcess/API/gtk/WebKitFaviconDatabase.cpp
    UIProcess/API/gtk/WebKitFaviconDatabase.h
    UIProcess/API/gtk/WebKitFaviconDatabasePrivate.h
    UIProcess/API/gtk/WebKitFileChooserRequest.cpp
    UIProcess/API/gtk/WebKitFileChooserRequest.h
    UIProcess/API/gtk/WebKitFileChooserRequestPrivate.h
    UIProcess/API/gtk/WebKitFindController.cpp
    UIProcess/API/gtk/WebKitFindController.h
    UIProcess/API/gtk/WebKitFormClient.cpp
    UIProcess/API/gtk/WebKitFormClient.h
    UIProcess/API/gtk/WebKitFormSubmissionRequest.cpp
    UIProcess/API/gtk/WebKitFormSubmissionRequest.h
    UIProcess/API/gtk/WebKitFormSubmissionRequestPrivate.h
    UIProcess/API/gtk/WebKitForwardDeclarations.h
    UIProcess/API/gtk/WebKitFullscreenClient.cpp
    UIProcess/API/gtk/WebKitFullscreenClient.h
    UIProcess/API/gtk/WebKitGeolocationPermissionRequest.cpp
    UIProcess/API/gtk/WebKitGeolocationPermissionRequest.h
    UIProcess/API/gtk/WebKitGeolocationPermissionRequestPrivate.h
    UIProcess/API/gtk/WebKitGeolocationProvider.cpp
    UIProcess/API/gtk/WebKitGeolocationProvider.h
    UIProcess/API/gtk/WebKitHitTestResult.cpp
    UIProcess/API/gtk/WebKitHitTestResult.h
    UIProcess/API/gtk/WebKitHitTestResultPrivate.h
    UIProcess/API/gtk/WebKitInjectedBundleClient.cpp
    UIProcess/API/gtk/WebKitInjectedBundleClient.h
    UIProcess/API/gtk/WebKitJavascriptResult.cpp
    UIProcess/API/gtk/WebKitJavascriptResult.h
    UIProcess/API/gtk/WebKitJavascriptResultPrivate.h
    UIProcess/API/gtk/WebKitLoaderClient.cpp
    UIProcess/API/gtk/WebKitLoaderClient.h
    UIProcess/API/gtk/WebKitMimeInfo.cpp
    UIProcess/API/gtk/WebKitMimeInfo.h
    UIProcess/API/gtk/WebKitMimeInfoPrivate.h
    UIProcess/API/gtk/WebKitNavigationPolicyDecision.cpp
    UIProcess/API/gtk/WebKitNavigationPolicyDecision.h
    UIProcess/API/gtk/WebKitNavigationPolicyDecisionPrivate.h
    UIProcess/API/gtk/WebKitPermissionRequest.cpp
    UIProcess/API/gtk/WebKitPermissionRequest.h
    UIProcess/API/gtk/WebKitPlugin.cpp
    UIProcess/API/gtk/WebKitPlugin.h
    UIProcess/API/gtk/WebKitPluginPrivate.h
    UIProcess/API/gtk/WebKitPolicyClient.cpp
    UIProcess/API/gtk/WebKitPolicyClient.h
    UIProcess/API/gtk/WebKitPolicyDecision.cpp
    UIProcess/API/gtk/WebKitPolicyDecision.h
    UIProcess/API/gtk/WebKitPolicyDecisionPrivate.h
    UIProcess/API/gtk/WebKitPrintOperation.cpp
    UIProcess/API/gtk/WebKitPrintOperation.h
    UIProcess/API/gtk/WebKitPrintOperationPrivate.h
    UIProcess/API/gtk/WebKitPrivate.cpp
    UIProcess/API/gtk/WebKitPrivate.h
    UIProcess/API/gtk/WebKitRequestManagerClient.cpp
    UIProcess/API/gtk/WebKitRequestManagerClient.h
    UIProcess/API/gtk/WebKitResponsePolicyDecision.cpp
    UIProcess/API/gtk/WebKitResponsePolicyDecision.h
    UIProcess/API/gtk/WebKitResponsePolicyDecisionPrivate.h
    UIProcess/API/gtk/WebKitScriptDialog.cpp
    UIProcess/API/gtk/WebKitScriptDialog.h
    UIProcess/API/gtk/WebKitScriptDialogPrivate.h
    UIProcess/API/gtk/WebKitSecurityManager.cpp
    UIProcess/API/gtk/WebKitSecurityManager.h
    UIProcess/API/gtk/WebKitSecurityManagerPrivate.h
    UIProcess/API/gtk/WebKitSettings.cpp
    UIProcess/API/gtk/WebKitSettings.h
    UIProcess/API/gtk/WebKitSettingsPrivate.h
    UIProcess/API/gtk/WebKitTextChecker.cpp
    UIProcess/API/gtk/WebKitTextChecker.h
    UIProcess/API/gtk/WebKitUIClient.cpp
    UIProcess/API/gtk/WebKitUIClient.h
    UIProcess/API/gtk/WebKitURIRequest.cpp
    UIProcess/API/gtk/WebKitURIRequest.h
    UIProcess/API/gtk/WebKitURIRequestPrivate.h
    UIProcess/API/gtk/WebKitURIResponse.cpp
    UIProcess/API/gtk/WebKitURIResponse.h
    UIProcess/API/gtk/WebKitURIResponsePrivate.h
    UIProcess/API/gtk/WebKitURISchemeRequest.cpp
    UIProcess/API/gtk/WebKitURISchemeRequest.h
    UIProcess/API/gtk/WebKitURISchemeRequestPrivate.h
    UIProcess/API/gtk/WebKitVersion.cpp
    UIProcess/API/gtk/WebKitVersion.h.in
    UIProcess/API/gtk/WebKitWebContext.cpp
    UIProcess/API/gtk/WebKitWebContext.h
    UIProcess/API/gtk/WebKitWebContextPrivate.h
    UIProcess/API/gtk/WebKitWebInspector.cpp
    UIProcess/API/gtk/WebKitWebInspector.h
    UIProcess/API/gtk/WebKitWebInspectorPrivate.h
    UIProcess/API/gtk/WebKitWebResource.cpp
    UIProcess/API/gtk/WebKitWebResource.h
    UIProcess/API/gtk/WebKitWebResourcePrivate.h
    UIProcess/API/gtk/WebKitWebViewBaseAccessible.cpp
    UIProcess/API/gtk/WebKitWebViewBaseAccessible.h
    UIProcess/API/gtk/WebKitWebViewBase.cpp
    UIProcess/API/gtk/WebKitWebViewBase.h
    UIProcess/API/gtk/WebKitWebViewBasePrivate.h
    UIProcess/API/gtk/WebKitWebView.cpp
    UIProcess/API/gtk/WebKitWebViewGroup.cpp
    UIProcess/API/gtk/WebKitWebViewGroup.h
    UIProcess/API/gtk/WebKitWebViewGroupPrivate.h
    UIProcess/API/gtk/WebKitWebView.h
    UIProcess/API/gtk/WebKitWebViewPrivate.h
    UIProcess/API/gtk/WebKitWindowProperties.cpp
    UIProcess/API/gtk/WebKitWindowProperties.h
    UIProcess/API/gtk/WebKitWindowPropertiesPrivate.h
    UIProcess/API/gtk/WebViewBaseInputMethodFilter.cpp
    UIProcess/API/gtk/WebViewBaseInputMethodFilter.h

    UIProcess/API/C/gtk/WKFullScreenClientGtk.cpp
    UIProcess/API/C/gtk/WKInspectorClientGtk.cpp
    UIProcess/API/C/gtk/WKView.cpp

    WebProcess/gtk/WebProcessMainGtk.cpp
)

set(WebKit2_INSTALLED_HEADERS
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitBackForwardList.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitBackForwardListItem.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitContextMenu.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitContextMenuActions.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitContextMenuItem.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitCookieManager.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitDefines.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitDownload.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitEditingCommands.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitError.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitFaviconDatabase.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitFileChooserRequest.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitFindController.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitFormSubmissionRequest.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitForwardDeclarations.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitGeolocationPermissionRequest.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitHitTestResult.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitJavascriptResult.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitMimeInfo.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitNavigationPolicyDecision.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitPermissionRequest.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitPlugin.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitPolicyDecision.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitPrintOperation.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitResponsePolicyDecision.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitScriptDialog.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitSecurityManager.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitSettings.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitURIRequest.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitURIResponse.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitURISchemeRequest.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitWebContext.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitWebInspector.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitWebResource.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitWebView.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitWebViewBase.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitWebViewGroup.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitWindowProperties.h
    ${WEBKIT2_DIR}/UIProcess/API/gtk/webkit2.h

    ${WEBKIT2_DIR}/WebProcess/InjectedBundle/API/gtk/WebKitWebExtension.h
    ${WEBKIT2_DIR}/WebProcess/InjectedBundle/API/gtk/WebKitWebPage.h
    ${WEBKIT2_DIR}/WebProcess/InjectedBundle/API/gtk/webkit-web-extension.h
)

list(APPEND WebKit2_MESSAGES_IN_FILES
    UIProcess/soup/WebSoupRequestManagerProxy.messages.in
    WebProcess/soup/WebSoupRequestManager.messages.in
)

# This is necessary because of a conflict between the GTK+ API WebKitVersion.h and one generated by WebCore.
list(INSERT WebKit2_INCLUDE_DIRECTORIES 0
    "${WEBKIT2_FORWARDING_HEADERS_DIR}"
    "${WEBKIT2_BUILT_API_DIR}"
    "${DERIVED_SOURCES_WEBKIT2GTK_DIR}"
)

list(APPEND WebKit2_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/gtk"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBCORE_DIR}/platform/text/enchant"
    "${WEBKIT2_DIR}/Shared/API/c/gtk"
    "${WEBKIT2_DIR}/Shared/Downloads/soup"
    "${WEBKIT2_DIR}/Shared/gtk"
    "${WEBKIT2_DIR}/Shared/soup"
    "${WEBKIT2_DIR}/UIProcess/API/C/cairo"
    "${WEBKIT2_DIR}/UIProcess/API/C/gtk"
    "${WEBKIT2_DIR}/UIProcess/API/C/soup"
    "${WEBKIT2_DIR}/UIProcess/API/cpp/gtk"
    "${WEBKIT2_DIR}/UIProcess/API/gtk"
    "${WEBKIT2_DIR}/UIProcess/gtk"
    "${WEBKIT2_DIR}/UIProcess/soup"
    "${WEBKIT2_DIR}/WebProcess/gtk"
    "${WEBKIT2_DIR}/WebProcess/soup"
    "${WEBKIT2_DIR}/WebProcess/WebCoreSupport/gtk"
    "${WEBKIT2_DIR}/WebProcess/WebCoreSupport/soup"
    "${WEBKIT2_DIR}/WebProcess/WebPage/atk"
    "${WEBKIT2_DIR}/WebProcess/WebPage/gtk"
    "${WTF_DIR}/wtf/gtk/"
    "${WTF_DIR}/wtf/gobject"
    ${WTF_DIR}
    ${CAIRO_INCLUDE_DIRS}
    ${ENCHANT_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
)

list(APPEND WebProcess_SOURCES
    gtk/MainGtk.cpp
)

set(WebKit2_MARSHAL_LIST ${WEBKIT2_DIR}/UIProcess/API/gtk/webkit2marshal.list)
add_custom_command(
    OUTPUT ${WEBKIT2_BUILT_API_DIR}/WebKitMarshal.cpp
           ${WEBKIT2_BUILT_API_DIR}/WebKitMarshal.h
    MAIN_DEPENDENCY ${WebKit2_MARSHAL_LIST}

    COMMAND echo extern \"C\" { > ${WEBKIT2_BUILT_API_DIR}/WebKitMarshal.cpp
    COMMAND glib-genmarshal --prefix=webkit_marshal ${WebKit2_MARSHAL_LIST} --body >> ${WEBKIT2_BUILT_API_DIR}/WebKitMarshal.cpp
    COMMAND echo } >> ${WEBKIT2_BUILT_API_DIR}/WebKitMarshal.cpp

    COMMAND glib-genmarshal --prefix=webkit_marshal ${WebKit2_MARSHAL_LIST} --header > ${WEBKIT2_BUILT_API_DIR}/WebKitMarshal.h
    VERBATIM)

add_custom_command(
    OUTPUT ${WEBKIT2_BUILT_API_DIR}/WebKitEnumTypes.h
           ${WEBKIT2_BUILT_API_DIR}/WebKitEnumTypes.cpp
    DEPENDS ${WebKit2_INSTALLED_HEADERS}

    COMMAND glib-mkenums --template ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitEnumTypes.h.template ${WebKit2_INSTALLED_HEADERS} | sed s/web_kit/webkit/ | sed s/WEBKIT_TYPE_KIT/WEBKIT_TYPE/ > ${WEBKIT2_BUILT_API_DIR}/WebKitEnumTypes.h

    COMMAND glib-mkenums --template ${WEBKIT2_DIR}/UIProcess/API/gtk/WebKitEnumTypes.cpp.template ${WebKit2_INSTALLED_HEADERS} | sed s/web_kit/webkit/ > ${WEBKIT2_BUILT_API_DIR}/WebKitEnumTypes.cpp
    VERBATIM)

# This symbolic link allows includes like #include <webkit2/WebkitWebView.h> which simulates installed headers.
add_custom_target(fake-installed-headers
    mkdir -p ${DERIVED_SOURCES_WEBKIT2_DIR}/webkit2gtk/include
    COMMAND ln -n -s -f ${WEBKIT2_DIR}/UIProcess/API/gtk ${WEBKIT2_FORWARDING_HEADERS_DIR}/webkit2
)

add_custom_target(gtk-forwarding-headers
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT2_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include gtk
)

add_custom_target(soup-forwarding-headers
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT2_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include soup
)

set(WEBKIT2_EXTRA_DEPENDENCIES
     gtk-forwarding-headers
     soup-forwarding-headers
     fake-installed-headers
)

if (ENABLE_PLUGIN_PROCESS)
    add_definitions(-DENABLE_PLUGIN_PROCESS=1)

    set(PluginProcess_EXECUTABLE_NAME WebKitPluginProcess)
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

    add_executable(${PluginProcess_EXECUTABLE_NAME} ${PluginProcess_SOURCES})
    target_link_libraries(${PluginProcess_EXECUTABLE_NAME} ${PluginProcess_LIBRARIES})
    install(TARGETS ${PluginProcess_EXECUTABLE_NAME} DESTINATION "${EXEC_INSTALL_DIR}")
endif () # ENABLE_PLUGIN_PROCESS
