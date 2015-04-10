set(WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}"
    "${WEBCORE_DIR}/Modules/battery"
    "${WEBCORE_DIR}/Modules/filesystem"
    "${WEBCORE_DIR}/Modules/gamepad"
    "${WEBCORE_DIR}/Modules/geolocation"
    "${WEBCORE_DIR}/Modules/indexeddb"
    "${WEBCORE_DIR}/Modules/indieui"
    "${WEBCORE_DIR}/Modules/mediasource"
    "${WEBCORE_DIR}/Modules/mediastream"
    "${WEBCORE_DIR}/Modules/navigatorcontentutils"
    "${WEBCORE_DIR}/Modules/networkinfo"
    "${WEBCORE_DIR}/Modules/notifications"
    "${WEBCORE_DIR}/Modules/proximity"
    "${WEBCORE_DIR}/Modules/quota"
    "${WEBCORE_DIR}/Modules/vibration"
    "${WEBCORE_DIR}/Modules/webaudio"
    "${WEBCORE_DIR}/Modules/webdatabase"
    "${WEBCORE_DIR}/Modules/websockets"
    "${WEBCORE_DIR}/accessibility"
    "${WEBCORE_DIR}/bindings"
    "${WEBCORE_DIR}/bindings/generic"
    "${WEBCORE_DIR}/bridge"
    "${WEBCORE_DIR}/bridge/c"
    "${WEBCORE_DIR}/css"
    "${WEBCORE_DIR}/dom"
    "${WEBCORE_DIR}/dom/default"
    "${WEBCORE_DIR}/editing"
    "${WEBCORE_DIR}/fileapi"
    "${WEBCORE_DIR}/history"
    "${WEBCORE_DIR}/html"
    "${WEBCORE_DIR}/html/canvas"
    "${WEBCORE_DIR}/html/forms"
    "${WEBCORE_DIR}/html/parser"
    "${WEBCORE_DIR}/html/shadow"
    "${WEBCORE_DIR}/html/track"
    "${WEBCORE_DIR}/inspector"
    "${WEBCORE_DIR}/loader"
    "${WEBCORE_DIR}/loader/appcache"
    "${WEBCORE_DIR}/loader/archive"
    "${WEBCORE_DIR}/loader/archive/mhtml"
    "${WEBCORE_DIR}/loader/cache"
    "${WEBCORE_DIR}/loader/icon"
    "${WEBCORE_DIR}/mathml"
    "${WEBCORE_DIR}/page"
    "${WEBCORE_DIR}/page/animation"
    "${WEBCORE_DIR}/page/scrolling"
    "${WEBCORE_DIR}/page/scrolling/coordinatedgraphics"
    "${WEBCORE_DIR}/platform"
    "${WEBCORE_DIR}/platform/animation"
    "${WEBCORE_DIR}/platform/audio"
    "${WEBCORE_DIR}/platform/graphics"
    "${WEBCORE_DIR}/platform/graphics/cpu/arm"
    "${WEBCORE_DIR}/platform/graphics/cpu/arm/filters"
    "${WEBCORE_DIR}/platform/graphics/filters"
    "${WEBCORE_DIR}/platform/graphics/filters/texmap"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/ng"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/texmap"
    "${WEBCORE_DIR}/platform/graphics/texmap/coordinated"
    "${WEBCORE_DIR}/platform/graphics/transforms"
    "${WEBCORE_DIR}/platform/image-decoders"
    "${WEBCORE_DIR}/platform/image-decoders/bmp"
    "${WEBCORE_DIR}/platform/image-decoders/gif"
    "${WEBCORE_DIR}/platform/image-decoders/ico"
    "${WEBCORE_DIR}/platform/image-decoders/jpeg"
    "${WEBCORE_DIR}/platform/image-decoders/png"
    "${WEBCORE_DIR}/platform/image-decoders/webp"
    "${WEBCORE_DIR}/platform/leveldb"
    "${WEBCORE_DIR}/platform/mediastream"
    "${WEBCORE_DIR}/platform/mock"
    "${WEBCORE_DIR}/platform/network"
    "${WEBCORE_DIR}/platform/sql"
    "${WEBCORE_DIR}/platform/text"
    "${WEBCORE_DIR}/platform/text/transcoder"
    "${WEBCORE_DIR}/plugins"
    "${WEBCORE_DIR}/rendering"
    "${WEBCORE_DIR}/rendering/mathml"
    "${WEBCORE_DIR}/rendering/shapes"
    "${WEBCORE_DIR}/rendering/style"
    "${WEBCORE_DIR}/rendering/svg"
    "${WEBCORE_DIR}/storage"
    "${WEBCORE_DIR}/svg"
    "${WEBCORE_DIR}/svg/animation"
    "${WEBCORE_DIR}/svg/graphics"
    "${WEBCORE_DIR}/svg/graphics/filters"
    "${WEBCORE_DIR}/svg/properties"
    "${WEBCORE_DIR}/websockets"
    "${WEBCORE_DIR}/workers"
    "${WEBCORE_DIR}/xml"
    "${WEBCORE_DIR}/xml/parser"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${JAVASCRIPTCORE_DIR}/API"
    "${JAVASCRIPTCORE_DIR}/assembler"
    "${JAVASCRIPTCORE_DIR}/bytecode"
    "${JAVASCRIPTCORE_DIR}/bytecompiler"
    "${JAVASCRIPTCORE_DIR}/dfg"
    "${JAVASCRIPTCORE_DIR}/disassembler"
    "${JAVASCRIPTCORE_DIR}/heap"
    "${JAVASCRIPTCORE_DIR}/debugger"
    "${JAVASCRIPTCORE_DIR}/interpreter"
    "${JAVASCRIPTCORE_DIR}/jit"
    "${JAVASCRIPTCORE_DIR}/llint"
    "${JAVASCRIPTCORE_DIR}/parser"
    "${JAVASCRIPTCORE_DIR}/profiler"
    "${JAVASCRIPTCORE_DIR}/runtime"
    "${JAVASCRIPTCORE_DIR}/yarr"
    "${WTF_DIR}"
    "${DERIVED_SOURCES_WEBCORE_DIR}"
    "${CMAKE_SOURCE_DIR}/Source"
    "${CMAKE_BINARY_DIR}"
)

set(WebCore_IDL_INCLUDES
    Modules/battery
    Modules/filesystem
    Modules/gamepad
    Modules/geolocation
    Modules/indexeddb
    Modules/indieui
    Modules/mediastream
    Modules/networkinfo
    Modules/notifications
    Modules/proximity
    Modules/quota
    Modules/speech
    Modules/vibration
    Modules/webaudio
    Modules/webdatabase
    Modules/websockets
    bindings/generic
    css
    dom
    fileapi
    html
    html/canvas
    html/shadow
    html/track
    inspector
    loader/appcache
    page
    plugins
    storage
    svg
    workers
    xml
)

set(WebCore_IDL_FILES
    Modules/battery/BatteryManager.idl
    Modules/battery/NavigatorBattery.idl

    Modules/filesystem/DataTransferItemFileSystem.idl
    Modules/filesystem/DirectoryEntry.idl
    Modules/filesystem/DirectoryEntrySync.idl
    Modules/filesystem/DirectoryReader.idl
    Modules/filesystem/DirectoryReaderSync.idl
    Modules/filesystem/DOMFileSystem.idl
    Modules/filesystem/DOMFileSystemSync.idl
    Modules/filesystem/DOMWindowFileSystem.idl
    Modules/filesystem/EntriesCallback.idl
    Modules/filesystem/Entry.idl
    Modules/filesystem/EntrySync.idl
    Modules/filesystem/EntryArray.idl
    Modules/filesystem/EntryArraySync.idl
    Modules/filesystem/EntryCallback.idl
    Modules/filesystem/ErrorCallback.idl
    Modules/filesystem/FileCallback.idl
    Modules/filesystem/FileEntry.idl
    Modules/filesystem/FileEntrySync.idl
    Modules/filesystem/FileSystemCallback.idl
    Modules/filesystem/FileWriterCallback.idl
    Modules/filesystem/FileWriter.idl
    Modules/filesystem/FileWriterSync.idl
    Modules/filesystem/HTMLInputElementFileSystem.idl
    Modules/filesystem/Metadata.idl
    Modules/filesystem/MetadataCallback.idl
    Modules/filesystem/WorkerGlobalScopeFileSystem.idl

    Modules/gamepad/Gamepad.idl
    Modules/gamepad/GamepadList.idl
    Modules/gamepad/NavigatorGamepad.idl

    Modules/geolocation/Coordinates.idl
    Modules/geolocation/Geolocation.idl
    Modules/geolocation/Geoposition.idl
    Modules/geolocation/NavigatorGeolocation.idl
    Modules/geolocation/PositionCallback.idl
    Modules/geolocation/PositionError.idl
    Modules/geolocation/PositionErrorCallback.idl

    Modules/indexeddb/DOMWindowIndexedDatabase.idl
    Modules/indexeddb/IDBAny.idl
    Modules/indexeddb/IDBCursor.idl
    Modules/indexeddb/IDBCursorWithValue.idl
    Modules/indexeddb/IDBDatabase.idl
    Modules/indexeddb/IDBFactory.idl
    Modules/indexeddb/IDBIndex.idl
    Modules/indexeddb/IDBKeyRange.idl
    Modules/indexeddb/IDBObjectStore.idl
    Modules/indexeddb/IDBOpenDBRequest.idl
    Modules/indexeddb/IDBRequest.idl
    Modules/indexeddb/IDBTransaction.idl
    Modules/indexeddb/IDBVersionChangeEvent.idl
    Modules/indexeddb/WorkerGlobalScopeIndexedDatabase.idl

    Modules/mediasource/MediaSource.idl
    Modules/mediasource/SourceBuffer.idl
    Modules/mediasource/SourceBufferList.idl

    Modules/mediastream/LocalMediaStream.idl
    Modules/mediastream/MediaStream.idl
    Modules/mediastream/MediaStreamEvent.idl
    Modules/mediastream/MediaStreamTrack.idl
    Modules/mediastream/MediaStreamTrackEvent.idl
    Modules/mediastream/NavigatorMediaStream.idl
    Modules/mediastream/NavigatorUserMediaError.idl
    Modules/mediastream/NavigatorUserMediaErrorCallback.idl
    Modules/mediastream/NavigatorUserMediaSuccessCallback.idl
    Modules/mediastream/RTCDTMFSender.idl
    Modules/mediastream/RTCDTMFToneChangeEvent.idl
    Modules/mediastream/RTCDataChannel.idl
    Modules/mediastream/RTCDataChannelEvent.idl
    Modules/mediastream/RTCErrorCallback.idl
    Modules/mediastream/RTCIceCandidate.idl
    Modules/mediastream/RTCIceCandidateEvent.idl
    Modules/mediastream/RTCPeerConnection.idl
    Modules/mediastream/RTCSessionDescription.idl
    Modules/mediastream/RTCSessionDescriptionCallback.idl
    Modules/mediastream/RTCStatsCallback.idl
    Modules/mediastream/RTCStatsReport.idl
    Modules/mediastream/RTCStatsResponse.idl

    Modules/navigatorcontentutils/NavigatorContentUtils.idl

    Modules/networkinfo/NavigatorNetworkInfoConnection.idl
    Modules/networkinfo/NetworkInfoConnection.idl

    Modules/notifications/DOMWindowNotifications.idl
    Modules/notifications/Notification.idl
    Modules/notifications/NotificationCenter.idl
    Modules/notifications/NotificationPermissionCallback.idl
    Modules/notifications/WorkerGlobalScopeNotifications.idl

    Modules/proximity/DeviceProximityEvent.idl

    Modules/vibration/NavigatorVibration.idl

    Modules/webaudio/AudioBuffer.idl
    Modules/webaudio/AudioBufferCallback.idl
    Modules/webaudio/AudioBufferSourceNode.idl
    Modules/webaudio/ChannelSplitterNode.idl
    Modules/webaudio/ChannelMergerNode.idl
    Modules/webaudio/AudioContext.idl
    Modules/webaudio/AudioDestinationNode.idl
    Modules/webaudio/GainNode.idl
    Modules/webaudio/AudioListener.idl
    Modules/webaudio/AudioNode.idl
    Modules/webaudio/PannerNode.idl
    Modules/webaudio/AudioParam.idl
    Modules/webaudio/AudioProcessingEvent.idl
    Modules/webaudio/BiquadFilterNode.idl
    Modules/webaudio/ConvolverNode.idl
    Modules/webaudio/DelayNode.idl
    Modules/webaudio/DynamicsCompressorNode.idl
    Modules/webaudio/ScriptProcessorNode.idl
    Modules/webaudio/MediaElementAudioSourceNode.idl
    Modules/webaudio/MediaStreamAudioSourceNode.idl
    Modules/webaudio/OfflineAudioContext.idl
    Modules/webaudio/OfflineAudioCompletionEvent.idl
    Modules/webaudio/OscillatorNode.idl
    Modules/webaudio/AnalyserNode.idl
    Modules/webaudio/WaveShaperNode.idl
    Modules/webaudio/PeriodicWave.idl

    Modules/webdatabase/Database.idl
    Modules/webdatabase/DatabaseCallback.idl
    Modules/webdatabase/DatabaseSync.idl
    Modules/webdatabase/DOMWindowWebDatabase.idl
    Modules/webdatabase/SQLError.idl
    Modules/webdatabase/SQLException.idl
    Modules/webdatabase/SQLResultSet.idl
    Modules/webdatabase/SQLResultSetRowList.idl
    Modules/webdatabase/SQLStatementCallback.idl
    Modules/webdatabase/SQLStatementErrorCallback.idl
    Modules/webdatabase/SQLTransaction.idl
    Modules/webdatabase/SQLTransactionCallback.idl
    Modules/webdatabase/SQLTransactionErrorCallback.idl
    Modules/webdatabase/SQLTransactionSync.idl
    Modules/webdatabase/SQLTransactionSyncCallback.idl

    Modules/websockets/CloseEvent.idl
    Modules/websockets/WebSocket.idl

    css/Counter.idl
    css/CSSCharsetRule.idl
    css/CSSFontFaceLoadEvent.idl
    css/CSSFontFaceRule.idl
    css/CSSHostRule.idl
    css/CSSImportRule.idl
    css/CSSMediaRule.idl
    css/CSSPageRule.idl
    css/CSSPrimitiveValue.idl
    css/CSSRule.idl
    css/CSSRuleList.idl
    css/CSSStyleDeclaration.idl
    css/CSSStyleRule.idl
    css/CSSStyleSheet.idl
    css/CSSSupportsRule.idl
    css/CSSValue.idl
    css/CSSValueList.idl
    css/DOMWindowCSS.idl
    css/FontLoader.idl
    css/MediaList.idl
    css/MediaQueryList.idl
    css/RGBColor.idl
    css/Rect.idl
    css/StyleMedia.idl
    css/StyleSheet.idl
    css/StyleSheetList.idl
    css/WebKitCSSFilterRule.idl
    css/WebKitCSSFilterValue.idl
    css/WebKitCSSKeyframeRule.idl
    css/WebKitCSSKeyframesRule.idl
    css/WebKitCSSMatrix.idl
    css/WebKitCSSMixFunctionValue.idl
    css/WebKitCSSRegionRule.idl
    css/WebKitCSSTransformValue.idl
    css/WebKitCSSViewportRule.idl

    dom/Attr.idl
    dom/BeforeLoadEvent.idl
    dom/CDATASection.idl
    dom/CharacterData.idl
    dom/ChildNode.idl
    dom/ClientRect.idl
    dom/ClientRectList.idl
    dom/Clipboard.idl
    dom/Comment.idl
    dom/CompositionEvent.idl
    dom/CustomEvent.idl
    dom/DataTransferItem.idl
    dom/DataTransferItemList.idl
    dom/DeviceMotionEvent.idl
    dom/DeviceOrientationEvent.idl
    dom/DOMCoreException.idl
    dom/DOMError.idl
    dom/DOMImplementation.idl
    dom/DOMStringList.idl
    dom/DOMStringMap.idl
    dom/DocumentFragment.idl
    dom/Document.idl
    dom/DocumentType.idl
    dom/Element.idl
    dom/Entity.idl
    dom/EntityReference.idl
    dom/ErrorEvent.idl
    dom/EventException.idl
    dom/Event.idl
    dom/EventTarget.idl
    dom/FocusEvent.idl
    dom/HashChangeEvent.idl
    dom/KeyboardEvent.idl
    dom/MessageChannel.idl
    dom/MessageEvent.idl
    dom/MessagePort.idl
    dom/MouseEvent.idl
    dom/MutationEvent.idl
    dom/MutationObserver.idl
    dom/MutationRecord.idl
    dom/NamedNodeMap.idl
    dom/NodeFilter.idl
    dom/Node.idl
    dom/NodeIterator.idl
    dom/NodeList.idl
    dom/Notation.idl
    dom/OverflowEvent.idl
    dom/PageTransitionEvent.idl
    dom/PopStateEvent.idl
    dom/ProcessingInstruction.idl
    dom/ProgressEvent.idl
    dom/PropertyNodeList.idl
    dom/RangeException.idl
    dom/Range.idl
    dom/RequestAnimationFrameCallback.idl
    dom/SecurityPolicyViolationEvent.idl
    dom/ShadowRoot.idl
    dom/StringCallback.idl
    dom/TextEvent.idl
    dom/Text.idl
    dom/TouchEvent.idl
    dom/Touch.idl
    dom/TouchList.idl
    dom/TransitionEvent.idl
    dom/TreeWalker.idl
    dom/UIEvent.idl
    dom/WebKitAnimationEvent.idl
    dom/WebKitNamedFlow.idl
    dom/DOMNamedFlowCollection.idl
    dom/WebKitTransitionEvent.idl
    dom/WheelEvent.idl

    fileapi/Blob.idl
    fileapi/File.idl
    fileapi/FileError.idl
    fileapi/FileException.idl
    fileapi/FileList.idl
    fileapi/FileReader.idl
    fileapi/FileReaderSync.idl

    html/DOMFormData.idl
    html/DOMSettableTokenList.idl
    html/DOMTokenList.idl
    html/DOMURL.idl
    html/HTMLAllCollection.idl
    html/HTMLAnchorElement.idl
    html/HTMLAppletElement.idl
    html/HTMLAreaElement.idl
    html/HTMLAudioElement.idl
    html/HTMLBaseElement.idl
    html/HTMLBaseFontElement.idl
    html/HTMLBodyElement.idl
    html/HTMLBRElement.idl
    html/HTMLButtonElement.idl
    html/HTMLCanvasElement.idl
    html/HTMLCollection.idl
    html/HTMLDataListElement.idl
    html/HTMLDetailsElement.idl
    html/HTMLDialogElement.idl
    html/HTMLDirectoryElement.idl
    html/HTMLDivElement.idl
    html/HTMLDListElement.idl
    html/HTMLDocument.idl
    html/HTMLElement.idl
    html/HTMLEmbedElement.idl
    html/HTMLFieldSetElement.idl
    html/HTMLFontElement.idl
    html/HTMLFormControlsCollection.idl
    html/HTMLFormElement.idl
    html/HTMLFrameElement.idl
    html/HTMLFrameSetElement.idl
    html/HTMLHeadElement.idl
    html/HTMLHeadingElement.idl
    html/HTMLHRElement.idl
    html/HTMLHtmlElement.idl
    html/HTMLIFrameElement.idl
    html/HTMLImageElement.idl
    html/HTMLInputElement.idl
    html/HTMLKeygenElement.idl
    html/HTMLLabelElement.idl
    html/HTMLLegendElement.idl
    html/HTMLLIElement.idl
    html/HTMLLinkElement.idl
    html/HTMLMapElement.idl
    html/HTMLMarqueeElement.idl
    html/HTMLMediaElement.idl
    html/HTMLMenuElement.idl
    html/HTMLMetaElement.idl
    html/HTMLMeterElement.idl
    html/HTMLModElement.idl
    html/HTMLObjectElement.idl
    html/HTMLOListElement.idl
    html/HTMLOptGroupElement.idl
    html/HTMLOptionElement.idl
    html/HTMLOptionsCollection.idl
    html/HTMLOutputElement.idl
    html/HTMLParagraphElement.idl
    html/HTMLParamElement.idl
    html/HTMLPreElement.idl
    html/HTMLProgressElement.idl
    html/HTMLPropertiesCollection.idl
    html/HTMLQuoteElement.idl
    html/HTMLScriptElement.idl
    html/HTMLSelectElement.idl
    html/HTMLSourceElement.idl
    html/HTMLSpanElement.idl
    html/HTMLStyleElement.idl
    html/HTMLTableCaptionElement.idl
    html/HTMLTableCellElement.idl
    html/HTMLTableColElement.idl
    html/HTMLTableElement.idl
    html/HTMLTableRowElement.idl
    html/HTMLTableSectionElement.idl
    html/HTMLTemplateElement.idl
    html/HTMLTextAreaElement.idl
    html/HTMLTitleElement.idl
    html/HTMLTrackElement.idl
    html/HTMLUListElement.idl
    html/HTMLUnknownElement.idl
    html/HTMLVideoElement.idl
    html/ImageData.idl
    html/MediaController.idl
    html/MediaError.idl
    html/MicroDataItemValue.idl
    html/RadioNodeList.idl
    html/TextMetrics.idl
    html/TimeRanges.idl
    html/ValidityState.idl
    html/VoidCallback.idl

    html/canvas/ArrayBuffer.idl
    html/canvas/ArrayBufferView.idl
    html/canvas/CanvasGradient.idl
    html/canvas/CanvasPattern.idl
    html/canvas/CanvasProxy.idl
    html/canvas/CanvasRenderingContext2D.idl
    html/canvas/CanvasRenderingContext.idl
    html/canvas/DataView.idl
    html/canvas/DOMPath.idl
    html/canvas/EXTDrawBuffers.idl
    html/canvas/EXTTextureFilterAnisotropic.idl
    html/canvas/Float32Array.idl
    html/canvas/Float64Array.idl
    html/canvas/Int16Array.idl
    html/canvas/Int32Array.idl
    html/canvas/Int8Array.idl
    html/canvas/OESStandardDerivatives.idl
    html/canvas/OESTextureFloat.idl
    html/canvas/OESTextureHalfFloat.idl
    html/canvas/OESVertexArrayObject.idl
    html/canvas/OESElementIndexUint.idl
    html/canvas/Uint16Array.idl
    html/canvas/Uint32Array.idl
    html/canvas/Uint8Array.idl
    html/canvas/Uint8ClampedArray.idl
    html/canvas/WebGLActiveInfo.idl
    html/canvas/WebGLBuffer.idl
    html/canvas/WebGLCompressedTextureATC.idl
    html/canvas/WebGLCompressedTexturePVRTC.idl
    html/canvas/WebGLCompressedTextureS3TC.idl
    html/canvas/WebGLContextAttributes.idl
    html/canvas/WebGLContextEvent.idl
    html/canvas/WebGLDebugRendererInfo.idl
    html/canvas/WebGLDebugShaders.idl
    html/canvas/WebGLDepthTexture.idl
    html/canvas/WebGLFramebuffer.idl
    html/canvas/WebGLLoseContext.idl
    html/canvas/WebGLProgram.idl
    html/canvas/WebGLRenderbuffer.idl
    html/canvas/WebGLRenderingContext.idl
    html/canvas/WebGLShader.idl
    html/canvas/WebGLShaderPrecisionFormat.idl
    html/canvas/WebGLTexture.idl
    html/canvas/WebGLUniformLocation.idl
    html/canvas/WebGLVertexArrayObjectOES.idl

    html/shadow/HTMLContentElement.idl

    inspector/InjectedScriptHost.idl
    inspector/InspectorFrontendHost.idl
    inspector/JavaScriptCallFrame.idl
    inspector/ScriptProfile.idl
    inspector/ScriptProfileNode.idl

    loader/appcache/DOMApplicationCache.idl

    page/BarProp.idl
    page/Console.idl
    page/Crypto.idl
    page/DOMSecurityPolicy.idl
    page/DOMSelection.idl
    page/DOMWindow.idl
    page/EventSource.idl
    page/History.idl
    page/Location.idl
    page/Navigator.idl
    page/Performance.idl
    page/PerformanceEntry.idl
    page/PerformanceEntryList.idl
    page/PerformanceNavigation.idl
    page/PerformanceResourceTiming.idl
    page/PerformanceTiming.idl
    page/Screen.idl
    page/SpeechInputEvent.idl
    page/SpeechInputResult.idl
    page/SpeechInputResultList.idl
    page/WebKitPoint.idl
    page/WindowBase64.idl
    page/WindowTimers.idl
    page/WorkerNavigator.idl

    plugins/DOMMimeType.idl
    plugins/DOMMimeTypeArray.idl
    plugins/DOMPlugin.idl
    plugins/DOMPluginArray.idl

    storage/Storage.idl
    storage/StorageEvent.idl

    workers/AbstractWorker.idl
    workers/DedicatedWorkerGlobalScope.idl
    workers/SharedWorker.idl
    workers/SharedWorkerGlobalScope.idl
    workers/Worker.idl
    workers/WorkerGlobalScope.idl
    workers/WorkerLocation.idl

    xml/DOMParser.idl
    xml/XMLHttpRequest.idl
    xml/XMLHttpRequestException.idl
    xml/XMLHttpRequestProgressEvent.idl
    xml/XMLHttpRequestUpload.idl
    xml/XMLSerializer.idl
    xml/XPathEvaluator.idl
    xml/XPathException.idl
    xml/XPathExpression.idl
    xml/XPathNSResolver.idl
    xml/XPathResult.idl
    xml/XSLTProcessor.idl
)

