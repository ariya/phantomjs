# -------------------------------------------------------------------
# Derived sources for WebCore
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

# This file is both a top level target, and included from Target.pri,
# so that the resulting generated sources can be added to SOURCES.
# We only set the template if we're a top level target, so that we
# don't override what Target.pri has already set.
sanitizedFile = $$toSanitizedPath($$_FILE_)
equals(sanitizedFile, $$toSanitizedPath($$_PRO_FILE_)):TEMPLATE = derived

mac {
    # FIXME: This runs the perl script every time. Is there a way we can run it only when deps change?
    fwheader_generator.commands = perl $${ROOT_WEBKIT_DIR}/Source/WebKit2/Scripts/generate-forwarding-headers.pl $${ROOT_WEBKIT_DIR}/Source/WebCore $${ROOT_BUILD_DIR}/Source/include mac
    fwheader_generator.depends = $${ROOT_WEBKIT_DIR}/Source/WebKit2/Scripts/generate-forwarding-headers.pl
    GENERATORS += fwheader_generator
}

MATHML_NAMES = $$PWD/mathml/mathtags.in

SVG_NAMES = $$PWD/svg/svgtags.in

XLINK_NAMES = $$PWD/svg/xlinkattrs.in

CSSBISON = $$PWD/css/CSSGrammar.y.in

enable?(XSLT) {
    XMLVIEWER_CSS = $$PWD/xml/XMLViewer.css
    XMLVIEWER_JS = $$PWD/xml/XMLViewer.js
}

FONT_NAMES = $$PWD/css/WebKitFontFamilyNames.in

HTML_NAMES = $$PWD/html/HTMLTagNames.in

XML_NAMES = $$PWD/xml/xmlattrs.in

XMLNS_NAMES = $$PWD/xml/xmlnsattrs.in

HTML_ENTITIES = $$PWD/html/parser/HTMLEntityNames.in

EVENTS_NAMES = $$PWD/dom/EventNames.in

EVENT_TARGET_FACTORY = $$PWD/dom/EventTargetFactory.in

DOM_EXCEPTIONS = $$PWD/dom/DOMExceptions.in

SETTINGS_MACROS = $$PWD/page/Settings.in

COLORDATA_GPERF = $$PWD/platform/ColorData.gperf

WALDOCSSPROPS = $$PWD/css/CSSPropertyNames.in

WALDOCSSVALUES = $$PWD/css/CSSValueKeywords.in

INSPECTOR_JSON = $$PWD/inspector/Inspector.json

INSPECTOR_BACKEND_COMMANDS_QRC = $$PWD/inspector/front-end/InspectorBackendCommands.qrc

INSPECTOR_OVERLAY_PAGE = $$PWD/inspector/InspectorOverlayPage.html

INJECTED_SCRIPT_SOURCE = $$PWD/inspector/InjectedScriptSource.js

INJECTED_SCRIPT_CANVAS_MODULE_SOURCE = $$PWD/inspector/InjectedScriptCanvasModuleSource.js

XPATHBISON = $$PWD/xml/XPathGrammar.y

enable?(SVG) {
    EXTRACSSPROPERTIES += $$PWD/css/SVGCSSPropertyNames.in
    EXTRACSSVALUES += $$PWD/css/SVGCSSValueKeywords.in
}

STYLESHEETS_EMBED = \
    $$PWD/css/html.css \
    $$PWD/css/quirks.css \
    $$PWD/css/mathml.css \
    $$PWD/css/svg.css \
    $$PWD/css/view-source.css \
    $$PWD/css/fullscreen.css \
    $$PWD/css/mediaControls.css \
    $$PWD/css/mediaControlsQt.css \
    $$PWD/css/mediaControlsQtFullscreen.css \
    $$PWD/css/plugIns.css \
    $$PWD/css/themeQtNoListboxes.css \
    $$PWD/css/mobileThemeQt.css

PLUGINS_EMBED = \
    $$PWD/Resources/plugIns.js

