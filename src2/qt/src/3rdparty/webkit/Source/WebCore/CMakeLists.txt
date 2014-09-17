SET(WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}"
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
    "${WEBCORE_DIR}/html/parser"
    "${WEBCORE_DIR}/html/shadow"
    "${WEBCORE_DIR}/inspector"
    "${WEBCORE_DIR}/loader"
    "${WEBCORE_DIR}/loader/appcache"
    "${WEBCORE_DIR}/loader/archive"
    "${WEBCORE_DIR}/loader/cache"
    "${WEBCORE_DIR}/loader/icon"
    "${WEBCORE_DIR}/mathml"
    "${WEBCORE_DIR}/notifications"
    "${WEBCORE_DIR}/page"
    "${WEBCORE_DIR}/page/animation"
    "${WEBCORE_DIR}/platform"
    "${WEBCORE_DIR}/platform/animation"
    "${WEBCORE_DIR}/platform/audio"
    "${WEBCORE_DIR}/platform/graphics"
    "${WEBCORE_DIR}/platform/graphics/filters"
    "${WEBCORE_DIR}/platform/graphics/filters/arm"
    "${WEBCORE_DIR}/platform/graphics/transforms"
    "${WEBCORE_DIR}/platform/image-decoders"
    "${WEBCORE_DIR}/platform/image-decoders/bmp"
    "${WEBCORE_DIR}/platform/image-decoders/gif"
    "${WEBCORE_DIR}/platform/image-decoders/ico"
    "${WEBCORE_DIR}/platform/image-decoders/jpeg"
    "${WEBCORE_DIR}/platform/image-decoders/png"
    "${WEBCORE_DIR}/platform/image-decoders/webp"
    "${WEBCORE_DIR}/platform/leveldb"
    "${WEBCORE_DIR}/platform/mock"
    "${WEBCORE_DIR}/platform/network"
    "${WEBCORE_DIR}/platform/sql"
    "${WEBCORE_DIR}/platform/text"
    "${WEBCORE_DIR}/platform/text/transcoder"
    "${WEBCORE_DIR}/plugins"
    "${WEBCORE_DIR}/rendering"
    "${WEBCORE_DIR}/rendering/mathml"
    "${WEBCORE_DIR}/rendering/style"
    "${WEBCORE_DIR}/rendering/svg"
    "${WEBCORE_DIR}/storage"
    "${WEBCORE_DIR}/svg"
    "${WEBCORE_DIR}/svg/animation"
    "${WEBCORE_DIR}/svg/graphics"
    "${WEBCORE_DIR}/svg/graphics/filters"
    "${WEBCORE_DIR}/svg/properties"
    "${WEBCORE_DIR}/webaudio"
    "${WEBCORE_DIR}/websockets"
    "${WEBCORE_DIR}/wml"
    "${WEBCORE_DIR}/workers"
    "${WEBCORE_DIR}/xml"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${JAVASCRIPTCORE_DIR}/API"
    "${JAVASCRIPTCORE_DIR}/assembler"
    "${JAVASCRIPTCORE_DIR}/bytecode"
    "${JAVASCRIPTCORE_DIR}/bytecompiler"
    "${JAVASCRIPTCORE_DIR}/heap"
    "${JAVASCRIPTCORE_DIR}/debugger"
    "${JAVASCRIPTCORE_DIR}/interpreter"
    "${JAVASCRIPTCORE_DIR}/jit"
    "${JAVASCRIPTCORE_DIR}/parser"
    "${JAVASCRIPTCORE_DIR}/profiler"
    "${JAVASCRIPTCORE_DIR}/runtime"
    "${JAVASCRIPTCORE_DIR}/wtf"
    "${JAVASCRIPTCORE_DIR}/wtf/unicode"
    "${JAVASCRIPTCORE_DIR}/yarr"
    "${DERIVED_SOURCES_WEBCORE_DIR}"
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}"
)

SET(WebCore_IDL_INCLUDES
    bindings/generic
    css
    dom
    fileapi
    html
    html/canvas
    inspector
    loader/appcache
    page
    plugins
    storage
    svg
    webaudio
    workers
    xml
)