# FIXME: The BlackBerry port doesn't support generating DOM bindings from the SVG IDL files.
# For now, we explicitly demarcate the SVG IDL files so that the BlackBerry port can skip them
# during DOM binding generation. See <https://bugs.webkit.org/show_bug.cgi?id=72773>.
set(WebCore_SVG_IDL_FILES
    svg/SVGAElement.idl
    svg/SVGAltGlyphDefElement.idl
    svg/SVGAltGlyphElement.idl
    svg/SVGAltGlyphItemElement.idl
    svg/SVGAngle.idl
    svg/SVGAnimateColorElement.idl
    svg/SVGAnimateMotionElement.idl
    svg/SVGAnimateElement.idl
    svg/SVGAnimateTransformElement.idl
    svg/SVGAnimatedAngle.idl
    svg/SVGAnimatedBoolean.idl
    svg/SVGAnimatedEnumeration.idl
    svg/SVGAnimatedInteger.idl
    svg/SVGAnimatedLength.idl
    svg/SVGAnimatedLengthList.idl
    svg/SVGAnimatedNumber.idl
    svg/SVGAnimatedNumberList.idl
    svg/SVGAnimatedPreserveAspectRatio.idl
    svg/SVGAnimatedRect.idl
    svg/SVGAnimatedString.idl
    svg/SVGAnimatedTransformList.idl
    svg/SVGAnimationElement.idl
    svg/SVGCircleElement.idl
    svg/SVGClipPathElement.idl
    svg/SVGColor.idl
    svg/SVGComponentTransferFunctionElement.idl
    svg/SVGCursorElement.idl
    svg/SVGDefsElement.idl
    svg/SVGDescElement.idl
    svg/SVGDocument.idl
    svg/SVGElement.idl
    svg/SVGElementInstance.idl
    svg/SVGElementInstanceList.idl
    svg/SVGEllipseElement.idl
    svg/SVGException.idl
    svg/SVGExternalResourcesRequired.idl
    svg/SVGFEBlendElement.idl
    svg/SVGFEColorMatrixElement.idl
    svg/SVGFEComponentTransferElement.idl
    svg/SVGFECompositeElement.idl
    svg/SVGFEConvolveMatrixElement.idl
    svg/SVGFEDiffuseLightingElement.idl
    svg/SVGFEDisplacementMapElement.idl
    svg/SVGFEDistantLightElement.idl
    svg/SVGFEDropShadowElement.idl
    svg/SVGFEFloodElement.idl
    svg/SVGFEFuncAElement.idl
    svg/SVGFEFuncBElement.idl
    svg/SVGFEFuncGElement.idl
    svg/SVGFEFuncRElement.idl
    svg/SVGFEGaussianBlurElement.idl
    svg/SVGFEImageElement.idl
    svg/SVGFEMergeElement.idl
    svg/SVGFEMergeNodeElement.idl
    svg/SVGFEMorphologyElement.idl
    svg/SVGFEOffsetElement.idl
    svg/SVGFEPointLightElement.idl
    svg/SVGFESpecularLightingElement.idl
    svg/SVGFESpotLightElement.idl
    svg/SVGFETileElement.idl
    svg/SVGFETurbulenceElement.idl
    svg/SVGFilterElement.idl
    svg/SVGFilterPrimitiveStandardAttributes.idl
    svg/SVGFitToViewBox.idl
    svg/SVGFontElement.idl
    svg/SVGFontFaceElement.idl
    svg/SVGFontFaceFormatElement.idl
    svg/SVGFontFaceNameElement.idl
    svg/SVGFontFaceSrcElement.idl
    svg/SVGFontFaceUriElement.idl
    svg/SVGForeignObjectElement.idl
    svg/SVGGElement.idl
    svg/SVGGlyphElement.idl
    svg/SVGGlyphRefElement.idl
    svg/SVGGradientElement.idl
    svg/SVGGraphicsElement.idl
    svg/SVGHKernElement.idl
    svg/SVGImageElement.idl
    svg/SVGLength.idl
    svg/SVGLengthList.idl
    svg/SVGLineElement.idl
    svg/SVGLinearGradientElement.idl
    svg/SVGMarkerElement.idl
    svg/SVGMaskElement.idl
    svg/SVGMatrix.idl
    svg/SVGMetadataElement.idl
    svg/SVGMissingGlyphElement.idl
    svg/SVGMPathElement.idl
    svg/SVGNumber.idl
    svg/SVGNumberList.idl
    svg/SVGPaint.idl
    svg/SVGPathElement.idl
    svg/SVGPathSeg.idl
    svg/SVGPathSegArcAbs.idl
    svg/SVGPathSegArcRel.idl
    svg/SVGPathSegClosePath.idl
    svg/SVGPathSegCurvetoCubicAbs.idl
    svg/SVGPathSegCurvetoCubicRel.idl
    svg/SVGPathSegCurvetoCubicSmoothAbs.idl
    svg/SVGPathSegCurvetoCubicSmoothRel.idl
    svg/SVGPathSegCurvetoQuadraticAbs.idl
    svg/SVGPathSegCurvetoQuadraticRel.idl
    svg/SVGPathSegCurvetoQuadraticSmoothAbs.idl
    svg/SVGPathSegCurvetoQuadraticSmoothRel.idl
    svg/SVGPathSegLinetoAbs.idl
    svg/SVGPathSegLinetoHorizontalAbs.idl
    svg/SVGPathSegLinetoHorizontalRel.idl
    svg/SVGPathSegLinetoRel.idl
    svg/SVGPathSegLinetoVerticalAbs.idl
    svg/SVGPathSegLinetoVerticalRel.idl
    svg/SVGPathSegList.idl
    svg/SVGPathSegMovetoAbs.idl
    svg/SVGPathSegMovetoRel.idl
    svg/SVGPatternElement.idl
    svg/SVGPoint.idl
    svg/SVGPointList.idl
    svg/SVGPolygonElement.idl
    svg/SVGPolylineElement.idl
    svg/SVGPreserveAspectRatio.idl
    svg/SVGRadialGradientElement.idl
    svg/SVGRect.idl
    svg/SVGRectElement.idl
    svg/SVGRenderingIntent.idl
    svg/SVGSVGElement.idl
    svg/SVGScriptElement.idl
    svg/SVGSetElement.idl
    svg/SVGStopElement.idl
    svg/SVGStringList.idl
    svg/SVGStyleElement.idl
    svg/SVGStyledElement.idl
    svg/SVGSwitchElement.idl
    svg/SVGSymbolElement.idl
    svg/SVGTRefElement.idl
    svg/SVGTSpanElement.idl
    svg/SVGTests.idl
    svg/SVGTextContentElement.idl
    svg/SVGTextElement.idl
    svg/SVGTextPathElement.idl
    svg/SVGTextPositioningElement.idl
    svg/SVGTitleElement.idl
    svg/SVGTransform.idl
    svg/SVGTransformList.idl
    svg/SVGURIReference.idl
    svg/SVGUnitTypes.idl
    svg/SVGUseElement.idl
    svg/SVGViewElement.idl
    svg/SVGViewSpec.idl
    svg/SVGVKernElement.idl
    svg/SVGZoomAndPan.idl
    svg/SVGZoomEvent.idl
)

list(APPEND WebCore_IDL_FILES
    ${WebCore_SVG_IDL_FILES}
)