IDL_BINDINGS += \
    $$PWD/Modules/filesystem/DOMFileSystem.idl \
    $$PWD/Modules/filesystem/DOMFileSystemSync.idl \
    $$PWD/Modules/filesystem/DOMWindowFileSystem.idl \
    $$PWD/Modules/filesystem/DirectoryEntry.idl \
    $$PWD/Modules/filesystem/DirectoryEntrySync.idl \
    $$PWD/Modules/filesystem/DirectoryReader.idl \
    $$PWD/Modules/filesystem/DirectoryReaderSync.idl \
    $$PWD/Modules/filesystem/EntriesCallback.idl \
    $$PWD/Modules/filesystem/Entry.idl \
    $$PWD/Modules/filesystem/EntryArray.idl \
    $$PWD/Modules/filesystem/EntryArraySync.idl \
    $$PWD/Modules/filesystem/EntryCallback.idl \
    $$PWD/Modules/filesystem/EntrySync.idl \
    $$PWD/Modules/filesystem/ErrorCallback.idl \
    $$PWD/Modules/filesystem/FileCallback.idl \
    $$PWD/Modules/filesystem/FileEntry.idl \
    $$PWD/Modules/filesystem/FileEntrySync.idl \
    $$PWD/Modules/filesystem/FileSystemCallback.idl \
    $$PWD/Modules/filesystem/FileWriter.idl \
    $$PWD/Modules/filesystem/FileWriterCallback.idl \
    $$PWD/Modules/filesystem/Metadata.idl \
    $$PWD/Modules/filesystem/MetadataCallback.idl \
    $$PWD/Modules/filesystem/WorkerGlobalScopeFileSystem.idl \
    $$PWD/Modules/geolocation/Coordinates.idl \
    $$PWD/Modules/geolocation/Geolocation.idl \
    $$PWD/Modules/geolocation/Geoposition.idl \
    $$PWD/Modules/geolocation/NavigatorGeolocation.idl \
    $$PWD/Modules/geolocation/PositionCallback.idl \
    $$PWD/Modules/geolocation/PositionError.idl \
    $$PWD/Modules/geolocation/PositionErrorCallback.idl \
    $$PWD/Modules/indexeddb/DOMWindowIndexedDatabase.idl \
    $$PWD/Modules/indexeddb/IDBAny.idl \
    $$PWD/Modules/indexeddb/IDBCursor.idl \
    $$PWD/Modules/indexeddb/IDBCursorWithValue.idl \
    $$PWD/Modules/indexeddb/IDBDatabase.idl \
    $$PWD/Modules/indexeddb/IDBFactory.idl \
    $$PWD/Modules/indexeddb/IDBIndex.idl \
    $$PWD/Modules/indexeddb/IDBKeyRange.idl \
    $$PWD/Modules/indexeddb/IDBObjectStore.idl \
    $$PWD/Modules/indexeddb/IDBOpenDBRequest.idl \
    $$PWD/Modules/indexeddb/IDBRequest.idl \
    $$PWD/Modules/indexeddb/IDBTransaction.idl \
    $$PWD/Modules/indexeddb/IDBVersionChangeEvent.idl \
    $$PWD/Modules/indexeddb/WorkerGlobalScopeIndexedDatabase.idl \
    $$PWD/Modules/notifications/DOMWindowNotifications.idl \
    $$PWD/Modules/notifications/Notification.idl \
    $$PWD/Modules/notifications/NotificationCenter.idl \
    $$PWD/Modules/notifications/NotificationPermissionCallback.idl \
    $$PWD/Modules/notifications/WorkerGlobalScopeNotifications.idl \
    $$PWD/Modules/quota/DOMWindowQuota.idl \
    $$PWD/Modules/quota/NavigatorStorageQuota.idl \
    $$PWD/Modules/quota/StorageInfo.idl \
    $$PWD/Modules/quota/StorageErrorCallback.idl \
    $$PWD/Modules/quota/StorageQuota.idl \
    $$PWD/Modules/quota/StorageQuotaCallback.idl \
    $$PWD/Modules/quota/StorageUsageCallback.idl \
    $$PWD/Modules/quota/WorkerNavigatorStorageQuota.idl \
    $$PWD/Modules/webaudio/AudioBuffer.idl \
    $$PWD/Modules/webaudio/AudioBufferCallback.idl \
    $$PWD/Modules/webaudio/AudioBufferSourceNode.idl \
    $$PWD/Modules/webaudio/ChannelMergerNode.idl \
    $$PWD/Modules/webaudio/ChannelSplitterNode.idl \
    $$PWD/Modules/webaudio/AudioContext.idl \
    $$PWD/Modules/webaudio/AudioDestinationNode.idl \
    $$PWD/Modules/webaudio/GainNode.idl \
    $$PWD/Modules/webaudio/AudioListener.idl \
    $$PWD/Modules/webaudio/AudioNode.idl \
    $$PWD/Modules/webaudio/PannerNode.idl \
    $$PWD/Modules/webaudio/AudioParam.idl \
    $$PWD/Modules/webaudio/AudioProcessingEvent.idl \
    $$PWD/Modules/webaudio/BiquadFilterNode.idl \
    $$PWD/Modules/webaudio/ConvolverNode.idl \
    $$PWD/Modules/webaudio/DelayNode.idl \
    $$PWD/Modules/webaudio/DynamicsCompressorNode.idl \
    $$PWD/Modules/webaudio/ScriptProcessorNode.idl \
    $$PWD/Modules/webaudio/MediaElementAudioSourceNode.idl \
    $$PWD/Modules/webaudio/MediaStreamAudioSourceNode.idl \
    $$PWD/Modules/webaudio/OfflineAudioContext.idl \
    $$PWD/Modules/webaudio/OfflineAudioCompletionEvent.idl \
    $$PWD/Modules/webaudio/OscillatorNode.idl \
    $$PWD/Modules/webaudio/AnalyserNode.idl \
    $$PWD/Modules/webaudio/WaveShaperNode.idl \
    $$PWD/Modules/webaudio/PeriodicWave.idl \
    $$PWD/Modules/webdatabase/DOMWindowWebDatabase.idl \
    $$PWD/Modules/webdatabase/Database.idl \
    $$PWD/Modules/webdatabase/DatabaseCallback.idl \
    $$PWD/Modules/webdatabase/DatabaseSync.idl \
    $$PWD/Modules/webdatabase/SQLError.idl \
    $$PWD/Modules/webdatabase/SQLException.idl \
    $$PWD/Modules/webdatabase/SQLResultSet.idl \
    $$PWD/Modules/webdatabase/SQLResultSetRowList.idl \
    $$PWD/Modules/webdatabase/SQLStatementCallback.idl \
    $$PWD/Modules/webdatabase/SQLStatementErrorCallback.idl \
    $$PWD/Modules/webdatabase/SQLTransaction.idl \
    $$PWD/Modules/webdatabase/SQLTransactionCallback.idl \
    $$PWD/Modules/webdatabase/SQLTransactionErrorCallback.idl \
    $$PWD/Modules/webdatabase/SQLTransactionSync.idl \
    $$PWD/Modules/webdatabase/SQLTransactionSyncCallback.idl \
    $$PWD/Modules/webdatabase/WorkerGlobalScopeWebDatabase.idl \
    $$PWD/Modules/websockets/CloseEvent.idl \
    $$PWD/Modules/websockets/WebSocket.idl \
    $$PWD/css/Counter.idl \
    $$PWD/css/CSSCharsetRule.idl \
    $$PWD/css/CSSFontFaceLoadEvent.idl \
    $$PWD/css/CSSFontFaceRule.idl \
    $$PWD/css/CSSHostRule.idl \
    $$PWD/css/CSSImportRule.idl \
    $$PWD/css/CSSMediaRule.idl \
    $$PWD/css/CSSPageRule.idl \
    $$PWD/css/CSSPrimitiveValue.idl \
    $$PWD/css/CSSRule.idl \
    $$PWD/css/CSSRuleList.idl \
    $$PWD/css/CSSStyleDeclaration.idl \
    $$PWD/css/CSSStyleRule.idl \
    $$PWD/css/CSSStyleSheet.idl \
    $$PWD/css/CSSSupportsRule.idl \
    $$PWD/css/CSSValue.idl \
    $$PWD/css/CSSValueList.idl \
    $$PWD/css/DOMWindowCSS.idl \
    $$PWD/css/FontLoader.idl \
    $$PWD/css/MediaList.idl \
    $$PWD/css/MediaQueryList.idl \
    $$PWD/css/Rect.idl \
    $$PWD/css/RGBColor.idl \
    $$PWD/css/StyleMedia.idl \
    $$PWD/css/StyleSheet.idl \
    $$PWD/css/StyleSheetList.idl \
    $$PWD/css/WebKitCSSFilterRule.idl \
    $$PWD/css/WebKitCSSFilterValue.idl \
    $$PWD/css/WebKitCSSKeyframeRule.idl \
    $$PWD/css/WebKitCSSKeyframesRule.idl \
    $$PWD/css/WebKitCSSMatrix.idl \
    $$PWD/css/WebKitCSSMixFunctionValue.idl \
    $$PWD/css/WebKitCSSRegionRule.idl \
    $$PWD/css/WebKitCSSTransformValue.idl \
    $$PWD/css/WebKitCSSViewportRule.idl \
    $$PWD/dom/Attr.idl \
    $$PWD/dom/BeforeLoadEvent.idl \
    $$PWD/dom/CharacterData.idl \
    $$PWD/dom/ChildNode.idl \
    $$PWD/dom/ClientRect.idl \
    $$PWD/dom/ClientRectList.idl \
    $$PWD/dom/Clipboard.idl \
    $$PWD/dom/CDATASection.idl \
    $$PWD/dom/Comment.idl \
    $$PWD/dom/CompositionEvent.idl \
    $$PWD/dom/CustomEvent.idl \
    $$PWD/dom/DataTransferItem.idl \
    $$PWD/dom/DataTransferItemList.idl \
    $$PWD/dom/DeviceMotionEvent.idl \
    $$PWD/dom/DeviceOrientationEvent.idl \
    $$PWD/dom/DocumentFragment.idl \
    $$PWD/dom/Document.idl \
    $$PWD/dom/DocumentType.idl \
    $$PWD/dom/DOMCoreException.idl \
    $$PWD/dom/DOMError.idl \
    $$PWD/dom/DOMImplementation.idl \
    $$PWD/dom/DOMStringList.idl \
    $$PWD/dom/DOMStringMap.idl \
    $$PWD/dom/Element.idl \
    $$PWD/dom/Entity.idl \
    $$PWD/dom/EntityReference.idl \
    $$PWD/dom/ErrorEvent.idl \
    $$PWD/dom/Event.idl \
    $$PWD/dom/EventException.idl \
