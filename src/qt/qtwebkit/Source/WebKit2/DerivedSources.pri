# -------------------------------------------------------------------
# Derived sources for WebKit2
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

# This file is both a top level target, and included from Target.pri,
# so that the resulting generated sources can be added to SOURCES.
# We only set the template if we're a top level target, so that we
# don't override what Target.pri has already set.
sanitizedFile = $$toSanitizedPath($$_FILE_)
equals(sanitizedFile, $$toSanitizedPath($$_PRO_FILE_)):TEMPLATE = derived

WEBCORE_GENERATED_SOURCES_DIR = ../WebCore/$${GENERATED_SOURCES_DESTDIR}

SOURCE_DIR = $${ROOT_WEBKIT_DIR}/Source

WEBCORE_GENERATED_HEADERS_FOR_WEBKIT2 += \
    $$WEBCORE_GENERATED_SOURCES_DIR/HTMLNames.h \
    $$WEBCORE_GENERATED_SOURCES_DIR/JSCSSStyleDeclaration.h \
    $$WEBCORE_GENERATED_SOURCES_DIR/JSDOMWindow.h \
    $$WEBCORE_GENERATED_SOURCES_DIR/JSElement.h \
    $$WEBCORE_GENERATED_SOURCES_DIR/JSHTMLElement.h \
    $$WEBCORE_GENERATED_SOURCES_DIR/JSNode.h \
    $$WEBCORE_GENERATED_SOURCES_DIR/JSNotification.h \
    $$WEBCORE_GENERATED_SOURCES_DIR/JSRange.h \
    $$WEBCORE_GENERATED_SOURCES_DIR/JSUint8Array.h \

defineReplace(message_header_generator_output) {
  FILENAME=$$basename(1)
  return($${GENERATED_SOURCES_DESTDIR}/$$replace(FILENAME, ".messages.in", "Messages.h"))
}

defineReplace(message_receiver_generator_output) {
  FILENAME=$$basename(1)
  return($${GENERATED_SOURCES_DESTDIR}/$$replace(FILENAME, ".messages.in", "MessageReceiver.cpp"))
}

VPATH = \
    PluginProcess \
    WebProcess/ApplicationCache \
    WebProcess/Battery \
    WebProcess/Cookies \
    WebProcess/FullScreen \
    WebProcess/Geolocation \
    WebProcess/IconDatabase \
    WebProcess/MediaCache \
    WebProcess/NetworkInfo \
    WebProcess/Notifications \
    WebProcess/Plugins \
    WebProcess/ResourceCache \
    WebProcess/Storage \
    WebProcess/WebCoreSupport \
    WebProcess/WebPage \
    WebProcess/WebPage/CoordinatedGraphics \
    WebProcess \
    UIProcess \
    UIProcess/CoordinatedGraphics \
    UIProcess/Downloads \
    UIProcess/Notifications \
    UIProcess/Plugins \
    UIProcess/Storage \
    Shared \
    Shared/Authentication \
    Shared/Plugins