set(WebCore_SOURCES
    Modules/battery/BatteryController.cpp
    Modules/battery/BatteryManager.cpp
    Modules/battery/BatteryStatus.cpp
    Modules/battery/NavigatorBattery.cpp

    Modules/filesystem/DOMFilePath.cpp
    Modules/filesystem/DOMFileSystem.cpp
    Modules/filesystem/DOMFileSystemBase.cpp
    Modules/filesystem/DOMFileSystemSync.cpp
    Modules/filesystem/DOMWindowFileSystem.cpp
    Modules/filesystem/DirectoryEntry.cpp
    Modules/filesystem/DirectoryEntrySync.cpp
    Modules/filesystem/DirectoryReader.cpp
    Modules/filesystem/DirectoryReaderSync.cpp
    Modules/filesystem/Entry.cpp
    Modules/filesystem/EntryArray.cpp
    Modules/filesystem/EntryArraySync.cpp
    Modules/filesystem/EntryBase.cpp
    Modules/filesystem/EntrySync.cpp
    Modules/filesystem/FileEntry.cpp
    Modules/filesystem/FileEntrySync.cpp
    Modules/filesystem/FileSystemCallbacks.cpp
    Modules/filesystem/FileWriter.cpp
    Modules/filesystem/FileWriterBase.cpp
    Modules/filesystem/FileWriterSync.cpp
    Modules/filesystem/HTMLInputElementFileSystem.cpp
    Modules/filesystem/LocalFileSystem.cpp
    Modules/filesystem/WorkerGlobalScopeFileSystem.cpp

    Modules/gamepad/Gamepad.cpp
    Modules/gamepad/GamepadList.cpp
    Modules/gamepad/NavigatorGamepad.cpp

    Modules/geolocation/Coordinates.cpp
    Modules/geolocation/Geolocation.cpp
    Modules/geolocation/GeolocationController.cpp
    Modules/geolocation/NavigatorGeolocation.cpp

    Modules/indexeddb/DOMWindowIndexedDatabase.cpp
    Modules/indexeddb/IDBAny.cpp
    Modules/indexeddb/IDBCursor.cpp
    Modules/indexeddb/IDBCursorBackendImpl.cpp
    Modules/indexeddb/IDBCursorWithValue.cpp
    Modules/indexeddb/IDBDatabase.cpp
    Modules/indexeddb/IDBDatabaseBackendImpl.cpp
    Modules/indexeddb/IDBDatabaseCallbacksImpl.cpp
    Modules/indexeddb/IDBDatabaseException.cpp
    Modules/indexeddb/IDBEventDispatcher.cpp
    Modules/indexeddb/IDBFactory.cpp
    Modules/indexeddb/IDBFactoryBackendImpl.cpp
    Modules/indexeddb/IDBFactoryBackendInterface.cpp
    Modules/indexeddb/IDBIndex.cpp
    Modules/indexeddb/IDBKey.cpp
    Modules/indexeddb/IDBKeyRange.cpp
    Modules/indexeddb/IDBKeyPath.cpp
    Modules/indexeddb/IDBBackingStore.cpp
    Modules/indexeddb/IDBLevelDBCoding.cpp
    Modules/indexeddb/IDBObjectStore.cpp
    Modules/indexeddb/IDBObjectStoreBackendImpl.cpp
    Modules/indexeddb/IDBOpenDBRequest.cpp
    Modules/indexeddb/IDBPendingTransactionMonitor.cpp
    Modules/indexeddb/IDBRequest.cpp
    Modules/indexeddb/IDBTransaction.cpp
    Modules/indexeddb/IDBTransactionBackendImpl.cpp
    Modules/indexeddb/IDBTransactionCoordinator.cpp
    Modules/indexeddb/IDBVersionChangeEvent.cpp
    Modules/indexeddb/PageGroupIndexedDatabase.cpp
    Modules/indexeddb/WorkerGlobalScopeIndexedDatabase.cpp

    Modules/mediasource/MediaSource.cpp
    Modules/mediasource/MediaSourceRegistry.cpp
    Modules/mediasource/SourceBuffer.cpp
    Modules/mediasource/SourceBufferList.cpp

    Modules/mediastream/LocalMediaStream.cpp
    Modules/mediastream/MediaConstraintsImpl.cpp
    Modules/mediastream/MediaStream.cpp
    Modules/mediastream/MediaStreamEvent.cpp
    Modules/mediastream/MediaStreamRegistry.cpp
    Modules/mediastream/MediaStreamTrack.cpp
    Modules/mediastream/MediaStreamTrackEvent.cpp
    Modules/mediastream/NavigatorMediaStream.cpp
    Modules/mediastream/RTCDTMFSender.cpp
    Modules/mediastream/RTCDTMFToneChangeEvent.cpp
    Modules/mediastream/RTCDataChannel.cpp
    Modules/mediastream/RTCDataChannelEvent.cpp
    Modules/mediastream/RTCIceCandidate.cpp
    Modules/mediastream/RTCIceCandidateEvent.cpp
    Modules/mediastream/RTCPeerConnection.cpp
    Modules/mediastream/RTCSessionDescription.cpp
    Modules/mediastream/RTCSessionDescriptionRequestImpl.cpp
    Modules/mediastream/RTCStatsReport.cpp
    Modules/mediastream/RTCStatsRequestImpl.cpp
    Modules/mediastream/RTCStatsResponse.cpp
    Modules/mediastream/RTCVoidRequestImpl.cpp
    Modules/mediastream/UserMediaController.cpp
    Modules/mediastream/UserMediaRequest.cpp

    Modules/navigatorcontentutils/NavigatorContentUtils.cpp

    Modules/networkinfo/NavigatorNetworkInfoConnection.cpp
    Modules/networkinfo/NetworkInfo.cpp
    Modules/networkinfo/NetworkInfoConnection.cpp
    Modules/networkinfo/NetworkInfoController.cpp

    Modules/notifications/DOMWindowNotifications.cpp
    Modules/notifications/NotificationCenter.cpp
    Modules/notifications/NotificationController.cpp
    Modules/notifications/Notification.cpp
    Modules/notifications/WorkerGlobalScopeNotifications.cpp

    Modules/proximity/DeviceProximityController.cpp
    Modules/proximity/DeviceProximityEvent.cpp

    Modules/vibration/NavigatorVibration.cpp
    Modules/vibration/Vibration.cpp

    Modules/webaudio/AsyncAudioDecoder.cpp
    Modules/webaudio/AudioBasicInspectorNode.cpp
    Modules/webaudio/AudioBasicProcessorNode.cpp
    Modules/webaudio/AudioBuffer.cpp
    Modules/webaudio/AudioBufferSourceNode.cpp
    Modules/webaudio/ChannelMergerNode.cpp
    Modules/webaudio/ChannelSplitterNode.cpp
    Modules/webaudio/AudioContext.cpp
    Modules/webaudio/AudioDestinationNode.cpp
    Modules/webaudio/GainNode.cpp
    Modules/webaudio/AudioListener.cpp
    Modules/webaudio/AudioNode.cpp
    Modules/webaudio/AudioNodeInput.cpp
    Modules/webaudio/AudioNodeOutput.cpp
    Modules/webaudio/PannerNode.cpp
    Modules/webaudio/AudioParam.cpp
    Modules/webaudio/AudioParamTimeline.cpp
    Modules/webaudio/AudioProcessingEvent.cpp
    Modules/webaudio/AudioScheduledSourceNode.cpp
    Modules/webaudio/AudioSummingJunction.cpp
    Modules/webaudio/BiquadDSPKernel.cpp
    Modules/webaudio/BiquadFilterNode.cpp
    Modules/webaudio/BiquadProcessor.cpp
    Modules/webaudio/ConvolverNode.cpp
    Modules/webaudio/DefaultAudioDestinationNode.cpp
    Modules/webaudio/DelayDSPKernel.cpp
    Modules/webaudio/DelayNode.cpp
    Modules/webaudio/DelayProcessor.cpp
    Modules/webaudio/DynamicsCompressorNode.cpp
    Modules/webaudio/ScriptProcessorNode.cpp
    Modules/webaudio/MediaElementAudioSourceNode.cpp
    Modules/webaudio/MediaStreamAudioSourceNode.cpp
    Modules/webaudio/OfflineAudioContext.cpp
    Modules/webaudio/OfflineAudioCompletionEvent.cpp
    Modules/webaudio/OfflineAudioDestinationNode.cpp
    Modules/webaudio/OscillatorNode.cpp
    Modules/webaudio/RealtimeAnalyser.cpp
    Modules/webaudio/AnalyserNode.cpp
    Modules/webaudio/WaveShaperDSPKernel.cpp
    Modules/webaudio/WaveShaperNode.cpp
    Modules/webaudio/WaveShaperProcessor.cpp
    Modules/webaudio/PeriodicWave.cpp

    Modules/webdatabase/ChangeVersionWrapper.cpp
    Modules/webdatabase/DOMWindowWebDatabase.cpp
    Modules/webdatabase/Database.cpp
    Modules/webdatabase/DatabaseAuthorizer.cpp
    Modules/webdatabase/DatabaseBackend.cpp
    Modules/webdatabase/DatabaseBackendBase.cpp
    Modules/webdatabase/DatabaseBackendContext.cpp
    Modules/webdatabase/DatabaseBackendSync.cpp
    Modules/webdatabase/DatabaseBase.cpp
    Modules/webdatabase/DatabaseContext.cpp
    Modules/webdatabase/DatabaseManager.cpp
    Modules/webdatabase/DatabaseServer.cpp
    Modules/webdatabase/DatabaseSync.cpp
    Modules/webdatabase/DatabaseTask.cpp
    Modules/webdatabase/DatabaseThread.cpp
    Modules/webdatabase/DatabaseTracker.cpp
    Modules/webdatabase/OriginLock.cpp
    Modules/webdatabase/SQLException.cpp
    Modules/webdatabase/SQLResultSet.cpp
    Modules/webdatabase/SQLResultSetRowList.cpp
    Modules/webdatabase/SQLStatement.cpp
    Modules/webdatabase/SQLStatementBackend.cpp
    Modules/webdatabase/SQLStatementSync.cpp
    Modules/webdatabase/SQLTransaction.cpp
    Modules/webdatabase/SQLTransactionBackend.cpp
    Modules/webdatabase/SQLTransactionBackendSync.cpp
    Modules/webdatabase/SQLTransactionClient.cpp
    Modules/webdatabase/SQLTransactionCoordinator.cpp
    Modules/webdatabase/SQLTransactionStateMachine.cpp
    Modules/webdatabase/SQLTransactionSync.cpp

    Modules/websockets/ThreadableWebSocketChannel.cpp
    Modules/websockets/ThreadableWebSocketChannelClientWrapper.cpp
    Modules/websockets/WebSocket.cpp
    Modules/websockets/WebSocketChannel.cpp
    Modules/websockets/WebSocketDeflater.cpp
    Modules/websockets/WebSocketDeflateFramer.cpp
    Modules/websockets/WebSocketExtensionDispatcher.cpp
    Modules/websockets/WebSocketExtensionParser.cpp
    Modules/websockets/WebSocketFrame.cpp
    Modules/websockets/WebSocketHandshake.cpp
    Modules/websockets/WorkerThreadableWebSocketChannel.cpp

    accessibility/AXObjectCache.cpp
    accessibility/AccessibilityARIAGrid.cpp
    accessibility/AccessibilityARIAGridCell.cpp
    accessibility/AccessibilityARIAGridRow.cpp
    accessibility/AccessibilityImageMapLink.cpp
    accessibility/AccessibilityList.cpp
    accessibility/AccessibilityListBox.cpp
    accessibility/AccessibilityListBoxOption.cpp
    accessibility/AccessibilityMediaControls.cpp
    accessibility/AccessibilityMenuList.cpp
    accessibility/AccessibilityMenuListOption.cpp
    accessibility/AccessibilityMenuListPopup.cpp
    accessibility/AccessibilityMockObject.cpp
    accessibility/AccessibilityNodeObject.cpp
    accessibility/AccessibilityObject.cpp
    accessibility/AccessibilityProgressIndicator.cpp
    accessibility/AccessibilityRenderObject.cpp
    accessibility/AccessibilityScrollbar.cpp
    accessibility/AccessibilityScrollView.cpp
    accessibility/AccessibilitySpinButton.cpp
    accessibility/AccessibilitySlider.cpp
    accessibility/AccessibilitySVGRoot.cpp
    accessibility/AccessibilityTable.cpp
    accessibility/AccessibilityTableCell.cpp
    accessibility/AccessibilityTableColumn.cpp
    accessibility/AccessibilityTableHeaderContainer.cpp
    accessibility/AccessibilityTableRow.cpp

    bindings/ScriptControllerBase.cpp

    bindings/generic/ActiveDOMCallback.cpp
    bindings/generic/BindingSecurity.cpp
    bindings/generic/RuntimeEnabledFeatures.cpp

    css/BasicShapeFunctions.cpp
    css/CSSAspectRatioValue.cpp
    css/CSSBasicShapes.cpp
    css/CSSBorderImage.cpp
    css/CSSBorderImageSliceValue.cpp
    css/CSSCalculationValue.cpp
    css/CSSCanvasValue.cpp
    css/CSSCharsetRule.cpp
    css/CSSComputedStyleDeclaration.cpp
    css/CSSCrossfadeValue.cpp
    css/CSSCursorImageValue.cpp
    css/CSSDefaultStyleSheets.cpp
    css/CSSFontFace.cpp
    css/CSSFontFaceLoadEvent.cpp
    css/CSSFontFaceRule.cpp
    css/CSSFontFaceSource.cpp
    css/CSSFontFaceSrcValue.cpp
    css/CSSFontSelector.cpp
    css/CSSFunctionValue.cpp
    css/CSSGradientValue.cpp
    css/CSSGroupingRule.cpp
    css/CSSHostRule.cpp
    css/CSSImageGeneratorValue.cpp
    css/CSSImageSetValue.cpp
    css/CSSImageValue.cpp
    css/CSSImportRule.cpp
    css/CSSInheritedValue.cpp
    css/CSSInitialValue.cpp
    css/CSSLineBoxContainValue.cpp
    css/CSSMediaRule.cpp
    css/CSSOMUtils.cpp
    css/CSSPageRule.cpp
    css/CSSParser.cpp
    css/CSSParserValues.cpp
    css/CSSPrimitiveValue.cpp
    css/CSSProperty.cpp
    css/CSSPropertySourceData.cpp
    css/CSSReflectValue.cpp
    css/CSSRule.cpp
    css/CSSRuleList.cpp
    css/CSSSegmentedFontFace.cpp
    css/CSSSelector.cpp
    css/CSSSelectorList.cpp
    css/CSSStyleRule.cpp
    css/CSSStyleSheet.cpp
    css/CSSSupportsRule.cpp
    css/CSSTimingFunctionValue.cpp
    css/CSSToStyleMap.cpp
    css/CSSUnicodeRangeValue.cpp
    css/CSSValue.cpp
    css/CSSValueList.cpp
    css/CSSValuePool.cpp
    css/DOMWindowCSS.cpp
    css/DeprecatedStyleBuilder.cpp
    css/DocumentRuleSets.cpp
    css/ElementRuleCollector.cpp
    css/FontFeatureValue.cpp
    css/FontLoader.cpp
    css/FontValue.cpp
    css/InspectorCSSOMWrappers.cpp
    css/LengthFunctions.cpp
    css/MediaFeatureNames.cpp
    css/MediaList.cpp
    css/MediaQuery.cpp
    css/MediaQueryEvaluator.cpp
    css/MediaQueryExp.cpp
    css/MediaQueryList.cpp
    css/MediaQueryListListener.cpp
    css/MediaQueryMatcher.cpp
    css/PageRuleCollector.cpp
    css/PropertySetCSSStyleDeclaration.cpp
    css/RGBColor.cpp
    css/RuleFeature.h
    css/RuleFeature.cpp
    css/RuleSet.h
    css/RuleSet.cpp
    css/SelectorChecker.cpp
    css/SelectorCheckerFastPath.cpp
    css/SelectorFilter.cpp
    css/ShadowValue.cpp
    css/StyleInvalidationAnalysis.cpp
    css/StyleMedia.cpp
    css/StylePropertySet.cpp
    css/StylePropertyShorthand.cpp
    css/StyleResolver.cpp
    css/StyleRule.cpp
    css/StyleRuleImport.cpp
    css/StyleScopeResolver.cpp
    css/StyleSheet.cpp
    css/StyleSheetContents.cpp
    css/StyleSheetList.cpp
    css/SVGCSSComputedStyleDeclaration.cpp
    css/SVGCSSParser.cpp
    css/SVGCSSStyleSelector.cpp
    css/TransformFunctions.cpp
    css/ViewportStyleResolver.cpp
    css/WebKitCSSArrayFunctionValue.cpp
    css/WebKitCSSFilterRule.cpp
    css/WebKitCSSFilterValue.cpp
    css/WebKitCSSKeyframeRule.cpp
    css/WebKitCSSKeyframesRule.cpp
    css/WebKitCSSMatrix.cpp
    css/WebKitCSSMatFunctionValue.cpp
    css/WebKitCSSMixFunctionValue.cpp
    css/WebKitCSSRegionRule.cpp
    css/WebKitCSSSVGDocumentValue.cpp
    css/WebKitCSSShaderValue.cpp
    css/WebKitCSSTransformValue.cpp
    css/WebKitCSSViewportRule.cpp

    dom/ActiveDOMObject.cpp
    dom/Attr.cpp
    dom/BeforeTextInsertedEvent.cpp
    dom/BeforeUnloadEvent.cpp
    dom/CDATASection.cpp
    dom/CharacterData.cpp
    dom/CheckedRadioButtons.cpp
    dom/ChildListMutationScope.cpp
    dom/ChildNodeList.cpp
    dom/ClassNodeList.cpp
    dom/ClientRect.cpp
    dom/ClientRectList.cpp
    dom/Clipboard.cpp
    dom/ClipboardEvent.cpp
    dom/Comment.cpp
    dom/ComposedShadowTreeWalker.cpp
    dom/CompositionEvent.cpp
    dom/ContainerNode.cpp
    dom/ContainerNodeAlgorithms.cpp
    dom/ContextDestructionObserver.cpp
    dom/ContextFeatures.cpp
    dom/CustomEvent.cpp
    dom/DatasetDOMStringMap.cpp
    dom/DataTransferItem.cpp
    dom/DecodedDataDocumentParser.cpp
    dom/DeviceMotionController.cpp
    dom/DeviceMotionData.cpp
    dom/DeviceMotionEvent.cpp
    dom/DeviceOrientationController.cpp
    dom/DeviceOrientationData.cpp
    dom/DeviceOrientationEvent.cpp
    dom/DOMCoreException.cpp
    dom/DOMError.cpp
    dom/DOMImplementation.cpp
    dom/DOMNamedFlowCollection.cpp
    dom/DOMStringList.cpp
    dom/DOMStringMap.cpp
    dom/Document.cpp
    dom/DocumentMarkerController.cpp
    dom/DocumentMarker.cpp
    dom/DocumentEventQueue.cpp
    dom/DocumentFragment.cpp
    dom/DocumentParser.cpp
    dom/DocumentOrderedMap.cpp
    dom/DocumentSharedObjectPool.cpp
    dom/DocumentStyleSheetCollection.cpp
    dom/DocumentType.cpp
    dom/Element.cpp
    dom/ElementRareData.cpp
    dom/ElementShadow.cpp
    dom/EntityReference.cpp
    dom/ErrorEvent.cpp
    dom/Event.cpp
    dom/EventContext.cpp
    dom/EventDispatchMediator.cpp
    dom/EventDispatcher.cpp
    dom/EventException.cpp
    dom/EventListenerMap.cpp
    dom/EventNames.cpp
    dom/EventPathWalker.cpp
    dom/EventRetargeter.cpp
    dom/EventTarget.cpp
    dom/ExceptionBase.cpp
    dom/ExceptionCodePlaceholder.cpp
    dom/FocusEvent.cpp
    dom/GenericEventQueue.cpp
    dom/GestureEvent.cpp
    dom/IconURL.cpp
    dom/IdTargetObserver.cpp
    dom/IdTargetObserverRegistry.cpp
    dom/LiveNodeList.cpp
    dom/KeyboardEvent.cpp
    dom/MessageChannel.cpp
    dom/MessageEvent.cpp
    dom/MessagePortChannel.cpp
    dom/MessagePort.cpp
    dom/MicroDataItemList.cpp
    dom/MouseEvent.cpp
    dom/MouseRelatedEvent.cpp
    dom/MutationEvent.cpp
    dom/MutationObserver.cpp
    dom/MutationObserverInterestGroup.cpp
    dom/MutationObserverRegistration.cpp
    dom/MutationRecord.cpp
    dom/NamedFlowCollection.cpp
    dom/NamedNodeMap.cpp
    dom/NameNodeList.cpp
    dom/Node.cpp
    dom/NodeFilterCondition.cpp
    dom/NodeFilter.cpp
    dom/NodeIterator.cpp
    dom/NodeRareData.cpp
    dom/NodeRenderingContext.cpp
    dom/NodeRenderingTraversal.cpp
    dom/NodeTraversal.cpp
    dom/Notation.cpp
    dom/OverflowEvent.cpp
    dom/PageTransitionEvent.cpp
    dom/PendingScript.cpp
    dom/PopStateEvent.cpp
    dom/Position.cpp
    dom/PositionIterator.cpp
    dom/ProcessingInstruction.cpp
    dom/ProgressEvent.cpp
    dom/PropertyNodeList.cpp
    dom/PseudoElement.cpp
    dom/QualifiedName.cpp
    dom/Range.cpp
    dom/RangeException.cpp
    dom/RegisteredEventListener.cpp
    dom/ScopedEventQueue.cpp
    dom/ScriptedAnimationController.cpp
    dom/ScriptableDocumentParser.cpp
    dom/ScriptElement.cpp
    dom/ScriptExecutionContext.cpp
    dom/ScriptRunner.cpp
    dom/SecurityContext.cpp
    dom/SelectorQuery.cpp
    dom/ShadowRoot.cpp
    dom/SpaceSplitString.cpp
    dom/StaticNodeList.cpp
    dom/StringCallback.cpp
    dom/StyledElement.cpp
    dom/StyleElement.cpp
    dom/TagNodeList.cpp
    dom/Text.cpp
    dom/TextEvent.cpp
    dom/Touch.cpp
    dom/TouchEvent.cpp
    dom/TouchList.cpp
    dom/TransformSourceLibxslt.cpp
    dom/TransitionEvent.cpp
    dom/Traversal.cpp
    dom/TreeScope.cpp
    dom/TreeScopeAdopter.cpp
    dom/TreeWalker.cpp
    dom/UIEvent.cpp
    dom/UIEventWithKeyState.cpp
    dom/UserActionElementSet.cpp
    dom/UserGestureIndicator.cpp
    dom/UserTypingGestureIndicator.cpp
    dom/ViewportArguments.cpp
    dom/VisitedLinkState.cpp
    dom/WebKitAnimationEvent.cpp
    dom/WebKitNamedFlow.cpp
    dom/WebKitTransitionEvent.cpp
    dom/WheelEvent.cpp
    dom/WindowEventContext.cpp

    dom/default/PlatformMessagePortChannel.cpp

    editing/AlternativeTextController.cpp
    editing/AppendNodeCommand.cpp
    editing/ApplyBlockElementCommand.cpp
    editing/ApplyStyleCommand.cpp
    editing/BreakBlockquoteCommand.cpp
    editing/CompositeEditCommand.cpp
    editing/CreateLinkCommand.cpp
    editing/DeleteButton.cpp
    editing/DeleteButtonController.cpp
    editing/DeleteFromTextNodeCommand.cpp
    editing/DeleteSelectionCommand.cpp
    editing/DictationAlternative.cpp
    editing/DictationCommand.cpp
    editing/EditCommand.cpp
    editing/EditingStyle.cpp
    editing/Editor.cpp
    editing/EditorCommand.cpp
    editing/FormatBlockCommand.cpp
    editing/FrameSelection.cpp
    editing/HTMLInterchange.cpp
    editing/IndentOutdentCommand.cpp
    editing/InsertIntoTextNodeCommand.cpp
    editing/InsertLineBreakCommand.cpp
    editing/InsertListCommand.cpp
    editing/InsertNodeBeforeCommand.cpp
    editing/InsertParagraphSeparatorCommand.cpp
    editing/InsertTextCommand.cpp
    editing/MarkupAccumulator.cpp
    editing/MergeIdenticalElementsCommand.cpp
    editing/ModifySelectionListLevel.cpp
    editing/MoveSelectionCommand.cpp
    editing/RemoveCSSPropertyCommand.cpp
    editing/RemoveFormatCommand.cpp
    editing/RemoveNodeCommand.cpp
    editing/RemoveNodePreservingChildrenCommand.cpp
    editing/RenderedPosition.cpp
    editing/ReplaceNodeWithSpanCommand.cpp
    editing/ReplaceSelectionCommand.cpp
    editing/SetNodeAttributeCommand.cpp
    editing/SetSelectionCommand.cpp
    editing/SimplifyMarkupCommand.cpp
    editing/SmartReplace.cpp
    editing/SpellChecker.cpp
    editing/SpellingCorrectionCommand.cpp
    editing/SplitElementCommand.cpp
    editing/SplitTextNodeCommand.cpp
    editing/SplitTextNodeContainingElementCommand.cpp
    editing/TextCheckingHelper.cpp
    editing/TextInsertionBaseCommand.cpp
    editing/TextIterator.cpp
    editing/TypingCommand.cpp
    editing/UnlinkCommand.cpp
    editing/VisiblePosition.cpp
    editing/VisibleSelection.cpp
    editing/VisibleUnits.cpp
    editing/WrapContentsInDummySpanCommand.cpp
    editing/htmlediting.cpp
    editing/markup.cpp

    fileapi/AsyncFileStream.cpp
    fileapi/Blob.cpp
    fileapi/BlobURL.cpp
    fileapi/File.cpp
    fileapi/FileException.cpp
    fileapi/FileList.cpp
    fileapi/FileReader.cpp
    fileapi/FileReaderLoader.cpp
    fileapi/FileReaderSync.cpp
    fileapi/FileThread.cpp
    fileapi/ThreadableBlobRegistry.cpp
    fileapi/WebKitBlobBuilder.cpp

    history/BackForwardController.cpp
    history/BackForwardListImpl.cpp
    history/CachedFrame.cpp
    history/CachedPage.cpp
    history/HistoryItem.cpp
    history/PageCache.cpp

    html/BaseButtonInputType.cpp
    html/BaseCheckableInputType.cpp
    html/BaseChooserOnlyDateAndTimeInputType.cpp
    html/BaseClickableWithKeyInputType.cpp
    html/BaseDateAndTimeInputType.cpp
    html/BaseTextInputType.cpp
    html/ButtonInputType.cpp
    html/CheckboxInputType.cpp
    html/ClassList.cpp
    html/ColorInputType.cpp
    html/DOMFormData.cpp
    html/DOMSettableTokenList.cpp
    html/DOMTokenList.cpp
    html/DOMURL.cpp
    html/DateInputType.cpp
    html/DateTimeInputType.cpp
    html/DateTimeLocalInputType.cpp
    html/EmailInputType.cpp
    html/FTPDirectoryDocument.cpp
    html/FileInputType.cpp
    html/FormAssociatedElement.cpp
    html/FormController.cpp
    html/FormDataList.cpp
    html/HTMLAllCollection.cpp
    html/HTMLAnchorElement.cpp
    html/HTMLAppletElement.cpp
    html/HTMLAreaElement.cpp
    html/HTMLAudioElement.cpp
    html/HTMLBRElement.cpp
    html/HTMLBaseElement.cpp
    html/HTMLBaseFontElement.cpp
    html/HTMLBodyElement.cpp
    html/HTMLButtonElement.cpp
    html/HTMLCanvasElement.cpp
    html/HTMLCollection.cpp
    html/HTMLDListElement.cpp
    html/HTMLDataListElement.cpp
    html/HTMLDetailsElement.cpp
    html/HTMLDialogElement.cpp
    html/HTMLDirectoryElement.cpp
    html/HTMLDivElement.cpp
    html/HTMLDocument.cpp
    html/HTMLElement.cpp
    html/HTMLEmbedElement.cpp
    html/HTMLFieldSetElement.cpp
    html/HTMLFontElement.cpp
    html/HTMLFormControlsCollection.cpp
    html/HTMLFormControlElement.cpp
    html/HTMLFormControlElementWithState.cpp
    html/HTMLFormElement.cpp
    html/HTMLFrameElement.cpp
    html/HTMLFrameElementBase.cpp
    html/HTMLFrameOwnerElement.cpp
    html/HTMLFrameSetElement.cpp
    html/HTMLHRElement.cpp
    html/HTMLHeadElement.cpp
    html/HTMLHeadingElement.cpp
    html/HTMLHtmlElement.cpp
    html/HTMLIFrameElement.cpp
    html/HTMLImageElement.cpp
    html/HTMLImageLoader.cpp
    html/HTMLInputElement.cpp
    html/HTMLKeygenElement.cpp
    html/HTMLLIElement.cpp
    html/HTMLLabelElement.cpp
    html/HTMLLegendElement.cpp
    html/HTMLLinkElement.cpp
    html/HTMLMapElement.cpp
    html/HTMLMarqueeElement.cpp
    html/HTMLMediaElement.cpp
    html/HTMLMenuElement.cpp
    html/HTMLMetaElement.cpp
    html/HTMLMeterElement.cpp
    html/HTMLModElement.cpp
    html/HTMLNameCollection.cpp
    html/HTMLOListElement.cpp
    html/HTMLObjectElement.cpp
    html/HTMLOptGroupElement.cpp
    html/HTMLOptionElement.cpp
    html/HTMLOptionsCollection.cpp
    html/HTMLOutputElement.cpp
    html/HTMLParagraphElement.cpp
    html/HTMLParamElement.cpp
    html/HTMLParserErrorCodes.cpp
    html/HTMLPlugInElement.cpp
    html/HTMLPlugInImageElement.cpp
    html/HTMLPreElement.cpp
    html/HTMLProgressElement.cpp
    html/HTMLPropertiesCollection.cpp
    html/HTMLQuoteElement.cpp
    html/HTMLScriptElement.cpp
    html/HTMLSelectElement.cpp
    html/HTMLSourceElement.cpp
    html/HTMLSpanElement.cpp
    html/HTMLStyleElement.cpp
    html/HTMLSummaryElement.cpp
    html/HTMLTableCaptionElement.cpp
    html/HTMLTableCellElement.cpp
    html/HTMLTableColElement.cpp
    html/HTMLTableElement.cpp
    html/HTMLTablePartElement.cpp
    html/HTMLTableRowElement.cpp
    html/HTMLTableRowsCollection.cpp
    html/HTMLTableSectionElement.cpp
    html/HTMLTemplateElement.cpp
    html/HTMLTextAreaElement.cpp
    html/HTMLTextFormControlElement.cpp
    html/HTMLTitleElement.cpp
    html/HTMLTrackElement.cpp
    html/HTMLUListElement.cpp
    html/HTMLVideoElement.cpp
    html/HTMLViewSourceDocument.cpp
    html/HiddenInputType.cpp
    html/ImageData.cpp
    html/ImageDocument.cpp
    html/ImageInputType.cpp
    html/InputType.cpp
    html/InputTypeNames.cpp
    html/LabelableElement.cpp
    html/LabelsNodeList.cpp
    html/LinkRelAttribute.cpp
    html/MediaController.cpp
    html/MediaDocument.cpp
    html/MediaFragmentURIParser.cpp
    html/MicroDataAttributeTokenList.cpp
    html/MicroDataItemValue.cpp
    html/MonthInputType.cpp
    html/NumberInputType.cpp
    html/PasswordInputType.cpp
    html/PluginDocument.cpp
    html/RadioInputType.cpp
    html/RadioNodeList.cpp
    html/RangeInputType.cpp
    html/ResetInputType.cpp
    html/SearchInputType.cpp
    html/StepRange.cpp
    html/SubmitInputType.cpp
    html/TelephoneInputType.cpp
    html/TextDocument.cpp
    html/TextFieldInputType.cpp
    html/TextInputType.cpp
    html/TimeInputType.cpp
    html/TimeRanges.cpp
    html/TypeAhead.cpp
    html/URLInputType.cpp
    html/ValidationMessage.cpp
    html/ValidityState.cpp
    html/WeekInputType.cpp

    html/canvas/CanvasContextAttributes.cpp
    html/canvas/CanvasGradient.cpp
    html/canvas/CanvasPathMethods.cpp
    html/canvas/CanvasPattern.cpp
    html/canvas/CanvasProxy.cpp
    html/canvas/CanvasRenderingContext.cpp
    html/canvas/CanvasRenderingContext2D.cpp
    html/canvas/CanvasStyle.cpp
    html/canvas/DataView.cpp
    html/canvas/EXTDrawBuffers.cpp
    html/canvas/EXTTextureFilterAnisotropic.cpp
    html/canvas/OESStandardDerivatives.cpp
    html/canvas/OESTextureFloat.cpp
    html/canvas/OESTextureHalfFloat.cpp
    html/canvas/OESVertexArrayObject.cpp
    html/canvas/OESElementIndexUint.cpp
    html/canvas/WebGLBuffer.cpp
    html/canvas/WebGLCompressedTextureATC.cpp
    html/canvas/WebGLCompressedTexturePVRTC.cpp
    html/canvas/WebGLCompressedTextureS3TC.cpp
    html/canvas/WebGLContextAttributes.cpp
    html/canvas/WebGLContextEvent.cpp
    html/canvas/WebGLContextGroup.cpp
    html/canvas/WebGLContextObject.cpp
    html/canvas/WebGLDebugRendererInfo.cpp
    html/canvas/WebGLDebugShaders.cpp
    html/canvas/WebGLDepthTexture.cpp
    html/canvas/WebGLExtension.cpp
    html/canvas/WebGLFramebuffer.cpp
    html/canvas/WebGLGetInfo.cpp
    html/canvas/WebGLLoseContext.cpp
    html/canvas/WebGLObject.cpp
    html/canvas/WebGLProgram.cpp
    html/canvas/WebGLRenderbuffer.cpp
    html/canvas/WebGLRenderingContext.cpp
    html/canvas/WebGLShader.cpp
    html/canvas/WebGLShaderPrecisionFormat.cpp
    html/canvas/WebGLSharedObject.cpp
    html/canvas/WebGLTexture.cpp
    html/canvas/WebGLUniformLocation.cpp
    html/canvas/WebGLVertexArrayObjectOES.cpp

    html/forms/FileIconLoader.cpp

    html/parser/BackgroundHTMLInputStream.cpp
    html/parser/BackgroundHTMLParser.cpp
    html/parser/CSSPreloadScanner.cpp
    html/parser/CompactHTMLToken.cpp
    html/parser/HTMLConstructionSite.cpp
    html/parser/HTMLDocumentParser.cpp
    html/parser/HTMLElementStack.cpp
    html/parser/HTMLEntityParser.cpp
    html/parser/HTMLEntitySearch.cpp
    html/parser/HTMLParserIdioms.cpp
    html/parser/HTMLParserOptions.cpp
    html/parser/HTMLParserScheduler.cpp
    html/parser/HTMLParserThread.cpp
    html/parser/HTMLFormattingElementList.cpp
    html/parser/HTMLIdentifier.cpp
    html/parser/HTMLMetaCharsetParser.cpp
    html/parser/HTMLPreloadScanner.cpp
    html/parser/HTMLResourcePreloader.cpp
    html/parser/HTMLScriptRunner.cpp
    html/parser/HTMLSourceTracker.cpp
    html/parser/HTMLTokenizer.cpp
    html/parser/HTMLTreeBuilder.cpp
    html/parser/HTMLTreeBuilderSimulator.cpp
    html/parser/HTMLViewSourceParser.cpp
    html/parser/TextDocumentParser.cpp
    html/parser/TextViewSourceParser.cpp
    html/parser/XSSAuditor.cpp
    html/parser/XSSAuditorDelegate.cpp

    html/shadow/ContentDistributor.cpp
    html/shadow/DetailsMarkerControl.cpp
    html/shadow/HTMLContentElement.cpp
    html/shadow/InsertionPoint.cpp
    html/shadow/MediaControlElementTypes.cpp
    html/shadow/MediaControlElements.cpp
    html/shadow/MediaControls.cpp
    html/shadow/MediaControlsApple.cpp
    html/shadow/MeterShadowElement.cpp
    html/shadow/ProgressShadowElement.cpp
    html/shadow/SliderThumbElement.cpp
    html/shadow/SpinButtonElement.cpp
    html/shadow/TextControlInnerElements.cpp

    inspector/ConsoleMessage.cpp
    inspector/ContentSearchUtils.cpp
    inspector/DOMEditor.cpp
    inspector/DOMPatchSupport.cpp
    inspector/IdentifiersFactory.cpp
    inspector/InjectedScript.cpp
    inspector/InjectedScriptBase.cpp
    inspector/InjectedScriptCanvasModule.cpp
    inspector/InjectedScriptHost.cpp
    inspector/InjectedScriptManager.cpp
    inspector/InjectedScriptModule.cpp
    inspector/InspectorAgent.cpp
    inspector/InspectorApplicationCacheAgent.cpp
    inspector/InspectorBaseAgent.cpp
    inspector/InspectorPageAgent.cpp
    inspector/InspectorCSSAgent.cpp
    inspector/InspectorCanvasAgent.cpp
    inspector/InspectorClient.cpp
    inspector/InspectorConsoleAgent.cpp
    inspector/InspectorController.cpp
    inspector/InspectorCounters.cpp
    inspector/InspectorDOMAgent.cpp
    inspector/InspectorDOMDebuggerAgent.cpp
    inspector/InspectorDOMStorageAgent.cpp
    inspector/InspectorDatabaseAgent.cpp
    inspector/InspectorDatabaseResource.cpp
    inspector/InspectorDebuggerAgent.cpp
    inspector/InspectorFileSystemAgent.cpp
    inspector/InspectorFrontendClientLocal.cpp
    inspector/InspectorFrontendHost.cpp
    inspector/InspectorHeapProfilerAgent.cpp
    inspector/InspectorHistory.cpp
    inspector/InspectorIndexedDBAgent.cpp
    inspector/InspectorInputAgent.cpp
    inspector/InspectorInstrumentation.cpp
    inspector/InspectorLayerTreeAgent.cpp
    inspector/InspectorMemoryAgent.cpp
    inspector/InspectorOverlay.cpp
    inspector/InspectorProfilerAgent.cpp
    inspector/InspectorResourceAgent.cpp
    inspector/InspectorRuntimeAgent.cpp
    inspector/InspectorState.cpp
    inspector/InspectorStyleSheet.cpp
    inspector/InspectorStyleTextEditor.cpp
    inspector/InspectorTimelineAgent.cpp
    inspector/InspectorValues.cpp
    inspector/InspectorWorkerAgent.cpp
    inspector/InstrumentingAgents.cpp
    inspector/NetworkResourcesData.cpp
    inspector/PageConsoleAgent.cpp
    inspector/PageDebuggerAgent.cpp
    inspector/PageRuntimeAgent.cpp
    inspector/ScriptArguments.cpp
    inspector/ScriptCallFrame.cpp
    inspector/ScriptCallStack.cpp
    inspector/TimelineRecordFactory.cpp
    inspector/TimelineTraceEventProcessor.cpp
    inspector/WorkerConsoleAgent.cpp
    inspector/WorkerDebuggerAgent.cpp
    inspector/WorkerInspectorController.cpp
    inspector/WorkerRuntimeAgent.cpp

    loader/CookieJar.cpp
    loader/CrossOriginAccessControl.cpp
    loader/CrossOriginPreflightResultCache.cpp
    loader/DocumentLoadTiming.cpp
    loader/DocumentLoader.cpp
    loader/DocumentThreadableLoader.cpp
    loader/DocumentWriter.cpp
    loader/EmptyClients.cpp
    loader/FTPDirectoryParser.cpp
    loader/FormState.cpp
    loader/FormSubmission.cpp
    loader/FrameLoadRequest.cpp
    loader/FrameLoader.cpp
    loader/FrameLoaderStateMachine.cpp
    loader/HistoryController.cpp
    loader/ImageLoader.cpp
    loader/LinkLoader.cpp
    loader/LoaderStrategy.cpp
    loader/MixedContentChecker.cpp
    loader/NavigationAction.cpp
    loader/NetscapePlugInStreamLoader.cpp
    loader/PingLoader.cpp
    loader/PlaceholderDocument.cpp
    loader/PolicyCallback.cpp
    loader/PolicyChecker.cpp
    loader/ProgressTracker.cpp
    loader/NavigationScheduler.cpp
    loader/ResourceBuffer.cpp
    loader/ResourceLoadNotifier.cpp
    loader/ResourceLoadScheduler.cpp
    loader/ResourceLoader.cpp
    loader/SinkDocument.cpp
    loader/SubframeLoader.cpp
    loader/SubresourceLoader.cpp
    loader/TextResourceDecoder.cpp
    loader/TextTrackLoader.cpp
    loader/ThreadableLoader.cpp
    loader/WorkerThreadableLoader.cpp

    loader/appcache/ApplicationCache.cpp
    loader/appcache/ApplicationCacheGroup.cpp
    loader/appcache/ApplicationCacheHost.cpp
    loader/appcache/ApplicationCacheResource.cpp
    loader/appcache/ApplicationCacheStorage.cpp
    loader/appcache/DOMApplicationCache.cpp
    loader/appcache/ManifestParser.cpp

    loader/archive/Archive.cpp
    loader/archive/ArchiveFactory.cpp
    loader/archive/ArchiveResource.cpp
    loader/archive/ArchiveResourceCollection.cpp

    loader/archive/mhtml/MHTMLArchive.cpp
    loader/archive/mhtml/MHTMLParser.cpp

    loader/cache/CachedCSSStyleSheet.cpp
    loader/cache/CachedFont.cpp
    loader/cache/CachedImage.cpp
    loader/cache/CachedRawResource.cpp
    loader/cache/CachedResource.cpp
    loader/cache/CachedResourceHandle.cpp
    loader/cache/CachedResourceLoader.cpp
    loader/cache/CachedResourceRequest.cpp
    loader/cache/CachedResourceRequestInitiators.cpp
    loader/cache/CachedScript.cpp
    loader/cache/CachedShader.cpp
    loader/cache/CachedSVGDocument.cpp
    loader/cache/CachedSVGDocumentReference.cpp
    loader/cache/CachedXSLStyleSheet.cpp
    loader/cache/MemoryCache.cpp

    loader/icon/IconController.cpp
    loader/icon/IconDatabase.cpp
    loader/icon/IconDatabaseBase.cpp
    loader/icon/IconLoader.cpp
    loader/icon/IconRecord.cpp
    loader/icon/PageURLRecord.cpp

    mathml/MathMLElement.cpp
    mathml/MathMLInlineContainerElement.cpp
    mathml/MathMLMathElement.cpp
    mathml/MathMLTextElement.cpp

    page/AutoscrollController.cpp
    page/BarProp.cpp
    page/CaptionUserPreferences.cpp
    page/Chrome.cpp
    page/Console.cpp
    page/ContentSecurityPolicy.cpp
    page/ContextMenuController.cpp
    page/Crypto.cpp
    page/DeviceController.cpp
    page/DiagnosticLoggingKeys.cpp
    page/DOMSecurityPolicy.cpp
    page/DOMSelection.cpp
    page/DOMTimer.cpp
    page/DOMWindow.cpp
    page/DOMWindowExtension.cpp
    page/DOMWindowProperty.cpp
    page/DragController.cpp
    page/EventHandler.cpp
    page/EventSource.cpp
    page/FeatureObserver.cpp
    page/FocusController.cpp
    page/Frame.cpp
    page/FrameActionScheduler.cpp
    page/FrameDestructionObserver.cpp
    page/FrameTree.cpp
    page/FrameView.cpp
    page/GroupSettings.cpp
    page/GestureTapHighlighter.cpp
    page/History.cpp
    page/Location.cpp
    page/MouseEventWithHitTestResults.cpp
    page/Navigator.cpp
    page/NavigatorBase.cpp
    page/OriginAccessEntry.cpp
    page/Page.cpp
    page/PageActivityAssertionToken.cpp
    page/PageConsole.cpp
    page/PageGroup.cpp
    page/PageGroupLoadDeferrer.cpp
    page/PageSerializer.cpp
    page/PageThrottler.cpp
    page/PageVisibilityState.cpp
    page/Performance.cpp
    page/PerformanceEntry.cpp
    page/PerformanceEntryList.cpp
    page/PerformanceNavigation.cpp
    page/PerformanceResourceTiming.cpp
    page/PerformanceTiming.cpp
    page/PrintContext.cpp
    page/Screen.cpp
    page/SecurityOrigin.cpp
    page/SecurityPolicy.cpp
    page/Settings.cpp
    page/SpatialNavigation.cpp
    page/SuspendableTimer.cpp
    page/UserContentURLPattern.cpp
    page/WindowFeatures.cpp
    page/WindowFocusAllowedIndicator.cpp
    page/WorkerNavigator.cpp

    page/animation/AnimationBase.cpp
    page/animation/AnimationController.cpp
    page/animation/CompositeAnimation.cpp
    page/animation/CSSPropertyAnimation.cpp
    page/animation/ImplicitAnimation.cpp
    page/animation/KeyframeAnimation.cpp

    page/scrolling/ScrollingConstraints.cpp
    page/scrolling/ScrollingCoordinator.cpp
    page/scrolling/ScrollingStateFixedNode.cpp
    page/scrolling/ScrollingStateNode.cpp
    page/scrolling/ScrollingStateScrollingNode.cpp
    page/scrolling/ScrollingStateStickyNode.cpp
    page/scrolling/ScrollingStateTree.cpp
    page/scrolling/coordinatedgraphics/ScrollingCoordinatorCoordinatedGraphics.cpp
    page/scrolling/coordinatedgraphics/ScrollingStateNodeCoordinatedGraphics.cpp
    page/scrolling/coordinatedgraphics/ScrollingStateScrollingNodeCoordinatedGraphics.cpp

    platform/Arena.cpp
    platform/AsyncFileSystem.cpp
    platform/CalculationValue.cpp
    platform/Clock.cpp
    platform/ContextMenu.cpp
    platform/ContextMenuItem.cpp
    platform/ContentType.cpp
    platform/CrossThreadCopier.cpp
    platform/DatabaseStrategy.cpp
    platform/DateComponents.cpp
    platform/Decimal.cpp
    platform/DragData.cpp
    platform/DragImage.cpp
    platform/FileChooser.cpp
    platform/FileStream.cpp
    platform/FileSystem.cpp
    platform/ClockGeneric.cpp
    platform/HistogramSupport.cpp
    platform/KURL.cpp
    platform/KillRingNone.cpp
    platform/Language.cpp
    platform/Length.cpp
    platform/LengthBox.cpp
    platform/LinkHash.cpp
    platform/Logging.cpp
    platform/MIMETypeRegistry.cpp
    platform/MemoryPressureHandler.cpp
    platform/NotImplemented.cpp
    platform/PlatformEvent.cpp
    platform/PlatformInstrumentation.cpp
    platform/PlatformStrategies.cpp
    platform/RunLoop.cpp
    platform/RuntimeApplicationChecks.cpp
    platform/ScrollAnimator.cpp
    platform/ScrollableArea.cpp
    platform/Scrollbar.cpp
    platform/ScrollbarTheme.cpp
    platform/ScrollbarThemeComposite.cpp
    platform/ScrollView.cpp
    platform/SharedBuffer.cpp
    platform/SharedBufferChunkReader.cpp
    platform/SchemeRegistry.cpp
    platform/ThreadGlobalData.cpp
    platform/ThreadTimers.cpp
    platform/Timer.cpp
    platform/UUID.cpp
    platform/Widget.cpp

    platform/animation/Animation.cpp
    platform/animation/AnimationList.cpp

    platform/audio/AudioBus.cpp
    platform/audio/AudioChannel.cpp
    platform/audio/AudioDSPKernelProcessor.cpp
    platform/audio/AudioFIFO.cpp
    platform/audio/AudioPullFIFO.cpp
    platform/audio/AudioResampler.cpp
    platform/audio/AudioResamplerKernel.cpp
    platform/audio/AudioUtilities.cpp
    platform/audio/Biquad.cpp
    platform/audio/Cone.cpp
    platform/audio/DirectConvolver.cpp
    platform/audio/Distance.cpp
    platform/audio/DownSampler.cpp
    platform/audio/DynamicsCompressor.cpp
    platform/audio/DynamicsCompressorKernel.cpp
    platform/audio/EqualPowerPanner.cpp
    platform/audio/FFTConvolver.cpp
    platform/audio/FFTFrame.cpp
    platform/audio/HRTFDatabase.cpp
    platform/audio/HRTFDatabaseLoader.cpp
    platform/audio/HRTFElevation.cpp
    platform/audio/HRTFKernel.cpp
    platform/audio/HRTFPanner.cpp
    platform/audio/MultiChannelResampler.cpp
    platform/audio/Panner.cpp
    platform/audio/Reverb.cpp
    platform/audio/ReverbAccumulationBuffer.cpp
    platform/audio/ReverbConvolver.cpp
    platform/audio/ReverbConvolverStage.cpp
    platform/audio/ReverbInputBuffer.cpp
    platform/audio/SincResampler.cpp
    platform/audio/UpSampler.cpp
    platform/audio/VectorMath.cpp
    platform/audio/ZeroPole.cpp

    platform/graphics/BitmapImage.cpp
    platform/graphics/Color.cpp
    platform/graphics/CrossfadeGeneratedImage.cpp
    platform/graphics/FloatPoint.cpp
    platform/graphics/FloatPoint3D.cpp
    platform/graphics/FloatPolygon.cpp
    platform/graphics/FloatQuad.cpp
    platform/graphics/FloatRect.cpp
    platform/graphics/FloatSize.cpp
    platform/graphics/Font.cpp
    platform/graphics/FontCache.cpp
    platform/graphics/FontData.cpp
    platform/graphics/FontDescription.cpp
    platform/graphics/FontGenericFamilies.cpp
    platform/graphics/FontGlyphs.cpp
    platform/graphics/FontFastPath.cpp
    platform/graphics/FontFeatureSettings.cpp
    platform/graphics/LayoutBoxExtent.cpp
    platform/graphics/LayoutRect.cpp
    platform/graphics/GeneratedImage.cpp
    platform/graphics/GeneratorGeneratedImage.cpp
    platform/graphics/GlyphPageTreeNode.cpp
    platform/graphics/Gradient.cpp
    platform/graphics/GraphicsContext.cpp
    platform/graphics/GraphicsLayer.cpp
    platform/graphics/GraphicsLayerAnimation.cpp
    platform/graphics/GraphicsLayerUpdater.cpp
    platform/graphics/GraphicsLayerTransform.cpp
    platform/graphics/GraphicsTypes.cpp
    platform/graphics/Image.cpp
    platform/graphics/ImageBuffer.cpp
    platform/graphics/ImageOrientation.cpp
    platform/graphics/IntRect.cpp
    platform/graphics/MediaPlayer.cpp
    platform/graphics/Path.cpp
    platform/graphics/PathTraversalState.cpp
    platform/graphics/Pattern.cpp
    platform/graphics/Region.cpp
    platform/graphics/RoundedRect.cpp
    platform/graphics/SegmentedFontData.cpp
    platform/graphics/ShadowBlur.cpp
    platform/graphics/SimpleFontData.cpp
    platform/graphics/StringTruncator.cpp
    platform/graphics/SurrogatePairAwareTextIterator.cpp
    platform/graphics/SVGGlyph.cpp
    platform/graphics/TextRun.cpp
    platform/graphics/TiledBackingStore.cpp
    platform/graphics/WidthIterator.cpp

    platform/graphics/cpu/arm/filters/FELightingNEON.cpp

    platform/graphics/filters/texmap/CustomFilterValidatedProgramTextureMapper.cpp
    platform/graphics/filters/CustomFilterCompiledProgram.cpp
    platform/graphics/filters/CustomFilterGlobalContext.cpp
    platform/graphics/filters/CustomFilterMesh.cpp
    platform/graphics/filters/CustomFilterMeshGenerator.cpp
    platform/graphics/filters/CustomFilterOperation.cpp
    platform/graphics/filters/CustomFilterParameterList.cpp
    platform/graphics/filters/ValidatedCustomFilterOperation.cpp
    platform/graphics/filters/CustomFilterProgram.cpp
    platform/graphics/filters/CustomFilterProgramInfo.cpp
    platform/graphics/filters/CustomFilterRenderer.cpp
    platform/graphics/filters/CustomFilterValidatedProgram.cpp
    platform/graphics/filters/DistantLightSource.cpp
    platform/graphics/filters/FEBlend.cpp
    platform/graphics/filters/FEColorMatrix.cpp
    platform/graphics/filters/FEComponentTransfer.cpp
    platform/graphics/filters/FEComposite.cpp
    platform/graphics/filters/FEConvolveMatrix.cpp
    platform/graphics/filters/FECustomFilter.cpp
    platform/graphics/filters/FEDiffuseLighting.cpp
    platform/graphics/filters/FEDisplacementMap.cpp
    platform/graphics/filters/FEDropShadow.cpp
    platform/graphics/filters/FEFlood.cpp
    platform/graphics/filters/FEGaussianBlur.cpp
    platform/graphics/filters/FELighting.cpp
    platform/graphics/filters/FEMerge.cpp
    platform/graphics/filters/FEMorphology.cpp
    platform/graphics/filters/FEOffset.cpp
    platform/graphics/filters/FESpecularLighting.cpp
    platform/graphics/filters/FETile.cpp
    platform/graphics/filters/FETurbulence.cpp
    platform/graphics/filters/FilterEffect.cpp
    platform/graphics/filters/FilterOperation.cpp
    platform/graphics/filters/FilterOperations.cpp
    platform/graphics/filters/PointLightSource.cpp
    platform/graphics/filters/SourceAlpha.cpp
    platform/graphics/filters/SourceGraphic.cpp
    platform/graphics/filters/SpotLightSource.cpp

    platform/graphics/texmap/coordinated/AreaAllocator.cpp
    platform/graphics/texmap/coordinated/CompositingCoordinator.cpp
    platform/graphics/texmap/coordinated/CoordinatedBackingStore.cpp
    platform/graphics/texmap/coordinated/CoordinatedGraphicsLayer.cpp
    platform/graphics/texmap/coordinated/CoordinatedGraphicsScene.cpp
    platform/graphics/texmap/coordinated/CoordinatedImageBacking.cpp
    platform/graphics/texmap/coordinated/CoordinatedSurface.cpp
    platform/graphics/texmap/coordinated/CoordinatedTile.cpp
    platform/graphics/texmap/coordinated/UpdateAtlas.cpp
    platform/graphics/texmap/TextureMapper.cpp
    platform/graphics/texmap/TextureMapperBackingStore.cpp
    platform/graphics/texmap/TextureMapperFPSCounter.cpp
    platform/graphics/texmap/TextureMapperImageBuffer.cpp
    platform/graphics/texmap/TextureMapperLayer.cpp
    platform/graphics/texmap/TextureMapperSurfaceBackingStore.cpp
    platform/graphics/texmap/TextureMapperTile.cpp
    platform/graphics/texmap/TextureMapperTiledBackingStore.cpp

    platform/graphics/transforms/AffineTransform.cpp
    platform/graphics/transforms/Matrix3DTransformOperation.cpp
    platform/graphics/transforms/MatrixTransformOperation.cpp
    platform/graphics/transforms/PerspectiveTransformOperation.cpp
    platform/graphics/transforms/RotateTransformOperation.cpp
    platform/graphics/transforms/ScaleTransformOperation.cpp
    platform/graphics/transforms/SkewTransformOperation.cpp
    platform/graphics/transforms/TransformOperations.cpp
    platform/graphics/transforms/TransformationMatrix.cpp
    platform/graphics/transforms/TransformState.cpp
    platform/graphics/transforms/TranslateTransformOperation.cpp

    platform/graphics/ImageSource.cpp

    platform/image-decoders/ImageDecoder.cpp
    platform/image-decoders/bmp/BMPImageDecoder.cpp
    platform/image-decoders/bmp/BMPImageReader.cpp
    platform/image-decoders/gif/GIFImageDecoder.cpp
    platform/image-decoders/gif/GIFImageReader.cpp
    platform/image-decoders/ico/ICOImageDecoder.cpp
    platform/image-decoders/jpeg/JPEGImageDecoder.cpp
    platform/image-decoders/png/PNGImageDecoder.cpp
    platform/image-decoders/webp/WEBPImageDecoder.cpp

    platform/leveldb/LevelDBDatabase.cpp
    platform/leveldb/LevelDBTransaction.cpp
    platform/leveldb/LevelDBWriteBatch.cpp

    platform/mediastream/MediaStreamCenter.cpp
    platform/mediastream/MediaStreamSource.cpp
    platform/mediastream/RTCIceCandidateDescriptor.cpp
    platform/mediastream/RTCPeerConnectionHandler.cpp
    platform/mediastream/RTCSessionDescriptionDescriptor.cpp

    platform/mock/DeviceMotionClientMock.cpp
    platform/mock/DeviceOrientationClientMock.cpp
    platform/mock/GeolocationClientMock.cpp
    platform/mock/ScrollbarThemeMock.cpp

    platform/network/AuthenticationChallengeBase.cpp
    platform/network/BlobData.cpp
    platform/network/BlobRegistry.cpp
    platform/network/BlobRegistryImpl.cpp
    platform/network/BlobResourceHandle.cpp
    platform/network/Credential.cpp
    platform/network/CredentialStorage.cpp
    platform/network/DataURL.cpp
    platform/network/DNSResolveQueue.cpp
    platform/network/FormDataBuilder.cpp
    platform/network/FormData.cpp
    platform/network/HTTPHeaderMap.cpp
    platform/network/HTTPParsers.cpp
    platform/network/MIMEHeader.cpp
    platform/network/NetworkStateNotifier.cpp
    platform/network/ParsedContentType.cpp
    platform/network/ProtectionSpace.cpp
    platform/network/ProxyServer.cpp
    platform/network/ResourceErrorBase.cpp
    platform/network/ResourceHandle.cpp
    platform/network/ResourceHandleClient.cpp
    platform/network/ResourceLoadTiming.cpp
    platform/network/ResourceRequestBase.cpp
    platform/network/ResourceResponseBase.cpp
    platform/network/SocketStreamHandleBase.cpp

    platform/sql/SQLValue.cpp
    platform/sql/SQLiteAuthorizer.cpp
    platform/sql/SQLiteDatabase.cpp
    platform/sql/SQLiteFileSystem.cpp
    platform/sql/SQLiteStatement.cpp
    platform/sql/SQLiteTransaction.cpp

    platform/text/BidiContext.cpp
    platform/text/DateTimeFormat.cpp
    platform/text/Hyphenation.cpp
    platform/text/LineEnding.cpp
    platform/text/LocaleToScriptMappingDefault.cpp
    platform/text/PlatformLocale.cpp
    platform/text/QuotedPrintable.cpp
    platform/text/RegularExpression.cpp
    platform/text/SegmentedString.cpp
    platform/text/TextBoundaries.cpp
    platform/text/TextBreakIterator.cpp
    platform/text/TextCodec.cpp
    platform/text/TextCodecLatin1.cpp
    platform/text/TextCodecUTF16.cpp
    platform/text/TextCodecUTF8.cpp
    platform/text/TextCodecUserDefined.cpp
    platform/text/TextEncoding.cpp
    platform/text/TextEncodingRegistry.cpp
    platform/text/TextStream.cpp
    platform/text/UnicodeRange.cpp

    platform/text/transcoder/FontTranscoder.cpp

    plugins/DOMMimeTypeArray.cpp
    plugins/DOMMimeType.cpp
    plugins/DOMPluginArray.cpp
    plugins/DOMPlugin.cpp
    plugins/PluginDatabase.cpp
    plugins/PluginData.cpp
    plugins/PluginDebug.cpp
    plugins/PluginMainThreadScheduler.cpp
    plugins/PluginStream.cpp
    plugins/PluginView.cpp

    rendering/AutoTableLayout.cpp
    rendering/BidiRun.cpp
    rendering/CounterNode.cpp
    rendering/EllipsisBox.cpp
    rendering/FilterEffectRenderer.cpp
    rendering/FixedTableLayout.cpp
    rendering/FlowThreadController.cpp
    rendering/HitTestingTransformState.cpp
    rendering/HitTestLocation.cpp
    rendering/HitTestResult.cpp
    rendering/InlineBox.cpp
    rendering/InlineFlowBox.cpp
    rendering/InlineTextBox.cpp
    rendering/LayoutState.cpp
    rendering/LayoutRepainter.cpp
    rendering/PointerEventsHitRules.cpp
    rendering/RenderApplet.cpp
    rendering/RenderArena.cpp
    rendering/RenderBR.cpp
    rendering/RenderBlock.cpp
    rendering/RenderBlockLineLayout.cpp
    rendering/RenderBox.cpp
    rendering/RenderBoxModelObject.cpp
    rendering/RenderButton.cpp
    rendering/RenderCombineText.cpp
    rendering/RenderCounter.cpp
    rendering/RenderDeprecatedFlexibleBox.cpp
    rendering/RenderDetailsMarker.cpp
    rendering/RenderDialog.cpp
    rendering/RenderEmbeddedObject.cpp
    rendering/RenderFieldset.cpp
    rendering/RenderFileUploadControl.cpp
    rendering/RenderFlexibleBox.cpp
    rendering/RenderGrid.cpp
    rendering/RenderFlowThread.cpp
    rendering/RenderFrame.cpp
    rendering/RenderFrameBase.cpp
    rendering/RenderFrameSet.cpp
    rendering/RenderFullScreen.cpp
    rendering/RenderGeometryMap.cpp
    rendering/RenderHTMLCanvas.cpp
    rendering/RenderIFrame.cpp
    rendering/RenderImage.cpp
    rendering/RenderImageResource.cpp
    rendering/RenderImageResourceStyleImage.cpp
    rendering/RenderInline.cpp
    rendering/RenderLayer.cpp
    rendering/RenderLayerBacking.cpp
    rendering/RenderLayerCompositor.cpp
    rendering/RenderLayerFilterInfo.cpp
    rendering/RenderLayerModelObject.cpp
    rendering/RenderLineBoxList.cpp
    rendering/RenderListBox.cpp
    rendering/RenderListItem.cpp
    rendering/RenderListMarker.cpp
    rendering/RenderMarquee.cpp
    rendering/RenderMedia.cpp
    rendering/RenderMediaControlElements.cpp
    rendering/RenderMediaControls.cpp
    rendering/RenderMenuList.cpp
    rendering/RenderMeter.cpp
    rendering/RenderMultiColumnBlock.cpp
    rendering/RenderMultiColumnFlowThread.cpp
    rendering/RenderMultiColumnSet.cpp
    rendering/RenderNamedFlowThread.cpp
    rendering/RenderObject.cpp
    rendering/RenderObjectChildList.cpp
    rendering/RenderPart.cpp
    rendering/RenderProgress.cpp
    rendering/RenderQuote.cpp
    rendering/RenderRegion.cpp
    rendering/RenderRegionSet.cpp
    rendering/RenderReplaced.cpp
    rendering/RenderReplica.cpp
    rendering/RenderRuby.cpp
    rendering/RenderRubyBase.cpp
    rendering/RenderRubyRun.cpp
    rendering/RenderRubyText.cpp
    rendering/RenderScrollbar.cpp
    rendering/RenderScrollbarPart.cpp
    rendering/RenderScrollbarTheme.cpp
    rendering/RenderSearchField.cpp
    rendering/RenderSlider.cpp
    rendering/RenderSnapshottedPlugIn.cpp
    rendering/RenderTable.cpp
    rendering/RenderTableCaption.cpp
    rendering/RenderTableCell.cpp
    rendering/RenderTableCol.cpp
    rendering/RenderTableRow.cpp
    rendering/RenderTableSection.cpp
    rendering/RenderText.cpp
    rendering/RenderTextControl.cpp
    rendering/RenderTextControlMultiLine.cpp
    rendering/RenderTextControlSingleLine.cpp
    rendering/RenderTextFragment.cpp
    rendering/RenderTextTrackCue.cpp
    rendering/RenderTheme.cpp
    rendering/RenderTreeAsText.cpp
    rendering/RenderVideo.cpp
    rendering/RenderView.cpp
    rendering/RenderWidget.cpp
    rendering/RenderWordBreak.cpp
    rendering/RootInlineBox.cpp
    rendering/ScrollBehavior.cpp
    rendering/TextAutosizer.cpp
    rendering/break_lines.cpp

    rendering/mathml/RenderMathMLBlock.cpp
    rendering/mathml/RenderMathMLFenced.cpp
    rendering/mathml/RenderMathMLFraction.cpp
    rendering/mathml/RenderMathMLMath.cpp
    rendering/mathml/RenderMathMLOperator.cpp
    rendering/mathml/RenderMathMLRoot.cpp
    rendering/mathml/RenderMathMLRow.cpp
    rendering/mathml/RenderMathMLSpace.cpp
    rendering/mathml/RenderMathMLSquareRoot.cpp
    rendering/mathml/RenderMathMLSubSup.cpp
    rendering/mathml/RenderMathMLUnderOver.cpp

    rendering/shapes/PolygonShape.cpp
    rendering/shapes/RectangleShape.cpp
    rendering/shapes/Shape.cpp
    rendering/shapes/ShapeInfo.cpp
    rendering/shapes/ShapeInsideInfo.cpp
    rendering/shapes/ShapeInterval.cpp
    rendering/shapes/ShapeOutsideInfo.cpp

    rendering/style/BasicShapes.cpp
    rendering/style/ContentData.cpp
    rendering/style/CounterDirectives.cpp
    rendering/style/FillLayer.cpp
    rendering/style/KeyframeList.cpp
    rendering/style/NinePieceImage.cpp
    rendering/style/QuotesData.cpp
    rendering/style/RenderStyle.cpp
    rendering/style/ShadowData.cpp
    rendering/style/StyleBackgroundData.cpp
    rendering/style/StyleBoxData.cpp
    rendering/style/StyleCachedImage.cpp
    rendering/style/StyleCachedImageSet.cpp
    rendering/style/StyleCachedShader.cpp
    rendering/style/StyleCustomFilterProgram.cpp
    rendering/style/StyleCustomFilterProgramCache.cpp
    rendering/style/StyleDeprecatedFlexibleBoxData.cpp
    rendering/style/StyleFilterData.cpp
    rendering/style/StyleFlexibleBoxData.cpp
    rendering/style/StyleGeneratedImage.cpp
    rendering/style/StyleGridData.cpp
    rendering/style/StyleGridItemData.cpp
    rendering/style/StyleInheritedData.cpp
    rendering/style/StyleMarqueeData.cpp
    rendering/style/StyleMultiColData.cpp
    rendering/style/StyleRareInheritedData.cpp
    rendering/style/StyleRareNonInheritedData.cpp
    rendering/style/StyleSurroundData.cpp
    rendering/style/StyleTransformData.cpp
    rendering/style/StyleVisualData.cpp
    rendering/style/SVGRenderStyle.cpp
    rendering/style/SVGRenderStyleDefs.cpp

    rendering/svg/RenderSVGBlock.cpp
    rendering/svg/RenderSVGContainer.cpp
    rendering/svg/RenderSVGEllipse.cpp
    rendering/svg/RenderSVGGradientStop.cpp
    rendering/svg/RenderSVGForeignObject.cpp
    rendering/svg/RenderSVGHiddenContainer.cpp
    rendering/svg/RenderSVGImage.cpp
    rendering/svg/RenderSVGInline.cpp
    rendering/svg/RenderSVGInlineText.cpp
    rendering/svg/RenderSVGModelObject.cpp
    rendering/svg/RenderSVGPath.cpp
    rendering/svg/RenderSVGRect.cpp
    rendering/svg/RenderSVGResource.cpp
    rendering/svg/RenderSVGResourceClipper.cpp
    rendering/svg/RenderSVGResourceContainer.cpp
    rendering/svg/RenderSVGResourceFilter.cpp
    rendering/svg/RenderSVGResourceFilterPrimitive.cpp
    rendering/svg/RenderSVGResourceGradient.cpp
    rendering/svg/RenderSVGResourceLinearGradient.cpp
    rendering/svg/RenderSVGResourceMarker.cpp
    rendering/svg/RenderSVGResourceMasker.cpp
    rendering/svg/RenderSVGResourcePattern.cpp
    rendering/svg/RenderSVGResourceRadialGradient.cpp
    rendering/svg/RenderSVGResourceSolidColor.cpp
    rendering/svg/RenderSVGRoot.cpp
    rendering/svg/RenderSVGShape.cpp
    rendering/svg/RenderSVGTSpan.cpp
    rendering/svg/RenderSVGText.cpp
    rendering/svg/RenderSVGTextPath.cpp
    rendering/svg/RenderSVGTransformableContainer.cpp
    rendering/svg/RenderSVGViewportContainer.cpp
    rendering/svg/SVGInlineFlowBox.cpp
    rendering/svg/SVGInlineTextBox.cpp
    rendering/svg/SVGPathData.cpp
    rendering/svg/SVGRenderSupport.cpp
    rendering/svg/SVGRenderTreeAsText.cpp
    rendering/svg/SVGRenderingContext.cpp
    rendering/svg/SVGResources.cpp
    rendering/svg/SVGResourcesCache.cpp
    rendering/svg/SVGResourcesCycleSolver.cpp
    rendering/svg/SVGRootInlineBox.cpp
    rendering/svg/SVGTextChunk.cpp
    rendering/svg/SVGTextChunkBuilder.cpp
    rendering/svg/SVGTextLayoutAttributes.cpp
    rendering/svg/SVGTextLayoutAttributesBuilder.cpp
    rendering/svg/SVGTextLayoutEngine.cpp
    rendering/svg/SVGTextLayoutEngineBaseline.cpp
    rendering/svg/SVGTextLayoutEngineSpacing.cpp
    rendering/svg/SVGTextRunRenderingContext.cpp
    rendering/svg/SVGTextMetrics.cpp
    rendering/svg/SVGTextMetricsBuilder.cpp
    rendering/svg/SVGTextQuery.cpp

    storage/StorageThread.cpp
    storage/Storage.cpp
    storage/StorageAreaImpl.cpp
    storage/StorageAreaSync.cpp
    storage/StorageEvent.cpp
    storage/StorageEventDispatcher.cpp
    storage/StorageMap.cpp
    storage/StorageNamespace.cpp
    storage/StorageNamespaceImpl.cpp
    storage/StorageStrategy.cpp
    storage/StorageSyncManager.cpp
    storage/StorageTracker.cpp

    svg/ColorDistance.cpp
    svg/SVGAElement.cpp
    svg/SVGAltGlyphDefElement.cpp
    svg/SVGAltGlyphElement.cpp
    svg/SVGAltGlyphItemElement.cpp
    svg/SVGAngle.cpp
    svg/SVGAnimateColorElement.cpp
    svg/SVGAnimatedAngle.cpp
    svg/SVGAnimatedBoolean.cpp
    svg/SVGAnimatedColor.cpp
    svg/SVGAnimatedEnumeration.cpp
    svg/SVGAnimatedInteger.cpp
    svg/SVGAnimatedIntegerOptionalInteger.cpp
    svg/SVGAnimatedLength.cpp
    svg/SVGAnimatedLengthList.cpp
    svg/SVGAnimatedNumber.cpp
    svg/SVGAnimatedNumberList.cpp
    svg/SVGAnimatedNumberOptionalNumber.cpp
    svg/SVGAnimatedPath.cpp
    svg/SVGAnimatedPointList.cpp
    svg/SVGAnimatedPreserveAspectRatio.cpp
    svg/SVGAnimatedRect.cpp
    svg/SVGAnimatedString.cpp
    svg/SVGAnimatedTransformList.cpp
    svg/SVGAnimatedType.cpp
    svg/SVGAnimatedTypeAnimator.cpp
    svg/SVGAnimateElement.cpp
    svg/SVGAnimateMotionElement.cpp
    svg/SVGAnimateTransformElement.cpp
    svg/SVGAnimationElement.cpp
    svg/SVGCircleElement.cpp
    svg/SVGClipPathElement.cpp
    svg/SVGColor.cpp
    svg/SVGComponentTransferFunctionElement.cpp
    svg/SVGCursorElement.cpp
    svg/SVGDefsElement.cpp
    svg/SVGDescElement.cpp
    svg/SVGDocument.cpp
    svg/SVGDocumentExtensions.cpp
    svg/SVGElement.cpp
    svg/SVGElementInstance.cpp
    svg/SVGElementInstanceList.cpp
    svg/SVGEllipseElement.cpp
    svg/SVGException.cpp
    svg/SVGExternalResourcesRequired.cpp
    svg/SVGFEBlendElement.cpp
    svg/SVGFEColorMatrixElement.cpp
    svg/SVGFEComponentTransferElement.cpp
    svg/SVGFECompositeElement.cpp
    svg/SVGFEConvolveMatrixElement.cpp
    svg/SVGFEDiffuseLightingElement.cpp
    svg/SVGFEDisplacementMapElement.cpp
    svg/SVGFEDistantLightElement.cpp
    svg/SVGFEDropShadowElement.cpp
    svg/SVGFEFloodElement.cpp
    svg/SVGFEFuncAElement.cpp
    svg/SVGFEFuncBElement.cpp
    svg/SVGFEFuncGElement.cpp
    svg/SVGFEFuncRElement.cpp
    svg/SVGFEGaussianBlurElement.cpp
    svg/SVGFEImageElement.cpp
    svg/SVGFELightElement.cpp
    svg/SVGFEMergeElement.cpp
    svg/SVGFEMergeNodeElement.cpp
    svg/SVGFEMorphologyElement.cpp
    svg/SVGFEOffsetElement.cpp
    svg/SVGFEPointLightElement.cpp
    svg/SVGFESpecularLightingElement.cpp
    svg/SVGFESpotLightElement.cpp
    svg/SVGFETileElement.cpp
    svg/SVGFETurbulenceElement.cpp
    svg/SVGFilterElement.cpp
    svg/SVGFilterPrimitiveStandardAttributes.cpp
    svg/SVGFitToViewBox.cpp
    svg/SVGFontData.cpp
    svg/SVGFontElement.cpp
    svg/SVGFontFaceElement.cpp
    svg/SVGFontFaceFormatElement.cpp
    svg/SVGFontFaceNameElement.cpp
    svg/SVGFontFaceSrcElement.cpp
    svg/SVGFontFaceUriElement.cpp
    svg/SVGForeignObjectElement.cpp
    svg/SVGGElement.cpp
    svg/SVGGlyphElement.cpp
    svg/SVGGlyphRefElement.cpp
    svg/SVGGradientElement.cpp
    svg/SVGGraphicsElement.cpp
    svg/SVGHKernElement.cpp
    svg/SVGImageElement.cpp
    svg/SVGImageLoader.cpp
    svg/SVGLangSpace.cpp
    svg/SVGLength.cpp
    svg/SVGLengthContext.cpp
    svg/SVGLengthList.cpp
    svg/SVGLineElement.cpp
    svg/SVGLinearGradientElement.cpp
    svg/SVGLocatable.cpp
    svg/SVGMPathElement.cpp
    svg/SVGMarkerElement.cpp
    svg/SVGMaskElement.cpp
    svg/SVGMetadataElement.cpp
    svg/SVGMissingGlyphElement.cpp
    svg/SVGNumberList.cpp
    svg/SVGPaint.cpp
    svg/SVGParserUtilities.cpp
    svg/SVGPathBlender.cpp
    svg/SVGPathBuilder.cpp
    svg/SVGPathByteStreamBuilder.cpp
    svg/SVGPathByteStreamSource.cpp
    svg/SVGPathElement.cpp
    svg/SVGPathParser.cpp
    svg/SVGPathSegList.cpp
    svg/SVGPathSegListBuilder.cpp
    svg/SVGPathSegListSource.cpp
    svg/SVGPathStringBuilder.cpp
    svg/SVGPathStringSource.cpp
    svg/SVGPathTraversalStateBuilder.cpp
    svg/SVGPathUtilities.cpp
    svg/SVGPatternElement.cpp
    svg/SVGPointList.cpp
    svg/SVGPolyElement.cpp
    svg/SVGPolygonElement.cpp
    svg/SVGPolylineElement.cpp
    svg/SVGPreserveAspectRatio.cpp
    svg/SVGRadialGradientElement.cpp
    svg/SVGRectElement.cpp
    svg/SVGSVGElement.cpp
    svg/SVGScriptElement.cpp
    svg/SVGSetElement.cpp
    svg/SVGStopElement.cpp
    svg/SVGStringList.cpp
    svg/SVGStyleElement.cpp
    svg/SVGStyledElement.cpp
    svg/SVGSwitchElement.cpp
    svg/SVGSymbolElement.cpp
    svg/SVGTRefElement.cpp
    svg/SVGTSpanElement.cpp
    svg/SVGTests.cpp
    svg/SVGTextContentElement.cpp
    svg/SVGTextElement.cpp
    svg/SVGTextPathElement.cpp
    svg/SVGTextPositioningElement.cpp
    svg/SVGTitleElement.cpp
    svg/SVGTransform.cpp
    svg/SVGTransformDistance.cpp
    svg/SVGTransformList.cpp
    svg/SVGTransformable.cpp
    svg/SVGURIReference.cpp
    svg/SVGUseElement.cpp
    svg/SVGViewElement.cpp
    svg/SVGViewSpec.cpp
    svg/SVGVKernElement.cpp
    svg/SVGZoomAndPan.cpp
    svg/SVGZoomEvent.cpp

    svg/animation/SMILTime.cpp
    svg/animation/SMILTimeContainer.cpp
    svg/animation/SVGSMILElement.cpp

    svg/graphics/SVGImage.cpp
    svg/graphics/SVGImageCache.cpp
    svg/graphics/SVGImageForContainer.cpp

    svg/graphics/filters/SVGFEImage.cpp
    svg/graphics/filters/SVGFilter.cpp
    svg/graphics/filters/SVGFilterBuilder.cpp

    svg/properties/SVGAnimatedProperty.cpp
    svg/properties/SVGAttributeToPropertyMap.cpp
    svg/properties/SVGPathSegListPropertyTearOff.cpp

    workers/AbstractWorker.cpp
    workers/DedicatedWorkerGlobalScope.cpp
    workers/DedicatedWorkerThread.cpp
    workers/DefaultSharedWorkerRepository.cpp
    workers/SharedWorker.cpp
    workers/SharedWorkerGlobalScope.cpp
    workers/SharedWorkerRepository.cpp
    workers/SharedWorkerThread.cpp
    workers/Worker.cpp
    workers/WorkerGlobalScope.cpp
    workers/WorkerEventQueue.cpp
    workers/WorkerLocation.cpp
    workers/WorkerMessagingProxy.cpp
    workers/WorkerRunLoop.cpp
    workers/WorkerScriptLoader.cpp
    workers/WorkerThread.cpp

    xml/DOMParser.cpp
    xml/NativeXPathNSResolver.cpp
    xml/XMLErrors.cpp
    xml/XMLHttpRequest.cpp
    xml/XMLHttpRequestException.cpp
    xml/XMLHttpRequestProgressEventThrottle.cpp
    xml/XMLHttpRequestUpload.cpp
    xml/XMLTreeViewer.cpp
    xml/XMLSerializer.cpp
    xml/XPathEvaluator.cpp
    xml/XPathException.cpp
    xml/XPathExpression.cpp
    xml/XPathExpressionNode.cpp
    xml/XPathFunctions.cpp
    xml/XPathNSResolver.cpp
    xml/XPathNodeSet.cpp
    xml/XPathParser.cpp
    xml/XPathPath.cpp
    xml/XPathPredicate.cpp
    xml/XPathResult.cpp
    xml/XPathStep.cpp
    xml/XPathUtil.cpp
    xml/XPathValue.cpp
    xml/XPathVariableReference.cpp
    xml/XSLImportRule.cpp
    xml/XSLStyleSheetLibxslt.cpp
    xml/XSLTExtensions.cpp
    xml/XSLTProcessor.cpp
    xml/XSLTProcessorLibxslt.cpp
    xml/XSLTUnicodeSort.cpp

    xml/parser/XMLDocumentParser.cpp
    xml/parser/XMLDocumentParserLibxml2.cpp
    xml/parser/XMLDocumentParserScope.cpp
)