#    $$PWD/dom/EventListener.idl \
    $$PWD/dom/EventTarget.idl \
    $$PWD/dom/FocusEvent.idl \
    $$PWD/dom/HashChangeEvent.idl \
    $$PWD/dom/KeyboardEvent.idl \
    $$PWD/dom/MouseEvent.idl \
    $$PWD/dom/MessageChannel.idl \
    $$PWD/dom/MessageEvent.idl \
    $$PWD/dom/MessagePort.idl \
    $$PWD/dom/MutationEvent.idl \
    $$PWD/dom/MutationObserver.idl \
    $$PWD/dom/MutationRecord.idl \
    $$PWD/dom/NamedNodeMap.idl \
    $$PWD/dom/Node.idl \
    $$PWD/dom/NodeFilter.idl \
    $$PWD/dom/NodeIterator.idl \
    $$PWD/dom/NodeList.idl \
    $$PWD/dom/Notation.idl \
    $$PWD/dom/OverflowEvent.idl \
    $$PWD/dom/PageTransitionEvent.idl \
    $$PWD/dom/PopStateEvent.idl \
    $$PWD/dom/ProcessingInstruction.idl \
    $$PWD/dom/ProgressEvent.idl \
    $$PWD/dom/PropertyNodeList.idl \
    $$PWD/dom/RangeException.idl \
    $$PWD/dom/Range.idl \
    $$PWD/dom/RequestAnimationFrameCallback.idl \
    $$PWD/dom/ShadowRoot.idl \
    $$PWD/dom/StringCallback.idl \
    $$PWD/dom/Text.idl \
    $$PWD/dom/TextEvent.idl \
    $$PWD/dom/Touch.idl \
    $$PWD/dom/TouchEvent.idl \
    $$PWD/dom/TouchList.idl \
    $$PWD/dom/TransitionEvent.idl \
    $$PWD/dom/TreeWalker.idl \
    $$PWD/dom/UIEvent.idl \
    $$PWD/dom/WebKitAnimationEvent.idl \
    $$PWD/dom/WebKitNamedFlow.idl \
    $$PWD/dom/DOMNamedFlowCollection.idl \
    $$PWD/dom/WebKitTransitionEvent.idl \
    $$PWD/dom/WheelEvent.idl \
    $$PWD/fileapi/Blob.idl \
    $$PWD/fileapi/File.idl \
    $$PWD/fileapi/FileError.idl \
    $$PWD/fileapi/FileException.idl \
    $$PWD/fileapi/FileList.idl \
    $$PWD/fileapi/FileReader.idl \
    $$PWD/fileapi/FileReaderSync.idl \
    $$PWD/html/canvas/ArrayBufferView.idl \
    $$PWD/html/canvas/ArrayBuffer.idl \
    $$PWD/html/canvas/DataView.idl \
    $$PWD/html/canvas/Int8Array.idl \
    $$PWD/html/canvas/Float32Array.idl \
    $$PWD/html/canvas/Float64Array.idl \
    $$PWD/html/canvas/CanvasGradient.idl \
    $$PWD/html/canvas/Int32Array.idl \
    $$PWD/html/canvas/CanvasPattern.idl \
    $$PWD/html/canvas/CanvasProxy.idl \
    $$PWD/html/canvas/CanvasRenderingContext.idl \
    $$PWD/html/canvas/CanvasRenderingContext2D.idl \
    $$PWD/html/canvas/DOMPath.idl \
    $$PWD/html/canvas/EXTDrawBuffers.idl \
    $$PWD/html/canvas/EXTTextureFilterAnisotropic.idl \
    $$PWD/html/canvas/OESStandardDerivatives.idl \
    $$PWD/html/canvas/OESTextureFloat.idl \
    $$PWD/html/canvas/OESTextureHalfFloat.idl \
    $$PWD/html/canvas/OESVertexArrayObject.idl \
    $$PWD/html/canvas/OESElementIndexUint.idl \
    $$PWD/html/canvas/WebGLActiveInfo.idl \
    $$PWD/html/canvas/WebGLBuffer.idl \
    $$PWD/html/canvas/WebGLCompressedTextureATC.idl \
    $$PWD/html/canvas/WebGLCompressedTexturePVRTC.idl \
    $$PWD/html/canvas/WebGLCompressedTextureS3TC.idl \
    $$PWD/html/canvas/WebGLContextAttributes.idl \
    $$PWD/html/canvas/WebGLContextEvent.idl \
    $$PWD/html/canvas/WebGLDebugRendererInfo.idl \
    $$PWD/html/canvas/WebGLDebugShaders.idl \
    $$PWD/html/canvas/WebGLDepthTexture.idl \
    $$PWD/html/canvas/WebGLFramebuffer.idl \
    $$PWD/html/canvas/WebGLLoseContext.idl \
    $$PWD/html/canvas/WebGLProgram.idl \
    $$PWD/html/canvas/WebGLRenderbuffer.idl \
    $$PWD/html/canvas/WebGLRenderingContext.idl \
    $$PWD/html/canvas/WebGLShader.idl \
    $$PWD/html/canvas/WebGLShaderPrecisionFormat.idl \
    $$PWD/html/canvas/Int16Array.idl \
    $$PWD/html/canvas/WebGLTexture.idl \
    $$PWD/html/canvas/WebGLUniformLocation.idl \
    $$PWD/html/canvas/WebGLVertexArrayObjectOES.idl \
    $$PWD/html/canvas/Uint8Array.idl \
    $$PWD/html/canvas/Uint8ClampedArray.idl \
    $$PWD/html/canvas/Uint32Array.idl \
    $$PWD/html/canvas/Uint16Array.idl \
    $$PWD/html/DOMFormData.idl \
    $$PWD/html/DOMSettableTokenList.idl \
    $$PWD/html/DOMTokenList.idl \
    $$PWD/html/DOMURL.idl \
    $$PWD/html/HTMLAllCollection.idl \
    $$PWD/html/HTMLAudioElement.idl \
    $$PWD/html/HTMLAnchorElement.idl \
    $$PWD/html/HTMLAppletElement.idl \
    $$PWD/html/HTMLAreaElement.idl \
    $$PWD/html/HTMLBaseElement.idl \
    $$PWD/html/HTMLBaseFontElement.idl \
    $$PWD/html/HTMLBodyElement.idl \
    $$PWD/html/HTMLBRElement.idl \
    $$PWD/html/HTMLButtonElement.idl \
    $$PWD/html/HTMLCanvasElement.idl \
    $$PWD/html/HTMLCollection.idl \
    $$PWD/html/HTMLDataListElement.idl \
    $$PWD/html/HTMLDetailsElement.idl \
    $$PWD/html/HTMLDialogElement.idl \
    $$PWD/html/HTMLDirectoryElement.idl \
    $$PWD/html/HTMLDivElement.idl \
    $$PWD/html/HTMLDListElement.idl \
    $$PWD/html/HTMLDocument.idl \
    $$PWD/html/HTMLElement.idl \
    $$PWD/html/HTMLEmbedElement.idl \
    $$PWD/html/HTMLFieldSetElement.idl \
    $$PWD/html/HTMLFontElement.idl \
    $$PWD/html/HTMLFormControlsCollection.idl \
    $$PWD/html/HTMLFormElement.idl \
    $$PWD/html/HTMLFrameElement.idl \
    $$PWD/html/HTMLFrameSetElement.idl \
    $$PWD/html/HTMLHeadElement.idl \
    $$PWD/html/HTMLHeadingElement.idl \
    $$PWD/html/HTMLHRElement.idl \
    $$PWD/html/HTMLHtmlElement.idl \
    $$PWD/html/HTMLIFrameElement.idl \
    $$PWD/html/HTMLImageElement.idl \
    $$PWD/html/HTMLInputElement.idl \
    $$PWD/html/HTMLKeygenElement.idl \
    $$PWD/html/HTMLLabelElement.idl \
    $$PWD/html/HTMLLegendElement.idl \
    $$PWD/html/HTMLLIElement.idl \
    $$PWD/html/HTMLLinkElement.idl \
    $$PWD/html/HTMLMapElement.idl \
    $$PWD/html/HTMLMarqueeElement.idl \
    $$PWD/html/HTMLMediaElement.idl \
    $$PWD/html/HTMLMenuElement.idl \
    $$PWD/html/HTMLMetaElement.idl \
    $$PWD/html/HTMLMeterElement.idl \
    $$PWD/html/HTMLModElement.idl \
    $$PWD/html/HTMLObjectElement.idl \
    $$PWD/html/HTMLOListElement.idl \
    $$PWD/html/HTMLOptGroupElement.idl \
    $$PWD/html/HTMLOptionElement.idl \
    $$PWD/html/HTMLOptionsCollection.idl \
    $$PWD/html/HTMLOutputElement.idl \
    $$PWD/html/HTMLParagraphElement.idl \
    $$PWD/html/HTMLParamElement.idl \
    $$PWD/html/HTMLPreElement.idl \
    $$PWD/html/HTMLProgressElement.idl \
    $$PWD/html/HTMLPropertiesCollection.idl \
    $$PWD/html/HTMLQuoteElement.idl \
    $$PWD/html/HTMLScriptElement.idl \
    $$PWD/html/HTMLSelectElement.idl \
    $$PWD/html/HTMLSourceElement.idl \
    $$PWD/html/HTMLSpanElement.idl \
    $$PWD/html/HTMLStyleElement.idl \
    $$PWD/html/HTMLTableCaptionElement.idl \
    $$PWD/html/HTMLTableCellElement.idl \
    $$PWD/html/HTMLTableColElement.idl \
    $$PWD/html/HTMLTableElement.idl \
    $$PWD/html/HTMLTableRowElement.idl \
    $$PWD/html/HTMLTableSectionElement.idl \
    $$PWD/html/HTMLTextAreaElement.idl \
    $$PWD/html/HTMLTitleElement.idl \
    $$PWD/html/HTMLTrackElement.idl \
    $$PWD/html/HTMLUListElement.idl \
    $$PWD/html/HTMLUnknownElement.idl \
    $$PWD/html/HTMLVideoElement.idl \
    $$PWD/html/ImageData.idl \
    $$PWD/html/MediaController.idl \
    $$PWD/html/MediaError.idl \
    $$PWD/html/MicroDataItemValue.idl \
    $$PWD/html/RadioNodeList.idl \
    $$PWD/html/TextMetrics.idl \
    $$PWD/html/TimeRanges.idl \
    $$PWD/html/ValidityState.idl \
    $$PWD/html/VoidCallback.idl \
    $$PWD/html/shadow/HTMLContentElement.idl \
    $$PWD/inspector/InjectedScriptHost.idl \
    $$PWD/inspector/InspectorFrontendHost.idl \
    $$PWD/inspector/JavaScriptCallFrame.idl \
    $$PWD/inspector/ScriptProfile.idl \
    $$PWD/inspector/ScriptProfileNode.idl \
    $$PWD/loader/appcache/DOMApplicationCache.idl \
    $$PWD/page/BarProp.idl \
    $$PWD/page/Console.idl \
    $$PWD/page/Crypto.idl \
    $$PWD/page/DOMSecurityPolicy.idl \
    $$PWD/page/DOMSelection.idl \
    $$PWD/page/DOMWindow.idl \
    $$PWD/page/EventSource.idl \
    $$PWD/page/History.idl \
    $$PWD/page/Location.idl \
    $$PWD/page/Navigator.idl \
    $$PWD/page/Performance.idl \
    $$PWD/page/PerformanceEntry.idl \
    $$PWD/page/PerformanceEntryList.idl \
    $$PWD/page/PerformanceNavigation.idl \
    $$PWD/page/PerformanceResourceTiming.idl \
    $$PWD/page/PerformanceTiming.idl \
    $$PWD/page/Screen.idl \
    $$PWD/page/SpeechInputEvent.idl \
    $$PWD/page/SpeechInputResult.idl \
    $$PWD/page/SpeechInputResultList.idl \
    $$PWD/page/WebKitPoint.idl \
    $$PWD/page/WindowBase64.idl \
    $$PWD/page/WindowTimers.idl \
    $$PWD/page/WorkerNavigator.idl \
    $$PWD/plugins/DOMPlugin.idl \
    $$PWD/plugins/DOMMimeType.idl \
    $$PWD/plugins/DOMPluginArray.idl \
    $$PWD/plugins/DOMMimeTypeArray.idl \
    $$PWD/storage/Storage.idl \
    $$PWD/storage/StorageEvent.idl \
    $$PWD/testing/Internals.idl \
    $$PWD/testing/InternalSettings.idl \
    $$PWD/testing/MallocStatistics.idl \
    $$PWD/testing/MemoryInfo.idl \
    $$PWD/testing/TypeConversions.idl \
    $$PWD/workers/AbstractWorker.idl \
    $$PWD/workers/DedicatedWorkerGlobalScope.idl \
    $$PWD/workers/SharedWorker.idl \
    $$PWD/workers/SharedWorkerGlobalScope.idl \
    $$PWD/workers/Worker.idl \
    $$PWD/workers/WorkerGlobalScope.idl \
    $$PWD/workers/WorkerLocation.idl \
    $$PWD/xml/DOMParser.idl \
    $$PWD/xml/XMLHttpRequest.idl \
    $$PWD/xml/XMLHttpRequestException.idl \
    $$PWD/xml/XMLHttpRequestProgressEvent.idl \
    $$PWD/xml/XMLHttpRequestUpload.idl \
    $$PWD/xml/XMLSerializer.idl \
    $$PWD/xml/XPathNSResolver.idl \
    $$PWD/xml/XPathException.idl \
    $$PWD/xml/XPathExpression.idl \
    $$PWD/xml/XPathResult.idl \
    $$PWD/xml/XPathEvaluator.idl \
    $$PWD/xml/XSLTProcessor.idl