SET(WebCore_IDL_FILES
    css/Counter.idl
    css/CSSCharsetRule.idl
    css/CSSFontFaceRule.idl
    css/CSSImportRule.idl
    css/CSSMediaRule.idl
    css/CSSPageRule.idl
    css/CSSPrimitiveValue.idl
    css/CSSRule.idl
    css/CSSRuleList.idl
    css/CSSStyleDeclaration.idl
    css/CSSStyleRule.idl
    css/CSSStyleSheet.idl
    css/CSSValue.idl
    css/CSSValueList.idl
    css/MediaList.idl
    css/MediaQueryList.idl
    css/Rect.idl
    css/RGBColor.idl
    css/StyleMedia.idl
    css/StyleSheet.idl
    css/StyleSheetList.idl
    css/WebKitCSSKeyframeRule.idl
    css/WebKitCSSKeyframesRule.idl
    css/WebKitCSSMatrix.idl
    css/WebKitCSSTransformValue.idl

    dom/Attr.idl
    dom/BeforeLoadEvent.idl
    dom/CDATASection.idl
    dom/CharacterData.idl
    dom/ClientRect.idl
    dom/ClientRectList.idl
    dom/Clipboard.idl
    dom/Comment.idl
    dom/CompositionEvent.idl
    dom/CustomEvent.idl
    dom/DataTransferItem.idl
    dom/DataTransferItems.idl
    dom/DeviceMotionEvent.idl
    dom/DeviceOrientationEvent.idl
    dom/DocumentFragment.idl
    dom/Document.idl
    dom/DocumentType.idl
    dom/DOMCoreException.idl
    dom/DOMImplementation.idl
    dom/DOMStringList.idl
    dom/DOMStringMap.idl
    dom/Element.idl
    dom/Entity.idl
    dom/EntityReference.idl
    dom/ErrorEvent.idl
    dom/EventException.idl
    dom/Event.idl
    dom/HashChangeEvent.idl
    dom/KeyboardEvent.idl
    dom/MessageChannel.idl
    dom/MessageEvent.idl
    dom/MessagePort.idl
    dom/MouseEvent.idl
    dom/MutationEvent.idl
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
    dom/RangeException.idl
    dom/Range.idl
    dom/StringCallback.idl
    dom/TextEvent.idl
    dom/Text.idl
    dom/TouchEvent.idl
    dom/Touch.idl
    dom/TouchList.idl
    dom/TreeWalker.idl
    dom/UIEvent.idl
    dom/WebKitAnimationEvent.idl
    dom/WebKitTransitionEvent.idl
    dom/WheelEvent.idl

    fileapi/Blob.idl
    fileapi/FileError.idl
    fileapi/File.idl
    fileapi/FileList.idl
    fileapi/FileReader.idl
    fileapi/WebKitBlobBuilder.idl

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
    html/HTMLBlockquoteElement.idl
    html/HTMLBodyElement.idl
    html/HTMLBRElement.idl
    html/HTMLButtonElement.idl
    html/HTMLCanvasElement.idl
    html/HTMLCollection.idl
    html/HTMLDataListElement.idl
    html/HTMLDetailsElement.idl
    html/HTMLDirectoryElement.idl
    html/HTMLDivElement.idl
    html/HTMLDListElement.idl
    html/HTMLDocument.idl
    html/HTMLElement.idl
    html/HTMLEmbedElement.idl
    html/HTMLFieldSetElement.idl
    html/HTMLFontElement.idl
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
    html/HTMLIsIndexElement.idl
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
    html/HTMLQuoteElement.idl
    html/HTMLScriptElement.idl
    html/HTMLSelectElement.idl
    html/HTMLSourceElement.idl
    html/HTMLStyleElement.idl
    html/HTMLTableCaptionElement.idl
    html/HTMLTableCellElement.idl
    html/HTMLTableColElement.idl
    html/HTMLTableElement.idl
    html/HTMLTableRowElement.idl
    html/HTMLTableSectionElement.idl
    html/HTMLTextAreaElement.idl
    html/HTMLTitleElement.idl
    html/HTMLTrackElement.idl
    html/HTMLUListElement.idl
    html/HTMLVideoElement.idl
    html/ImageData.idl
    html/MediaError.idl
    html/TextMetrics.idl
    html/TimeRanges.idl
    html/ValidityState.idl
    html/VoidCallback.idl

    html/canvas/ArrayBuffer.idl
    html/canvas/ArrayBufferView.idl
    html/canvas/CanvasGradient.idl
    html/canvas/CanvasPattern.idl
    html/canvas/CanvasRenderingContext2D.idl
    html/canvas/CanvasRenderingContext.idl
    html/canvas/DataView.idl
    html/canvas/Float32Array.idl
    html/canvas/Int16Array.idl
    html/canvas/Int32Array.idl
    html/canvas/Int8Array.idl
    html/canvas/OESStandardDerivatives.idl
    html/canvas/OESTextureFloat.idl
    html/canvas/OESVertexArrayObject.idl
    html/canvas/Uint16Array.idl
    html/canvas/Uint32Array.idl
    html/canvas/Uint8Array.idl
    html/canvas/WebGLActiveInfo.idl
    html/canvas/WebGLBuffer.idl
    html/canvas/WebGLContextAttributes.idl
    html/canvas/WebGLFramebuffer.idl
    html/canvas/WebGLProgram.idl
    html/canvas/WebGLRenderbuffer.idl
    html/canvas/WebGLRenderingContext.idl
    html/canvas/WebGLShader.idl
    html/canvas/WebGLTexture.idl
    html/canvas/WebGLUniformLocation.idl
    html/canvas/WebGLVertexArrayObjectOES.idl
    html/canvas/WebKitLoseContext.idl

    inspector/InjectedScriptHost.idl
    inspector/InspectorFrontendHost.idl
    inspector/JavaScriptCallFrame.idl
    inspector/ScriptProfile.idl
    inspector/ScriptProfileNode.idl

    loader/appcache/DOMApplicationCache.idl

    page/BarInfo.idl
    page/Console.idl
    page/Coordinates.idl
    page/Crypto.idl
    page/DOMSelection.idl
    page/DOMWindow.idl
    page/EventSource.idl
    page/Geolocation.idl
    page/Geoposition.idl
    page/History.idl
    page/Location.idl
    page/MemoryInfo.idl
    page/Navigator.idl
    page/NavigatorUserMediaErrorCallback.idl
    page/NavigatorUserMediaSuccessCallback.idl
    page/Performance.idl
    page/PerformanceNavigation.idl
    page/PerformanceTiming.idl
    page/PositionError.idl
    page/Screen.idl
    page/SpeechInputEvent.idl
    page/SpeechInputResult.idl
    page/SpeechInputResultList.idl
    page/WebKitAnimation.idl
    page/WebKitAnimationList.idl
    page/WebKitPoint.idl
    page/WorkerNavigator.idl

    plugins/DOMMimeType.idl
    plugins/DOMMimeTypeArray.idl
    plugins/DOMPlugin.idl
    plugins/DOMPluginArray.idl

    storage/Database.idl
    storage/DatabaseCallback.idl
    storage/DatabaseSync.idl
    storage/IDBAny.idl
    storage/IDBCursor.idl
    storage/IDBDatabaseError.idl
    storage/IDBDatabaseException.idl
    storage/IDBDatabase.idl
    storage/IDBFactory.idl
    storage/IDBIndex.idl
    storage/IDBKey.idl
    storage/IDBKeyRange.idl
    storage/IDBObjectStore.idl
    storage/IDBRequest.idl
    storage/IDBTransaction.idl
    storage/SQLError.idl
    storage/SQLException.idl
    storage/SQLResultSet.idl
    storage/SQLResultSetRowList.idl
    storage/SQLStatementCallback.idl
    storage/SQLStatementErrorCallback.idl
    storage/SQLTransaction.idl
    storage/SQLTransactionCallback.idl
    storage/SQLTransactionErrorCallback.idl
    storage/SQLTransactionSync.idl
    storage/SQLTransactionSyncCallback.idl
    storage/Storage.idl
    storage/StorageEvent.idl
    storage/StorageInfo.idl
    storage/StorageInfoErrorCallback.idl
    storage/StorageInfoUsageCallback.idl

    webaudio/AudioBuffer.idl
    webaudio/AudioBufferSourceNode.idl
    webaudio/AudioChannelSplitter.idl
    webaudio/AudioChannelMerger.idl
    webaudio/AudioContext.idl
    webaudio/AudioDestinationNode.idl
    webaudio/AudioGain.idl
    webaudio/AudioGainNode.idl
    webaudio/AudioListener.idl
    webaudio/AudioNode.idl
    webaudio/AudioPannerNode.idl
    webaudio/AudioParam.idl
    webaudio/AudioProcessingEvent.idl
    webaudio/AudioSourceNode.idl
    webaudio/ConvolverNode.idl
    webaudio/DelayNode.idl
    webaudio/HighPass2FilterNode.idl
    webaudio/JavaScriptAudioNode.idl
    webaudio/LowPass2FilterNode.idl
    webaudio/RealtimeAnalyserNode.idl

    websockets/WebSocket.idl

    workers/AbstractWorker.idl
    workers/DedicatedWorkerContext.idl
    workers/SharedWorker.idl
    workers/SharedWorkerContext.idl
    workers/Worker.idl
    workers/WorkerContext.idl
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

SET(WebCore_SOURCES
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
    accessibility/AccessibilityObject.cpp
    accessibility/AccessibilityRenderObject.cpp
    accessibility/AccessibilityScrollbar.cpp
    accessibility/AccessibilityScrollView.cpp
    accessibility/AccessibilitySlider.cpp
    accessibility/AccessibilityTable.cpp
    accessibility/AccessibilityTableCell.cpp
    accessibility/AccessibilityTableColumn.cpp
    accessibility/AccessibilityTableHeaderContainer.cpp
    accessibility/AccessibilityTableRow.cpp

    bindings/ScriptControllerBase.cpp

    bindings/generic/ActiveDOMCallback.cpp
    bindings/generic/RuntimeEnabledFeatures.cpp

    bindings/js/CallbackFunction.cpp

    css/CSSBorderImageValue.cpp
    css/CSSCanvasValue.cpp
    css/CSSCharsetRule.cpp
    css/CSSComputedStyleDeclaration.cpp
    css/CSSCursorImageValue.cpp
    css/CSSFontFace.cpp
    css/CSSFontFaceRule.cpp
    css/CSSFontFaceSource.cpp
    css/CSSFontFaceSrcValue.cpp
    css/CSSFontSelector.cpp
    css/CSSFunctionValue.cpp
    css/CSSGradientValue.cpp
    css/CSSImageGeneratorValue.cpp
    css/CSSImageValue.cpp
    css/CSSImportRule.cpp
    css/CSSInheritedValue.cpp
    css/CSSInitialValue.cpp
    css/CSSLineBoxContainValue.cpp
    css/CSSMediaRule.cpp
    css/CSSMutableStyleDeclaration.cpp
    css/CSSOMUtils.cpp
    css/CSSPageRule.cpp
    css/CSSParser.cpp
    css/CSSParserValues.cpp
    css/CSSPrimitiveValue.cpp
    css/CSSPrimitiveValueCache.cpp
    css/CSSProperty.cpp
    css/CSSPropertyLonghand.cpp
    css/CSSPropertySourceData.cpp
    css/CSSReflectValue.cpp
    css/CSSRule.cpp
    css/CSSRuleList.cpp
    css/CSSSegmentedFontFace.cpp
    css/CSSSelector.cpp
    css/CSSSelectorList.cpp
    css/CSSStyleApplyProperty.cpp
    css/CSSStyleDeclaration.cpp
    css/CSSStyleRule.cpp
    css/CSSStyleSelector.cpp
    css/CSSStyleSheet.cpp
    css/CSSTimingFunctionValue.cpp
    css/CSSUnicodeRangeValue.cpp
    css/CSSValueList.cpp
    css/FontFamilyValue.cpp
    css/FontValue.cpp
    css/MediaFeatureNames.cpp
    css/MediaList.cpp
    css/MediaQuery.cpp
    css/MediaQueryEvaluator.cpp
    css/MediaQueryExp.cpp
    css/MediaQueryList.cpp
    css/MediaQueryListListener.cpp
    css/MediaQueryMatcher.cpp
    css/RGBColor.cpp
    css/ShadowValue.cpp
    css/StyleBase.cpp
    css/StyleList.cpp
    css/StyleMedia.cpp
    css/StyleSheet.cpp
    css/StyleSheetList.cpp
    css/WebKitCSSKeyframeRule.cpp
    css/WebKitCSSKeyframesRule.cpp
    css/WebKitCSSMatrix.cpp
    css/WebKitCSSTransformValue.cpp

    dom/ActiveDOMObject.cpp
    dom/Attr.cpp
    dom/Attribute.cpp
    dom/BeforeTextInsertedEvent.cpp
    dom/BeforeUnloadEvent.cpp
    dom/CDATASection.cpp
    dom/CharacterData.cpp
    dom/CheckedRadioButtons.cpp
    dom/ChildNodeList.cpp
    dom/ClassNodeList.cpp
    dom/ClientRect.cpp
    dom/ClientRectList.cpp
    dom/Clipboard.cpp
    dom/ClipboardEvent.cpp
    dom/Comment.cpp
    dom/CompositionEvent.cpp
    dom/ContainerNode.cpp
    dom/CSSMappedAttributeDeclaration.cpp
    dom/CustomEvent.cpp
    dom/DatasetDOMStringMap.cpp
    dom/DecodedDataDocumentParser.cpp
    dom/DeviceMotionController.cpp
    dom/DeviceMotionData.cpp
    dom/DeviceMotionEvent.cpp
    dom/DeviceOrientation.cpp
    dom/DeviceOrientationController.cpp
    dom/DeviceOrientationEvent.cpp
    dom/Document.cpp
    dom/DocumentMarkerController.cpp
    dom/DocumentFragment.cpp
    dom/DocumentParser.cpp
    dom/DocumentOrderedMap.cpp
    dom/DocumentType.cpp
    dom/DOMImplementation.cpp
    dom/DOMStringList.cpp
    dom/DOMStringMap.cpp
    dom/DynamicNodeList.cpp
    dom/EditingText.cpp
    dom/Element.cpp
    dom/EntityReference.cpp
    dom/ErrorEvent.cpp
    dom/Event.cpp
    dom/EventContext.cpp
    dom/EventDispatcher.cpp
    dom/EventNames.cpp
    dom/EventTarget.cpp
    dom/EventQueue.cpp
    dom/ExceptionBase.cpp
    dom/ExceptionCode.cpp
    dom/IconURL.cpp
    dom/InputElement.cpp
    dom/KeyboardEvent.cpp
    dom/MessageChannel.cpp
    dom/MessageEvent.cpp
    dom/MessagePortChannel.cpp
    dom/MessagePort.cpp
    dom/MouseEvent.cpp
    dom/MouseRelatedEvent.cpp
    dom/MutationEvent.cpp
    dom/NamedNodeMap.cpp
    dom/NameNodeList.cpp
    dom/Node.cpp
    dom/NodeFilterCondition.cpp
    dom/NodeFilter.cpp
    dom/NodeIterator.cpp
    dom/Notation.cpp
    dom/OptionElement.cpp
    dom/OptionGroupElement.cpp
    dom/OverflowEvent.cpp
    dom/PageTransitionEvent.cpp
    dom/PendingScript.cpp
    dom/PopStateEvent.cpp
    dom/Position.cpp
    dom/PositionIterator.cpp
    dom/ProcessingInstruction.cpp
    dom/ProgressEvent.cpp
    dom/QualifiedName.cpp
    dom/Range.cpp
    dom/RegisteredEventListener.cpp
    dom/ScopedEventQueue.cpp
    dom/ScriptableDocumentParser.cpp
    dom/ScriptElement.cpp
    dom/ScriptExecutionContext.cpp
    dom/ScriptRunner.cpp
    dom/SelectElement.cpp
    dom/SelectorNodeList.cpp
    dom/ShadowRoot.cpp
    dom/SpaceSplitString.cpp
    dom/StaticHashSetNodeList.cpp
    dom/StaticNodeList.cpp
    dom/StyledElement.cpp
    dom/StyleElement.cpp
    dom/TagNodeList.cpp
    dom/Text.cpp
    dom/TextEvent.cpp
    dom/Touch.cpp
    dom/TouchEvent.cpp
    dom/TouchList.cpp
    dom/TransformSourceLibxslt.cpp
    dom/Traversal.cpp
    dom/TreeScope.cpp
    dom/TreeWalker.cpp
    dom/UIEvent.cpp
    dom/UIEventWithKeyState.cpp
    dom/UserGestureIndicator.cpp
    dom/UserTypingGestureIndicator.cpp
    dom/ViewportArguments.cpp
    dom/WebKitAnimationEvent.cpp
    dom/WebKitTransitionEvent.cpp
    dom/WheelEvent.cpp
    dom/WindowEventContext.cpp
    dom/XMLDocumentParser.cpp
    dom/XMLDocumentParserLibxml2.cpp
    dom/XMLDocumentParserScope.cpp

    dom/default/PlatformMessagePortChannel.cpp

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
    editing/EditCommand.cpp
    editing/EditingStyle.cpp
    editing/Editor.cpp
    editing/EditorCommand.cpp
    editing/FormatBlockCommand.cpp
    editing/HTMLInterchange.cpp
    editing/IndentOutdentCommand.cpp
    editing/InsertIntoTextNodeCommand.cpp
    editing/InsertLineBreakCommand.cpp
    editing/InsertListCommand.cpp
    editing/InsertNodeBeforeCommand.cpp
    editing/InsertParagraphSeparatorCommand.cpp
    editing/InsertTextCommand.cpp
    editing/JoinTextNodesCommand.cpp
    editing/MarkupAccumulator.cpp
    editing/MergeIdenticalElementsCommand.cpp
    editing/ModifySelectionListLevel.cpp
    editing/MoveSelectionCommand.cpp
    editing/RemoveCSSPropertyCommand.cpp
    editing/RemoveFormatCommand.cpp
    editing/RemoveNodeCommand.cpp
    editing/RemoveNodePreservingChildrenCommand.cpp
    editing/ReplaceNodeWithSpanCommand.cpp
    editing/ReplaceSelectionCommand.cpp
    editing/SelectionController.cpp
    editing/SetNodeAttributeCommand.cpp
    editing/SmartReplace.cpp
    editing/SpellChecker.cpp
    editing/SpellingCorrectionController.cpp
    editing/SplitElementCommand.cpp
    editing/SplitTextNodeCommand.cpp
    editing/SplitTextNodeContainingElementCommand.cpp
    editing/TextCheckingHelper.cpp
    editing/TextIterator.cpp
    editing/TypingCommand.cpp
    editing/UnlinkCommand.cpp
    editing/VisiblePosition.cpp
    editing/VisibleSelection.cpp
    editing/WrapContentsInDummySpanCommand.cpp
    editing/htmlediting.cpp
    editing/markup.cpp
    editing/visible_units.cpp

    fileapi/Blob.cpp
    fileapi/BlobURL.cpp
    fileapi/File.cpp
    fileapi/FileList.cpp
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
    html/BaseDateAndTimeInputType.cpp
    html/BaseTextInputType.cpp
    html/ButtonInputType.cpp
    html/CheckboxInputType.cpp
    html/ClassList.cpp
    html/CollectionCache.cpp
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
    html/FormDataList.cpp
    html/HTMLAllCollection.cpp
    html/HTMLAnchorElement.cpp
    html/HTMLAppletElement.cpp
    html/HTMLAreaElement.cpp
    html/HTMLBRElement.cpp
    html/HTMLBaseElement.cpp
    html/HTMLBaseFontElement.cpp
    html/HTMLBlockquoteElement.cpp
    html/HTMLBodyElement.cpp
    html/HTMLButtonElement.cpp
    html/HTMLCanvasElement.cpp
    html/HTMLCollection.cpp
    html/HTMLDListElement.cpp
    html/HTMLDataListElement.cpp
    html/HTMLDetailsElement.cpp
    html/HTMLDirectoryElement.cpp
    html/HTMLDivElement.cpp
    html/HTMLDocument.cpp
    html/HTMLElement.cpp
    html/HTMLEmbedElement.cpp
    html/HTMLFieldSetElement.cpp
    html/HTMLFontElement.cpp
    html/HTMLFormCollection.cpp
    html/HTMLFormControlElement.cpp
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
    html/HTMLIsIndexElement.cpp
    html/HTMLKeygenElement.cpp
    html/HTMLLIElement.cpp
    html/HTMLLabelElement.cpp
    html/HTMLLegendElement.cpp
    html/HTMLLinkElement.cpp
    html/HTMLMapElement.cpp
    html/HTMLMarqueeElement.cpp
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
    html/HTMLQuoteElement.cpp
    html/HTMLScriptElement.cpp
    html/HTMLSelectElement.cpp
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
    html/HTMLTextAreaElement.cpp
    html/HTMLTitleElement.cpp
    html/HTMLUListElement.cpp
    html/HTMLViewSourceDocument.cpp
    html/HiddenInputType.cpp
    html/ImageData.cpp
    html/ImageDocument.cpp
    html/ImageInputType.cpp
    html/InputType.cpp
    html/IsIndexInputType.cpp
    html/LabelsNodeList.cpp
    html/MediaDocument.cpp
    html/MonthInputType.cpp
    html/NumberInputType.cpp
    html/PasswordInputType.cpp
    html/PluginDocument.cpp
    html/RadioInputType.cpp
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
    html/URLInputType.cpp
    html/ValidationMessage.cpp
    html/ValidityState.cpp
    html/WeekInputType.cpp

    html/canvas/ArrayBuffer.cpp
    html/canvas/ArrayBufferView.cpp
    html/canvas/CanvasGradient.cpp
    html/canvas/CanvasPattern.cpp
    html/canvas/CanvasPixelArray.cpp
    html/canvas/CanvasRenderingContext.cpp
    html/canvas/CanvasRenderingContext2D.cpp
    html/canvas/CanvasStyle.cpp
    html/canvas/DataView.cpp
    html/canvas/Float32Array.cpp
    html/canvas/Int16Array.cpp
    html/canvas/Int32Array.cpp
    html/canvas/Int8Array.cpp
    html/canvas/Uint16Array.cpp
    html/canvas/Uint32Array.cpp
    html/canvas/Uint8Array.cpp

    html/parser/CSSPreloadScanner.cpp
    html/parser/HTMLConstructionSite.cpp
    html/parser/HTMLDocumentParser.cpp
    html/parser/HTMLElementStack.cpp
    html/parser/HTMLEntityParser.cpp
    html/parser/HTMLEntitySearch.cpp
    html/parser/HTMLParserIdioms.cpp
    html/parser/HTMLParserScheduler.cpp
    html/parser/HTMLFormattingElementList.cpp
    html/parser/HTMLMetaCharsetParser.cpp
    html/parser/HTMLPreloadScanner.cpp
    html/parser/HTMLScriptRunner.cpp
    html/parser/HTMLSourceTracker.cpp
    html/parser/HTMLTokenizer.cpp
    html/parser/HTMLTreeBuilder.cpp
    html/parser/HTMLViewSourceParser.cpp
    html/parser/TextDocumentParser.cpp
    html/parser/TextViewSourceParser.cpp
    html/parser/XSSFilter.cpp

    html/shadow/DetailsMarkerControl.cpp
    html/shadow/MediaControls.cpp
    html/shadow/MediaControlRootElement.cpp
    html/shadow/MeterShadowElement.cpp
    html/shadow/ProgressShadowElement.cpp
    html/shadow/SliderThumbElement.cpp
    html/shadow/TextControlInnerElements.cpp

    inspector/ConsoleMessage.cpp
    inspector/DOMNodeHighlighter.cpp
    inspector/EventsCollector.cpp
    inspector/InjectedScript.cpp
    inspector/InjectedScriptHost.cpp
    inspector/InjectedScriptManager.cpp
    inspector/InspectorAgent.cpp
    inspector/InspectorApplicationCacheAgent.cpp
    inspector/InspectorPageAgent.cpp
    inspector/InspectorCSSAgent.cpp
    inspector/InspectorClient.cpp
    inspector/InspectorConsoleAgent.cpp
    inspector/InspectorController.cpp
    inspector/InspectorDOMAgent.cpp
    inspector/InspectorDOMDebuggerAgent.cpp
    inspector/InspectorDOMStorageAgent.cpp
    inspector/InspectorDOMStorageResource.cpp
    inspector/InspectorDatabaseAgent.cpp
    inspector/InspectorDatabaseResource.cpp
    inspector/InspectorDebuggerAgent.cpp
    inspector/InspectorFrontendClientLocal.cpp
    inspector/InspectorFrontendHost.cpp
    inspector/InspectorFrontendProxy.cpp
    inspector/InspectorInstrumentation.cpp
    inspector/InspectorProfilerAgent.cpp
    inspector/InspectorResourceAgent.cpp
    inspector/InspectorRuntimeAgent.cpp
    inspector/InspectorState.cpp
    inspector/InspectorStyleSheet.cpp
    inspector/InspectorTimelineAgent.cpp
    inspector/InspectorValues.cpp
    inspector/InspectorWorkerAgent.cpp
    inspector/PageDebuggerAgent.cpp
    inspector/ScriptArguments.cpp
    inspector/ScriptCallFrame.cpp
    inspector/ScriptCallStack.cpp
    inspector/TimelineRecordFactory.cpp
    inspector/WorkerDebuggerAgent.cpp
    inspector/WorkerInspectorController.cpp

    loader/CrossOriginAccessControl.cpp
    loader/CrossOriginPreflightResultCache.cpp
    loader/DocumentLoader.cpp
    loader/DocumentThreadableLoader.cpp
    loader/DocumentWriter.cpp
    loader/FTPDirectoryParser.cpp
    loader/FormState.cpp
    loader/FormSubmission.cpp
    loader/FrameLoader.cpp
    loader/FrameLoaderStateMachine.cpp
    loader/HistoryController.cpp
    loader/ImageLoader.cpp
    loader/MainResourceLoader.cpp
    loader/NavigationAction.cpp
    loader/NetscapePlugInStreamLoader.cpp
    loader/PingLoader.cpp
    loader/PlaceholderDocument.cpp
    loader/PolicyCallback.cpp
    loader/PolicyChecker.cpp
    loader/ProgressTracker.cpp
    loader/NavigationScheduler.cpp
    loader/ResourceLoadNotifier.cpp
    loader/ResourceLoadScheduler.cpp
    loader/ResourceLoader.cpp
    loader/SinkDocument.cpp
    loader/SubframeLoader.cpp
    loader/SubresourceLoader.cpp
    loader/TextResourceDecoder.cpp
    loader/ThreadableLoader.cpp
    loader/WorkerThreadableLoader.cpp

    loader/appcache/ApplicationCache.cpp
    loader/appcache/ApplicationCacheGroup.cpp
    loader/appcache/ApplicationCacheHost.cpp
    loader/appcache/ApplicationCacheResource.cpp
    loader/appcache/ApplicationCacheStorage.cpp
    loader/appcache/DOMApplicationCache.cpp
    loader/appcache/ManifestParser.cpp

    loader/archive/ArchiveFactory.cpp
    loader/archive/ArchiveResource.cpp
    loader/archive/ArchiveResourceCollection.cpp

    loader/cache/CachedCSSStyleSheet.cpp
    loader/cache/CachedFont.cpp
    loader/cache/CachedImage.cpp
    loader/cache/CachedResource.cpp
    loader/cache/CachedResourceClientWalker.cpp
    loader/cache/CachedResourceHandle.cpp
    loader/cache/CachedResourceLoader.cpp
    loader/cache/CachedResourceRequest.cpp
    loader/cache/CachedScript.cpp
    loader/cache/CachedXSLStyleSheet.cpp
    loader/cache/MemoryCache.cpp

    loader/icon/IconDatabase.cpp
    loader/icon/IconDatabaseBase.cpp
    loader/icon/IconLoader.cpp
    loader/icon/IconRecord.cpp
    loader/icon/PageURLRecord.cpp

    page/BarInfo.cpp
    page/Chrome.cpp
    page/Console.cpp
    page/ContentSecurityPolicy.cpp
    page/ContextMenuController.cpp
    page/Crypto.cpp
    page/DOMSelection.cpp
    page/DOMTimer.cpp
    page/DOMWindow.cpp
    page/DragController.cpp
    page/EventHandler.cpp
    page/EventSource.cpp
    page/FocusController.cpp
    page/Frame.cpp
    page/FrameActionScheduler.cpp
    page/FrameTree.cpp
    page/FrameView.cpp
    page/Geolocation.cpp
    page/GeolocationController.cpp
    page/GeolocationPositionCache.cpp
    page/GroupSettings.cpp
    page/History.cpp
    page/Location.cpp
    page/MemoryInfo.cpp
    page/MouseEventWithHitTestResults.cpp
    page/Navigator.cpp
    page/NavigatorBase.cpp
    page/OriginAccessEntry.cpp
    page/Page.cpp
    page/PageGroup.cpp
    page/PageGroupLoadDeferrer.cpp
    page/PageSerializer.cpp
    page/Performance.cpp
    page/PerformanceNavigation.cpp
    page/PerformanceTiming.cpp
    page/PluginHalter.cpp
    page/PrintContext.cpp
    page/Screen.cpp
    page/SecurityOrigin.cpp
    page/Settings.cpp
    page/SpatialNavigation.cpp
    page/SuspendableTimer.cpp
    page/UserContentURLPattern.cpp
    page/WebKitAnimation.cpp
    page/WebKitAnimationList.cpp
    page/WindowFeatures.cpp
    page/WorkerNavigator.cpp

    page/animation/AnimationBase.cpp
    page/animation/AnimationController.cpp
    page/animation/CompositeAnimation.cpp
    page/animation/ImplicitAnimation.cpp
    page/animation/KeyframeAnimation.cpp

    platform/Arena.cpp
    platform/ContextMenu.cpp
    platform/ContextMenuItem.cpp
    platform/ContentType.cpp
    platform/CrossThreadCopier.cpp
    platform/DateComponents.cpp
    platform/DefaultLocalizationStrategy.cpp
    platform/DragData.cpp
    platform/DragImage.cpp
    platform/FileChooser.cpp
    platform/FileSystem.cpp
    platform/GeolocationService.cpp
    platform/KURL.cpp
    platform/KillRingNone.cpp
    platform/Language.cpp
    platform/Length.cpp
    platform/LinkHash.cpp
    platform/Logging.cpp
    platform/MIMETypeRegistry.cpp
    platform/RuntimeApplicationChecks.cpp
    platform/ScrollAnimator.cpp
    platform/ScrollableArea.cpp
    platform/Scrollbar.cpp
    platform/ScrollbarThemeComposite.cpp
    platform/ScrollView.cpp
    platform/SharedBuffer.cpp
    platform/SharedBufferCRLFLineReader.cpp
    platform/SchemeRegistry.cpp
    platform/ThreadGlobalData.cpp
    platform/ThreadTimers.cpp
    platform/Timer.cpp
    platform/UUID.cpp
    platform/Widget.cpp

    platform/animation/Animation.cpp
    platform/animation/AnimationList.cpp

    platform/graphics/BitmapImage.cpp
    platform/graphics/Color.cpp
    platform/graphics/ContextShadow.cpp
    platform/graphics/FloatPoint.cpp
    platform/graphics/FloatPoint3D.cpp
    platform/graphics/FloatQuad.cpp
    platform/graphics/FloatRect.cpp
    platform/graphics/FloatSize.cpp
    platform/graphics/Font.cpp
    platform/graphics/FontCache.cpp
    platform/graphics/FontData.cpp
    platform/graphics/FontDescription.cpp
    platform/graphics/FontFallbackList.cpp
    platform/graphics/FontFamily.cpp
    platform/graphics/FontFastPath.cpp
    platform/graphics/GeneratedImage.cpp
    platform/graphics/GlyphPageTreeNode.cpp
    platform/graphics/Gradient.cpp
    platform/graphics/GraphicsContext.cpp
    platform/graphics/GraphicsTypes.cpp
    platform/graphics/Image.cpp
    platform/graphics/ImageBuffer.cpp
    platform/graphics/IntRect.cpp
    platform/graphics/Path.cpp
    platform/graphics/PathTraversalState.cpp
    platform/graphics/Pattern.cpp
    platform/graphics/RoundedIntRect.cpp
    platform/graphics/SegmentedFontData.cpp
    platform/graphics/ShadowBlur.cpp
    platform/graphics/SimpleFontData.cpp
    platform/graphics/StringTruncator.cpp
    platform/graphics/WidthIterator.cpp

    platform/graphics/filters/DistantLightSource.cpp
    platform/graphics/filters/FEBlend.cpp
    platform/graphics/filters/FEColorMatrix.cpp
    platform/graphics/filters/FEComponentTransfer.cpp
    platform/graphics/filters/FEComposite.cpp
    platform/graphics/filters/FEConvolveMatrix.cpp
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
    platform/graphics/filters/LightSource.cpp
    platform/graphics/filters/PointLightSource.cpp
    platform/graphics/filters/SpotLightSource.cpp
    platform/graphics/filters/SourceAlpha.cpp
    platform/graphics/filters/SourceGraphic.cpp

    platform/graphics/filters/arm/FEGaussianBlurNEON.cpp
    platform/graphics/filters/arm/FEGaussianBlurNEON.h
    platform/graphics/filters/arm/FELightingNEON.cpp
    platform/graphics/filters/arm/FELightingNEON.h

    platform/graphics/transforms/AffineTransform.cpp
    platform/graphics/transforms/Matrix3DTransformOperation.cpp
    platform/graphics/transforms/MatrixTransformOperation.cpp
    platform/graphics/transforms/PerspectiveTransformOperation.cpp
    platform/graphics/transforms/RotateTransformOperation.cpp
    platform/graphics/transforms/ScaleTransformOperation.cpp
    platform/graphics/transforms/SkewTransformOperation.cpp
    platform/graphics/transforms/TransformOperations.cpp
    platform/graphics/transforms/TransformationMatrix.cpp
    platform/graphics/transforms/TranslateTransformOperation.cpp

    platform/mock/DeviceOrientationClientMock.cpp
    platform/mock/GeolocationClientMock.cpp
    platform/mock/GeolocationServiceMock.cpp

    platform/network/AuthenticationChallengeBase.cpp
    platform/network/BlobData.cpp
    platform/network/BlobRegistryImpl.cpp
    platform/network/BlobResourceHandle.cpp
    platform/network/Credential.cpp
    platform/network/DataURL.cpp
    platform/network/FormDataBuilder.cpp
    platform/network/FormData.cpp
    platform/network/HTTPHeaderMap.cpp
    platform/network/HTTPParsers.cpp
    platform/network/NetworkStateNotifier.cpp
    platform/network/ProtectionSpace.cpp
    platform/network/ProxyServer.cpp
    platform/network/ResourceErrorBase.cpp
    platform/network/ResourceHandle.cpp
    platform/network/ResourceRequestBase.cpp
    platform/network/ResourceResponseBase.cpp
    platform/network/SocketStreamHandleBase.cpp

    platform/text/Base64.cpp
    platform/text/BidiContext.cpp
    platform/text/LineEnding.cpp
    platform/text/LocalizedDateNone.cpp
    platform/text/LocalizedNumberNone.cpp
    platform/text/Hyphenation.cpp
    platform/text/RegularExpression.cpp
    platform/text/SegmentedString.cpp
    platform/text/String.cpp
    platform/text/TextBoundaries.cpp
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
    plugins/PluginData.cpp
    plugins/PluginMainThreadScheduler.cpp

    rendering/AutoTableLayout.cpp
    rendering/BidiRun.cpp
    rendering/CounterNode.cpp
    rendering/EllipsisBox.cpp
    rendering/FixedTableLayout.cpp
    rendering/HitTestResult.cpp
    rendering/InlineBox.cpp
    rendering/InlineFlowBox.cpp
    rendering/InlineTextBox.cpp
    rendering/LayoutState.cpp
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
    rendering/RenderDetails.cpp
    rendering/RenderDetailsMarker.cpp
    rendering/RenderEmbeddedObject.cpp
    rendering/RenderFieldset.cpp
    rendering/RenderFileUploadControl.cpp
    rendering/RenderFlexibleBox.cpp
    rendering/RenderFrame.cpp
    rendering/RenderFrameBase.cpp
    rendering/RenderFrameSet.cpp
    rendering/RenderHTMLCanvas.cpp
    rendering/RenderIFrame.cpp
    rendering/RenderImage.cpp
    rendering/RenderImageResource.cpp
    rendering/RenderImageResourceStyleImage.cpp
    rendering/RenderInline.cpp
    rendering/RenderLayer.cpp
    rendering/RenderLineBoxList.cpp
    rendering/RenderListBox.cpp
    rendering/RenderListItem.cpp
    rendering/RenderListMarker.cpp
    rendering/RenderMarquee.cpp
    rendering/RenderMenuList.cpp
    rendering/RenderMeter.cpp
    rendering/RenderObject.cpp
    rendering/RenderObjectChildList.cpp
    rendering/RenderPart.cpp
    rendering/RenderProgress.cpp
    rendering/RenderQuote.cpp
    rendering/RenderReplaced.cpp
    rendering/RenderReplica.cpp
    rendering/RenderRuby.cpp
    rendering/RenderRubyBase.cpp
    rendering/RenderRubyRun.cpp
    rendering/RenderRubyText.cpp
    rendering/RenderScrollbar.cpp
    rendering/RenderScrollbarPart.cpp
    rendering/RenderScrollbarTheme.cpp
    rendering/RenderSlider.cpp
    rendering/RenderSummary.cpp
    rendering/RenderTable.cpp
    rendering/RenderTableCell.cpp
    rendering/RenderTableCol.cpp
    rendering/RenderTableRow.cpp
    rendering/RenderTableSection.cpp
    rendering/RenderText.cpp
    rendering/RenderTextControl.cpp
    rendering/RenderTextControlMultiLine.cpp
    rendering/RenderTextControlSingleLine.cpp
    rendering/RenderTextFragment.cpp
    rendering/RenderTheme.cpp
    rendering/RenderTreeAsText.cpp
    rendering/RenderView.cpp
    rendering/RenderWidget.cpp
    rendering/RenderWordBreak.cpp
    rendering/RootInlineBox.cpp
    rendering/ScrollBehavior.cpp
    rendering/ShadowElement.cpp
    rendering/TransformState.cpp
    rendering/break_lines.cpp

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
    rendering/style/StyleFlexibleBoxData.cpp
    rendering/style/StyleGeneratedImage.cpp
    rendering/style/StyleInheritedData.cpp
    rendering/style/StyleMarqueeData.cpp
    rendering/style/StyleMultiColData.cpp
    rendering/style/StyleRareInheritedData.cpp
    rendering/style/StyleRareNonInheritedData.cpp
    rendering/style/StyleSurroundData.cpp
    rendering/style/StyleTransformData.cpp
    rendering/style/StyleVisualData.cpp

    storage/AbstractDatabase.cpp
    storage/ChangeVersionWrapper.cpp
    storage/Database.cpp
    storage/DatabaseAuthorizer.cpp
    storage/DatabaseSync.cpp
    storage/DatabaseTask.cpp
    storage/DatabaseThread.cpp
    storage/DatabaseTracker.cpp
    storage/IDBAny.cpp
    storage/IDBDatabase.cpp
    storage/IDBDatabaseBackendImpl.cpp
    storage/IDBCursor.cpp
    storage/IDBCursorBackendImpl.cpp
    storage/IDBEventDispatcher.cpp
    storage/IDBIndex.cpp
    storage/IDBIndexBackendImpl.cpp
    storage/IDBKey.cpp
    storage/IDBKeyRange.cpp
    storage/IDBRequest.cpp
    storage/IDBObjectStore.cpp
    storage/IDBObjectStoreBackendImpl.cpp
    storage/IDBFactory.cpp
    storage/IDBFactoryBackendInterface.cpp
    storage/IDBFactoryBackendImpl.cpp
    storage/LocalStorageTask.cpp
    storage/LocalStorageThread.cpp
    storage/OriginQuotaManager.cpp
    storage/OriginUsageRecord.cpp
    storage/SQLResultSet.cpp
    storage/SQLResultSetRowList.cpp
    storage/SQLStatement.cpp
    storage/SQLStatementSync.cpp
    storage/SQLTransaction.cpp
    storage/SQLTransactionClient.cpp
    storage/SQLTransactionCoordinator.cpp
    storage/SQLTransactionSync.cpp
    storage/Storage.cpp
    storage/StorageAreaImpl.cpp
    storage/StorageAreaSync.cpp
    storage/StorageEvent.cpp
    storage/StorageEventDispatcher.cpp
    storage/StorageInfo.cpp
    storage/StorageMap.cpp
    storage/StorageNamespace.cpp
    storage/StorageNamespaceImpl.cpp
    storage/StorageSyncManager.cpp
    storage/StorageTracker.cpp

    websockets/ThreadableWebSocketChannel.cpp
    websockets/WebSocket.cpp
    websockets/WebSocketChannel.cpp
    websockets/WebSocketHandshake.cpp
    websockets/WebSocketHandshakeRequest.cpp
    websockets/WebSocketHandshakeResponse.cpp
    websockets/WorkerThreadableWebSocketChannel.cpp

    workers/AbstractWorker.cpp
    workers/DedicatedWorkerContext.cpp
    workers/DedicatedWorkerThread.cpp
    workers/DefaultSharedWorkerRepository.cpp
    workers/SharedWorker.cpp
    workers/SharedWorkerContext.cpp
    workers/SharedWorkerThread.cpp
    workers/Worker.cpp
    workers/WorkerContext.cpp
    workers/WorkerLocation.cpp
    workers/WorkerMessagingProxy.cpp
    workers/WorkerRunLoop.cpp
    workers/WorkerScriptLoader.cpp
    workers/WorkerThread.cpp

    xml/DOMParser.cpp
    xml/NativeXPathNSResolver.cpp
    xml/XMLHttpRequest.cpp
    xml/XMLHttpRequestProgressEventThrottle.cpp
    xml/XMLHttpRequestUpload.cpp
    xml/XMLTreeViewer.cpp
    xml/XMLSerializer.cpp
    xml/XPathEvaluator.cpp
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
)

SET(WebCore_CSS_PROPERTY_NAMES
    ${WEBCORE_DIR}/css/CSSPropertyNames.in
)

SET(WebCore_CSS_VALUE_KEYWORDS
    ${WEBCORE_DIR}/css/CSSValueKeywords.in
)

SET(WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/html.css
    ${WEBCORE_DIR}/css/mathml.css
    ${WEBCORE_DIR}/css/mediaControls.css
    ${WEBCORE_DIR}/css/mediaControlsEfl.css
    ${WEBCORE_DIR}/css/quirks.css
    ${WEBCORE_DIR}/css/svg.css
    ${WEBCORE_DIR}/css/view-source.css
)

IF (ENABLE_BLOB)
    LIST(APPEND WebCore_SOURCES
        fileapi/FileReader.cpp
        fileapi/FileReaderLoader.cpp
        fileapi/FileThread.cpp
        fileapi/FileStreamProxy.cpp
        platform/FileStream.cpp
    )
ENDIF ()

IF (ENABLE_DATABASE)
    LIST(APPEND WebCore_SOURCES
        platform/sql/SQLValue.cpp
        platform/sql/SQLiteAuthorizer.cpp
        platform/sql/SQLiteDatabase.cpp
        platform/sql/SQLiteFileSystem.cpp
        platform/sql/SQLiteStatement.cpp
        platform/sql/SQLiteTransaction.cpp
    )
ENDIF ()

IF (ENABLE_DATA_TRANSFER_ITEMS)
    LIST(APPEND WebCore_IDL_FILES
    )
    LIST(APPEND WebCore_SOURCES
        dom/DataTransferItem.cpp
        dom/DataTransferItems.cpp
        dom/StringCallback.cpp
    )
ENDIF ()

IF (ENABLE_LEVELDB)
    LIST(APPEND WebCore_SOURCES
        platform/leveldb/LevelDBDatabase.cpp
        platform/leveldb/LevelDBIterator.cpp
    )
ENDIF ()

IF (ENABLE_MATHML)
    LIST(APPEND WebCore_SOURCES
        mathml/MathMLElement.cpp
        mathml/MathMLInlineContainerElement.cpp
        mathml/MathMLMathElement.cpp
        mathml/MathMLTextElement.cpp
        rendering/mathml/RenderMathMLBlock.cpp
        rendering/mathml/RenderMathMLFenced.cpp
        rendering/mathml/RenderMathMLFraction.cpp
        rendering/mathml/RenderMathMLMath.cpp
        rendering/mathml/RenderMathMLOperator.cpp
        rendering/mathml/RenderMathMLRoot.cpp
        rendering/mathml/RenderMathMLRow.cpp
        rendering/mathml/RenderMathMLSquareRoot.cpp
        rendering/mathml/RenderMathMLSubSup.cpp
        rendering/mathml/RenderMathMLUnderOver.cpp
    )
ENDIF ()

IF (ENABLE_NETSCAPE_PLUGIN_API)
    LIST(APPEND WebCore_SOURCES
        plugins/PluginDebug.cpp
        plugins/PluginStream.cpp
        plugins/PluginPackage.cpp
        plugins/PluginView.cpp
        plugins/npapi.cpp
    )
ELSE ()
    LIST(APPEND WebCore_SOURCES
        plugins/PluginPackageNone.cpp
        plugins/PluginViewNone.cpp
    )
ENDIF ()

IF (ENABLE_NOTIFICATIONS)
    LIST(APPEND WebCore_SOURCES
        bindings/js/JSDesktopNotificationsCustom.cpp
    )
    LIST(APPEND WebCore_IDL_FILES
        notifications/Notification.idl
        notifications/NotificationCenter.idl
    )
ENDIF ()

if (ENABLE_FILE_SYSTEM)
    LIST(APPEND WebCore_SOURCES
        bindings/js/JSDirectoryEntryCustom.cpp
        bindings/js/JSDirectoryEntrySyncCustom.cpp
        bindings/js/JSEntryCustom.cpp
        bindings/js/JSEntrySyncCustom.cpp
        fileapi/DirectoryEntry.cpp
        fileapi/DirectoryEntrySync.cpp
        fileapi/DirectoryReader.cpp
        fileapi/DirectoryReaderSync.cpp
        fileapi/DOMFilePath.cpp
        fileapi/DOMFileSystem.cpp
        fileapi/DOMFileSystemBase.cpp
        fileapi/DOMFileSystemSync.cpp
        fileapi/Entry.cpp
        fileapi/EntryArray.cpp
        fileapi/EntryArraySync.cpp
        fileapi/EntrySync.cpp
        fileapi/FileEntry.cpp
        fileapi/FileEntrySync.cpp
        fileapi/FileSystemCallbacks.cpp
        fileapi/FileWriter.cpp
        fileapi/FileWriterBase.cpp
        fileapi/FileWriterSync.cpp
        fileapi/LocalFileSystem.cpp
        platform/AsyncFileSystem.cpp
    )
    LIST(APPEND WebCore_IDL_FILES
        fileapi/DirectoryEntry.idl
        fileapi/DirectoryEntrySync.idl
        fileapi/DirectoryReader.idl
        fileapi/DirectoryReaderSync.idl
        fileapi/DOMFileSystem.idl
        fileapi/DOMFileSystemSync.idl
        fileapi/EntriesCallback.idl
        fileapi/Entry.idl
        fileapi/EntrySync.idl
        fileapi/EntryArray.idl
        fileapi/EntryArraySync.idl
        fileapi/EntryCallback.idl
        fileapi/ErrorCallback.idl
        fileapi/FileCallback.idl
        fileapi/FileEntry.idl
        fileapi/FileEntrySync.idl
        fileapi/FileSystemCallback.idl
        fileapi/FileWriterCallback.idl
        fileapi/FileWriter.idl
        fileapi/FileWriterSync.idl
        fileapi/WebKitFlags.idl
        fileapi/Metadata.idl
        fileapi/MetadataCallback.idl
    )
ENDIF ()

IF (ENABLE_SVG)
    LIST(APPEND WebCore_SOURCES
        bindings/js/JSSVGElementInstanceCustom.cpp
        bindings/js/JSSVGLengthCustom.cpp
        bindings/js/JSSVGPathSegCustom.cpp
        css/SVGCSSComputedStyleDeclaration.cpp
        css/SVGCSSParser.cpp
        css/SVGCSSStyleSelector.cpp
        platform/graphics/SVGGlyph.cpp
        rendering/PointerEventsHitRules.cpp
        rendering/style/SVGRenderStyle.cpp
        rendering/style/SVGRenderStyleDefs.cpp
        rendering/svg/RenderSVGBlock.cpp
        rendering/svg/RenderSVGContainer.cpp
        rendering/svg/RenderSVGGradientStop.cpp
        rendering/svg/RenderSVGForeignObject.cpp
        rendering/svg/RenderSVGHiddenContainer.cpp
        rendering/svg/RenderSVGImage.cpp
        rendering/svg/RenderSVGInline.cpp
        rendering/svg/RenderSVGInlineText.cpp
        rendering/svg/RenderSVGModelObject.cpp
        rendering/svg/RenderSVGPath.cpp
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
        rendering/svg/RenderSVGShadowTreeRootContainer.cpp
        rendering/svg/RenderSVGTSpan.cpp
        rendering/svg/RenderSVGText.cpp
        rendering/svg/RenderSVGTextPath.cpp
        rendering/svg/RenderSVGTransformableContainer.cpp
        rendering/svg/RenderSVGViewportContainer.cpp
        rendering/svg/SVGImageBufferTools.cpp
        rendering/svg/SVGInlineFlowBox.cpp
        rendering/svg/SVGInlineTextBox.cpp
        rendering/svg/SVGMarkerLayoutInfo.cpp
        rendering/svg/SVGRenderSupport.cpp
        rendering/svg/SVGRenderTreeAsText.cpp
        rendering/svg/SVGResources.cpp
        rendering/svg/SVGResourcesCache.cpp
        rendering/svg/SVGResourcesCycleSolver.cpp
        rendering/svg/SVGRootInlineBox.cpp
        rendering/svg/SVGShadowTreeElements.cpp
        rendering/svg/SVGTextChunk.cpp
        rendering/svg/SVGTextChunkBuilder.cpp
        rendering/svg/SVGTextLayoutAttributes.cpp
        rendering/svg/SVGTextLayoutAttributesBuilder.cpp
        rendering/svg/SVGTextLayoutEngine.cpp
        rendering/svg/SVGTextLayoutEngineBaseline.cpp
        rendering/svg/SVGTextLayoutEngineSpacing.cpp
        rendering/svg/SVGTextMetrics.cpp
        rendering/svg/SVGTextQuery.cpp
        svg/ColorDistance.cpp
        svg/SVGAElement.cpp
        svg/SVGAltGlyphElement.cpp
        svg/SVGAngle.cpp
        svg/SVGAnimateColorElement.cpp
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
        svg/SVGFont.cpp
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
        svg/SVGGradientElement.cpp
        svg/SVGHKernElement.cpp
        svg/SVGImageElement.cpp
        svg/SVGImageLoader.cpp
        svg/SVGLangSpace.cpp
        svg/SVGLength.cpp
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
        svg/SVGPathParserFactory.cpp
        svg/SVGPathSegList.cpp
        svg/SVGPathSegListBuilder.cpp
        svg/SVGPathSegListSource.cpp
        svg/SVGPathStringBuilder.cpp
        svg/SVGPathStringSource.cpp
        svg/SVGPathTraversalStateBuilder.cpp
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
        svg/SVGStylable.cpp
        svg/SVGStyleElement.cpp
        svg/SVGStyledElement.cpp
        svg/SVGStyledLocatableElement.cpp
        svg/SVGStyledTransformableElement.cpp
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
        svg/SVGVKernElement.cpp
        svg/SVGViewSpec.cpp
        svg/SVGZoomAndPan.cpp
        svg/SVGZoomEvent.cpp
        svg/animation/SMILTime.cpp
        svg/animation/SMILTimeContainer.cpp
        svg/animation/SVGSMILElement.cpp
        svg/graphics/SVGImage.cpp
        svg/graphics/filters/SVGFEImage.cpp
        svg/graphics/filters/SVGFilter.cpp
        svg/graphics/filters/SVGFilterBuilder.cpp
        svg/properties/SVGPathSegListPropertyTearOff.cpp
    )

    LIST(APPEND WebCore_IDL_FILES
        svg/SVGAElement.idl
        svg/SVGAltGlyphElement.idl
        svg/SVGAngle.idl
        svg/SVGAnimateColorElement.idl
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
        svg/SVGFontElement.idl
        svg/SVGFontFaceElement.idl
        svg/SVGFontFaceFormatElement.idl
        svg/SVGFontFaceNameElement.idl
        svg/SVGFontFaceSrcElement.idl
        svg/SVGFontFaceUriElement.idl
        svg/SVGForeignObjectElement.idl
        svg/SVGGElement.idl
        svg/SVGGlyphElement.idl
        svg/SVGGradientElement.idl
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
        svg/SVGSwitchElement.idl
        svg/SVGSymbolElement.idl
        svg/SVGTRefElement.idl
        svg/SVGTSpanElement.idl
        svg/SVGTextContentElement.idl
        svg/SVGTextElement.idl
        svg/SVGTextPathElement.idl
        svg/SVGTextPositioningElement.idl
        svg/SVGTitleElement.idl
        svg/SVGTransform.idl
        svg/SVGTransformList.idl
        svg/SVGUnitTypes.idl
        svg/SVGUseElement.idl
        svg/SVGViewElement.idl
        svg/SVGVKernElement.idl
        svg/SVGZoomEvent.idl
    )

    LIST(APPEND WebCore_CSS_PROPERTY_NAMES
        ${WEBCORE_DIR}/css/SVGCSSPropertyNames.in
    )

    LIST(APPEND WebCore_CSS_VALUE_KEYWORDS
        ${WEBCORE_DIR}/css/SVGCSSValueKeywords.in
    )
ENDIF()

IF (ENABLE_VIDEO)
    LIST(APPEND WebCore_SOURCES
        bindings/js/JSAudioConstructor.cpp
        html/HTMLAudioElement.cpp
        html/HTMLMediaElement.cpp
        html/HTMLSourceElement.cpp
        html/HTMLVideoElement.cpp
        html/shadow/MediaControlElements.cpp
        html/TimeRanges.cpp
        platform/graphics/MediaPlayer.cpp
        rendering/RenderMedia.cpp
        rendering/RenderVideo.cpp
    )
ENDIF()

IF (ENABLE_VIDEO_TRACK)
    LIST(APPEND WebCore_SOURCES
        html/HTMLTrackElement.cpp
    )
ENDIF()

IF (ENABLE_PROGRESS_TAG)
    LIST(APPEND WebCore_SOURCES
        accessibility/AccessibilityProgressIndicator.cpp
    )
ENDIF ()

IF (ENABLE_MEDIA_STREAM)
    LIST(APPEND WebCore_SOURCES
        bindings/js/JSNavigatorCustom.cpp
        page/MediaStreamController.cpp
        page/MediaStreamFrameController.cpp
    )

    LIST(APPEND WebCore_IDL_FILES
        page/NavigatorUserMediaError.idl
    )
ENDIF ()


# Modules that the bindings generator scripts may use
SET(SCRIPTS_BINDINGS
    ${WEBCORE_DIR}/bindings/scripts/IDLParser.pm
    ${WEBCORE_DIR}/bindings/scripts/IDLStructure.pm
    ${WEBCORE_DIR}/bindings/scripts/InFilesParser.pm
)

INCLUDE(${WEBCORE_DIR}/UseJSC.cmake)
 
# All Web Inspector generated files are created with this one call to CodeGeneratorInspector.pm
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/Inspector.idl
    MAIN_DEPENDENCY inspector/Inspector.json
    DEPENDS ${WEBCORE_DIR}/inspector/generate-inspector-idl ${WEBCORE_DIR}/inspector/Inspector.json
    COMMAND ${PYTHON_EXECUTABLE} ${WEBCORE_DIR}/inspector/generate-inspector-idl -o ${DERIVED_SOURCES_WEBCORE_DIR}/Inspector.idl ${WEBCORE_DIR}/inspector/Inspector.json
    VERBATIM)

ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorBackendDispatcher.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorBackendDispatcher.h ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorFrontend.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorFrontend.h
    MAIN_DEPENDENCY ${DERIVED_SOURCES_WEBCORE_DIR}/Inspector.idl
    DEPENDS ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl ${SCRIPTS_BINDINGS} ${WEBCORE_DIR}/inspector/CodeGeneratorInspector.pm ${DERIVED_SOURCES_WEBCORE_DIR}/Inspector.idl
    COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts -I${WEBCORE_DIR}/inspector ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl --defines "${FEATURE_DEFINES_JAVASCRIPT}" --generator Inspector ${IDL_INCLUDES} --outputDir "${DERIVED_SOURCES_WEBCORE_DIR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" ${DERIVED_SOURCES_WEBCORE_DIR}/Inspector.idl
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorBackendDispatcher.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorFrontend.cpp)


