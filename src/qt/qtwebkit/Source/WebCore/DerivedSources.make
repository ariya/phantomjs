# Copyright (C) 2006, 2007, 2008, 2012 Apple Inc. All rights reserved.
# Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
# Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
# 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

VPATH = \
    $(WebCore) \
    $(WebCore)/Modules/encryptedmedia \
    $(WebCore)/Modules/filesystem \
    $(WebCore)/Modules/geolocation \
    $(WebCore)/Modules/indexeddb \
    $(WebCore)/Modules/indieui \
    $(WebCore)/Modules/mediasource \
    $(WebCore)/Modules/notifications \
    $(WebCore)/Modules/quota \
    $(WebCore)/Modules/speech \
    $(WebCore)/Modules/webaudio \
    $(WebCore)/Modules/webdatabase \
    $(WebCore)/Modules/websockets \
    $(WebCore)/bindings/generic \
    $(WebCore)/bindings/js \
    $(WebCore)/bindings/objc \
    $(WebCore)/css \
    $(WebCore)/dom \
    $(WebCore)/editing \
    $(WebCore)/fileapi \
    $(WebCore)/html \
    $(WebCore)/html/canvas \
    $(WebCore)/html/shadow \
    $(WebCore)/html/track \
    $(WebCore)/inspector \
    $(WebCore)/loader/appcache \
    $(WebCore)/page \
    $(WebCore)/plugins \
    $(WebCore)/storage \
    $(WebCore)/xml \
    $(WebCore)/workers \
    $(WebCore)/svg \
    $(WebCore)/testing \
    $(WebCore)/websockets \
#