enable?(SVG) {
  IDL_BINDINGS += \
    $$PWD/svg/SVGAElement.idl \
    $$PWD/svg/SVGAltGlyphDefElement.idl \
    $$PWD/svg/SVGAltGlyphElement.idl \
    $$PWD/svg/SVGAltGlyphItemElement.idl \
    $$PWD/svg/SVGAngle.idl \
    $$PWD/svg/SVGAnimateColorElement.idl \
    $$PWD/svg/SVGAnimateMotionElement.idl \
    $$PWD/svg/SVGAnimatedAngle.idl \
    $$PWD/svg/SVGAnimatedBoolean.idl \
    $$PWD/svg/SVGAnimatedEnumeration.idl \
    $$PWD/svg/SVGAnimatedInteger.idl \
    $$PWD/svg/SVGAnimatedLength.idl \
    $$PWD/svg/SVGAnimatedLengthList.idl \
    $$PWD/svg/SVGAnimatedNumber.idl \
    $$PWD/svg/SVGAnimatedNumberList.idl \
    $$PWD/svg/SVGAnimatedPreserveAspectRatio.idl \
    $$PWD/svg/SVGAnimatedRect.idl \
    $$PWD/svg/SVGAnimatedString.idl \
    $$PWD/svg/SVGAnimatedTransformList.idl \
    $$PWD/svg/SVGAnimateElement.idl \
    $$PWD/svg/SVGAnimateTransformElement.idl \
    $$PWD/svg/SVGAnimationElement.idl \
    $$PWD/svg/SVGCircleElement.idl \
    $$PWD/svg/SVGClipPathElement.idl \
    $$PWD/svg/SVGColor.idl \
    $$PWD/svg/SVGComponentTransferFunctionElement.idl \
    $$PWD/svg/SVGCursorElement.idl \
    $$PWD/svg/SVGDefsElement.idl \
    $$PWD/svg/SVGDescElement.idl \
    $$PWD/svg/SVGDocument.idl \
    $$PWD/svg/SVGElement.idl \
    $$PWD/svg/SVGElementInstance.idl \
    $$PWD/svg/SVGElementInstanceList.idl \
    $$PWD/svg/SVGEllipseElement.idl \
    $$PWD/svg/SVGException.idl \
    $$PWD/svg/SVGExternalResourcesRequired.idl \
    $$PWD/svg/SVGFEBlendElement.idl \
    $$PWD/svg/SVGFEColorMatrixElement.idl \
    $$PWD/svg/SVGFEComponentTransferElement.idl \
    $$PWD/svg/SVGFECompositeElement.idl \
    $$PWD/svg/SVGFEConvolveMatrixElement.idl \
    $$PWD/svg/SVGFEDiffuseLightingElement.idl \
    $$PWD/svg/SVGFEDisplacementMapElement.idl \
    $$PWD/svg/SVGFEDistantLightElement.idl \
    $$PWD/svg/SVGFEDropShadowElement.idl \
    $$PWD/svg/SVGFEFloodElement.idl \
    $$PWD/svg/SVGFEFuncAElement.idl \
    $$PWD/svg/SVGFEFuncBElement.idl \
    $$PWD/svg/SVGFEFuncGElement.idl \
    $$PWD/svg/SVGFEFuncRElement.idl \
    $$PWD/svg/SVGFEGaussianBlurElement.idl \
    $$PWD/svg/SVGFEImageElement.idl \
    $$PWD/svg/SVGFEMergeElement.idl \
    $$PWD/svg/SVGFEMergeNodeElement.idl \
    $$PWD/svg/SVGFEMorphologyElement.idl \
    $$PWD/svg/SVGFEOffsetElement.idl \
    $$PWD/svg/SVGFEPointLightElement.idl \
    $$PWD/svg/SVGFESpecularLightingElement.idl \
    $$PWD/svg/SVGFESpotLightElement.idl \
    $$PWD/svg/SVGFETileElement.idl \
    $$PWD/svg/SVGFETurbulenceElement.idl \
    $$PWD/svg/SVGFilterElement.idl \
    $$PWD/svg/SVGFilterPrimitiveStandardAttributes.idl \
    $$PWD/svg/SVGFitToViewBox.idl \
    $$PWD/svg/SVGFontElement.idl \
    $$PWD/svg/SVGFontFaceElement.idl \
    $$PWD/svg/SVGFontFaceFormatElement.idl \
    $$PWD/svg/SVGFontFaceNameElement.idl \
    $$PWD/svg/SVGFontFaceSrcElement.idl \
    $$PWD/svg/SVGFontFaceUriElement.idl \
    $$PWD/svg/SVGForeignObjectElement.idl \
    $$PWD/svg/SVGGElement.idl \
    $$PWD/svg/SVGGlyphElement.idl \
    $$PWD/svg/SVGGlyphRefElement.idl \
    $$PWD/svg/SVGGradientElement.idl \
    $$PWD/svg/SVGGraphicsElement.idl \
    $$PWD/svg/SVGHKernElement.idl \
    $$PWD/svg/SVGImageElement.idl \
    $$PWD/svg/SVGLength.idl \
    $$PWD/svg/SVGLengthList.idl \
    $$PWD/svg/SVGLinearGradientElement.idl \
    $$PWD/svg/SVGLineElement.idl \
    $$PWD/svg/SVGMarkerElement.idl \
    $$PWD/svg/SVGMaskElement.idl \
    $$PWD/svg/SVGMatrix.idl \
    $$PWD/svg/SVGMetadataElement.idl \
    $$PWD/svg/SVGMissingGlyphElement.idl \
    $$PWD/svg/SVGMPathElement.idl \
    $$PWD/svg/SVGNumber.idl \
    $$PWD/svg/SVGNumberList.idl \
    $$PWD/svg/SVGPaint.idl \
    $$PWD/svg/SVGPathElement.idl \
    $$PWD/svg/SVGPathSegArcAbs.idl \
    $$PWD/svg/SVGPathSegArcRel.idl \
    $$PWD/svg/SVGPathSegClosePath.idl \
    $$PWD/svg/SVGPathSegCurvetoCubicAbs.idl \
    $$PWD/svg/SVGPathSegCurvetoCubicRel.idl \
    $$PWD/svg/SVGPathSegCurvetoCubicSmoothAbs.idl \
    $$PWD/svg/SVGPathSegCurvetoCubicSmoothRel.idl \
    $$PWD/svg/SVGPathSegCurvetoQuadraticAbs.idl \
    $$PWD/svg/SVGPathSegCurvetoQuadraticRel.idl \
    $$PWD/svg/SVGPathSegCurvetoQuadraticSmoothAbs.idl \
    $$PWD/svg/SVGPathSegCurvetoQuadraticSmoothRel.idl \
    $$PWD/svg/SVGPathSeg.idl \
    $$PWD/svg/SVGPathSegLinetoAbs.idl \
    $$PWD/svg/SVGPathSegLinetoHorizontalAbs.idl \
    $$PWD/svg/SVGPathSegLinetoHorizontalRel.idl \
    $$PWD/svg/SVGPathSegLinetoRel.idl \
    $$PWD/svg/SVGPathSegLinetoVerticalAbs.idl \
    $$PWD/svg/SVGPathSegLinetoVerticalRel.idl \
    $$PWD/svg/SVGPathSegList.idl \
    $$PWD/svg/SVGPathSegMovetoAbs.idl \
    $$PWD/svg/SVGPathSegMovetoRel.idl \
    $$PWD/svg/SVGPatternElement.idl \
    $$PWD/svg/SVGPoint.idl \
    $$PWD/svg/SVGPointList.idl \
    $$PWD/svg/SVGPolygonElement.idl \
    $$PWD/svg/SVGPolylineElement.idl \
    $$PWD/svg/SVGPreserveAspectRatio.idl \
    $$PWD/svg/SVGRadialGradientElement.idl \
    $$PWD/svg/SVGRectElement.idl \
    $$PWD/svg/SVGRect.idl \
    $$PWD/svg/SVGRenderingIntent.idl \
    $$PWD/svg/SVGScriptElement.idl \
    $$PWD/svg/SVGSetElement.idl \
    $$PWD/svg/SVGStopElement.idl \
    $$PWD/svg/SVGStringList.idl \
    $$PWD/svg/SVGStyleElement.idl \
    $$PWD/svg/SVGStyledElement.idl \
    $$PWD/svg/SVGSVGElement.idl \
    $$PWD/svg/SVGSwitchElement.idl \
    $$PWD/svg/SVGSymbolElement.idl \
    $$PWD/svg/SVGTests.idl \
    $$PWD/svg/SVGTextContentElement.idl \
    $$PWD/svg/SVGTextElement.idl \
    $$PWD/svg/SVGTextPathElement.idl \
    $$PWD/svg/SVGTextPositioningElement.idl \
    $$PWD/svg/SVGTitleElement.idl \
    $$PWD/svg/SVGTransform.idl \
    $$PWD/svg/SVGTransformList.idl \
    $$PWD/svg/SVGTRefElement.idl \
    $$PWD/svg/SVGTSpanElement.idl \
    $$PWD/svg/SVGURIReference.idl \
    $$PWD/svg/SVGUnitTypes.idl \
    $$PWD/svg/SVGUseElement.idl \
    $$PWD/svg/SVGViewElement.idl \
    $$PWD/svg/SVGVKernElement.idl \
    $$PWD/svg/SVGViewSpec.idl \
    $$PWD/svg/SVGZoomAndPan.idl \
    $$PWD/svg/SVGZoomEvent.idl
}