# Generate InjectedScriptSource.h
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptSource.h
    MAIN_DEPENDENCY inspector/InjectedScriptSource.js
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl InjectedScriptSource_js ${WEBCORE_DIR}/inspector/InjectedScriptSource.js ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptSource.h
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/InjectedScriptSource.h)


MAKE_HASH_TOOLS(${WEBCORE_DIR}/platform/ColorData)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/ColorData.cpp)


MAKE_HASH_TOOLS(${WEBCORE_DIR}/html/DocTypeStrings)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/DocTypeStrings.cpp)


# Generate XMLViewerCSS.h
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerCSS.h
    MAIN_DEPENDENCY xml/XMLViewer.css
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl XMLViewer_css ${WEBCORE_DIR}/xml/XMLViewer.css ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerCSS.h
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerCSS.h)


# Generate XMLViewerJS.h
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerJS.h
    MAIN_DEPENDENCY xml/XMLViewer.js
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl XMLViewer_js ${WEBCORE_DIR}/xml/XMLViewer.js ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerJS.h
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XMLViewerJS.h)


# Generate HTML entity table
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLEntityTable.cpp
    MAIN_DEPENDENCY ${WEBCORE_DIR}/html/parser/HTMLEntityNames.in
    DEPENDS ${WEBCORE_DIR}/html/parser/create-html-entity-table
    COMMAND ${PYTHON_EXECUTABLE} ${WEBCORE_DIR}/html/parser/create-html-entity-table -o ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLEntityTable.cpp ${WEBCORE_DIR}/html/parser/HTMLEntityNames.in
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLEntityTable.cpp)