BINDING_IDLS = \
    $(WebCore)/Modules/encryptedmedia/MediaKeyMessageEvent.idl \
    $(WebCore)/Modules/encryptedmedia/MediaKeyNeededEvent.idl \
    $(WebCore)/Modules/encryptedmedia/MediaKeySession.idl \
    $(WebCore)/Modules/encryptedmedia/MediaKeys.idl \
    $(WebCore)/Modules/filesystem/DOMFileSystem.idl \
    $(WebCore)/Modules/filesystem/DOMFileSystemSync.idl \
    $(WebCore)/Modules/filesystem/DOMWindowFileSystem.idl \
    $(WebCore)/Modules/filesystem/DirectoryEntry.idl \
    $(WebCore)/Modules/filesystem/DirectoryEntrySync.idl \
    $(WebCore)/Modules/filesystem/DirectoryReader.idl \
    $(WebCore)/Modules/filesystem/DirectoryReaderSync.idl \
    $(WebCore)/Modules/filesystem/EntriesCallback.idl \
    $(WebCore)/Modules/filesystem/Entry.idl \
    $(WebCore)/Modules/filesystem/EntryArray.idl \
    $(WebCore)/Modules/filesystem/EntryArraySync.idl \
    $(WebCore)/Modules/filesystem/EntryCallback.idl \
    $(WebCore)/Modules/filesystem/EntrySync.idl \
    $(WebCore)/Modules/filesystem/ErrorCallback.idl \
    $(WebCore)/Modules/filesystem/FileCallback.idl \
    $(WebCore)/Modules/filesystem/FileEntry.idl \
    $(WebCore)/Modules/filesystem/FileEntrySync.idl \
    $(WebCore)/Modules/filesystem/FileSystemCallback.idl \
    $(WebCore)/Modules/filesystem/FileWriter.idl \
    $(WebCore)/Modules/filesystem/FileWriterCallback.idl \
    $(WebCore)/Modules/filesystem/FileWriterSync.idl \
    $(WebCore)/Modules/filesystem/Metadata.idl \
    $(WebCore)/Modules/filesystem/MetadataCallback.idl \
    $(WebCore)/Modules/filesystem/WorkerGlobalScopeFileSystem.idl \
    $(WebCore)/Modules/geolocation/Coordinates.idl \
    $(WebCore)/Modules/geolocation/Geolocation.idl \
    $(WebCore)/Modules/geolocation/Geoposition.idl \
    $(WebCore)/Modules/geolocation/NavigatorGeolocation.idl \
    $(WebCore)/Modules/geolocation/PositionCallback.idl \
    $(WebCore)/Modules/geolocation/PositionError.idl \
    $(WebCore)/Modules/geolocation/PositionErrorCallback.idl \
    $(WebCore)/Modules/indexeddb/DOMWindowIndexedDatabase.idl \
    $(WebCore)/Modules/indexeddb/IDBAny.idl \
    $(WebCore)/Modules/indexeddb/IDBCursor.idl \
    $(WebCore)/Modules/indexeddb/IDBDatabase.idl \
    $(WebCore)/Modules/indexeddb/IDBFactory.idl \
    $(WebCore)/Modules/indexeddb/IDBIndex.idl \
    $(WebCore)/Modules/indexeddb/IDBKeyRange.idl \
    $(WebCore)/Modules/indexeddb/IDBObjectStore.idl \
    $(WebCore)/Modules/indexeddb/IDBRequest.idl \
    $(WebCore)/Modules/indexeddb/IDBTransaction.idl \
    $(WebCore)/Modules/indexeddb/IDBVersionChangeEvent.idl \
    $(WebCore)/Modules/indexeddb/WorkerGlobalScopeIndexedDatabase.idl \
    $(WebCore)/Modules/indieui/UIRequestEvent.idl \
    $(WebCore)/Modules/mediasource/MediaSource.idl \
    $(WebCore)/Modules/mediasource/SourceBuffer.idl \
    $(WebCore)/Modules/mediasource/SourceBufferList.idl \
    $(WebCore)/Modules/notifications/DOMWindowNotifications.idl \
    $(WebCore)/Modules/notifications/Notification.idl \
    $(WebCore)/Modules/notifications/NotificationCenter.idl \
    $(WebCore)/Modules/notifications/NotificationPermissionCallback.idl \
    $(WebCore)/Modules/notifications/WorkerGlobalScopeNotifications.idl \
    $(WebCore)/Modules/quota/DOMWindowQuota.idl \
    $(WebCore)/Modules/quota/NavigatorStorageQuota.idl \
    $(WebCore)/Modules/quota/StorageInfo.idl \
    $(WebCore)/Modules/quota/StorageErrorCallback.idl \
    $(WebCore)/Modules/quota/StorageQuota.idl \
    $(WebCore)/Modules/quota/StorageQuotaCallback.idl \
    $(WebCore)/Modules/quota/StorageUsageCallback.idl \
    $(WebCore)/Modules/quota/WorkerNavigatorStorageQuota.idl \
    $(WebCore)/Modules/speech/DOMWindowSpeechSynthesis.idl \
    $(WebCore)/Modules/speech/SpeechSynthesis.idl \
    $(WebCore)/Modules/speech/SpeechSynthesisEvent.idl \
    $(WebCore)/Modules/speech/SpeechSynthesisUtterance.idl \
    $(WebCore)/Modules/speech/SpeechSynthesisVoice.idl \
    $(WebCore)/Modules/webaudio/AudioBuffer.idl \
    $(WebCore)/Modules/webaudio/AudioBufferCallback.idl \
    $(WebCore)/Modules/webaudio/AudioBufferSourceNode.idl \
    $(WebCore)/Modules/webaudio/ChannelMergerNode.idl \
    $(WebCore)/Modules/webaudio/ChannelSplitterNode.idl \
    $(WebCore)/Modules/webaudio/AudioContext.idl \
    $(WebCore)/Modules/webaudio/AudioDestinationNode.idl \
    $(WebCore)/Modules/webaudio/GainNode.idl \
    $(WebCore)/Modules/webaudio/AudioListener.idl \
    $(WebCore)/Modules/webaudio/AudioNode.idl \
    $(WebCore)/Modules/webaudio/PannerNode.idl \
    $(WebCore)/Modules/webaudio/AudioParam.idl \
    $(WebCore)/Modules/webaudio/AudioProcessingEvent.idl \
    $(WebCore)/Modules/webaudio/BiquadFilterNode.idl \
    $(WebCore)/Modules/webaudio/ConvolverNode.idl \
    $(WebCore)/Modules/webaudio/DelayNode.idl \
    $(WebCore)/Modules/webaudio/DynamicsCompressorNode.idl \
    $(WebCore)/Modules/webaudio/ScriptProcessorNode.idl \
    $(WebCore)/Modules/webaudio/MediaElementAudioSourceNode.idl \
    $(WebCore)/Modules/webaudio/MediaStreamAudioSourceNode.idl \
    $(WebCore)/Modules/webaudio/OscillatorNode.idl \
    $(WebCore)/Modules/webaudio/OfflineAudioContext.idl \
    $(WebCore)/Modules/webaudio/OfflineAudioCompletionEvent.idl \
    $(WebCore)/Modules/webaudio/AnalyserNode.idl \
    $(WebCore)/Modules/webaudio/WaveShaperNode.idl \
    $(WebCore)/Modules/webaudio/PeriodicWave.idl \
    $(WebCore)/Modules/webdatabase/DOMWindowWebDatabase.idl \
    $(WebCore)/Modules/webdatabase/Database.idl \
    $(WebCore)/Modules/webdatabase/DatabaseCallback.idl \
    $(WebCore)/Modules/webdatabase/DatabaseSync.idl \
    $(WebCore)/Modules/webdatabase/SQLError.idl \
    $(WebCore)/Modules/webdatabase/SQLException.idl \
    $(WebCore)/Modules/webdatabase/SQLResultSet.idl \
    $(WebCore)/Modules/webdatabase/SQLResultSetRowList.idl \
    $(WebCore)/Modules/webdatabase/SQLStatementCallback.idl \
    $(WebCore)/Modules/webdatabase/SQLStatementErrorCallback.idl \
    $(WebCore)/Modules/webdatabase/SQLTransaction.idl \
    $(WebCore)/Modules/webdatabase/SQLTransactionCallback.idl \
    $(WebCore)/Modules/webdatabase/SQLTransactionErrorCallback.idl \
    $(WebCore)/Modules/webdatabase/SQLTransactionSync.idl \
    $(WebCore)/Modules/webdatabase/SQLTransactionSyncCallback.idl \
    $(WebCore)/Modules/webdatabase/WorkerGlobalScopeWebDatabase.idl \
    $(WebCore)/Modules/websockets/CloseEvent.idl \
    $(WebCore)/Modules/websockets/WebSocket.idl \
    $(WebCore)/css/CSSCharsetRule.idl \
    $(WebCore)/css/CSSFontFaceLoadEvent.idl \
    $(WebCore)/css/CSSFontFaceRule.idl \
    $(WebCore)/css/CSSHostRule.idl \
    $(WebCore)/css/CSSImportRule.idl \
    $(WebCore)/css/CSSMediaRule.idl \
    $(WebCore)/css/CSSPageRule.idl \
    $(WebCore)/css/CSSPrimitiveValue.idl \
    $(WebCore)/css/CSSRule.idl \
    $(WebCore)/css/CSSRuleList.idl \
    $(WebCore)/css/CSSStyleDeclaration.idl \
    $(WebCore)/css/CSSStyleRule.idl \
    $(WebCore)/css/CSSStyleSheet.idl \
    $(WebCore)/css/CSSSupportsRule.idl \
    $(WebCore)/css/CSSUnknownRule.idl \
    $(WebCore)/css/CSSValue.idl \
    $(WebCore)/css/CSSValueList.idl \
    $(WebCore)/css/Counter.idl \
    $(WebCore)/css/DOMWindowCSS.idl \
    $(WebCore)/css/FontLoader.idl \
    $(WebCore)/css/MediaList.idl \
    $(WebCore)/css/MediaQueryList.idl \
    $(WebCore)/css/MediaQueryListListener.idl \
    $(WebCore)/css/RGBColor.idl \
    $(WebCore)/css/Rect.idl \
    $(WebCore)/css/StyleMedia.idl \
    $(WebCore)/css/StyleSheet.idl \
    $(WebCore)/css/StyleSheetList.idl \
    $(WebCore)/css/WebKitCSSFilterValue.idl \
    $(WebCore)/css/WebKitCSSFilterRule.idl \
    $(WebCore)/css/WebKitCSSKeyframeRule.idl \
    $(WebCore)/css/WebKitCSSKeyframesRule.idl \
    $(WebCore)/css/WebKitCSSMatrix.idl \
    $(WebCore)/css/WebKitCSSMixFunctionValue.idl \
    $(WebCore)/css/WebKitCSSRegionRule.idl \
    $(WebCore)/css/WebKitCSSTransformValue.idl \
    $(WebCore)/css/WebKitCSSViewportRule.idl \
    $(WebCore)/dom/Attr.idl \
    $(WebCore)/dom/BeforeLoadEvent.idl \
    $(WebCore)/dom/CDATASection.idl \
    $(WebCore)/dom/CharacterData.idl \
    $(WebCore)/dom/ChildNode.idl \
    $(WebCore)/dom/ClientRect.idl \
    $(WebCore)/dom/ClientRectList.idl \
    $(WebCore)/dom/Clipboard.idl \
    $(WebCore)/dom/Comment.idl \
    $(WebCore)/dom/CompositionEvent.idl \
    $(WebCore)/dom/CustomElementConstructor.idl \
    $(WebCore)/dom/CustomEvent.idl \
    $(WebCore)/dom/DOMCoreException.idl \
    $(WebCore)/dom/DOMError.idl \
    $(WebCore)/dom/DOMImplementation.idl \
    $(WebCore)/dom/DOMStringList.idl \
    $(WebCore)/dom/DOMStringMap.idl \
    $(WebCore)/dom/DataTransferItem.idl \
    $(WebCore)/dom/DataTransferItemList.idl \
    $(WebCore)/dom/DeviceMotionEvent.idl \
    $(WebCore)/dom/DeviceOrientationEvent.idl \
    $(WebCore)/dom/Document.idl \
    $(WebCore)/dom/DocumentFragment.idl \
    $(WebCore)/dom/DocumentType.idl \
    $(WebCore)/dom/Element.idl \
    $(WebCore)/dom/Entity.idl \
    $(WebCore)/dom/EntityReference.idl \
    $(WebCore)/dom/ErrorEvent.idl \
    $(WebCore)/dom/Event.idl \
    $(WebCore)/dom/EventException.idl \
    $(WebCore)/dom/EventListener.idl \
    $(WebCore)/dom/EventTarget.idl \
    $(WebCore)/dom/FocusEvent.idl \
    $(WebCore)/dom/HashChangeEvent.idl \
    $(WebCore)/dom/KeyboardEvent.idl \
    $(WebCore)/dom/MessageChannel.idl \
    $(WebCore)/dom/MessageEvent.idl \
    $(WebCore)/dom/MessagePort.idl \
    $(WebCore)/dom/MouseEvent.idl \
    $(WebCore)/dom/MutationEvent.idl \
    $(WebCore)/dom/MutationObserver.idl \
    $(WebCore)/dom/MutationRecord.idl \
    $(WebCore)/dom/DOMNamedFlowCollection.idl \
    $(WebCore)/dom/NamedNodeMap.idl \
    $(WebCore)/dom/Node.idl \
    $(WebCore)/dom/NodeFilter.idl \
    $(WebCore)/dom/NodeIterator.idl \
    $(WebCore)/dom/NodeList.idl \
    $(WebCore)/dom/Notation.idl \
    $(WebCore)/dom/OverflowEvent.idl \
    $(WebCore)/dom/PageTransitionEvent.idl \
    $(WebCore)/dom/PopStateEvent.idl \
    $(WebCore)/dom/ProcessingInstruction.idl \
    $(WebCore)/dom/ProgressEvent.idl \
    $(WebCore)/dom/ProgressEvent.idl \
    $(WebCore)/dom/PropertyNodeList.idl \
    $(WebCore)/dom/Range.idl \
    $(WebCore)/dom/RangeException.idl \
    $(WebCore)/dom/RequestAnimationFrameCallback.idl \
    $(WebCore)/dom/ShadowRoot.idl \
    $(WebCore)/dom/StringCallback.idl \
    $(WebCore)/dom/Text.idl \
    $(WebCore)/dom/TextEvent.idl \
    $(WebCore)/dom/Touch.idl \
    $(WebCore)/dom/TouchEvent.idl \
    $(WebCore)/dom/TouchList.idl \
    $(WebCore)/dom/TransitionEvent.idl \
    $(WebCore)/dom/TreeWalker.idl \
    $(WebCore)/dom/UIEvent.idl \
    $(WebCore)/dom/WebKitAnimationEvent.idl \
    $(WebCore)/dom/WebKitNamedFlow.idl \
    $(WebCore)/dom/WebKitTransitionEvent.idl \
    $(WebCore)/dom/WheelEvent.idl \
    $(WebCore)/fileapi/Blob.idl \
    $(WebCore)/fileapi/File.idl \
    $(WebCore)/fileapi/FileError.idl \
    $(WebCore)/fileapi/FileException.idl \
    $(WebCore)/fileapi/FileList.idl \
    $(WebCore)/fileapi/FileReader.idl \
    $(WebCore)/fileapi/FileReaderSync.idl \
    $(WebCore)/html/DOMFormData.idl \
    $(WebCore)/html/DOMSettableTokenList.idl \
    $(WebCore)/html/DOMTokenList.idl \
    $(WebCore)/html/DOMURL.idl \
    $(WebCore)/html/HTMLAllCollection.idl \
    $(WebCore)/html/HTMLAnchorElement.idl \
    $(WebCore)/html/HTMLAppletElement.idl \
    $(WebCore)/html/HTMLAreaElement.idl \
    $(WebCore)/html/HTMLAudioElement.idl \
    $(WebCore)/html/HTMLBRElement.idl \
    $(WebCore)/html/HTMLBaseElement.idl \
    $(WebCore)/html/HTMLBaseFontElement.idl \
    $(WebCore)/html/HTMLBodyElement.idl \
    $(WebCore)/html/HTMLButtonElement.idl \
    $(WebCore)/html/HTMLCanvasElement.idl \
    $(WebCore)/html/HTMLCollection.idl \
    $(WebCore)/html/HTMLDListElement.idl \
    $(WebCore)/html/HTMLDataListElement.idl \
    $(WebCore)/html/HTMLDetailsElement.idl \
    $(WebCore)/html/HTMLDialogElement.idl \
    $(WebCore)/html/HTMLDirectoryElement.idl \
    $(WebCore)/html/HTMLDivElement.idl \
    $(WebCore)/html/HTMLDocument.idl \
    $(WebCore)/html/HTMLElement.idl \
    $(WebCore)/html/HTMLEmbedElement.idl \
    $(WebCore)/html/HTMLFieldSetElement.idl \
    $(WebCore)/html/HTMLFontElement.idl \
    $(WebCore)/html/HTMLFormControlsCollection.idl \
    $(WebCore)/html/HTMLFormElement.idl \
    $(WebCore)/html/HTMLFrameElement.idl \
    $(WebCore)/html/HTMLFrameSetElement.idl \
    $(WebCore)/html/HTMLHRElement.idl \
    $(WebCore)/html/HTMLHeadElement.idl \
    $(WebCore)/html/HTMLHeadingElement.idl \
    $(WebCore)/html/HTMLHtmlElement.idl \
    $(WebCore)/html/HTMLIFrameElement.idl \
    $(WebCore)/html/HTMLImageElement.idl \
    $(WebCore)/html/HTMLInputElement.idl \
    $(WebCore)/html/HTMLKeygenElement.idl \
    $(WebCore)/html/HTMLLIElement.idl \
    $(WebCore)/html/HTMLLabelElement.idl \
    $(WebCore)/html/HTMLLegendElement.idl \
    $(WebCore)/html/HTMLLinkElement.idl \
    $(WebCore)/html/HTMLMapElement.idl \
    $(WebCore)/html/HTMLMarqueeElement.idl \
    $(WebCore)/html/HTMLMediaElement.idl \
    $(WebCore)/html/HTMLMenuElement.idl \
    $(WebCore)/html/HTMLMetaElement.idl \
    $(WebCore)/html/HTMLMeterElement.idl \
    $(WebCore)/html/HTMLModElement.idl \
    $(WebCore)/html/HTMLOListElement.idl \
    $(WebCore)/html/HTMLObjectElement.idl \
    $(WebCore)/html/HTMLOptGroupElement.idl \
    $(WebCore)/html/HTMLOptionElement.idl \
    $(WebCore)/html/HTMLOptionsCollection.idl \
    $(WebCore)/html/HTMLOutputElement.idl \
    $(WebCore)/html/HTMLParagraphElement.idl \
    $(WebCore)/html/HTMLParamElement.idl \
    $(WebCore)/html/HTMLPreElement.idl \
    $(WebCore)/html/HTMLProgressElement.idl \
    $(WebCore)/html/HTMLPropertiesCollection.idl \
    $(WebCore)/html/HTMLQuoteElement.idl \
    $(WebCore)/html/HTMLScriptElement.idl \
    $(WebCore)/html/HTMLSelectElement.idl \
    $(WebCore)/html/HTMLSourceElement.idl \
    $(WebCore)/html/HTMLSpanElement.idl \
    $(WebCore)/html/HTMLStyleElement.idl \
    $(WebCore)/html/HTMLTableCaptionElement.idl \
    $(WebCore)/html/HTMLTableCellElement.idl \
    $(WebCore)/html/HTMLTableColElement.idl \
    $(WebCore)/html/HTMLTableElement.idl \
    $(WebCore)/html/HTMLTableRowElement.idl \
    $(WebCore)/html/HTMLTableSectionElement.idl \
    $(WebCore)/html/HTMLTemplateElement.idl \
    $(WebCore)/html/HTMLTextAreaElement.idl \
    $(WebCore)/html/HTMLTitleElement.idl \
    $(WebCore)/html/HTMLTrackElement.idl \
    $(WebCore)/html/HTMLUListElement.idl \
    $(WebCore)/html/HTMLUnknownElement.idl \
    $(WebCore)/html/HTMLVideoElement.idl \
    $(WebCore)/html/ImageData.idl \
    $(WebCore)/html/MediaController.idl \
    $(WebCore)/html/MediaError.idl \
    $(WebCore)/html/MediaKeyError.idl \
    $(WebCore)/html/MediaKeyEvent.idl \
    $(WebCore)/html/MicroDataItemValue.idl \
    $(WebCore)/html/RadioNodeList.idl \
    $(WebCore)/html/TextMetrics.idl \
    $(WebCore)/html/TimeRanges.idl \
    $(WebCore)/html/ValidityState.idl \
    $(WebCore)/html/VoidCallback.idl \
    $(WebCore)/html/canvas/ArrayBuffer.idl \
    $(WebCore)/html/canvas/ArrayBufferView.idl \
    $(WebCore)/html/canvas/CanvasGradient.idl \
    $(WebCore)/html/canvas/CanvasPattern.idl \
    $(WebCore)/html/canvas/CanvasProxy.idl \
    $(WebCore)/html/canvas/CanvasRenderingContext.idl \
    $(WebCore)/html/canvas/CanvasRenderingContext2D.idl \
    $(WebCore)/html/canvas/DataView.idl \
    $(WebCore)/html/canvas/DOMPath.idl \
    $(WebCore)/html/canvas/EXTDrawBuffers.idl \
    $(WebCore)/html/canvas/EXTTextureFilterAnisotropic.idl \
    $(WebCore)/html/canvas/Float32Array.idl \
    $(WebCore)/html/canvas/Float64Array.idl \
    $(WebCore)/html/canvas/Int16Array.idl \
    $(WebCore)/html/canvas/Int32Array.idl \
    $(WebCore)/html/canvas/Int8Array.idl \
    $(WebCore)/html/canvas/OESElementIndexUint.idl \
    $(WebCore)/html/canvas/OESStandardDerivatives.idl \
    $(WebCore)/html/canvas/OESTextureFloat.idl \
    $(WebCore)/html/canvas/OESTextureHalfFloat.idl \
    $(WebCore)/html/canvas/OESVertexArrayObject.idl \
    $(WebCore)/html/canvas/Uint16Array.idl \
    $(WebCore)/html/canvas/Uint32Array.idl \
    $(WebCore)/html/canvas/Uint8Array.idl \
    $(WebCore)/html/canvas/Uint8ClampedArray.idl \
    $(WebCore)/html/canvas/WebGLActiveInfo.idl \
    $(WebCore)/html/canvas/WebGLBuffer.idl \
    $(WebCore)/html/canvas/WebGLCompressedTextureATC.idl \
    $(WebCore)/html/canvas/WebGLCompressedTexturePVRTC.idl \
    $(WebCore)/html/canvas/WebGLCompressedTextureS3TC.idl \
    $(WebCore)/html/canvas/WebGLContextAttributes.idl \
    $(WebCore)/html/canvas/WebGLContextEvent.idl \
    $(WebCore)/html/canvas/WebGLDebugRendererInfo.idl \
    $(WebCore)/html/canvas/WebGLDebugShaders.idl \
    $(WebCore)/html/canvas/WebGLDepthTexture.idl \
    $(WebCore)/html/canvas/WebGLFramebuffer.idl \
    $(WebCore)/html/canvas/WebGLLoseContext.idl \
    $(WebCore)/html/canvas/WebGLProgram.idl \
    $(WebCore)/html/canvas/WebGLRenderbuffer.idl \
    $(WebCore)/html/canvas/WebGLRenderingContext.idl \
    $(WebCore)/html/canvas/WebGLShader.idl \
    $(WebCore)/html/canvas/WebGLShaderPrecisionFormat.idl \
    $(WebCore)/html/canvas/WebGLTexture.idl \
    $(WebCore)/html/canvas/WebGLUniformLocation.idl \
    $(WebCore)/html/canvas/WebGLVertexArrayObjectOES.idl \
    $(WebCore)/html/shadow/HTMLContentElement.idl \
    $(WebCore)/html/track/AudioTrack.idl \
    $(WebCore)/html/track/AudioTrackList.idl \
    $(WebCore)/html/track/TextTrack.idl \
    $(WebCore)/html/track/TextTrackCue.idl \
    $(WebCore)/html/track/TextTrackCueList.idl \
    $(WebCore)/html/track/TextTrackList.idl \
    $(WebCore)/html/track/TrackEvent.idl \
    $(WebCore)/html/track/VideoTrack.idl \
    $(WebCore)/html/track/VideoTrackList.idl \
    $(WebCore)/inspector/InjectedScriptHost.idl \
    $(WebCore)/inspector/InspectorFrontendHost.idl \
    $(WebCore)/inspector/ScriptProfile.idl \
    $(WebCore)/inspector/ScriptProfileNode.idl \
    $(WebCore)/loader/appcache/DOMApplicationCache.idl \
    $(WebCore)/page/AbstractView.idl \
    $(WebCore)/page/BarProp.idl \
    $(WebCore)/page/Console.idl \
    $(WebCore)/page/Crypto.idl \
    $(WebCore)/page/DOMSecurityPolicy.idl \
    $(WebCore)/page/DOMSelection.idl \
    $(WebCore)/page/DOMWindow.idl \
    $(WebCore)/page/EventSource.idl \
    $(WebCore)/page/History.idl \
    $(WebCore)/page/Location.idl \
    $(WebCore)/page/Navigator.idl \
    $(WebCore)/page/Performance.idl \
    $(WebCore)/page/PerformanceNavigation.idl \
    $(WebCore)/page/PerformanceTiming.idl \
    $(WebCore)/page/Screen.idl \
    $(WebCore)/page/SpeechInputEvent.idl \
    $(WebCore)/page/SpeechInputResult.idl \
    $(WebCore)/page/SpeechInputResultList.idl \
    $(WebCore)/page/WebKitPoint.idl \
    $(WebCore)/page/WindowBase64.idl \
    $(WebCore)/page/WindowTimers.idl \
    $(WebCore)/page/WorkerNavigator.idl \
    $(WebCore)/plugins/DOMMimeType.idl \
    $(WebCore)/plugins/DOMMimeTypeArray.idl \
    $(WebCore)/plugins/DOMPlugin.idl \
    $(WebCore)/plugins/DOMPluginArray.idl \
    $(WebCore)/storage/Storage.idl \
    $(WebCore)/storage/StorageEvent.idl \
    $(WebCore)/svg/SVGAElement.idl \
    $(WebCore)/svg/SVGAltGlyphDefElement.idl \
    $(WebCore)/svg/SVGAltGlyphElement.idl \
    $(WebCore)/svg/SVGAltGlyphItemElement.idl \
    $(WebCore)/svg/SVGAngle.idl \
    $(WebCore)/svg/SVGAnimateColorElement.idl \
    $(WebCore)/svg/SVGAnimateElement.idl \
    $(WebCore)/svg/SVGAnimateMotionElement.idl \
    $(WebCore)/svg/SVGAnimateTransformElement.idl \
    $(WebCore)/svg/SVGAnimatedAngle.idl \
    $(WebCore)/svg/SVGAnimatedBoolean.idl \
    $(WebCore)/svg/SVGAnimatedEnumeration.idl \
    $(WebCore)/svg/SVGAnimatedInteger.idl \
    $(WebCore)/svg/SVGAnimatedLength.idl \
    $(WebCore)/svg/SVGAnimatedLengthList.idl \
    $(WebCore)/svg/SVGAnimatedNumber.idl \
    $(WebCore)/svg/SVGAnimatedNumberList.idl \
    $(WebCore)/svg/SVGAnimatedPreserveAspectRatio.idl \
    $(WebCore)/svg/SVGAnimatedRect.idl \
    $(WebCore)/svg/SVGAnimatedString.idl \
    $(WebCore)/svg/SVGAnimatedTransformList.idl \
    $(WebCore)/svg/SVGAnimationElement.idl \
    $(WebCore)/svg/SVGCircleElement.idl \
    $(WebCore)/svg/SVGClipPathElement.idl \
    $(WebCore)/svg/SVGColor.idl \
    $(WebCore)/svg/SVGComponentTransferFunctionElement.idl \
    $(WebCore)/svg/SVGCursorElement.idl \
    $(WebCore)/svg/SVGDefsElement.idl \
    $(WebCore)/svg/SVGDescElement.idl \
    $(WebCore)/svg/SVGDocument.idl \
    $(WebCore)/svg/SVGElement.idl \
    $(WebCore)/svg/SVGElementInstance.idl \
    $(WebCore)/svg/SVGElementInstanceList.idl \
    $(WebCore)/svg/SVGEllipseElement.idl \
    $(WebCore)/svg/SVGException.idl \
    $(WebCore)/svg/SVGExternalResourcesRequired.idl \
    $(WebCore)/svg/SVGFEBlendElement.idl \
    $(WebCore)/svg/SVGFEColorMatrixElement.idl \
    $(WebCore)/svg/SVGFEComponentTransferElement.idl \
    $(WebCore)/svg/SVGFECompositeElement.idl \
    $(WebCore)/svg/SVGFEConvolveMatrixElement.idl \
    $(WebCore)/svg/SVGFEDiffuseLightingElement.idl \
    $(WebCore)/svg/SVGFEDisplacementMapElement.idl \
    $(WebCore)/svg/SVGFEDistantLightElement.idl \
    $(WebCore)/svg/SVGFEDropShadowElement.idl \
    $(WebCore)/svg/SVGFEFloodElement.idl \
    $(WebCore)/svg/SVGFEFuncAElement.idl \
    $(WebCore)/svg/SVGFEFuncBElement.idl \
    $(WebCore)/svg/SVGFEFuncGElement.idl \
    $(WebCore)/svg/SVGFEFuncRElement.idl \
    $(WebCore)/svg/SVGFEGaussianBlurElement.idl \
    $(WebCore)/svg/SVGFEImageElement.idl \
    $(WebCore)/svg/SVGFEMergeElement.idl \
    $(WebCore)/svg/SVGFEMergeNodeElement.idl \
    $(WebCore)/svg/SVGFEMorphologyElement.idl \
    $(WebCore)/svg/SVGFEOffsetElement.idl \
    $(WebCore)/svg/SVGFEPointLightElement.idl \
    $(WebCore)/svg/SVGFESpecularLightingElement.idl \
    $(WebCore)/svg/SVGFESpotLightElement.idl \
    $(WebCore)/svg/SVGFETileElement.idl \
    $(WebCore)/svg/SVGFETurbulenceElement.idl \
    $(WebCore)/svg/SVGFilterElement.idl \
    $(WebCore)/svg/SVGFilterPrimitiveStandardAttributes.idl \
    $(WebCore)/svg/SVGFitToViewBox.idl \
    $(WebCore)/svg/SVGFontElement.idl \
    $(WebCore)/svg/SVGFontFaceElement.idl \
    $(WebCore)/svg/SVGFontFaceFormatElement.idl \
    $(WebCore)/svg/SVGFontFaceNameElement.idl \
    $(WebCore)/svg/SVGFontFaceSrcElement.idl \
    $(WebCore)/svg/SVGFontFaceUriElement.idl \
    $(WebCore)/svg/SVGForeignObjectElement.idl \
    $(WebCore)/svg/SVGGElement.idl \
    $(WebCore)/svg/SVGGlyphElement.idl \
    $(WebCore)/svg/SVGGlyphRefElement.idl \
    $(WebCore)/svg/SVGGradientElement.idl \
    $(WebCore)/svg/SVGGraphicsElement.idl \
    $(WebCore)/svg/SVGHKernElement.idl \
    $(WebCore)/svg/SVGImageElement.idl \
    $(WebCore)/svg/SVGLength.idl \
    $(WebCore)/svg/SVGLengthList.idl \
    $(WebCore)/svg/SVGLineElement.idl \
    $(WebCore)/svg/SVGLinearGradientElement.idl \
    $(WebCore)/svg/SVGMPathElement.idl \
    $(WebCore)/svg/SVGMarkerElement.idl \
    $(WebCore)/svg/SVGMaskElement.idl \
    $(WebCore)/svg/SVGMatrix.idl \
    $(WebCore)/svg/SVGMetadataElement.idl \
    $(WebCore)/svg/SVGMissingGlyphElement.idl \
    $(WebCore)/svg/SVGNumber.idl \
    $(WebCore)/svg/SVGNumberList.idl \
    $(WebCore)/svg/SVGPaint.idl \
    $(WebCore)/svg/SVGPathElement.idl \
    $(WebCore)/svg/SVGPathSeg.idl \
    $(WebCore)/svg/SVGPathSegArcAbs.idl \
    $(WebCore)/svg/SVGPathSegArcRel.idl \
    $(WebCore)/svg/SVGPathSegClosePath.idl \
    $(WebCore)/svg/SVGPathSegCurvetoCubicAbs.idl \
    $(WebCore)/svg/SVGPathSegCurvetoCubicRel.idl \
    $(WebCore)/svg/SVGPathSegCurvetoCubicSmoothAbs.idl \
    $(WebCore)/svg/SVGPathSegCurvetoCubicSmoothRel.idl \
    $(WebCore)/svg/SVGPathSegCurvetoQuadraticAbs.idl \
    $(WebCore)/svg/SVGPathSegCurvetoQuadraticRel.idl \
    $(WebCore)/svg/SVGPathSegCurvetoQuadraticSmoothAbs.idl \
    $(WebCore)/svg/SVGPathSegCurvetoQuadraticSmoothRel.idl \
    $(WebCore)/svg/SVGPathSegLinetoAbs.idl \
    $(WebCore)/svg/SVGPathSegLinetoHorizontalAbs.idl \
    $(WebCore)/svg/SVGPathSegLinetoHorizontalRel.idl \
    $(WebCore)/svg/SVGPathSegLinetoRel.idl \
    $(WebCore)/svg/SVGPathSegLinetoVerticalAbs.idl \
    $(WebCore)/svg/SVGPathSegLinetoVerticalRel.idl \
    $(WebCore)/svg/SVGPathSegList.idl \
    $(WebCore)/svg/SVGPathSegMovetoAbs.idl \
    $(WebCore)/svg/SVGPathSegMovetoRel.idl \
    $(WebCore)/svg/SVGPatternElement.idl \
    $(WebCore)/svg/SVGPoint.idl \
    $(WebCore)/svg/SVGPointList.idl \
    $(WebCore)/svg/SVGPolygonElement.idl \
    $(WebCore)/svg/SVGPolylineElement.idl \
    $(WebCore)/svg/SVGPreserveAspectRatio.idl \
    $(WebCore)/svg/SVGRadialGradientElement.idl \
    $(WebCore)/svg/SVGRect.idl \
    $(WebCore)/svg/SVGRectElement.idl \
    $(WebCore)/svg/SVGRenderingIntent.idl \
    $(WebCore)/svg/SVGSVGElement.idl \
    $(WebCore)/svg/SVGScriptElement.idl \
    $(WebCore)/svg/SVGSetElement.idl \
    $(WebCore)/svg/SVGStopElement.idl \
    $(WebCore)/svg/SVGStringList.idl \
    $(WebCore)/svg/SVGStyleElement.idl \
    $(WebCore)/svg/SVGStyledElement.idl \
    $(WebCore)/svg/SVGSwitchElement.idl \
    $(WebCore)/svg/SVGSymbolElement.idl \
    $(WebCore)/svg/SVGTRefElement.idl \
    $(WebCore)/svg/SVGTSpanElement.idl \
    $(WebCore)/svg/SVGTests.idl \
    $(WebCore)/svg/SVGTextContentElement.idl \
    $(WebCore)/svg/SVGTextElement.idl \
    $(WebCore)/svg/SVGTextPathElement.idl \
    $(WebCore)/svg/SVGTextPositioningElement.idl \
    $(WebCore)/svg/SVGTitleElement.idl \
    $(WebCore)/svg/SVGTransform.idl \
    $(WebCore)/svg/SVGTransformList.idl \
    $(WebCore)/svg/SVGURIReference.idl \
    $(WebCore)/svg/SVGUnitTypes.idl \
    $(WebCore)/svg/SVGUseElement.idl \
    $(WebCore)/svg/SVGVKernElement.idl \
    $(WebCore)/svg/SVGViewElement.idl \
    $(WebCore)/svg/SVGViewSpec.idl \
    $(WebCore)/svg/SVGZoomAndPan.idl \
    $(WebCore)/svg/SVGZoomEvent.idl \
    $(WebCore)/testing/Internals.idl \
    $(WebCore)/testing/InternalSettings.idl \
    $(WebCore)/testing/MallocStatistics.idl \
    $(WebCore)/testing/MemoryInfo.idl \
    $(WebCore)/testing/TypeConversions.idl \
    $(WebCore)/workers/AbstractWorker.idl \
    $(WebCore)/workers/DedicatedWorkerGlobalScope.idl \
    $(WebCore)/workers/SharedWorker.idl \
    $(WebCore)/workers/SharedWorkerGlobalScope.idl \
    $(WebCore)/workers/Worker.idl \
    $(WebCore)/workers/WorkerGlobalScope.idl \
    $(WebCore)/workers/WorkerLocation.idl \
    $(WebCore)/xml/DOMParser.idl \
    $(WebCore)/xml/XMLHttpRequest.idl \
    $(WebCore)/xml/XMLHttpRequestException.idl \
    $(WebCore)/xml/XMLHttpRequestProgressEvent.idl \
    $(WebCore)/xml/XMLHttpRequestUpload.idl \
    $(WebCore)/xml/XMLSerializer.idl \
    $(WebCore)/xml/XPathEvaluator.idl \
    $(WebCore)/xml/XPathException.idl \
    $(WebCore)/xml/XPathExpression.idl \
    $(WebCore)/xml/XPathNSResolver.idl \
    $(WebCore)/xml/XPathResult.idl \
    $(WebCore)/xml/XSLTProcessor.idl \
    InternalSettingsGenerated.idl