enable?(GAMEPAD) {
  IDL_BINDINGS += \
    $$PWD/Modules/gamepad/Gamepad.idl \
    $$PWD/Modules/gamepad/GamepadList.idl \
    $$PWD/Modules/gamepad/NavigatorGamepad.idl
}

enable?(VIDEO_TRACK) {
  IDL_BINDINGS += \
    $$PWD/html/track/AudioTrack.idl \
    $$PWD/html/track/AudioTrackList.idl \
    $$PWD/html/track/TextTrack.idl \
    $$PWD/html/track/TextTrackCue.idl \
    $$PWD/html/track/TextTrackCueList.idl \
    $$PWD/html/track/TextTrackList.idl \
    $$PWD/html/track/TrackEvent.idl \
    $$PWD/html/track/VideoTrack.idl \
    $$PWD/html/track/VideoTrackList.idl
}

enable?(MEDIA_SOURCE) {
  IDL_BINDINGS += \
    $$PWD/Modules/mediasource/MediaSource.idl \
    $$PWD/Modules/mediasource/SourceBuffer.idl \
    $$PWD/Modules/mediasource/SourceBufferList.idl
}

qtPrepareTool(QMAKE_MOC, moc)

mathmlnames.output = MathMLNames.cpp
mathmlnames.input = MATHML_NAMES
mathmlnames.depends = $$PWD/mathml/mathattrs.in
mathmlnames.script = $$PWD/dom/make_names.pl
mathmlnames.commands = perl -I$$PWD/bindings/scripts $$mathmlnames.script --tags $$PWD/mathml/mathtags.in --attrs $$PWD/mathml/mathattrs.in --extraDefines \"$${DEFINES} $$configDefines()\" --preprocessor \"$${QMAKE_MOC} -E\" --factory --wrapperFactory --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
mathmlnames.extra_sources = MathMLElementFactory.cpp
GENERATORS += mathmlnames