# Generate tokenizer
FILE(TO_NATIVE_PATH ${PERL_EXECUTABLE} PERL_EXECUTABLE_NATIVE_PATH)
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/tokenizer.cpp
    MAIN_DEPENDENCY ${WEBCORE_DIR}/css/maketokenizer
    DEPENDS ${WEBCORE_DIR}/css/tokenizer.flex
    COMMAND ${FLEX_EXECUTABLE} -t ${WEBCORE_DIR}/css/tokenizer.flex | ${PERL_EXECUTABLE_NATIVE_PATH} ${WEBCORE_DIR}/css/maketokenizer > ${DERIVED_SOURCES_WEBCORE_DIR}/tokenizer.cpp
    VERBATIM)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/CSSParser.cpp tokenizer.cpp)


# Generate CSS property names
ADD_CUSTOM_COMMAND (
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.in ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.h ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.gperf
    MAIN_DEPENDENCY ${WEBCORE_DIR}/css/makeprop.pl
    DEPENDS ${WebCore_CSS_PROPERTY_NAMES}
    WORKING_DIRECTORY ${DERIVED_SOURCES_WEBCORE_DIR}
    COMMAND ${PERL_EXECUTABLE} -ne "print" ${WebCore_CSS_PROPERTY_NAMES} > ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.in
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/css/makeprop.pl
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/CSSPropertyNames.cpp)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/CSSParser.cpp CSSValueKeywords.h)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.cpp CSSPropertyNames.h)