#

.PHONY : all

DOM_CLASSES=$(basename $(notdir $(BINDING_IDLS)))

JS_DOM_HEADERS=$(filter-out JSMediaQueryListListener.h JSEventListener.h, $(DOM_CLASSES:%=JS%.h))

WEB_DOM_HEADERS :=

all : \
    $(SUPPLEMENTAL_DEPENDENCY_FILE) \
    $(WINDOW_CONSTRUCTORS_FILE) \
    $(WORKERGLOBALSCOPE_CONSTRUCTORS_FILE) \
    $(JS_DOM_HEADERS) \
    $(WEB_DOM_HEADERS) \
    \
    JSJavaScriptCallFrame.h \
    \
    CSSGrammar.cpp \
    CSSPropertyNames.h \
    CSSValueKeywords.h \
    ColorData.cpp \
    EventFactory.cpp \
    EventTargetInterfaces.h \
    ExceptionCodeDescription.cpp \
    HTMLElementFactory.cpp \
    HTMLEntityTable.cpp \
    HTMLNames.cpp \
    JSSVGElementWrapperFactory.cpp \
    PlugInsResources.h \
    SVGElementFactory.cpp \
    SVGNames.cpp \
    UserAgentStyleSheets.h \
    WebKitFontFamilyNames.cpp \
    WebKitFontFamilyNames.h \
    XLinkNames.cpp \
    XMLNSNames.cpp \
    XMLNames.cpp \
    MathMLElementFactory.cpp \
    MathMLNames.cpp \
    XPathGrammar.cpp \