# GCC 4.6.x crashes when building this file with -O3 and -DNDEBUG.
# References: https://bugs.webkit.org/show_bug.cgi?id=97268
#             https://bugs.webkit.org/show_bug.cgi?id=114627
if (CMAKE_COMPILER_IS_GNUCXX) # Can restrict based on the version.
    string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _CXXFLAGS_VARNAME)

    string(FIND ${${_CXXFLAGS_VARNAME}} "-O3" _CXXFLAGS_O3_INDEX)
    if (_CXXFLAGS_O3_INDEX GREATER -1)
        string(REPLACE "-O3" "-O2" _CXXFLAGS ${${_CXXFLAGS_VARNAME}})
        set_source_files_properties(Modules/websockets/WebSocketDeflater.cpp
                                    PROPERTIES COMPILE_FLAGS "${_CXXFLAGS}")
    endif ()
endif ()

set(WebCore_CSS_PROPERTY_NAMES
    ${WEBCORE_DIR}/css/CSSPropertyNames.in
)

set(WebCore_CSS_VALUE_KEYWORDS
    ${WEBCORE_DIR}/css/CSSValueKeywords.in
)

set(WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/html.css
    ${WEBCORE_DIR}/css/fullscreen.css
    ${WEBCORE_DIR}/css/mathml.css
    ${WEBCORE_DIR}/css/mediaControls.css
    ${WEBCORE_DIR}/css/plugIns.css
    ${WEBCORE_DIR}/css/quirks.css
    ${WEBCORE_DIR}/css/svg.css
    ${WEBCORE_DIR}/css/view-source.css
)