# Generate CSS value keywords
ADD_CUSTOM_COMMAND (
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.in ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.h ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.gperf
    MAIN_DEPENDENCY ${WEBCORE_DIR}/css/makevalues.pl
    DEPENDS ${WebCore_CSS_VALUE_KEYWORDS}
    WORKING_DIRECTORY ${DERIVED_SOURCES_WEBCORE_DIR}
    COMMAND ${PERL_EXECUTABLE} -ne "print lc" ${WebCore_CSS_VALUE_KEYWORDS} > ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.in
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/css/makevalues.pl
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/CSSValueKeywords.cpp)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/CSSParser.cpp CSSValueKeywords.h)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/CSSPrimitiveValueMappings.h CSSValueKeywords.h)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.cpp CSSValueKeywords.h)


# Generate user agent styles
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheetsData.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheets.h
    MAIN_DEPENDENCY ${WEBCORE_DIR}/css/make-css-file-arrays.pl
    DEPENDS ${WebCore_USER_AGENT_STYLE_SHEETS}
    COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/css/make-css-file-arrays.pl ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheets.h ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheetsData.cpp ${WebCore_USER_AGENT_STYLE_SHEETS}
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/UserAgentStyleSheetsData.cpp)
ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/css/CSSStyleSelector.cpp UserAgentStyleSheetsData.cpp UserAgentStyleSheets.h)