#

# --------

ADDITIONAL_IDL_DEFINES :=

ifeq ($(OS),MACOS)

FRAMEWORK_FLAGS = $(shell echo $(BUILT_PRODUCTS_DIR) $(FRAMEWORK_SEARCH_PATHS) | perl -e 'print "-F " . join(" -F ", split(" ", <>));')
HEADER_FLAGS = $(shell echo $(BUILT_PRODUCTS_DIR) $(HEADER_SEARCH_PATHS) | perl -e 'print "-I" . join(" -I", split(" ", <>));')
TEXT_PREPROCESSOR_FLAGS=-E -P -x c -traditional

ifneq ($(SDKROOT),)
	SDK_FLAGS=-isysroot $(SDKROOT)
endif

ifeq ($(shell $(CC) -x c++ -E -P -dM $(SDK_FLAGS) $(FRAMEWORK_FLAGS) $(HEADER_FLAGS) -include "wtf/Platform.h" /dev/null | grep ENABLE_ORIENTATION_EVENTS | cut -d' ' -f3), 1)
    ENABLE_ORIENTATION_EVENTS = 1
endif

ifeq ($(PLATFORM_FEATURE_DEFINES),)
PLATFORM_FEATURE_DEFINES = Configurations/FeatureDefines.xcconfig
endif