# GENERATOR 5-C:
svgnames.output = SVGNames.cpp
svgnames.input = SVG_NAMES
svgnames.depends = $$PWD/svg/svgattrs.in
svgnames.script = $$PWD/dom/make_names.pl
svgnames.commands = perl -I$$PWD/bindings/scripts $$svgnames.script --tags $$PWD/svg/svgtags.in --attrs $$PWD/svg/svgattrs.in --extraDefines \"$${DEFINES} $$configDefines()\" --preprocessor \"$${QMAKE_MOC} -E\" --factory --wrapperFactory --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
svgnames.extra_sources = SVGElementFactory.cpp
    svgnames.extra_sources += JSSVGElementWrapperFactory.cpp
GENERATORS += svgnames

# GENERATOR 5-D:
xlinknames.output = XLinkNames.cpp
xlinknames.script = $$PWD/dom/make_names.pl
xlinknames.commands = perl -I$$PWD/bindings/scripts $$xlinknames.script --attrs $$PWD/svg/xlinkattrs.in --preprocessor \"$${QMAKE_MOC} -E\" --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
xlinknames.input = XLINK_NAMES
GENERATORS += xlinknames

# GENERATOR 6-A:
cssprops.script = $$PWD/css/makeprop.pl
cssprops.output = CSSPropertyNames.cpp
cssprops.input = WALDOCSSPROPS
cssprops.commands = perl -ne \"print $1\" ${QMAKE_FILE_NAME} $${EXTRACSSPROPERTIES} > ${QMAKE_FUNC_FILE_OUT_PATH}/${QMAKE_FILE_BASE}.in && cd ${QMAKE_FUNC_FILE_OUT_PATH} && perl -I$$PWD/bindings/scripts $$cssprops.script --defines \"$$javascriptFeatureDefines()\" --preprocessor \"$${QMAKE_MOC} -E\" ${QMAKE_FILE_NAME} && $(DEL_FILE) ${QMAKE_FILE_BASE}.in ${QMAKE_FILE_BASE}.gperf
cssprops.depends = ${QMAKE_FILE_NAME} $${EXTRACSSPROPERTIES} $$cssprops.script
GENERATORS += cssprops

# GENERATOR 6-B:
cssvalues.script = $$PWD/css/makevalues.pl
cssvalues.output = CSSValueKeywords.cpp
cssvalues.input = WALDOCSSVALUES
cssvalues.commands = perl -ne \"print $1\" ${QMAKE_FILE_NAME} $$EXTRACSSVALUES > ${QMAKE_FUNC_FILE_OUT_PATH}/${QMAKE_FILE_BASE}.in && cd ${QMAKE_FUNC_FILE_OUT_PATH} && perl -I$$PWD/bindings/scripts $$cssvalues.script --defines \"$$javascriptFeatureDefines()\" --preprocessor \"$${QMAKE_MOC} -E\" ${QMAKE_FILE_NAME} && $(DEL_FILE) ${QMAKE_FILE_BASE}.in ${QMAKE_FILE_BASE}.gperf
cssvalues.depends = ${QMAKE_FILE_NAME} $${EXTRACSSVALUES} $$cssvalues.script
cssvalues.clean = ${QMAKE_FILE_OUT} ${QMAKE_FUNC_FILE_OUT_PATH}/${QMAKE_FILE_BASE}.h
GENERATORS += cssvalues

INTERNAL_SETTINGS_GENERATED_IDL = InternalSettingsGenerated.idl
# GENERATOR 6-C:
settingsmacros.output = $$INTERNAL_SETTINGS_GENERATED_IDL InternalSettingsGenerated.cpp
settingsmacros.input = SETTINGS_MACROS
settingsmacros.script = $$PWD/page/make_settings.pl
settingsmacros.commands = perl -I$$PWD/bindings/scripts $$settingsmacros.script --input $$SETTINGS_MACROS --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
settingsmacros.depends = $$PWD/page/make_settings.pl $$SETTINGS_MACROS
settingsmacros.add_output_to_sources = false
settingsmacros.extra_sources = InternalSettingsGenerated.cpp
GENERATORS += settingsmacros

# make_settings.pl generates this file. We can't use ${QMAKE_FUNC_FILE_OUT_PATH} here since generateBindings.input
# doesn't know how to resolve ${QMAKE_FUNC_FILE_OUT_PATH}.
IDL_BINDINGS += generated/$$INTERNAL_SETTINGS_GENERATED_IDL

# GENERATOR 0: Resolve [Supplemental] dependency in IDLs
SUPPLEMENTAL_DEPENDENCY_FILE = supplemental_dependency.tmp
WINDOW_CONSTRUCTORS_FILE = DOMWindowConstructors.idl
WORKERGLOBALSCOPE_CONSTRUCTORS_FILE = WorkerGlobalScopeConstructors.idl
SHAREDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE = SharedWorkerGlobalScopeConstructors.idl
DEDICATEDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE = DedicatedWorkerGlobalScopeConstructors.idl
IDL_FILES_TMP = ${QMAKE_FUNC_FILE_OUT_PATH}/idl_files.tmp
PREPROCESS_IDLS_SCRIPT = $$PWD/bindings/scripts/preprocess-idls.pl
IDL_ATTRIBUTES_FILE = $$PWD/bindings/scripts/IDLAttributes.txt

preprocessIdls.input = IDL_ATTRIBUTES_FILE
preprocessIdls.script = $$PREPROCESS_IDLS_SCRIPT
# FIXME : We need to use only perl at some point.
win_cmd_shell: preprocessIdls.commands = type nul > $$IDL_FILES_TMP $$EOC
else: preprocessIdls.commands = cat /dev/null > $$IDL_FILES_TMP $$EOC
for(binding, IDL_BINDINGS) {
    # We need "$$binding" instead of "$$binding ", because Windows' echo writes trailing whitespaces. (http://wkb.ug/88304)
    # A space is omitted between "$$IDL_FILES_TMP" and "$$EOC" to also avoid writing trailing whitespace. (http://wkb.ug/95730)
    preprocessIdls.commands += echo $$binding>> $$IDL_FILES_TMP$$EOC
}
preprocessIdls.commands += perl -I$$PWD/bindings/scripts $$preprocessIdls.script \
                               --defines \"$$javascriptFeatureDefines()\" \
                               --idlFilesList $$IDL_FILES_TMP \
                               --supplementalDependencyFile ${QMAKE_FUNC_FILE_OUT_PATH}/$$SUPPLEMENTAL_DEPENDENCY_FILE \
                               --windowConstructorsFile ${QMAKE_FUNC_FILE_OUT_PATH}/$$WINDOW_CONSTRUCTORS_FILE \
                               --workerGlobalScopeConstructorsFile ${QMAKE_FUNC_FILE_OUT_PATH}/$$WORKERGLOBALSCOPE_CONSTRUCTORS_FILE \
                               --sharedWorkerGlobalScopeConstructorsFile ${QMAKE_FUNC_FILE_OUT_PATH}/$$SHAREDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE \
                               --dedicatedWorkerGlobalScopeConstructorsFile ${QMAKE_FUNC_FILE_OUT_PATH}/$$DEDICATEDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE
preprocessIdls.output = $$SUPPLEMENTAL_DEPENDENCY_FILE $$WINDOW_CONSTRUCTORS_FILE $$WORKERGLOBALSCOPE_CONSTRUCTORS_FILE $$SHAREDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE $$DEDICATEDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE
preprocessIdls.add_output_to_sources = false
preprocessIdls.depends = $$IDL_BINDINGS
GENERATORS += preprocessIdls

# GENERATOR 1: Generate .h and .cpp from IDLs
generateBindings.input = IDL_BINDINGS
generateBindings.script = $$PWD/bindings/scripts/generate-bindings.pl
generateBindings.commands = $$setEnvironmentVariable(SOURCE_ROOT, $$toSystemPath($$PWD)) && perl -I$$PWD/bindings/scripts $$generateBindings.script \
                            --defines \"$$javascriptFeatureDefines()\" \
                            --generator JS \
                            --include Modules/filesystem \
                            --include Modules/geolocation \
                            --include Modules/indexeddb \
                            --include Modules/mediasource \
                            --include Modules/notifications \
                            --include Modules/quota \
                            --include Modules/webaudio \
                            --include Modules/webdatabase \
                            --include Modules/websockets \
                            --include css \
                            --include dom \
                            --include editing \
                            --include fileapi \
                            --include html \
                            --include html/canvas \
                            --include html/shadow \
                            --include html/track \
                            --include inspector \
                            --include loader/appcache \
                            --include page \
                            --include plugins \
                            --include storage \
                            --include svg \
                            --include testing \
                            --include workers \
                            --include xml \
                            --outputDir ${QMAKE_FUNC_FILE_OUT_PATH} \
                            --supplementalDependencyFile ${QMAKE_FUNC_FILE_OUT_PATH}/$$SUPPLEMENTAL_DEPENDENCY_FILE \
                            --idlAttributesFile $${IDL_ATTRIBUTES_FILE} \
                            --preprocessor \"$${QMAKE_MOC} -E\" ${QMAKE_FILE_NAME}
generateBindings.output = JS${QMAKE_FILE_BASE}.cpp
generateBindings.depends = ${QMAKE_FUNC_FILE_OUT_PATH}/$$SUPPLEMENTAL_DEPENDENCY_FILE \
                           $$PWD/bindings/scripts/CodeGenerator.pm \
                           $$PWD/bindings/scripts/CodeGeneratorJS.pm \
                           $$PWD/bindings/scripts/IDLParser.pm \
                           $$PWD/bindings/scripts/InFilesParser.pm \
                           $$PWD/bindings/scripts/preprocessor.pm \
                           $$IDL_ATTRIBUTES_FILE
GENERATORS += generateBindings

# GENERATOR 2: inspector idl compiler
inspectorValidate.output = InspectorProtocolVersion.h
inspectorValidate.input = INSPECTOR_JSON
inspectorValidate.script = $$PWD/inspector/generate-inspector-protocol-version
inspectorValidate.commands = python $$inspectorValidate.script -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
inspectorValidate.depends = $$PWD/inspector/generate-inspector-protocol-version
inspectorValidate.add_output_to_sources = false
GENERATORS += inspectorValidate

inspectorJSON.output = InspectorFrontend.cpp InspectorBackendDispatcher.cpp InspectorTypeBuilder.cpp
inspectorJSON.input = INSPECTOR_JSON
inspectorJSON.script = $$PWD/inspector/CodeGeneratorInspector.py
inspectorJSON.commands = python $$inspectorJSON.script $$PWD/inspector/Inspector.json --output_h_dir ${QMAKE_FUNC_FILE_OUT_PATH} --output_cpp_dir ${QMAKE_FUNC_FILE_OUT_PATH}
inspectorJSON.depends = $$inspectorJSON.script
GENERATORS += inspectorJSON

inspectorBackendCommands.output = InspectorBackendCommands.qrc
inspectorBackendCommands.input = INSPECTOR_BACKEND_COMMANDS_QRC
inspectorBackendCommands.commands = $$QMAKE_COPY $$toSystemPath($$INSPECTOR_BACKEND_COMMANDS_QRC) ${QMAKE_FUNC_FILE_OUT_PATH}$${QMAKE_DIR_SEP}InspectorBackendCommands.qrc
inspectorBackendCommands.depends = $$INSPECTOR_JSON
inspectorBackendCommands.add_output_to_sources = false
GENERATORS += inspectorBackendCommands

inspectorOverlayPage.output = InspectorOverlayPage.h
inspectorOverlayPage.input = INSPECTOR_OVERLAY_PAGE
inspectorOverlayPage.commands = perl $$PWD/inspector/xxd.pl InspectorOverlayPage_html ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
inspectorOverlayPage.add_output_to_sources = false
GENERATORS += inspectorOverlayPage

# GENERATOR 2: inspector injected script source compiler
injectedScriptSource.output = InjectedScriptSource.h
injectedScriptSource.input = INJECTED_SCRIPT_SOURCE
injectedScriptSource.commands = perl $$PWD/inspector/xxd.pl InjectedScriptSource_js ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
injectedScriptSource.add_output_to_sources = false
GENERATORS += injectedScriptSource

# GENERATOR 3: inspector canvas injected script source compiler
InjectedScriptCanvasModuleSource.output = InjectedScriptCanvasModuleSource.h
InjectedScriptCanvasModuleSource.input = INJECTED_SCRIPT_CANVAS_MODULE_SOURCE
InjectedScriptCanvasModuleSource.commands = perl $$PWD/inspector/xxd.pl InjectedScriptCanvasModuleSource_js ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
InjectedScriptCanvasModuleSource.add_output_to_sources = false
GENERATORS += InjectedScriptCanvasModuleSource

# GENERATOR 4: CSS grammar
cssbison.output = CSSGrammar.cpp
cssbison.input = CSSBISON
cssbison.script = $$PWD/css/makegrammar.pl
cssbison.commands = perl -I $$PWD/bindings/scripts $$cssbison.script --outputDir ${QMAKE_FUNC_FILE_OUT_PATH} --extraDefines \"$${DEFINES} $$configDefines()\" --preprocessor \"$${QMAKE_MOC} -E\" --symbolsPrefix cssyy ${QMAKE_FILE_NAME}
cssbison.depends = ${QMAKE_FILE_NAME}
GENERATORS += cssbison

# GENERATOR 5-A:
htmlnames.output = HTMLNames.cpp
htmlnames.input = HTML_NAMES
htmlnames.script = $$PWD/dom/make_names.pl
htmlnames.depends = $$PWD/html/HTMLAttributeNames.in
htmlnames.commands = perl -I$$PWD/bindings/scripts $$htmlnames.script --tags $$PWD/html/HTMLTagNames.in --attrs $$PWD/html/HTMLAttributeNames.in --extraDefines \"$${DEFINES} $$configDefines()\" --preprocessor \"$${QMAKE_MOC} -E\"  --factory --wrapperFactory --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
htmlnames.extra_sources = HTMLElementFactory.cpp
htmlnames.extra_sources += JSHTMLElementWrapperFactory.cpp
GENERATORS += htmlnames

# GENERATOR 5-B:
xmlnsnames.output = XMLNSNames.cpp
xmlnsnames.input = XMLNS_NAMES
xmlnsnames.script = $$PWD/dom/make_names.pl
xmlnsnames.commands = perl -I$$PWD/bindings/scripts $$xmlnsnames.script --attrs $$PWD/xml/xmlnsattrs.in --preprocessor \"$${QMAKE_MOC} -E\" --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
GENERATORS += xmlnsnames