GENERATE_GRAMMAR(cssyy ${WEBCORE_DIR}/css/CSSGrammar.y ${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.h ${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.cpp)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/CSSGrammar.cpp)


GENERATE_GRAMMAR(xpathyy ${WEBCORE_DIR}/xml/XPathGrammar.y ${DERIVED_SOURCES_WEBCORE_DIR}/XPathGrammar.h ${DERIVED_SOURCES_WEBCORE_DIR}/XPathGrammar.cpp)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XPathGrammar.cpp)



ADD_SOURCE_WEBCORE_DERIVED_DEPENDENCIES(${WEBCORE_DIR}/html/HTMLTreeBuilder.cpp MathMLNames.cpp)


GENERATE_DOM_NAMES(HTML ${WEBCORE_DIR}/html/HTMLAttributeNames.in ${WEBCORE_DIR}/html/HTMLTagNames.in "${HTML_FEATURES}")
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLNames.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/HTMLElementFactory.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/JSHTMLElementWrapperFactory.cpp)


GENERATE_DOM_NAMES(MathML ${WEBCORE_DIR}/mathml/mathattrs.in ${WEBCORE_DIR}/mathml/mathtags.in "${MATHML_FEATURES}")
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/MathMLNames.cpp)
IF (ENABLE_MATHML)
    LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/MathMLElementFactory.cpp)