endif # MACOS

ifndef ENABLE_ORIENTATION_EVENTS
    ENABLE_ORIENTATION_EVENTS = 0
endif

ifndef ENABLE_DRAGGABLE_REGION
    ENABLE_DRAGGABLE_REGION = 0
endif

ifeq ($(ENABLE_ORIENTATION_EVENTS), 1)
    ADDITIONAL_IDL_DEFINES := $(ADDITIONAL_IDL_DEFINES) ENABLE_ORIENTATION_EVENTS
endif

ifeq ($(ENABLE_DRAGGABLE_REGION), 1)
    ADDITIONAL_IDL_DEFINES := $(ADDITIONAL_IDL_DEFINES) ENABLE_DRAGGABLE_REGION
endif

# --------

# CSS property names and value keywords

WEBCORE_CSS_PROPERTY_NAMES := $(WebCore)/css/CSSPropertyNames.in
WEBCORE_CSS_VALUE_KEYWORDS := $(WebCore)/css/CSSValueKeywords.in

ifeq ($(findstring ENABLE_SVG,$(FEATURE_DEFINES)), ENABLE_SVG)
    WEBCORE_CSS_PROPERTY_NAMES := $(WEBCORE_CSS_PROPERTY_NAMES) $(WebCore)/css/SVGCSSPropertyNames.in
    WEBCORE_CSS_VALUE_KEYWORDS := $(WEBCORE_CSS_VALUE_KEYWORDS) $(WebCore)/css/SVGCSSValueKeywords.in