MESSAGE_RECEIVERS = \
    AuthenticationManager.messages.in \
    CoordinatedLayerTreeHostProxy.messages.in \
    DownloadProxy.messages.in \
    DrawingAreaProxy.messages.in \
    EventDispatcher.messages.in \
    PluginControllerProxy.messages.in \
    PluginProcess.messages.in \
    PluginProcessConnection.messages.in \
    PluginProcessConnectionManager.messages.in \
    PluginProcessProxy.messages.in \
    PluginProxy.messages.in \
    StorageAreaMap.messages.in \
    StorageManager.messages.in \
    WebApplicationCacheManager.messages.in \
    WebApplicationCacheManagerProxy.messages.in \
    WebBatteryManager.messages.in \
    WebBatteryManagerProxy.messages.in \
    WebConnection.messages.in \
    WebContext.messages.in \
    WebCookieManager.messages.in \
    WebCookieManagerProxy.messages.in \
    WebDatabaseManager.messages.in \
    WebDatabaseManagerProxy.messages.in \
    WebGeolocationManager.messages.in \
    WebGeolocationManagerProxy.messages.in \
    WebIconDatabase.messages.in \
    WebIconDatabaseProxy.messages.in \
    WebInspectorProxy.messages.in \
    WebMediaCacheManager.messages.in \
    WebMediaCacheManagerProxy.messages.in \
    WebNetworkInfoManager.messages.in \
    WebNetworkInfoManagerProxy.messages.in \
    WebNotificationManager.messages.in \
    WebFullScreenManager.messages.in \
    WebFullScreenManagerProxy.messages.in \
    CoordinatedLayerTreeHost.messages.in \
    DrawingArea.messages.in \
    WebInspector.messages.in \
    WebPage.messages.in \
    WebPageGroupProxy.messages.in \
    WebPageProxy.messages.in \
    WebProcess.messages.in \
    WebProcessConnection.messages.in \
    WebProcessProxy.messages.in \
    WebResourceCacheManager.messages.in \
    WebResourceCacheManagerProxy.messages.in \
    WebVibrationProxy.messages.in \
    NPObjectMessageReceiver.messages.in

SCRIPTS = \
    $$PWD/Scripts/generate-message-receiver.py \
    $$PWD/Scripts/generate-messages-header.py \
    $$PWD/Scripts/webkit2/__init__.py \
    $$PWD/Scripts/webkit2/messages.py \
    $$PWD/Scripts/webkit2/model.py \
    $$PWD/Scripts/webkit2/parser.py

message_header_generator.commands = $${PYTHON} $${SOURCE_DIR}/WebKit2/Scripts/generate-messages-header.py ${QMAKE_FILE_IN} > ${QMAKE_FILE_OUT}
message_header_generator.input = MESSAGE_RECEIVERS
message_header_generator.depends = $$SCRIPTS
message_header_generator.output_function = message_header_generator_output
message_header_generator.add_output_to_sources = false
GENERATORS += message_header_generator

message_receiver_generator.commands = $${PYTHON} $${SOURCE_DIR}/WebKit2/Scripts/generate-message-receiver.py  ${QMAKE_FILE_IN} > ${QMAKE_FILE_OUT}
message_receiver_generator.input = MESSAGE_RECEIVERS
message_receiver_generator.depends = $$SCRIPTS
message_receiver_generator.output_function = message_receiver_generator_output
GENERATORS += message_receiver_generator

fwheader_generator.commands = perl $${SOURCE_DIR}/WebKit2/Scripts/generate-forwarding-headers.pl $${SOURCE_DIR}/WebKit2 $${ROOT_BUILD_DIR}/Source/include qt
fwheader_generator.depends = $${SOURCE_DIR}/WebKit2/Scripts/generate-forwarding-headers.pl
generated_files.depends += fwheader_generator
GENERATORS += fwheader_generator

for(header, WEBCORE_GENERATED_HEADERS_FOR_WEBKIT2) {
    header_name = $$basename(header)
    header_path = $$header
    header_target = $$replace(header_path, [^a-zA-Z0-9_], -)
    header_target = "qtheader-$${header_target}"
    dest_dir = $${ROOT_BUILD_DIR}/Source/include/WebCore

    eval($${header_target}.target = $$dest_dir/$$header_name)
    eval($${header_target}.depends = $$header_path)

    win32: eval($${header_target}.commands = ($${QMAKE_MKDIR} $$toSystemPath($$dest_dir) 2>nul || echo>nul))
    else: eval($${header_target}.commands = $${QMAKE_MKDIR} $$toSystemPath($$dest_dir) )

    eval($${header_target}.commands += && echo $${DOUBLE_ESCAPED_QUOTE}\$${LITERAL_HASH}include \\\"$$header_path\\\"$${DOUBLE_ESCAPED_QUOTE} > $$eval($${header_target}.target))

    GENERATORS += $$header_target
}