ENDIF ()


GENERATE_DOM_NAMES(SVG ${WEBCORE_DIR}/svg/svgattrs.in ${WEBCORE_DIR}/svg/svgtags.in "${SVG_FEATURES}")
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/SVGNames.cpp)
IF (ENABLE_SVG)
    LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/SVGElementFactory.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/JSSVGElementWrapperFactory.cpp)
ENDIF ()


GENERATE_DOM_NAMES(XLink ${WEBCORE_DIR}/svg/xlinkattrs.in)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XLinkNames.cpp)


GENERATE_DOM_NAMES(XMLNS ${WEBCORE_DIR}/xml/xmlnsattrs.in)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XMLNSNames.cpp)


GENERATE_DOM_NAMES(XML ${WEBCORE_DIR}/xml/xmlattrs.in)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/XMLNames.cpp)


ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h
    MAIN_DEPENDENCY ${WEBKIT_DIR}/scripts/generate-webkitversion.pl
    DEPENDS ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT_DIR}/scripts/generate-webkitversion.pl --config ${WEBKIT_DIR}/mac/Configurations/Version.xcconfig --outputDir ${DERIVED_SOURCES_WEBCORE_DIR}
    VERBATIM)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/WebKitVersion.h)


SET(WebCore_LIBRARIES
    ${JavaScriptCore_LIBRARY_NAME}
)