endif

CSSPropertyNames.h : $(WEBCORE_CSS_PROPERTY_NAMES) css/makeprop.pl $(PLATFORM_FEATURE_DEFINES)
	cat $(WEBCORE_CSS_PROPERTY_NAMES) > CSSPropertyNames.in
	perl -I$(WebCore)/bindings/scripts "$(WebCore)/css/makeprop.pl" --defines "$(FEATURE_DEFINES)"

CSSValueKeywords.h : $(WEBCORE_CSS_VALUE_KEYWORDS) css/makevalues.pl $(PLATFORM_FEATURE_DEFINES)
	cat $(WEBCORE_CSS_VALUE_KEYWORDS) > CSSValueKeywords.in
	perl -I$(WebCore)/bindings/scripts "$(WebCore)/css/makevalues.pl" --defines "$(FEATURE_DEFINES)"

# --------

# XMLViewer CSS

all : XMLViewerCSS.h

XMLViewerCSS.h : xml/XMLViewer.css
	perl $(WebCore)/inspector/xxd.pl XMLViewer_css $(WebCore)/xml/XMLViewer.css XMLViewerCSS.h

# --------

# XMLViewer JS

all : XMLViewerJS.h

XMLViewerJS.h : xml/XMLViewer.js
	perl $(WebCore)/inspector/xxd.pl XMLViewer_js $(WebCore)/xml/XMLViewer.js XMLViewerJS.h

# --------

# HTML entity names

HTMLEntityTable.cpp : html/parser/HTMLEntityNames.in $(WebCore)/html/parser/create-html-entity-table
	python $(WebCore)/html/parser/create-html-entity-table -o HTMLEntityTable.cpp $(WebCore)/html/parser/HTMLEntityNames.in

# --------

# color names

ColorData.cpp : platform/ColorData.gperf $(WebCore)/make-hash-tools.pl
	perl $(WebCore)/make-hash-tools.pl . $(WebCore)/platform/ColorData.gperf

# --------

# Path to bison

ifeq ($(OS),MACOS)
BISON=$(shell xcrun -find bison)
else
BISON=bison
endif

# --------

# CSS grammar
CSSGrammar.cpp : css/CSSGrammar.y.in $(PLATFORM_FEATURE_DEFINES)
	perl -I $(WebCore)/bindings/scripts $(WebCore)/css/makegrammar.pl --extraDefines "$(FEATURE_DEFINES)" --outputDir . --bison "$(BISON)" --symbolsPrefix cssyy $<

# --------

# XPath grammar
XPathGrammar.cpp : xml/XPathGrammar.y $(PROJECT_FILE)
	perl $(WebCore)/css/makegrammar.pl --outputDir . --bison "$(BISON)" --symbolsPrefix xpathyy $<

# --------

# user agent style sheets

USER_AGENT_STYLE_SHEETS = $(WebCore)/css/html.css $(WebCore)/css/quirks.css $(WebCore)/css/view-source.css $(WebCore)/css/themeWin.css $(WebCore)/css/themeWinQuirks.css $(WebCore)/css/plugIns.css

ifeq ($(findstring ENABLE_SVG,$(FEATURE_DEFINES)), ENABLE_SVG)
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/svg.css
endif

ifeq ($(findstring ENABLE_MATHML,$(FEATURE_DEFINES)), ENABLE_MATHML)
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/mathml.css
endif

ifeq ($(findstring ENABLE_VIDEO,$(FEATURE_DEFINES)), ENABLE_VIDEO)
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/mediaControls.css
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/mediaControlsQuickTime.css
endif

ifeq ($(findstring ENABLE_FULLSCREEN_API,$(FEATURE_DEFINES)), ENABLE_FULLSCREEN_API)
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/fullscreen.css $(WebCore)/css/fullscreenQuickTime.css
endif

UserAgentStyleSheets.h : css/make-css-file-arrays.pl bindings/scripts/preprocessor.pm $(USER_AGENT_STYLE_SHEETS) $(PLATFORM_FEATURE_DEFINES)
	perl -I$(WebCore)/bindings/scripts $< --defines "$(FEATURE_DEFINES)" $@ UserAgentStyleSheetsData.cpp $(USER_AGENT_STYLE_SHEETS)