set(WebCore_PLUG_INS_RESOURCES
    ${WEBCORE_DIR}/Resources/plugIns.js
)

set(WebCore_LIBRARIES
    WTF
    JavaScriptCore
)

if (ENABLE_SQL_DATABASE AND ENABLE_WORKERS)
    list(APPEND WebCore_SOURCES
        Modules/webdatabase/WorkerGlobalScopeWebDatabase.cpp
    )

    list(APPEND WebCore_IDL_FILES
        Modules/webdatabase/WorkerGlobalScopeWebDatabase.idl
    )
endif ()

if (ENABLE_NETSCAPE_PLUGIN_API)
    list(APPEND WebCore_SOURCES
        plugins/PluginPackage.cpp
        plugins/npapi.cpp
    )
else ()
    list(APPEND WebCore_SOURCES
        plugins/PluginPackageNone.cpp
        plugins/PluginViewNone.cpp
    )
endif ()

if (ENABLE_SVG)
    list(APPEND WebCore_CSS_PROPERTY_NAMES
        ${WEBCORE_DIR}/css/SVGCSSPropertyNames.in
    )
    list(APPEND WebCore_CSS_VALUE_KEYWORDS
        ${WEBCORE_DIR}/css/SVGCSSValueKeywords.in
    )
endif ()