# GENERATOR 5-C:
xmlnames.output = XMLNames.cpp
xmlnames.input = XML_NAMES
xmlnames.script = $$PWD/dom/make_names.pl
xmlnames.commands = perl -I$$PWD/bindings/scripts $$xmlnames.script --attrs $$PWD/xml/xmlattrs.in --preprocessor \"$${QMAKE_MOC} -E\" --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
GENERATORS += xmlnames

# GENERATOR 5-D:
fontnames.output = WebKitFontFamilyNames.cpp
fontnames.input = FONT_NAMES
fontnames.script = $$PWD/dom/make_names.pl
fontnames.commands = perl -I$$PWD/bindings/scripts $$fontnames.script --fonts $$FONT_NAMES --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
entities.depends = $$PWD/dom/make_names.pl $$FONT_NAMES
GENERATORS += fontnames

# GENERATOR 5-E:
eventfactory.output = EventFactory.cpp
eventfactory.input = EVENTS_NAMES
eventfactory.script = $$PWD/dom/make_event_factory.pl
eventfactory.commands = perl -I$$PWD/bindings/scripts $$eventfactory.script --input $$EVENTS_NAMES --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
eventfactory.depends = $$PWD/dom/make_event_factory.pl $$EVENTS_NAMES
GENERATORS += eventfactory

# GENERATOR 5-F:
eventtargetfactory.output = EventTargetInterfaces.h
eventtargetfactory.add_output_to_sources = false
eventtargetfactory.input = EVENT_TARGET_FACTORY
eventtargetfactory.script = $$PWD/dom/make_event_factory.pl
eventtargetfactory.commands = perl -I$$PWD/bindings/scripts $$eventfactory.script --input $$EVENT_TARGET_FACTORY --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
eventtargetfactory.depends = $$PWD/dom/make_event_factory.pl $$EVENT_TARGET_FACTORY
GENERATORS += eventtargetfactory

# GENERATOR 5-G:
exceptioncodedescription.output = ExceptionCodeDescription.cpp
exceptioncodedescription.input = DOM_EXCEPTIONS
exceptioncodedescription.script = $$PWD/dom/make_dom_exceptions.pl
exceptioncodedescription.commands = perl -I$$PWD/bindings/scripts $$exceptioncodedescription.script --input $$DOM_EXCEPTIONS --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}
exceptioncodedescription.depends = $$PWD/dom/make_dom_exceptions.pl $$DOM_EXCEPTIONS
GENERATORS += exceptioncodedescription

# GENERATOR 8-A:
entities.output = HTMLEntityTable.cpp
entities.input = HTML_ENTITIES
entities.script = $$PWD/html/parser/create-html-entity-table
entities.commands = python $$entities.script -o ${QMAKE_FILE_OUT} $$HTML_ENTITIES
entities.clean = ${QMAKE_FILE_OUT}
entities.depends = $$PWD/html/parser/create-html-entity-table
GENERATORS += entities

# GENERATOR 8-B:
colordata.output = ColorData.cpp
colordata.input = COLORDATA_GPERF
colordata.script = $$PWD/make-hash-tools.pl
colordata.commands = perl $$colordata.script ${QMAKE_FUNC_FILE_OUT_PATH} $$COLORDATA_GPERF
colordata.clean = ${QMAKE_FILE_OUT}
colordata.depends = $$PWD/make-hash-tools.pl
GENERATORS += colordata

enable?(XSLT) {
    # GENERATOR 8-C:
    xmlviewercss.output = XMLViewerCSS.h
    xmlviewercss.input = XMLVIEWER_CSS
    xmlviewercss.script = $$PWD/inspector/xxd.pl
    xmlviewercss.commands = perl $$xmlviewercss.script XMLViewer_css $$XMLVIEWER_CSS ${QMAKE_FILE_OUT}
    xmlviewercss.clean = ${QMAKE_FILE_OUT}
    xmlviewercss.depends = $$PWD/inspector/xxd.pl
    xmlviewercss.add_output_to_sources = false
    GENERATORS += xmlviewercss

    # GENERATOR 8-D:
    xmlviewerjs.output = XMLViewerJS.h
    xmlviewerjs.input = XMLVIEWER_JS
    xmlviewerjs.script = $$PWD/inspector/xxd.pl
    xmlviewerjs.commands = perl $$xmlviewerjs.script XMLViewer_js $$XMLVIEWER_JS ${QMAKE_FILE_OUT}
    xmlviewerjs.clean = ${QMAKE_FILE_OUT}
    xmlviewerjs.depends = $$PWD/inspector/xxd.pl
    xmlviewerjs.add_output_to_sources = false
    GENERATORS += xmlviewerjs
}

# GENERATOR 9:
stylesheets.script = $$PWD/css/make-css-file-arrays.pl
stylesheets.output = UserAgentStyleSheetsData.cpp
stylesheets.input = stylesheets.script
stylesheets.commands = perl $$stylesheets.script ${QMAKE_FUNC_FILE_OUT_PATH}/UserAgentStyleSheets.h ${QMAKE_FILE_OUT} $$STYLESHEETS_EMBED
stylesheets.depends = $$STYLESHEETS_EMBED
stylesheets.clean = ${QMAKE_FILE_OUT} ${QMAKE_FUNC_FILE_OUT_PATH}/UserAgentStyleSheets.h
GENERATORS += stylesheets

# GENERATOR 10:
pluginsresources.script = $$PWD/css/make-css-file-arrays.pl
pluginsresources.output = PlugInsResourcesData.cpp
pluginsresources.input = pluginsresources.script
pluginsresources.commands = perl $$pluginsresources.script ${QMAKE_FUNC_FILE_OUT_PATH}/PlugInsResources.h ${QMAKE_FILE_OUT} $$PLUGINS_EMBED
pluginsresources.depends = $$PLUGINS_EMBED
pluginsresources.clean = ${QMAKE_FILE_OUT} ${QMAKE_FUNC_FILE_OUT_PATH}/PlugInsResources.h
GENERATORS += pluginsresources

# GENERATOR 11: XPATH grammar
xpathbison.output = ${QMAKE_FILE_BASE}.cpp
xpathbison.input = XPATHBISON
xpathbison.commands = bison -d -p xpathyy ${QMAKE_FILE_NAME} -o ${QMAKE_FUNC_FILE_OUT_PATH}/${QMAKE_FILE_BASE}.tab.c && $(MOVE) ${QMAKE_FUNC_FILE_OUT_PATH}$${QMAKE_DIR_SEP}${QMAKE_FILE_BASE}.tab.c ${QMAKE_FUNC_FILE_OUT_PATH}$${QMAKE_DIR_SEP}${QMAKE_FILE_BASE}.cpp && $(MOVE) ${QMAKE_FUNC_FILE_OUT_PATH}$${QMAKE_DIR_SEP}${QMAKE_FILE_BASE}.tab.h ${QMAKE_FUNC_FILE_OUT_PATH}$${QMAKE_DIR_SEP}${QMAKE_FILE_BASE}.h
xpathbison.depends = ${QMAKE_FILE_NAME}
GENERATORS += xpathbison

# GENERATOR 12: WebKit Version
# The appropriate Apple-maintained Version.xcconfig file for WebKit version information is in Source/WebKit/mac/Configurations/.
webkitversion.script = $$PWD/../WebKit/scripts/generate-webkitversion.pl
webkitversion.output = WebKitVersion.h
webkitversion.input = webkitversion.script
webkitversion.commands = perl $$webkitversion.script --config $$PWD/../WebKit/mac/Configurations/Version.xcconfig --outputDir ${QMAKE_FUNC_FILE_OUT_PATH}/
webkitversion.clean = ${QMAKE_FUNC_FILE_OUT_PATH}/WebKitVersion.h
webkitversion.add_output_to_sources = false
GENERATORS += webkitversion