# --------

# plugIns resources

PLUG_INS_RESOURCES = $(WebCore)/Resources/plugIns.js

PlugInsResources.h : css/make-css-file-arrays.pl bindings/scripts/preprocessor.pm $(PLUG_INS_RESOURCES) $(PLATFORM_FEATURE_DEFINES)
	perl -I$(WebCore)/bindings/scripts $< --defines "$(FEATURE_DEFINES)" $@ PlugInsResourcesData.cpp $(PLUG_INS_RESOURCES)

# --------

WebKitFontFamilyNames.cpp WebKitFontFamilyNames.h : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm css/WebKitFontFamilyNames.in
	perl -I $(WebCore)/bindings/scripts $< --fonts $(WebCore)/css/WebKitFontFamilyNames.in

# HTML tag and attribute names

ifeq ($(findstring ENABLE_DATALIST_ELEMENT,$(FEATURE_DEFINES)), ENABLE_DATALIST_ELEMENT)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_DATALIST_ELEMENT=1
endif

ifeq ($(findstring ENABLE_DETAILS_ELEMENT,$(FEATURE_DEFINES)), ENABLE_DETAILS_ELEMENT)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_DETAILS_ELEMENT=1
endif

ifeq ($(findstring ENABLE_ENCRYPTED_MEDIA_V2,$(FEATURE_DEFINES)), ENABLE_ENCRYPTED_MEDIA_V2)
	HTML_FLAGS := $(HTML_FLAGS) ENABLE_ENCRYPTED_MEDIA_V2=1
endif

ifeq ($(findstring ENABLE_METER_ELEMENT,$(FEATURE_DEFINES)), ENABLE_METER_ELEMENT)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_METER_ELEMENT=1
endif

ifeq ($(findstring ENABLE_MICRODATA,$(FEATURE_DEFINES)), ENABLE_MICRODATA)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_MICRODATA=1
endif

ifeq ($(findstring ENABLE_PROGRESS_ELEMENT,$(FEATURE_DEFINES)), ENABLE_PROGRESS_ELEMENT)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_PROGRESS_ELEMENT=1
endif

ifeq ($(findstring ENABLE_VIDEO,$(FEATURE_DEFINES)), ENABLE_VIDEO)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_VIDEO=1
endif

ifeq ($(findstring ENABLE_VIDEO_TRACK,$(FEATURE_DEFINES)), ENABLE_VIDEO_TRACK)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_VIDEO_TRACK=0
endif

ifeq ($(findstring ENABLE_SHADOW_DOM,$(FEATURE_DEFINES)), ENABLE_SHADOW_DOM)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_SHADOW_DOM=1
endif

ifeq ($(findstring ENABLE_ENCRYPTED_MEDIA,$(FEATURE_DEFINES)), ENABLE_ENCRYPTED_MEDIA)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_ENCRYPTED_MEDIA=1
endif

ifdef HTML_FLAGS

HTMLElementFactory.cpp HTMLNames.cpp : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm html/HTMLTagNames.in html/HTMLAttributeNames.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/html/HTMLTagNames.in --attrs $(WebCore)/html/HTMLAttributeNames.in --factory --wrapperFactory --extraDefines "$(HTML_FLAGS)"

else

HTMLElementFactory.cpp HTMLNames.cpp : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm html/HTMLTagNames.in html/HTMLAttributeNames.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/html/HTMLTagNames.in --attrs $(WebCore)/html/HTMLAttributeNames.in --factory --wrapperFactory

endif

JSHTMLElementWrapperFactory.cpp : HTMLNames.cpp

XMLNSNames.cpp : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm xml/xmlnsattrs.in
	perl -I $(WebCore)/bindings/scripts $< --attrs $(WebCore)/xml/xmlnsattrs.in

XMLNames.cpp : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm xml/xmlattrs.in
	perl -I $(WebCore)/bindings/scripts $< --attrs $(WebCore)/xml/xmlattrs.in

# --------

# SVG tag and attribute names, and element factory

ifeq ($(findstring ENABLE_SVG_FONTS,$(FEATURE_DEFINES)), ENABLE_SVG_FONTS)
    SVG_FLAGS := $(SVG_FLAGS) ENABLE_SVG_FONTS=1
endif

ifeq ($(findstring ENABLE_FILTERS,$(FEATURE_DEFINES)), ENABLE_FILTERS)
    SVG_FLAGS := $(SVG_FLAGS) ENABLE_FILTERS=1
endif

# SVG tag and attribute names (need to pass an extra flag if svg experimental features are enabled)

ifdef SVG_FLAGS

SVGElementFactory.cpp SVGNames.cpp : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm svg/svgtags.in svg/svgattrs.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/svg/svgtags.in --attrs $(WebCore)/svg/svgattrs.in --extraDefines "$(SVG_FLAGS)" --factory --wrapperFactory
else

SVGElementFactory.cpp SVGNames.cpp : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm svg/svgtags.in svg/svgattrs.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/svg/svgtags.in --attrs $(WebCore)/svg/svgattrs.in --factory --wrapperFactory

endif

JSSVGElementWrapperFactory.cpp : SVGNames.cpp

XLinkNames.cpp : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm svg/xlinkattrs.in
	perl -I $(WebCore)/bindings/scripts $< --attrs $(WebCore)/svg/xlinkattrs.in

# --------

# Register event constructors and targets

EventFactory.cpp EventHeaders.h EventInterfaces.h : dom/make_event_factory.pl dom/EventNames.in
	perl -I $(WebCore)/bindings/scripts $< --input $(WebCore)/dom/EventNames.in

EventTargetHeaders.h EventTargetInterfaces.h : dom/make_event_factory.pl dom/EventTargetFactory.in
	perl -I $(WebCore)/bindings/scripts $< --input $(WebCore)/dom/EventTargetFactory.in

ExceptionCodeDescription.cpp ExceptionCodeDescription.h ExceptionHeaders.h ExceptionInterfaces.h : dom/make_dom_exceptions.pl dom/DOMExceptions.in
	perl -I $(WebCore)/bindings/scripts $< --input $(WebCore)/dom/DOMExceptions.in

# --------

# MathML tag and attribute names, and element factory

MathMLElementFactory.cpp MathMLNames.cpp : dom/make_names.pl bindings/scripts/Hasher.pm bindings/scripts/StaticString.pm mathml/mathtags.in mathml/mathattrs.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/mathml/mathtags.in --attrs $(WebCore)/mathml/mathattrs.in --factory --wrapperFactory

# --------

InternalSettingsGenerated.idl InternalSettingsGenerated.cpp InternalSettingsGenerated.h SettingsMacros.h : page/make_settings.pl page/Settings.in
	perl -I $(WebCore)/bindings/scripts $< --input $(WebCore)/page/Settings.in

# --------

# Common generator things

GENERATE_SCRIPTS = \
    bindings/scripts/CodeGenerator.pm \
    bindings/scripts/IDLParser.pm \
    bindings/scripts/generate-bindings.pl \
    bindings/scripts/preprocessor.pm

PREPROCESS_IDLS_SCRIPTS = \
    bindings/scripts/preprocess-idls.pl

generator_script = perl $(addprefix -I $(WebCore)/, $(sort $(dir $(1)))) $(WebCore)/bindings/scripts/generate-bindings.pl

preprocess_idls_script = perl $(addprefix -I $(WebCore)/, $(sort $(dir $(1)))) $(WebCore)/bindings/scripts/preprocess-idls.pl

# JS bindings generator

IDL_INCLUDES = \
    $(WebCore)/Modules \
    $(WebCore)/css \
    $(WebCore)/dom \
    $(WebCore)/fileapi \
    $(WebCore)/html \
    $(WebCore)/html/canvas \
    $(WebCore)/html/shadow \
    $(WebCore)/html/track \
    $(WebCore)/inspector \
    $(WebCore)/loader/appcache \
    $(WebCore)/page \
    $(WebCore)/plugins \
    $(WebCore)/storage \
    $(WebCore)/svg \
    $(WebCore)/workers \
    $(WebCore)/xml