if (ENABLE_SMOOTH_SCROLLING)
    list(APPEND WebCore_SOURCES
        platform/ScrollAnimatorNone.cpp
    )
endif ()

if (ENABLE_WEBGL)
    set(WTF_USE_3D_GRAPHICS 1)
    add_definitions(-DWTF_USE_3D_GRAPHICS=1)

    list(APPEND WebCore_SOURCES
        html/canvas/OESStandardDerivatives.cpp
        html/canvas/OESTextureFloat.cpp
        html/canvas/OESTextureHalfFloat.cpp
        html/canvas/OESVertexArrayObject.cpp
        html/canvas/OESElementIndexUint.cpp
        html/canvas/WebGLBuffer.cpp
        html/canvas/WebGLCompressedTextureATC.cpp
        html/canvas/WebGLCompressedTexturePVRTC.cpp
        html/canvas/WebGLCompressedTextureS3TC.cpp
        html/canvas/WebGLContextAttributes.cpp
        html/canvas/WebGLContextEvent.cpp
        html/canvas/WebGLContextGroup.cpp
        html/canvas/WebGLContextObject.cpp
        html/canvas/WebGLDebugRendererInfo.cpp
        html/canvas/WebGLDebugShaders.cpp
        html/canvas/WebGLDepthTexture.cpp
        html/canvas/WebGLFramebuffer.cpp
        html/canvas/WebGLGetInfo.cpp
        html/canvas/WebGLLoseContext.cpp
        html/canvas/WebGLObject.cpp
        html/canvas/WebGLProgram.cpp
        html/canvas/WebGLRenderbuffer.cpp
        html/canvas/WebGLRenderingContext.cpp
        html/canvas/WebGLShader.cpp
        html/canvas/WebGLShaderPrecisionFormat.cpp
        html/canvas/WebGLSharedObject.cpp
        html/canvas/WebGLTexture.cpp
        html/canvas/WebGLUniformLocation.cpp
        html/canvas/WebGLVertexArrayObjectOES.cpp
        html/canvas/WebGLExtension.cpp
        html/canvas/EXTDrawBuffers.cpp
        html/canvas/EXTTextureFilterAnisotropic.cpp
        html/canvas/OESStandardDerivatives.cpp
        html/canvas/OESTextureFloat.cpp
        html/canvas/OESTextureHalfFloat.cpp
        html/canvas/OESVertexArrayObject.cpp
        html/canvas/OESElementIndexUint.cpp
    )
    list(APPEND WebCore_IDL_FILES
        html/canvas/EXTDrawBuffers.idl
        html/canvas/EXTTextureFilterAnisotropic.idl
        html/canvas/OESStandardDerivatives.idl
        html/canvas/OESTextureFloat.idl
        html/canvas/OESTextureHalfFloat.idl
        html/canvas/OESVertexArrayObject.idl
        html/canvas/OESElementIndexUint.idl
        html/canvas/WebGLActiveInfo.idl
        html/canvas/WebGLBuffer.idl
        html/canvas/WebGLCompressedTextureATC.idl
        html/canvas/WebGLCompressedTexturePVRTC.idl
        html/canvas/WebGLCompressedTextureS3TC.idl
        html/canvas/WebGLContextAttributes.idl
        html/canvas/WebGLContextEvent.idl
        html/canvas/WebGLDebugRendererInfo.idl
        html/canvas/WebGLDebugShaders.idl
        html/canvas/WebGLDepthTexture.idl
        html/canvas/WebGLFramebuffer.idl
        html/canvas/WebGLLoseContext.idl
        html/canvas/WebGLProgram.idl
        html/canvas/WebGLRenderbuffer.idl
        html/canvas/WebGLRenderingContext.idl
        html/canvas/WebGLShader.idl
        html/canvas/WebGLShaderPrecisionFormat.idl
        html/canvas/WebGLTexture.idl
        html/canvas/WebGLUniformLocation.idl
        html/canvas/WebGLVertexArrayObjectOES.idl
    )