SET(WebCore_LINK_FLAGS "")
INCLUDE_IF_EXISTS(${WEBCORE_DIR}/CMakeLists${PORT}.txt)

WEBKIT_WRAP_SOURCELIST(${WebCore_IDL_FILES} ${WebCore_SOURCES})
ADD_DEFINITIONS(-DBUILDING_WebCore)
INCLUDE_DIRECTORIES(${WebCore_INCLUDE_DIRECTORIES})
ADD_LIBRARY(${WebCore_LIBRARY_NAME} ${WebCore_LIBRARY_TYPE} ${WebCore_SOURCES})
ADD_DEPENDENCIES(${WebCore_LIBRARY_NAME} ${JavaScriptCore_LIBRARY_NAME})
TARGET_LINK_LIBRARIES(${WebCore_LIBRARY_NAME} ${WebCore_LIBRARIES})

IF (WebCore_LINK_FLAGS)
    ADD_TARGET_PROPERTIES(${WebCore_LIBRARY_NAME} LINK_FLAGS "${WebCore_LINK_FLAGS}")
ENDIF ()

IF (SHARED_CORE)
    SET_TARGET_PROPERTIES(${WebCore_LIBRARY_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    INSTALL(TARGETS ${WebCore_LIBRARY_NAME} DESTINATION lib)
ENDIF ()