IDL_COMMON_ARGS = $(IDL_INCLUDES:%=--include %) --write-dependencies --outputDir .

JS_BINDINGS_SCRIPTS = $(GENERATE_SCRIPTS) bindings/scripts/CodeGeneratorJS.pm

SUPPLEMENTAL_DEPENDENCY_FILE = ./SupplementalDependencies.txt
SUPPLEMENTAL_MAKEFILE_DEPS = ./SupplementalDependencies.dep
WINDOW_CONSTRUCTORS_FILE = ./DOMWindowConstructors.idl
WORKERGLOBALSCOPE_CONSTRUCTORS_FILE = ./WorkerGlobalScopeConstructors.idl
SHAREDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE = ./SharedWorkerGlobalScopeConstructors.idl
DEDICATEDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE = ./DedicatedWorkerGlobalScopeConstructors.idl
IDL_FILES_TMP = ./idl_files.tmp
ADDITIONAL_IDLS = $(WebCore)/inspector/JavaScriptCallFrame.idl
IDL_ATTRIBUTES_FILE = $(WebCore)/bindings/scripts/IDLAttributes.txt

# The following two lines get a space character stored in a variable.
# See <http://blog.jgc.org/2007/06/escaping-comma-and-space-in-gnu-make.html>.
space :=
space +=

$(SUPPLEMENTAL_MAKEFILE_DEPS) : $(PREPROCESS_IDLS_SCRIPTS) $(BINDING_IDLS) $(ADDITIONAL_IDLS) $(PLATFORM_FEATURE_DEFINES)
	printf "$(subst $(space),,$(patsubst %,%\n,$(BINDING_IDLS) $(ADDITIONAL_IDLS)))" > $(IDL_FILES_TMP)
	$(call preprocess_idls_script, $(PREPROCESS_IDLS_SCRIPTS)) --defines "$(FEATURE_DEFINES) $(ADDITIONAL_IDL_DEFINES) LANGUAGE_JAVASCRIPT" --idlFilesList $(IDL_FILES_TMP) --supplementalDependencyFile $(SUPPLEMENTAL_DEPENDENCY_FILE) --windowConstructorsFile $(WINDOW_CONSTRUCTORS_FILE) --workerGlobalScopeConstructorsFile $(WORKERGLOBALSCOPE_CONSTRUCTORS_FILE) --sharedWorkerGlobalScopeConstructorsFile $(SHAREDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE) --dedicatedWorkerGlobalScopeConstructorsFile $(DEDICATEDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE) --supplementalMakefileDeps $@
	rm -f $(IDL_FILES_TMP)

JS%.h : %.idl $(JS_BINDINGS_SCRIPTS) $(IDL_ATTRIBUTES_FILE) $(WINDOW_CONSTRUCTORS_FILE) $(WORKERGLOBALSCOPE_CONSTRUCTORS_FILE) $(PLATFORM_FEATURE_DEFINES)
	$(call generator_script, $(JS_BINDINGS_SCRIPTS)) $(IDL_COMMON_ARGS) --defines "$(FEATURE_DEFINES) $(ADDITIONAL_IDL_DEFINES) LANGUAGE_JAVASCRIPT" --generator JS --idlAttributesFile $(IDL_ATTRIBUTES_FILE) --supplementalDependencyFile $(SUPPLEMENTAL_DEPENDENCY_FILE) $<

-include $(SUPPLEMENTAL_MAKEFILE_DEPS)

# Inspector interfaces generator

all : InspectorProtocolVersion.h

InspectorProtocolVersion.h : Inspector.json inspector/generate-inspector-protocol-version
	python $(WebCore)/inspector/generate-inspector-protocol-version -o InspectorProtocolVersion.h $(WebCore)/inspector/Inspector.json

all : InspectorFrontend.h

INSPECTOR_GENERATOR_SCRIPTS = inspector/CodeGeneratorInspector.py

InspectorFrontend.h : Inspector.json $(INSPECTOR_GENERATOR_SCRIPTS)
	python $(WebCore)/inspector/CodeGeneratorInspector.py $(WebCore)/inspector/Inspector.json --output_h_dir . --output_cpp_dir .

all : InspectorOverlayPage.h

InspectorOverlayPage.h : InspectorOverlayPage.html
	perl $(WebCore)/inspector/xxd.pl InspectorOverlayPage_html $(WebCore)/inspector/InspectorOverlayPage.html InspectorOverlayPage.h

all : InjectedScriptSource.h

InjectedScriptSource.h : InjectedScriptSource.js
	perl $(WebCore)/inspector/xxd.pl InjectedScriptSource_js $(WebCore)/inspector/InjectedScriptSource.js InjectedScriptSource.h

all : InjectedScriptCanvasModuleSource.h

InjectedScriptCanvasModuleSource.h : InjectedScriptCanvasModuleSource.js
	perl $(WebCore)/inspector/xxd.pl InjectedScriptCanvasModuleSource_js $(WebCore)/inspector/InjectedScriptCanvasModuleSource.js InjectedScriptCanvasModuleSource.h

-include $(JS_DOM_HEADERS:.h=.dep)

# ------------------------

# Mac-specific rules

ifeq ($(OS),MACOS)

OBJC_DOM_HEADERS=$(filter-out DOMDOMWindow.h DOMDOMMimeType.h DOMDOMPlugin.h,$(DOM_CLASSES:%=DOM%.h))

all : $(OBJC_DOM_HEADERS)

all : CharsetData.cpp

# --------

# character set name table

CharsetData.cpp : platform/text/mac/make-charset-table.pl platform/text/mac/character-sets.txt platform/text/mac/mac-encodings.txt
	perl $^ kTextEncoding > $@

# --------

ifneq ($(ACTION),installhdrs)

all : WebCore.exp WebCore.LP64.exp

WebCore.exp : $(BUILT_PRODUCTS_DIR)/WebCoreExportFileGenerator
	$^ > $@

# Switch NSRect, NSSize and NSPoint with their CG counterparts for the 64-bit exports file.
WebCore.LP64.exp : WebCore.exp
	cat $^ | sed -e s/7_NSRect/6CGRect/ -e s/7_NSSize/6CGSize/ -e s/8_NSPoint/7CGPoint/ > $@

endif # installhdrs

# --------

# Objective-C bindings

DOM_BINDINGS_SCRIPTS = $(GENERATE_BINDING_SCRIPTS) bindings/scripts/CodeGeneratorObjC.pm
DOM%.h : %.idl $(DOM_BINDINGS_SCRIPTS) bindings/objc/PublicDOMInterfaces.h $(PLATFORM_FEATURE_DEFINES)
	$(call generator_script, $(DOM_BINDINGS_SCRIPTS)) $(IDL_COMMON_ARGS) --defines "$(FEATURE_DEFINES) $(ADDITIONAL_IDL_DEFINES) LANGUAGE_OBJECTIVE_C" --generator ObjC --supplementalDependencyFile $(SUPPLEMENTAL_DEPENDENCY_FILE) $<

-include $(OBJC_DOM_HEADERS:.h=.dep)

# --------

endif # MACOS

# ------------------------

# Header detection

ifeq ($(OS),Windows_NT)

all : WebCoreHeaderDetection.h

WebCoreHeaderDetection.h : DerivedSources.make
	echo "/* Identifying AVFoundation Support */" > $@
	if [ -f "$(WEBKIT_LIBRARIES)/include/AVFoundationCF/AVCFBase.h" ]||[ -f "$(WEBKITLIBRARIESDIR)/include/AVFoundationCF/AVCFBase.h" ]; then echo "#define HAVE_AVCF 1" >> $@; else echo >> $@; fi
	if [ -f "$(WEBKIT_LIBRARIES)/include/AVFoundationCF/AVCFPlayerItemLegibleOutput.h" ]||[ -f "$(WEBKITLIBRARIESDIR)/include/AVFoundationCF/AVCFPlayerItemLegibleOutput.h" ]; then echo "#define HAVE_AVCF_LEGIBLE_OUTPUT 1" >> $@; else echo >> $@; fi

endif # Windows_NT