endif ()

if (ENABLE_VIDEO_TRACK)
    list(APPEND WebCore_IDL_FILES
        html/track/AudioTrack.idl
        html/track/AudioTrackList.idl
        html/track/TextTrack.idl
        html/track/TextTrackCue.idl
        html/track/TextTrackCueList.idl
        html/track/TextTrackList.idl
        html/track/TrackEvent.idl
        html/track/VideoTrack.idl
        html/track/VideoTrackList.idl
    )

    list(APPEND WebCore_SOURCES
        html/track/AudioTrack.cpp
        html/track/AudioTrackList.cpp
        html/track/InbandTextTrack.cpp
        html/track/LoadableTextTrack.cpp
        html/track/TextTrack.cpp
        html/track/TextTrackCue.cpp
        html/track/TextTrackCueGeneric.cpp
        html/track/TextTrackCueList.cpp
        html/track/TextTrackList.cpp
        html/track/TrackBase.cpp
        html/track/TrackEvent.cpp
        html/track/TrackListBase.cpp
        html/track/WebVTTElement.cpp
        html/track/WebVTTParser.cpp
        html/track/WebVTTTokenizer.cpp
        html/track/VideoTrack.cpp
        html/track/VideoTrackList.cpp

        loader/cache/CachedTextTrack.cpp

        platform/graphics/TextTrackRepresentation.cpp
    )
endif ()

if (ENABLE_QUOTA)
    list(APPEND WebCore_SOURCES
        Modules/quota/DOMWindowQuota.cpp
        Modules/quota/NavigatorStorageQuota.cpp
        Modules/quota/StorageErrorCallback.cpp
        Modules/quota/StorageInfo.cpp
        Modules/quota/StorageQuota.cpp
        Modules/quota/WorkerNavigatorStorageQuota.cpp
    )

    list(APPEND WebCore_IDL_FILES
        Modules/quota/DOMWindowQuota.idl
        Modules/quota/NavigatorStorageQuota.idl
        Modules/quota/StorageErrorCallback.idl
        Modules/quota/StorageInfo.idl
        Modules/quota/StorageQuota.idl
        Modules/quota/StorageQuotaCallback.idl
        Modules/quota/StorageUsageCallback.idl
        Modules/quota/WorkerNavigatorStorageQuota.idl
    )
endif ()


if (WTF_USE_ICU_UNICODE)
    list(APPEND WebCore_SOURCES
        platform/text/TextBreakIteratorICU.cpp
        platform/text/TextCodecICU.cpp
        platform/text/TextEncodingDetectorICU.cpp
    )
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${ICU_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${ICU_LIBRARIES}
    )
elseif (WTF_USE_WCHAR_UNICODE)
    list(APPEND WebCore_SOURCES
        platform/text/TextEncodingDetectorNone.cpp

        platform/text/wchar/TextBreakIteratorWchar.cpp
    )
endif ()


if (WTF_USE_LEVELDB)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${THIRDPARTY_DIR}/leveldb/include"
        "${THIRDPARTY_DIR}/leveldb"
    )
    list(APPEND LEVELDB_SOURCES
        ${THIRDPARTY_DIR}/leveldb/db/builder.cc
        ${THIRDPARTY_DIR}/leveldb/db/c.cc
        ${THIRDPARTY_DIR}/leveldb/db/dbformat.cc
        ${THIRDPARTY_DIR}/leveldb/db/db_impl.cc
        ${THIRDPARTY_DIR}/leveldb/db/db_iter.cc
        ${THIRDPARTY_DIR}/leveldb/db/filename.cc
        ${THIRDPARTY_DIR}/leveldb/db/log_reader.cc
        ${THIRDPARTY_DIR}/leveldb/db/log_writer.cc
        ${THIRDPARTY_DIR}/leveldb/db/memtable.cc
        ${THIRDPARTY_DIR}/leveldb/db/repair.cc
        ${THIRDPARTY_DIR}/leveldb/db/table_cache.cc
        ${THIRDPARTY_DIR}/leveldb/db/version_edit.cc
        ${THIRDPARTY_DIR}/leveldb/db/version_set.cc
        ${THIRDPARTY_DIR}/leveldb/db/write_batch.cc
        ${THIRDPARTY_DIR}/leveldb/helpers/memenv/memenv.cc
        ${THIRDPARTY_DIR}/leveldb/port/port_posix.cc
        ${THIRDPARTY_DIR}/leveldb/table/block_builder.cc
        ${THIRDPARTY_DIR}/leveldb/table/block.cc
        ${THIRDPARTY_DIR}/leveldb/table/filter_block.cc
        ${THIRDPARTY_DIR}/leveldb/table/format.cc
        ${THIRDPARTY_DIR}/leveldb/table/iterator.cc
        ${THIRDPARTY_DIR}/leveldb/table/merger.cc
        ${THIRDPARTY_DIR}/leveldb/table/table_builder.cc
        ${THIRDPARTY_DIR}/leveldb/table/table.cc
        ${THIRDPARTY_DIR}/leveldb/table/two_level_iterator.cc
        ${THIRDPARTY_DIR}/leveldb/util/arena.cc
        ${THIRDPARTY_DIR}/leveldb/util/bloom.cc
        ${THIRDPARTY_DIR}/leveldb/util/cache.cc
        ${THIRDPARTY_DIR}/leveldb/util/coding.cc
        ${THIRDPARTY_DIR}/leveldb/util/comparator.cc
        ${THIRDPARTY_DIR}/leveldb/util/crc32c.cc
        ${THIRDPARTY_DIR}/leveldb/util/env.cc
        ${THIRDPARTY_DIR}/leveldb/util/env_posix.cc
        ${THIRDPARTY_DIR}/leveldb/util/filter_policy.cc
        ${THIRDPARTY_DIR}/leveldb/util/hash.cc
        ${THIRDPARTY_DIR}/leveldb/util/histogram.cc
        ${THIRDPARTY_DIR}/leveldb/util/logging.cc
        ${THIRDPARTY_DIR}/leveldb/util/options.cc
        ${THIRDPARTY_DIR}/leveldb/util/status.cc
    )
endif ()

if (WTF_USE_3D_GRAPHICS)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${OPENGL_INCLUDE_DIR}
        "${THIRDPARTY_DIR}/ANGLE/src"
        "${THIRDPARTY_DIR}/ANGLE/include"
        "${THIRDPARTY_DIR}/ANGLE/include/KHR"
        "${THIRDPARTY_DIR}/ANGLE/include/GLSLANG"
        "${WEBCORE_DIR}/platform/graphics/gpu"
    )
    list(APPEND WebCore_LIBRARIES
        ${OPENGL_gl_LIBRARY}
    )

    if (BISON_VERSION EQUAL "2.5")
        # Bison 2.5 can not compile generated glslang.
        # See https://bugs.webkit.org/show_bug.cgi?id=93487
        list(APPEND ANGLESupport_SOURCES
            ${THIRDPARTY_DIR}/ANGLE/src/compiler/glslang_lex.cpp
            ${THIRDPARTY_DIR}/ANGLE/src/compiler/glslang_tab.cpp
        )
    else ()
        list(APPEND ANGLESupport_SOURCES
            ${DERIVED_SOURCES_WEBCORE_DIR}/glslang.cpp
            ${DERIVED_SOURCES_WEBCORE_DIR}/glslang_tab.cpp
        )
    endif ()

    list(APPEND ANGLESupport_SOURCES
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/BuiltInFunctionEmulator.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/CodeGenGLSL.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/Compiler.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/Diagnostics.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/DirectiveHandler.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/debug.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/depgraph/DependencyGraph.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/depgraph/DependencyGraphBuilder.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/depgraph/DependencyGraphOutput.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/depgraph/DependencyGraphTraverse.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/DetectCallDepth.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/DetectDiscontinuity.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/ForLoopUnroll.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/InfoSink.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/Initialize.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/InitializeDll.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/InitializeParseContext.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/Intermediate.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/intermOut.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/IntermTraverse.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/MapLongVariableNames.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/ossource_posix.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/OutputESSL.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/OutputGLSL.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/OutputGLSLBase.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/OutputHLSL.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/parseConst.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/ParseHelper.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/PoolAlloc.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/DiagnosticsBase.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/DirectiveHandlerBase.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/DirectiveParser.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/ExpressionParser.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/Input.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/Lexer.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/Macro.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/MacroExpander.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/Preprocessor.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/Token.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/preprocessor/Tokenizer.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/QualifierAlive.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/RemoveTree.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/SearchSymbol.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/ShaderLang.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/SymbolTable.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/timing/RestrictFragmentShaderTiming.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/timing/RestrictVertexShaderTiming.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/TranslatorESSL.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/TranslatorGLSL.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/TranslatorHLSL.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/Uniform.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/UnfoldShortCircuit.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/util.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/ValidateLimitations.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/VariableInfo.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/VariablePacker.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/compiler/VersionGLSL.cpp
        ${THIRDPARTY_DIR}/ANGLE/src/third_party/compiler/ArrayBoundsClamper.cpp
    )

    list(APPEND WebCore_SOURCES
        platform/graphics/ANGLEWebKitBridge.cpp
        platform/graphics/GraphicsContext3D.cpp
        platform/graphics/gpu/DrawingBuffer.cpp
    )
endif ()

set(WebCoreTestSupport_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/testing"
    "${WEBCORE_DIR}/platform/mock"
)

set(WebCoreTestSupport_IDL_INCLUDES
    testing
)

set(WebCoreTestSupport_IDL_FILES
    testing/Internals.idl
    testing/InternalSettings.idl
    testing/MallocStatistics.idl
    testing/MemoryInfo.idl
    testing/TypeConversions.idl
)

set(WebCoreTestSupport_SOURCES
    testing/Internals.cpp
    testing/InternalSettings.cpp
    platform/mock/PlatformSpeechSynthesizerMock.cpp
)

set(WebCoreTestSupport_LIBRARIES
    JavaScriptCore
    WTF
    WebCore
)

# Modules that the bindings generator scripts may use
set(SCRIPTS_RESOLVE_SUPPLEMENTAL
    ${WEBCORE_DIR}/bindings/scripts/IDLParser.pm
)
set(SCRIPTS_BINDINGS
    ${WEBCORE_DIR}/bindings/scripts/IDLParser.pm
    ${WEBCORE_DIR}/bindings/scripts/InFilesParser.pm
    ${WEBCORE_DIR}/bindings/scripts/preprocessor.pm
)
set(IDL_FILES_TMP ${DERIVED_SOURCES_WEBCORE_DIR}/idl_files.tmp)
set(SUPPLEMENTAL_DEPENDENCY_FILE ${DERIVED_SOURCES_WEBCORE_DIR}/supplemental_dependency.tmp)
set(WINDOW_CONSTRUCTORS_FILE ${DERIVED_SOURCES_WEBCORE_DIR}/DOMWindowConstructors.idl)
set(WORKERGLOBALSCOPE_CONSTRUCTORS_FILE ${DERIVED_SOURCES_WEBCORE_DIR}/WorkerGlobalScopeConstructors.idl)
set(SHAREDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE ${DERIVED_SOURCES_WEBCORE_DIR}/SharedWorkerGlobalScopeConstructors.idl)
set(DEDICATEDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE ${DERIVED_SOURCES_WEBCORE_DIR}/DedicatedWorkerGlobalScopeConstructors.idl)
set(IDL_ATTRIBUTES_FILE ${WEBCORE_DIR}/bindings/scripts/IDLAttributes.txt)

include(${WEBCORE_DIR}/UseJSC.cmake)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

# ANGLE tokenizer & parser
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/glslang.cpp
    MAIN_DEPENDENCY ${THIRDPARTY_DIR}/ANGLE/src/compiler/glslang.l
    COMMAND ${FLEX_EXECUTABLE} --noline --nounistd --outfile=glslang.cpp ${THIRDPARTY_DIR}/ANGLE/src/compiler/glslang.l
    WORKING_DIRECTORY ${DERIVED_SOURCES_WEBCORE_DIR}
    VERBATIM)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/glslang_tab.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/glslang_tab.h
    MAIN_DEPENDENCY ${THIRDPARTY_DIR}/ANGLE/src/compiler/glslang.y
    COMMAND ${BISON_EXECUTABLE} --no-lines --defines=glslang_tab.h --skeleton=yacc.c --output=glslang_tab.cpp ${THIRDPARTY_DIR}/ANGLE/src/compiler/glslang.y
    WORKING_DIRECTORY ${DERIVED_SOURCES_WEBCORE_DIR}
    VERBATIM)

# All Web Inspector generated files are created with this one call to CodeGeneratorInspector.pm
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorProtocolVersion.h
    MAIN_DEPENDENCY inspector/Inspector.json
    DEPENDS ${WEBCORE_DIR}/inspector/generate-inspector-protocol-version ${WEBCORE_DIR}/inspector/Inspector.json
    COMMAND ${PYTHON_EXECUTABLE} ${WEBCORE_DIR}/inspector/generate-inspector-protocol-version -o ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorProtocolVersion.h ${WEBCORE_DIR}/inspector/Inspector.json
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorProtocolVersion.h)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorBackendDispatcher.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorBackendDispatcher.h ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorFrontend.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorFrontend.h ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorTypeBuilder.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorTypeBuilder.h
    MAIN_DEPENDENCY inspector/Inspector.json
    DEPENDS ${WEBCORE_DIR}/inspector/CodeGeneratorInspector.py
    COMMAND ${PYTHON_EXECUTABLE} ${WEBCORE_DIR}/inspector/CodeGeneratorInspector.py ${WEBCORE_DIR}/inspector/Inspector.json --output_h_dir "${DERIVED_SOURCES_WEBCORE_DIR}" --output_cpp_dir "${DERIVED_SOURCES_WEBCORE_DIR}" --write_always
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorBackendDispatcher.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorFrontend.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorTypeBuilder.cpp)


# Generate InspectorOverlayPage.h
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorOverlayPage.h
    MAIN_DEPENDENCY inspector/InspectorOverlayPage.html
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl InspectorOverlayPage_html ${WEBCORE_DIR}/inspector/InspectorOverlayPage.html ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorOverlayPage.h
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorOverlayPage.h)


# Generate InjectedScriptSource.h
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptSource.h
    MAIN_DEPENDENCY inspector/InjectedScriptSource.js
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl InjectedScriptSource_js ${WEBCORE_DIR}/inspector/InjectedScriptSource.js ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptSource.h
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptSource.h)


# Generate InjectedScriptCanvasModuleSource.h
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptCanvasModuleSource.h
    MAIN_DEPENDENCY inspector/InjectedScriptCanvasModuleSource.js
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl InjectedScriptCanvasModuleSource_js ${WEBCORE_DIR}/inspector/InjectedScriptCanvasModuleSource.js ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptCanvasModuleSource.h
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptCanvasModuleSource.h)


MAKE_HASH_TOOLS(${WEBCORE_DIR}/platform/ColorData)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/ColorData.cpp)


# Generate XMLViewerCSS.h
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerCSS.h
    MAIN_DEPENDENCY xml/XMLViewer.css
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl XMLViewer_css ${WEBCORE_DIR}/xml/XMLViewer.css ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerCSS.h
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerCSS.h)


# Generate XMLViewerJS.h
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerJS.h
    MAIN_DEPENDENCY xml/XMLViewer.js
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl XMLViewer_js ${WEBCORE_DIR}/xml/XMLViewer.js ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerJS.h
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerJS.h)


# Generate HTML entity table
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLEntityTable.cpp
    MAIN_DEPENDENCY ${WEBCORE_DIR}/html/parser/HTMLEntityNames.in
    DEPENDS ${WEBCORE_DIR}/html/parser/create-html-entity-table
    COMMAND ${PYTHON_EXECUTABLE} ${WEBCORE_DIR}/html/parser/create-html-entity-table -o ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLEntityTable.cpp ${WEBCORE_DIR}/html/parser/HTMLEntityNames.in
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLEntityTable.cpp)

# Generate CSS property names
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.in ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.h ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.gperf
    MAIN_DEPENDENCY ${WEBCORE_DIR}/css/makeprop.pl
    DEPENDS ${WebCore_CSS_PROPERTY_NAMES}
    WORKING_DIRECTORY ${DERIVED_SOURCES_WEBCORE_DIR}
    COMMAND ${PERL_EXECUTABLE} -ne "print" ${WebCore_CSS_PROPERTY_NAMES} > ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.in
    COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/css/makeprop.pl --defines "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}"
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.cpp)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/CSSParser.cpp CSSValueKeywords.h)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.cpp CSSPropertyNames.h)


# Generate CSS value keywords
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.in ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.h ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.gperf
    MAIN_DEPENDENCY ${WEBCORE_DIR}/css/makevalues.pl
    DEPENDS ${WebCore_CSS_VALUE_KEYWORDS}
    WORKING_DIRECTORY ${DERIVED_SOURCES_WEBCORE_DIR}
    COMMAND ${PERL_EXECUTABLE} -ne "print" ${WebCore_CSS_VALUE_KEYWORDS} > ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.in
    COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/css/makevalues.pl --defines "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}"
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.cpp)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/CSSParser.cpp CSSValueKeywords.h)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/CSSPrimitiveValueMappings.h CSSValueKeywords.h)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.cpp CSSValueKeywords.h)

# Generate user agent styles
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheetsData.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheets.h
    MAIN_DEPENDENCY ${WEBCORE_DIR}/css/make-css-file-arrays.pl
    DEPENDS ${WebCore_USER_AGENT_STYLE_SHEETS} ${WEBCORE_DIR}/bindings/scripts/preprocessor.pm
    COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/css/make-css-file-arrays.pl --defines "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheets.h ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheetsData.cpp ${WebCore_USER_AGENT_STYLE_SHEETS}
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheetsData.cpp)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/StyleResolver.cpp UserAgentStyleSheetsData.cpp UserAgentStyleSheets.h)

# Generate plug-in resources
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/PlugInsResourcesData.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/PlugInsResources.h
    MAIN_DEPENDENCY ${WEBCORE_DIR}/css/make-css-file-arrays.pl
    DEPENDS ${WebCore_PLUG_INS_RESOURCES} ${WEBCORE_DIR}/bindings/scripts/preprocessor.pm
    COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/css/make-css-file-arrays.pl --defines "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" ${DERIVED_SOURCES_WEBCORE_DIR}/PlugInsResources.h ${DERIVED_SOURCES_WEBCORE_DIR}/PlugInsResourcesData.cpp ${WebCore_PLUG_INS_RESOURCES}
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/PlugInsResourcesData.cpp)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/StyleResolver.cpp PlugInsResourcesData.cpp PlugInsResources.h)

GENERATE_GRAMMAR(cssyy ${WEBCORE_DIR}/css/CSSGrammar.y.in ${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.h ${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.cpp "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR}")
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.cpp)


GENERATE_GRAMMAR(xpathyy ${WEBCORE_DIR}/xml/XPathGrammar.y ${DERIVED_SOURCES_WEBCORE_DIR}/XPathGrammar.h ${DERIVED_SOURCES_WEBCORE_DIR}/XPathGrammar.cpp "")
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XPathGrammar.cpp)



ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/html/HTMLTreeBuilder.cpp MathMLNames.cpp)


GENERATE_DOM_NAMES(HTML ${WEBCORE_DIR}/html/HTMLAttributeNames.in ${WEBCORE_DIR}/html/HTMLTagNames.in "")
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLNames.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLElementFactory.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/JSHTMLElementWrapperFactory.cpp)


GENERATE_EVENT_FACTORY(${WEBCORE_DIR}/dom/EventNames.in EventFactory.cpp)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/EventFactory.cpp)


GENERATE_EVENT_FACTORY(${WEBCORE_DIR}/dom/EventTargetFactory.in EventTargetInterfaces.h)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/dom/EventNames.cpp EventTargetInterfaces.h)


GENERATE_EXCEPTION_CODE_DESCRIPTION(${WEBCORE_DIR}/dom/DOMExceptions.in ExceptionCodeDescription.cpp)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/ExceptionCodeDescription.cpp)


GENERATE_SETTINGS_MACROS(${WEBCORE_DIR}/page/Settings.in SettingsMacros.h)
list(APPEND WebCoreTestSupport_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/InternalSettingsGenerated.cpp)


GENERATE_FONT_NAMES(${WEBCORE_DIR}/css/WebKitFontFamilyNames.in)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitFontFamilyNames.cpp)


GENERATE_DOM_NAMES(MathML ${WEBCORE_DIR}/mathml/mathattrs.in ${WEBCORE_DIR}/mathml/mathtags.in "")
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/MathMLNames.cpp)
if (ENABLE_MATHML)
    list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/MathMLElementFactory.cpp)
endif ()

# SVG extra defines need to map to a numerical value for correct preprocessing of svgtags.in.
set(SVG_EXTRA_DEFINES "")
if (ENABLE_FILTERS)
    list(APPEND SVG_EXTRA_DEFINES "ENABLE_FILTERS=1")
endif ()
if (ENABLE_SVG_FONTS)
    list(APPEND SVG_EXTRA_DEFINES "ENABLE_SVG_FONTS=1")
endif ()

GENERATE_DOM_NAMES(SVG ${WEBCORE_DIR}/svg/svgattrs.in ${WEBCORE_DIR}/svg/svgtags.in "${SVG_EXTRA_DEFINES}")
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/SVGNames.cpp)
if (ENABLE_SVG)
    list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/SVGElementFactory.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/JSSVGElementWrapperFactory.cpp)
endif ()


GENERATE_DOM_NAMES(XLink ${WEBCORE_DIR}/svg/xlinkattrs.in)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XLinkNames.cpp)


GENERATE_DOM_NAMES(XMLNS ${WEBCORE_DIR}/xml/xmlnsattrs.in)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XMLNSNames.cpp)


GENERATE_DOM_NAMES(XML ${WEBCORE_DIR}/xml/xmlattrs.in)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XMLNames.cpp)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h
    MAIN_DEPENDENCY ${WEBKIT_DIR}/scripts/generate-webkitversion.pl
    DEPENDS ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT_DIR}/scripts/generate-webkitversion.pl --config ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig --outputDir ${DERIVED_SOURCES_WEBCORE_DIR}
    VERBATIM)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h)

WEBKIT_WRAP_SOURCELIST(${WebCore_IDL_FILES} ${WebCore_SOURCES})
WEBKIT_WRAP_SOURCELIST(${WebCoreTestSupport_IDL_FILES} ${WebCoreTestSupport_SOURCES})

add_definitions(-DBUILDING_WebCore)
include_directories(${WebCore_INCLUDE_DIRECTORIES} ${WebCoreTestSupport_INCLUDE_DIRECTORIES})

add_library(WebCore ${WebCore_LIBRARY_TYPE} ${WebCore_SOURCES})
set_target_properties(WebCore PROPERTIES FOLDER "WebCore")
set_target_properties(WebCore PROPERTIES LINK_INTERFACE_LIBRARIES "")

if (WebCore_OUTPUT_NAME)
    set_target_properties(WebCore PROPERTIES OUTPUT_NAME ${WebCore_OUTPUT_NAME})
endif ()

add_library(WebCoreTestSupport ${WebCoreTestSupport_LIBRARY_TYPE} ${WebCoreTestSupport_SOURCES})
target_link_libraries(WebCoreTestSupport ${WebCoreTestSupport_LIBRARIES})
set_target_properties(WebCoreTestSupport PROPERTIES FOLDER "WebCore")

if (WebCoreTestSupport_OUTPUT_NAME)
    set_target_properties(WebCoreTestSupport PROPERTIES OUTPUT_NAME ${WebCoreTestSupport_OUTPUT_NAME})
endif ()

if (WTF_USE_LEVELDB)
    add_library(leveldb STATIC ${LEVELDB_SOURCES})
    target_link_libraries(leveldb ${CMAKE_THREAD_LIBS_INIT})
    set_target_properties(leveldb PROPERTIES COMPILE_DEFINITIONS "LEVELDB_PLATFORM_POSIX=1 OS_LINUX=1")
    list(APPEND WebCore_LIBRARIES leveldb)
    WEBKIT_SET_EXTRA_COMPILER_FLAGS(leveldb IGNORECXX_WARNINGS)
endif ()

if (WTF_USE_3D_GRAPHICS)
    add_library(ANGLESupport STATIC ${ANGLESupport_SOURCES})
    set_target_properties(ANGLESupport PROPERTIES FOLDER "WebCore")
    list(APPEND WebCore_LIBRARIES ANGLESupport)
    WEBKIT_SET_EXTRA_COMPILER_FLAGS(ANGLESupport IGNORECXX_WARNINGS)
endif ()

target_link_libraries(WebCore ${WebCore_LIBRARIES})

if (WebCore_OUTPUT_NAME)
    set_target_properties(WebCore PROPERTIES OUTPUT_NAME ${WebCore_OUTPUT_NAME})
endif ()

if (SHARED_CORE)
    set_target_properties(WebCore PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    install(TARGETS WebCore DESTINATION "${LIB_INSTALL_DIR}")
endif ()
