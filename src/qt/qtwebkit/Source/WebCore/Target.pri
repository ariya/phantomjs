# -------------------------------------------------------------------
# Target file for the WebCore library
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = lib
TARGET = WebCore

include(WebCore.pri)

WEBKIT += wtf
WEBKIT += javascriptcore

CONFIG += staticlib

# Do it in the WebCore static lib to support force_static_libs_as_shared
# since the QtWebKitWidgets lib wouldn't load QtWebKit in that case.
# This should match the opposite statement in api.pri for the QtWebKit lib.
!win* {
    RESOURCES += $$PWD/WebCore.qrc
    include_webinspector {
        RESOURCES += \
            $$PWD/inspector/front-end/WebKit.qrc \
            $${WEBCORE_GENERATED_SOURCES_DIR}/InspectorBackendCommands.qrc
    }
}

SOURCES += \
    Modules/geolocation/Coordinates.cpp \
    Modules/geolocation/Geolocation.cpp \
    Modules/geolocation/GeolocationController.cpp \
    Modules/geolocation/NavigatorGeolocation.cpp \
    Modules/webdatabase/DOMWindowWebDatabase.cpp \
    Modules/webdatabase/Database.cpp \
    Modules/webdatabase/DatabaseAuthorizer.cpp \
    Modules/webdatabase/DatabaseBackendBase.cpp \
    Modules/webdatabase/DatabaseContext.cpp \
    Modules/webdatabase/DatabaseServer.cpp \
    Modules/webdatabase/DatabaseSync.cpp \
    Modules/webdatabase/WorkerGlobalScopeWebDatabase.cpp \
    accessibility/AccessibilityImageMapLink.cpp \
    accessibility/AccessibilityMediaControls.cpp \
    accessibility/AccessibilityMenuList.cpp \
    accessibility/AccessibilityMenuListOption.cpp \
    accessibility/AccessibilityMenuListPopup.cpp \
    accessibility/AccessibilityMockObject.cpp \
    accessibility/AccessibilityObject.cpp \
    accessibility/AccessibilityList.cpp \
    accessibility/AccessibilityListBox.cpp \
    accessibility/AccessibilityListBoxOption.cpp \
    accessibility/AccessibilityNodeObject.cpp \
    accessibility/AccessibilityProgressIndicator.cpp \
    accessibility/AccessibilityRenderObject.cpp \
    accessibility/AccessibilityScrollbar.cpp \
    accessibility/AccessibilityScrollView.cpp \
    accessibility/AccessibilitySlider.cpp \
    accessibility/AccessibilitySpinButton.cpp \
    accessibility/AccessibilityARIAGrid.cpp \
    accessibility/AccessibilityARIAGridCell.cpp \
    accessibility/AccessibilityARIAGridRow.cpp \
    accessibility/AccessibilityTable.cpp \
    accessibility/AccessibilityTableCell.cpp \
    accessibility/AccessibilityTableColumn.cpp \
    accessibility/AccessibilityTableHeaderContainer.cpp \
    accessibility/AccessibilityTableRow.cpp \
    accessibility/AXObjectCache.cpp \
    bindings/generic/ActiveDOMCallback.cpp \
    bindings/generic/BindingSecurity.cpp \
    bindings/generic/RuntimeEnabledFeatures.cpp

SOURCES += \
     bindings/ScriptControllerBase.cpp \
     bindings/js/ArrayValue.cpp \
     bindings/js/BindingState.cpp \
     bindings/js/CallbackFunction.cpp \
     bindings/js/DOMObjectHashTableMap.cpp \
     bindings/js/DOMWrapperWorld.cpp \
     bindings/js/Dictionary.cpp \
     bindings/js/GCController.cpp \
     bindings/js/JSArrayBufferCustom.cpp \
     bindings/js/JSAudioBufferCustom.cpp \
     bindings/js/JSAttrCustom.cpp \
     bindings/js/JSBlobCustom.cpp \
     bindings/js/JSCDATASectionCustom.cpp \
     bindings/js/JSCSSFontFaceRuleCustom.cpp \
     bindings/js/JSCSSImportRuleCustom.cpp \
     bindings/js/JSCSSMediaRuleCustom.cpp \
     bindings/js/JSCSSPageRuleCustom.cpp \
     bindings/js/JSCSSRuleCustom.cpp \
     bindings/js/JSCSSRuleListCustom.cpp \
     bindings/js/JSCSSStyleDeclarationCustom.cpp \
     bindings/js/JSCSSStyleRuleCustom.cpp \
     bindings/js/JSCSSValueCustom.cpp \
     bindings/js/JSCallbackData.cpp \
     bindings/js/JSCanvasRenderingContext2DCustom.cpp \
     bindings/js/JSCanvasRenderingContextCustom.cpp \
     bindings/js/JSClipboardCustom.cpp \
     bindings/js/JSConsoleCustom.cpp \
     bindings/js/JSCryptoCustom.cpp \
     bindings/js/JSCustomXPathNSResolver.cpp \
     bindings/js/JSDictionary.cpp \
     bindings/js/JSDOMBinding.cpp \
     bindings/js/JSDOMFormDataCustom.cpp \
     bindings/js/JSDOMGlobalObject.cpp \
     bindings/js/JSDOMImplementationCustom.cpp \
     bindings/js/JSDOMMimeTypeArrayCustom.cpp \
     bindings/js/JSDOMPluginArrayCustom.cpp \
     bindings/js/JSDOMPluginCustom.cpp \
     bindings/js/JSDOMStringListCustom.cpp \
     bindings/js/JSDOMStringMapCustom.cpp \
     bindings/js/JSDOMTokenListCustom.cpp \
     bindings/js/JSDOMWindowBase.cpp \
     bindings/js/JSDOMWindowCustom.cpp \
     bindings/js/JSDOMWindowShell.cpp \
     bindings/js/JSDOMWrapper.cpp \
     bindings/js/JSDataViewCustom.cpp \
     bindings/js/JSDeviceMotionEventCustom.cpp \
     bindings/js/JSDeviceOrientationEventCustom.cpp \
     bindings/js/JSDocumentCustom.cpp \
     bindings/js/JSElementCustom.cpp \
     bindings/js/JSErrorHandler.cpp \
     bindings/js/JSEventCustom.cpp \
     bindings/js/JSEventListener.cpp \
     bindings/js/JSEventTargetCustom.cpp \
     bindings/js/JSExceptionBase.cpp \
     bindings/js/JSFileReaderCustom.cpp \
     bindings/js/JSGeolocationCustom.cpp \
     bindings/js/JSHTMLAllCollectionCustom.cpp \
     bindings/js/JSHTMLAppletElementCustom.cpp \
     bindings/js/JSHTMLCanvasElementCustom.cpp \
     bindings/js/JSHTMLCollectionCustom.cpp \
     bindings/js/JSHTMLDocumentCustom.cpp \
     bindings/js/JSHTMLElementCustom.cpp \
     bindings/js/JSHTMLEmbedElementCustom.cpp \
     bindings/js/JSHTMLFormControlsCollectionCustom.cpp \
     bindings/js/JSHTMLFormElementCustom.cpp \
     bindings/js/JSHTMLFrameElementCustom.cpp \
     bindings/js/JSHTMLFrameSetElementCustom.cpp \
     bindings/js/JSHTMLInputElementCustom.cpp \
     bindings/js/JSHTMLLinkElementCustom.cpp \
     bindings/js/JSHTMLMediaElementCustom.cpp \
     bindings/js/JSHTMLObjectElementCustom.cpp \
     bindings/js/JSHTMLOptionsCollectionCustom.cpp \
     bindings/js/JSHTMLSelectElementCustom.cpp \
     bindings/js/JSHTMLStyleElementCustom.cpp \
     bindings/js/JSHTMLTemplateElementCustom.cpp \
     bindings/js/JSHistoryCustom.cpp \
     bindings/js/JSImageConstructor.cpp \
     bindings/js/JSImageDataCustom.cpp \
     bindings/js/JSInjectedScriptHostCustom.cpp \
     bindings/js/JSInjectedScriptManager.cpp \
     bindings/js/JSInspectorFrontendHostCustom.cpp \
     bindings/js/JSLazyEventListener.cpp \
     bindings/js/JSLocationCustom.cpp \
     bindings/js/JSMainThreadExecState.cpp \
     bindings/js/JSMediaListCustom.cpp \
     bindings/js/JSMessageChannelCustom.cpp \
     bindings/js/JSMessageEventCustom.cpp \
     bindings/js/JSMessagePortCustom.cpp \
     bindings/js/JSMicroDataItemValueCustom.cpp \
     bindings/js/JSMutationCallback.cpp \
     bindings/js/JSMutationObserverCustom.cpp \
     bindings/js/JSNamedNodeMapCustom.cpp \
     bindings/js/JSNodeCustom.cpp \
     bindings/js/JSNodeFilterCondition.cpp \
     bindings/js/JSNodeFilterCustom.cpp \
     bindings/js/JSNodeIteratorCustom.cpp \
     bindings/js/JSNodeListCustom.cpp \
     bindings/js/JSPluginElementFunctions.cpp \
     bindings/js/JSPopStateEventCustom.cpp \
     bindings/js/JSProcessingInstructionCustom.cpp \
     bindings/js/JSRequestAnimationFrameCallbackCustom.cpp \
     bindings/js/JSRTCStatsResponseCustom.cpp \
     bindings/js/JSStorageCustom.cpp \
     bindings/js/JSStyleSheetCustom.cpp \
     bindings/js/JSStyleSheetListCustom.cpp \
     bindings/js/JSTextCustom.cpp \
     bindings/js/JSTouchCustom.cpp \
     bindings/js/JSTouchListCustom.cpp \
     bindings/js/JSTreeWalkerCustom.cpp \
     bindings/js/JSWebKitCSSKeyframeRuleCustom.cpp \
     bindings/js/JSWebKitCSSKeyframesRuleCustom.cpp \
     bindings/js/JSWebKitPointCustom.cpp \
     bindings/js/JSXMLHttpRequestCustom.cpp \
     bindings/js/JSXMLHttpRequestUploadCustom.cpp \
     bindings/js/JSXPathResultCustom.cpp \
     bindings/js/PageScriptDebugServer.cpp \
     bindings/js/ScheduledAction.cpp \
     bindings/js/ScriptCachedFrameData.cpp \
     bindings/js/ScriptCallStackFactory.cpp \
     bindings/js/ScriptController.cpp \
     bindings/js/ScriptControllerQt.cpp \
     bindings/js/ScriptDebugServer.cpp \
     bindings/js/ScriptEventListener.cpp \
     bindings/js/ScriptFunctionCall.cpp \
     bindings/js/ScriptGCEvent.cpp \
     bindings/js/ScriptObject.cpp \
     bindings/js/ScriptProfile.cpp \
     bindings/js/ScriptState.cpp \
     bindings/js/ScriptValue.cpp \
     bindings/js/SerializedScriptValue.cpp \
     bridge/IdentifierRep.cpp \
     bridge/NP_jsobject.cpp \
     bridge/c/CRuntimeObject.cpp \
     bridge/c/c_class.cpp \
     bridge/c/c_instance.cpp \
     bridge/c/c_runtime.cpp \
     bridge/c/c_utility.cpp \
     bridge/jsc/BridgeJSC.cpp \
     bridge/npruntime.cpp \
     bridge/qt/qt_class.cpp \
     bridge/qt/qt_instance.cpp \
     bridge/qt/qt_pixmapruntime.cpp \
     bridge/qt/qt_runtime.cpp \
     bridge/runtime_array.cpp \
     bridge/runtime_method.cpp \
     bridge/runtime_object.cpp \
     bridge/runtime_root.cpp \
     testing/js/WebCoreTestSupport.cpp


SOURCES += \
    Modules/filesystem/DOMFilePath.cpp \
    Modules/filesystem/DOMFileSystem.cpp \
    Modules/filesystem/DOMFileSystemBase.cpp \
    Modules/filesystem/DOMFileSystemSync.cpp \
    Modules/filesystem/DOMWindowFileSystem.cpp \
    Modules/filesystem/DirectoryEntry.cpp \
    Modules/filesystem/DirectoryEntrySync.cpp \
    Modules/filesystem/DirectoryReader.cpp \
    Modules/filesystem/DirectoryReaderSync.cpp \
    Modules/filesystem/Entry.cpp \
    Modules/filesystem/EntryArray.cpp \
    Modules/filesystem/EntryArraySync.cpp \
    Modules/filesystem/EntryBase.cpp \
    Modules/filesystem/EntrySync.cpp \
    Modules/filesystem/FileEntry.cpp \
    Modules/filesystem/FileEntrySync.cpp \
    Modules/filesystem/FileWriter.cpp \
    Modules/filesystem/FileWriterBase.cpp \
    Modules/filesystem/FileWriterSync.cpp \
    Modules/filesystem/LocalFileSystem.cpp \
    Modules/filesystem/WorkerGlobalScopeFileSystem.cpp \
    Modules/navigatorcontentutils/NavigatorContentUtils.cpp \
    Modules/notifications/DOMWindowNotifications.cpp \
    Modules/notifications/Notification.cpp \
    Modules/notifications/NotificationCenter.cpp \
    Modules/notifications/NotificationController.cpp \
    Modules/notifications/WorkerGlobalScopeNotifications.cpp \
    Modules/proximity/DeviceProximityController.cpp \
    Modules/proximity/DeviceProximityEvent.cpp \
    css/BasicShapeFunctions.cpp \
    css/CSSAspectRatioValue.cpp \
    css/CSSBasicShapes.cpp \
    css/CSSBorderImageSliceValue.cpp \
    css/CSSBorderImage.cpp \
    css/CSSCalculationValue.cpp \
    css/CSSCanvasValue.cpp \
    css/CSSCharsetRule.cpp \
    css/CSSComputedStyleDeclaration.cpp \
    css/CSSCrossfadeValue.cpp \
    css/CSSCursorImageValue.cpp \
    css/CSSFontFace.cpp \
    css/CSSDefaultStyleSheets.cpp \
    css/CSSFontFaceLoadEvent.cpp \
    css/CSSFontFaceRule.cpp \
    css/CSSFontFaceSrcValue.cpp \
    css/CSSFontSelector.cpp \
    css/CSSFontFaceSource.cpp \
    css/CSSFunctionValue.cpp \
    css/CSSGradientValue.cpp \
    css/CSSGroupingRule.cpp \
    css/CSSHostRule.cpp \
    css/CSSImageValue.cpp \
    css/CSSImageGeneratorValue.cpp \
    css/CSSImageSetValue.cpp \
    css/CSSImportRule.cpp \
    css/CSSInheritedValue.cpp \
    css/CSSInitialValue.cpp \
    css/CSSLineBoxContainValue.cpp \
    css/CSSMediaRule.cpp \
    css/CSSOMUtils.cpp \
    css/CSSPageRule.cpp \
    css/CSSParser.cpp \
    css/CSSParserValues.cpp \
    css/CSSPrimitiveValue.cpp \
    css/CSSProperty.cpp \
    css/CSSPropertySourceData.cpp \
    css/CSSReflectValue.cpp \
    css/CSSRule.cpp \
    css/CSSRuleList.cpp \
    css/CSSSelector.cpp \
    css/CSSSelectorList.cpp \
    css/CSSSegmentedFontFace.cpp \
    css/CSSStyleRule.cpp \
    css/CSSStyleSheet.cpp \
    css/CSSSupportsRule.cpp \
    css/CSSTimingFunctionValue.cpp \
    css/CSSToStyleMap.cpp \
    css/CSSUnicodeRangeValue.cpp \
    css/CSSValue.cpp \
    css/CSSValueList.cpp \
    css/CSSValuePool.cpp \
    css/DOMWindowCSS.cpp \
    css/DeprecatedStyleBuilder.cpp \
    css/DocumentRuleSets.cpp \
    css/ElementRuleCollector.cpp \
    css/FontFeatureValue.cpp \
    css/FontLoader.cpp \
    css/FontValue.cpp \
    css/InspectorCSSOMWrappers.cpp \
    css/LengthFunctions.cpp \
    css/MediaFeatureNames.cpp \
    css/MediaList.cpp \
    css/MediaQuery.cpp \
    css/MediaQueryEvaluator.cpp \
    css/MediaQueryExp.cpp \
    css/MediaQueryList.cpp \
    css/MediaQueryListListener.cpp \
    css/MediaQueryMatcher.cpp \
    css/PageRuleCollector.cpp \
    css/PropertySetCSSStyleDeclaration.cpp \
    css/RGBColor.cpp \
    css/RuleFeature.cpp \
    css/RuleSet.cpp \
    css/SelectorChecker.cpp \
    css/SelectorCheckerFastPath.cpp \
    css/SelectorFilter.cpp \
    css/ShadowValue.cpp \
    css/StyleInvalidationAnalysis.cpp \
    css/StyleMedia.cpp \
    css/StylePropertySet.cpp \
    css/StylePropertyShorthand.cpp \
    css/StyleResolver.cpp \
    css/StyleRule.cpp \
    css/StyleRuleImport.cpp \
    css/StyleScopeResolver.cpp \
    css/StyleSheet.cpp \
    css/StyleSheetContents.cpp \
    css/StyleSheetList.cpp \
    css/TransformFunctions.cpp \
    css/ViewportStyleResolver.cpp \
    css/WebKitCSSArrayFunctionValue.cpp \
    css/WebKitCSSFilterRule.cpp \
    css/WebKitCSSFilterValue.cpp \
    css/WebKitCSSKeyframeRule.cpp \
    css/WebKitCSSKeyframesRule.cpp \
    css/WebKitCSSMatrix.cpp \
    css/WebKitCSSMatFunctionValue.cpp \
    css/WebKitCSSMixFunctionValue.cpp \
    css/WebKitCSSRegionRule.cpp \
    css/WebKitCSSSVGDocumentValue.cpp \
    css/WebKitCSSShaderValue.cpp \
    css/WebKitCSSTransformValue.cpp \
    css/WebKitCSSViewportRule.cpp \
    dom/ActiveDOMObject.cpp \
    dom/Attr.cpp \
    dom/BeforeTextInsertedEvent.cpp \
    dom/BeforeUnloadEvent.cpp \
    dom/CDATASection.cpp \
    dom/CharacterData.cpp \
    dom/CheckedRadioButtons.cpp \
    dom/ChildListMutationScope.cpp \
    dom/ChildNodeList.cpp \
    dom/ClassNodeList.cpp \
    dom/ClientRect.cpp \
    dom/ClientRectList.cpp \
    dom/Clipboard.cpp \
    dom/ClipboardEvent.cpp \
    dom/Comment.cpp \
    dom/ComposedShadowTreeWalker.cpp \
    dom/CompositionEvent.cpp \
    dom/ContainerNode.cpp \
    dom/ContainerNodeAlgorithms.cpp \
    dom/ContextDestructionObserver.cpp \
    dom/ContextFeatures.cpp \
    dom/CustomEvent.cpp \
    dom/DecodedDataDocumentParser.cpp \
    dom/DeviceMotionController.cpp \
    dom/DeviceMotionData.cpp \
    dom/DeviceMotionEvent.cpp \
    dom/DeviceOrientationController.cpp \
    dom/DeviceOrientationData.cpp \
    dom/DeviceOrientationEvent.cpp \
    dom/Document.cpp \
    dom/DocumentEventQueue.cpp \
    dom/DocumentFragment.cpp \
    dom/DocumentMarkerController.cpp \
    dom/DocumentMarker.cpp \
    dom/DocumentOrderedMap.cpp \
    dom/DocumentParser.cpp \
    dom/DocumentSharedObjectPool.cpp \
    dom/DocumentStyleSheetCollection.cpp \
    dom/DocumentType.cpp \
    dom/DOMCoreException.cpp \
    dom/DOMError.cpp \
    dom/DOMImplementation.cpp \
    dom/DOMNamedFlowCollection.cpp \
    dom/DOMStringList.cpp \
    dom/DOMStringMap.cpp \
    dom/DatasetDOMStringMap.cpp \
    dom/Element.cpp \
    dom/ElementRareData.cpp \
    dom/ElementShadow.cpp \
    dom/EntityReference.cpp \
    dom/ErrorEvent.cpp \
    dom/Event.cpp \
    dom/EventContext.cpp \
    dom/EventDispatchMediator.cpp \
    dom/EventDispatcher.cpp \
    dom/EventException.cpp \
    dom/EventListenerMap.cpp \
    dom/EventNames.cpp \
    dom/EventPathWalker.cpp \
    dom/EventRetargeter.cpp \
    dom/EventTarget.cpp \
    dom/ExceptionBase.cpp \
    dom/ExceptionCodePlaceholder.cpp \
    dom/FocusEvent.cpp \
    dom/GenericEventQueue.cpp \
    dom/GestureEvent.cpp \
    dom/IconURL.cpp \
    dom/IdTargetObserver.cpp \
    dom/IdTargetObserverRegistry.cpp \
    dom/LiveNodeList.cpp \
    dom/KeyboardEvent.cpp \
    dom/MessageChannel.cpp \
    dom/MessageEvent.cpp \
    dom/MessagePort.cpp \
    dom/MessagePortChannel.cpp \
    dom/MicroDataItemList.cpp \
    dom/MouseEvent.cpp \
    dom/MouseRelatedEvent.cpp \
    dom/MutationEvent.cpp \
    dom/MutationObserver.cpp \
    dom/MutationObserverInterestGroup.cpp \
    dom/MutationObserverRegistration.cpp \
    dom/MutationRecord.cpp \
    dom/WebKitNamedFlow.cpp \
    dom/NamedFlowCollection.cpp \
    dom/NamedNodeMap.cpp \
    dom/NameNodeList.cpp \
    dom/Node.cpp \
    dom/NodeFilterCondition.cpp \
    dom/NodeFilter.cpp \
    dom/NodeIterator.cpp \
    dom/NodeRareData.cpp \
    dom/NodeRenderingContext.cpp \
    dom/NodeRenderingTraversal.cpp \
    dom/NodeTraversal.cpp \
    dom/Notation.cpp \
    dom/OverflowEvent.cpp \
    dom/PageTransitionEvent.cpp \
    dom/PendingScript.cpp \
    dom/PopStateEvent.cpp \
    dom/Position.cpp \
    dom/PositionIterator.cpp \
    dom/ProcessingInstruction.cpp \
    dom/ProgressEvent.cpp \
    dom/PropertyNodeList.cpp \
    dom/PseudoElement.cpp \
    dom/QualifiedName.cpp \
    dom/Range.cpp \
    dom/RangeException.cpp \
    dom/RawDataDocumentParser.h \
    dom/RegisteredEventListener.cpp \
    dom/ScopedEventQueue.cpp \
    dom/ScriptedAnimationController.cpp \
    dom/ScriptableDocumentParser.cpp \
    dom/ScriptElement.cpp \
    dom/ScriptExecutionContext.cpp \
    dom/ScriptRunner.cpp \
    dom/SecurityContext.cpp \
    dom/SelectorQuery.cpp \
    dom/ShadowRoot.cpp \
    dom/SpaceSplitString.cpp \
    dom/StaticNodeList.cpp \
    dom/StyledElement.cpp \
    dom/StyleElement.cpp \
    dom/TagNodeList.cpp \
    dom/Text.cpp \
    dom/TextEvent.cpp \
    dom/Touch.cpp \
    dom/TouchEvent.cpp \
    dom/TouchList.cpp \
    dom/TransitionEvent.cpp \
    dom/Traversal.cpp \
    dom/TreeScope.cpp \
    dom/TreeScopeAdopter.cpp \
    dom/TreeWalker.cpp \
    dom/UIEvent.cpp \
    dom/UIEventWithKeyState.cpp \
    dom/UserActionElementSet.cpp \
    dom/UserGestureIndicator.cpp \
    dom/UserTypingGestureIndicator.cpp \
    dom/ViewportArguments.cpp \
    dom/VisitedLinkState.cpp \
    dom/WebKitAnimationEvent.cpp \
    dom/WebKitTransitionEvent.cpp \
    dom/WheelEvent.cpp \
    dom/WindowEventContext.cpp \
    dom/default/PlatformMessagePortChannel.cpp \
    editing/AlternativeTextController.cpp \
    editing/AppendNodeCommand.cpp \
    editing/ApplyBlockElementCommand.cpp \
    editing/ApplyStyleCommand.cpp \
    editing/BreakBlockquoteCommand.cpp \
    editing/CompositeEditCommand.cpp \
    editing/CreateLinkCommand.cpp \
    editing/DeleteButtonController.cpp \
    editing/DeleteButton.cpp \
    editing/DeleteFromTextNodeCommand.cpp \
    editing/DeleteSelectionCommand.cpp \
    editing/DictationAlternative.cpp \
    editing/DictationCommand.cpp \
    editing/EditCommand.cpp \
    editing/EditingStyle.cpp \
    editing/Editor.cpp \
    editing/EditorCommand.cpp \
    editing/FormatBlockCommand.cpp \
    editing/FrameSelection.cpp \
    editing/htmlediting.cpp \
    editing/HTMLInterchange.cpp \
    editing/IndentOutdentCommand.cpp \
    editing/InsertIntoTextNodeCommand.cpp \
    editing/InsertLineBreakCommand.cpp \
    editing/InsertListCommand.cpp \
    editing/InsertNodeBeforeCommand.cpp \
    editing/InsertParagraphSeparatorCommand.cpp \
    editing/InsertTextCommand.cpp \
    editing/markup.cpp \
    editing/MarkupAccumulator.cpp \
    editing/MergeIdenticalElementsCommand.cpp \
    editing/ModifySelectionListLevel.cpp \
    editing/MoveSelectionCommand.cpp \
    editing/RemoveCSSPropertyCommand.cpp \
    editing/RemoveFormatCommand.cpp \
    editing/RemoveNodeCommand.cpp \
    editing/RemoveNodePreservingChildrenCommand.cpp \
    editing/RenderedPosition.cpp \
    editing/ReplaceNodeWithSpanCommand.cpp \
    editing/ReplaceSelectionCommand.cpp \
    editing/SetNodeAttributeCommand.cpp \
    editing/SetSelectionCommand.cpp \
    editing/SimplifyMarkupCommand.cpp \
    editing/SpellChecker.cpp \
    editing/SpellingCorrectionCommand.cpp \
    editing/SplitElementCommand.cpp \
    editing/SplitTextNodeCommand.cpp \
    editing/SplitTextNodeContainingElementCommand.cpp \
    editing/TextCheckingHelper.cpp \
    editing/TextInsertionBaseCommand.cpp \
    editing/TextIterator.cpp \
    editing/TypingCommand.cpp \
    editing/UnlinkCommand.cpp \
    editing/VisiblePosition.cpp \
    editing/VisibleSelection.cpp \
    editing/VisibleUnits.cpp \
    editing/WrapContentsInDummySpanCommand.cpp \
    fileapi/AsyncFileStream.cpp \
    fileapi/AsyncFileStream.cpp \
    fileapi/Blob.cpp \
    fileapi/BlobURL.cpp \
    fileapi/File.cpp \
    fileapi/FileException.cpp \
    fileapi/FileList.cpp \
    fileapi/FileReader.cpp \
    fileapi/FileReaderLoader.cpp \
    fileapi/FileReaderSync.cpp \
    fileapi/FileThread.cpp \
    fileapi/ThreadableBlobRegistry.cpp \
    fileapi/WebKitBlobBuilder.cpp \
    history/BackForwardController.cpp \
    history/BackForwardListImpl.cpp \
    history/CachedFrame.cpp \
    history/CachedPage.cpp \
    history/HistoryItem.cpp \
    history/qt/HistoryItemQt.cpp \
    history/PageCache.cpp \
    html/BaseButtonInputType.cpp \
    html/BaseCheckableInputType.cpp \
    html/BaseChooserOnlyDateAndTimeInputType.cpp \
    html/BaseClickableWithKeyInputType.cpp \
    html/BaseDateAndTimeInputType.cpp \
    html/BaseTextInputType.cpp \
    html/ButtonInputType.cpp \
    html/CheckboxInputType.cpp \
    html/ClassList.cpp \
    html/ColorInputType.cpp \
    html/DOMFormData.cpp \
    html/DOMSettableTokenList.cpp \
    html/DOMTokenList.cpp \
    html/DOMURL.cpp \
    html/DateInputType.cpp \
    html/DateTimeInputType.cpp \
    html/DateTimeLocalInputType.cpp \
    html/EmailInputType.cpp \
    html/FTPDirectoryDocument.cpp \
    html/FileInputType.cpp \
    html/FormAssociatedElement.cpp \
    html/FormController.cpp \
    html/FormDataList.cpp \
    html/HTMLAllCollection.cpp \
    html/HTMLAnchorElement.cpp \
    html/HTMLAppletElement.cpp \
    html/HTMLAreaElement.cpp \
    html/HTMLBRElement.cpp \
    html/HTMLBaseElement.cpp \
    html/HTMLBaseFontElement.cpp \
    html/HTMLBodyElement.cpp \
    html/HTMLButtonElement.cpp \
    html/HTMLCanvasElement.cpp \
    html/HTMLCollection.cpp \
    html/HTMLDListElement.cpp \
    html/HTMLDataListElement.cpp \
    html/HTMLDialogElement.cpp \
    html/HTMLDirectoryElement.cpp \
    html/HTMLDetailsElement.cpp \
    html/HTMLDivElement.cpp \
    html/HTMLDocument.cpp \
    html/HTMLElement.cpp \
    html/HTMLEmbedElement.cpp \
    html/HTMLFieldSetElement.cpp \
    html/HTMLFontElement.cpp \
    html/HTMLFormControlsCollection.cpp \
    html/HTMLFormControlElement.cpp \
    html/HTMLFormControlElementWithState.cpp \
    html/HTMLFormElement.cpp \
    html/HTMLFrameElement.cpp \
    html/HTMLFrameElementBase.cpp \
    html/HTMLFrameOwnerElement.cpp \
    html/HTMLFrameSetElement.cpp \
    html/HTMLHRElement.cpp \
    html/HTMLHeadElement.cpp \
    html/HTMLHeadingElement.cpp \
    html/HTMLHtmlElement.cpp \
    html/HTMLIFrameElement.cpp \
    html/HTMLImageElement.cpp \
    html/HTMLImageLoader.cpp \
    html/HTMLInputElement.cpp \
    html/HTMLKeygenElement.cpp \
    html/HTMLLIElement.cpp \
    html/HTMLLabelElement.cpp \
    html/HTMLLegendElement.cpp \
    html/HTMLLinkElement.cpp \
    html/HTMLMapElement.cpp \
    html/HTMLMarqueeElement.cpp \
    html/HTMLMenuElement.cpp \
    html/HTMLMetaElement.cpp \
    html/HTMLMeterElement.cpp \
    html/HTMLModElement.cpp \
    html/HTMLNameCollection.cpp \
    html/HTMLOListElement.cpp \
    html/HTMLObjectElement.cpp \
    html/HTMLOptGroupElement.cpp \
    html/HTMLOptionElement.cpp \
    html/HTMLOptionsCollection.cpp \
    html/HTMLOutputElement.cpp \
    html/HTMLParagraphElement.cpp \
    html/HTMLParamElement.cpp \
    html/HTMLParserErrorCodes.cpp \
    html/HTMLPlugInElement.cpp \
    html/HTMLPlugInImageElement.cpp \
    html/HTMLPreElement.cpp \
    html/HTMLProgressElement.cpp \
    html/HTMLPropertiesCollection.cpp \
    html/HTMLQuoteElement.cpp \
    html/HTMLScriptElement.cpp \
    html/HTMLSelectElement.cpp \
    html/HTMLSpanElement.cpp \
    html/HTMLStyleElement.cpp \
    html/HTMLSummaryElement.cpp \
    html/HTMLTableCaptionElement.cpp \
    html/HTMLTableCellElement.cpp \
    html/HTMLTableColElement.cpp \
    html/HTMLTableElement.cpp \
    html/HTMLTablePartElement.cpp \
    html/HTMLTableRowElement.cpp \
    html/HTMLTableRowsCollection.cpp \
    html/HTMLTableSectionElement.cpp \
    html/HTMLTemplateElement.cpp \
    html/HTMLTextAreaElement.cpp \
    html/HTMLTextFormControlElement.cpp \
    html/HTMLTitleElement.cpp \
    html/HTMLUListElement.cpp \
    html/HTMLViewSourceDocument.cpp \
    html/HiddenInputType.cpp \
    html/ImageData.cpp \
    html/ImageDocument.cpp \
    html/ImageInputType.cpp \
    html/InputType.cpp \
    html/InputTypeNames.cpp \
    html/LabelableElement.cpp \
    html/LabelsNodeList.cpp \
    html/LinkRelAttribute.cpp \
    html/MediaDocument.cpp \
    html/MicroDataAttributeTokenList.cpp \
    html/MicroDataItemValue.cpp \
    html/MonthInputType.cpp \
    html/NumberInputType.cpp \
    html/PasswordInputType.cpp \
    html/PluginDocument.cpp \
    html/RadioInputType.cpp \
    html/RadioNodeList.cpp \
    html/RangeInputType.cpp \
    html/ResetInputType.cpp \
    html/SearchInputType.cpp \
    html/StepRange.cpp \
    html/SubmitInputType.cpp \
    html/TelephoneInputType.cpp \
    html/TextDocument.cpp \
    html/TextFieldInputType.cpp \
    html/TextInputType.cpp \
    html/TimeInputType.cpp \
    html/TypeAhead.cpp \
    html/URLInputType.cpp \
    html/ValidationMessage.cpp \
    html/ValidityState.cpp \
    html/WeekInputType.cpp \
    html/canvas/CanvasGradient.cpp \
    html/canvas/CanvasPathMethods.cpp \
    html/canvas/CanvasPattern.cpp \
    html/canvas/CanvasProxy.cpp \
    html/canvas/CanvasRenderingContext.cpp \
    html/canvas/CanvasRenderingContext2D.cpp \
    html/canvas/CanvasStyle.cpp \
    html/canvas/DataView.cpp \
    html/forms/FileIconLoader.cpp \
    html/parser/BackgroundHTMLInputStream.cpp \
    html/parser/BackgroundHTMLParser.cpp \
    html/parser/CSSPreloadScanner.cpp \
    html/parser/CompactHTMLToken.cpp \
    html/parser/HTMLConstructionSite.cpp \
    html/parser/HTMLDocumentParser.cpp \
    html/parser/HTMLElementStack.cpp \
    html/parser/HTMLEntityParser.cpp \
    html/parser/HTMLEntitySearch.cpp \
    html/parser/HTMLFormattingElementList.cpp \
    html/parser/HTMLIdentifier.cpp \
    html/parser/HTMLMetaCharsetParser.cpp \
    html/parser/HTMLParserIdioms.cpp \
    html/parser/HTMLParserOptions.cpp \
    html/parser/HTMLParserScheduler.cpp \
    html/parser/HTMLParserThread.cpp \
    html/parser/HTMLPreloadScanner.cpp \
    html/parser/HTMLResourcePreloader.cpp \
    html/parser/HTMLScriptRunner.cpp \
    html/parser/HTMLSourceTracker.cpp \
    html/parser/HTMLTokenizer.cpp \
    html/parser/HTMLTreeBuilder.cpp \
    html/parser/HTMLTreeBuilderSimulator.cpp \
    html/parser/HTMLViewSourceParser.cpp \
    html/parser/TextDocumentParser.cpp \
    html/parser/TextViewSourceParser.cpp \
    html/parser/XSSAuditor.cpp \
    html/parser/XSSAuditorDelegate.cpp \
    html/shadow/ContentDistributor.cpp \
    html/shadow/DetailsMarkerControl.cpp \
    html/shadow/HTMLContentElement.cpp \
    html/shadow/InsertionPoint.cpp \
    html/shadow/MediaControls.cpp \
    html/shadow/MediaControlsApple.cpp \
    html/shadow/MeterShadowElement.cpp \
    html/shadow/ProgressShadowElement.cpp \
    html/shadow/SliderThumbElement.cpp \
    html/shadow/SpinButtonElement.cpp \
    html/shadow/TextControlInnerElements.cpp \
    inspector/ConsoleMessage.cpp \
    inspector/ContentSearchUtils.cpp \
    inspector/DOMEditor.cpp \
    inspector/DOMPatchSupport.cpp \
    inspector/IdentifiersFactory.cpp \
    inspector/InjectedScript.cpp \
    inspector/InjectedScriptBase.cpp \
    inspector/InjectedScriptCanvasModule.cpp \
    inspector/InjectedScriptHost.cpp \
    inspector/InjectedScriptManager.cpp \
    inspector/InjectedScriptModule.cpp \
    inspector/InspectorAgent.cpp \
    inspector/InspectorApplicationCacheAgent.cpp \
    inspector/InspectorBaseAgent.cpp \
    inspector/InspectorCSSAgent.cpp \
    inspector/InspectorCanvasAgent.cpp \
    inspector/InspectorClient.cpp \
    inspector/InspectorConsoleAgent.cpp \
    inspector/InspectorController.cpp \
    inspector/InspectorCounters.cpp \
    inspector/InspectorDatabaseAgent.cpp \
    inspector/InspectorDatabaseResource.cpp \
    inspector/InspectorDebuggerAgent.cpp \
    inspector/InspectorDOMAgent.cpp \
    inspector/InspectorDOMDebuggerAgent.cpp \
    inspector/InspectorDOMStorageAgent.cpp \
    inspector/InspectorFrontendClientLocal.cpp \
    inspector/InspectorFrontendHost.cpp \
    inspector/InspectorHeapProfilerAgent.cpp \
    inspector/InspectorHistory.cpp \
    inspector/InspectorInputAgent.cpp \
    inspector/InspectorInstrumentation.cpp \
    inspector/InspectorLayerTreeAgent.cpp \
    inspector/InspectorMemoryAgent.cpp \
    inspector/InspectorOverlay.cpp \
    inspector/InspectorPageAgent.cpp \
    inspector/InspectorProfilerAgent.cpp \
    inspector/InspectorResourceAgent.cpp \
    inspector/InspectorRuntimeAgent.cpp \
    inspector/InspectorState.cpp \
    inspector/InspectorStyleSheet.cpp \
    inspector/InspectorStyleTextEditor.cpp \
    inspector/InspectorTimelineAgent.cpp \
    inspector/InspectorValues.cpp \
    inspector/InspectorWorkerAgent.cpp \
    inspector/InstrumentingAgents.cpp \
    inspector/NetworkResourcesData.cpp \
    inspector/PageConsoleAgent.cpp \
    inspector/PageDebuggerAgent.cpp \
    inspector/PageRuntimeAgent.cpp \
    inspector/ScriptArguments.cpp \
    inspector/ScriptCallFrame.cpp \
    inspector/ScriptCallStack.cpp \
    inspector/TimelineRecordFactory.cpp \
    inspector/TimelineTraceEventProcessor.cpp \
    inspector/WorkerConsoleAgent.cpp \
    inspector/WorkerDebuggerAgent.cpp \
    inspector/WorkerInspectorController.cpp \
    inspector/WorkerRuntimeAgent.cpp \
    loader/appcache/ApplicationCache.cpp \
    loader/appcache/ApplicationCacheGroup.cpp \
    loader/appcache/ApplicationCacheHost.cpp \
    loader/appcache/ApplicationCacheStorage.cpp \
    loader/appcache/ApplicationCacheResource.cpp \
    loader/appcache/DOMApplicationCache.cpp \
    loader/appcache/ManifestParser.cpp \
    loader/archive/ArchiveResource.cpp \
    loader/archive/ArchiveResourceCollection.cpp \
    loader/cache/MemoryCache.cpp \
    loader/cache/CachedCSSStyleSheet.cpp \
    loader/cache/CachedFont.cpp \
    loader/cache/CachedImage.cpp \
    loader/cache/CachedRawResource.cpp \
    loader/cache/CachedResourceHandle.cpp \
    loader/cache/CachedResource.cpp \
    loader/cache/CachedScript.cpp \
    loader/cache/CachedShader.cpp \
    loader/cache/CachedSVGDocument.cpp \
    loader/cache/CachedSVGDocumentReference.cpp \
    loader/cache/CachedXSLStyleSheet.cpp \
    loader/CookieJar.cpp \
    loader/CrossOriginAccessControl.cpp \
    loader/CrossOriginPreflightResultCache.cpp \
    loader/cache/CachedResourceLoader.cpp \
    loader/cache/CachedResourceRequest.cpp \
    loader/cache/CachedResourceRequestInitiators.cpp \
    loader/DocumentLoadTiming.cpp \
    loader/DocumentLoader.cpp \
    loader/DocumentThreadableLoader.cpp \
    loader/DocumentWriter.cpp \
    loader/EmptyClients.cpp \
    loader/FormState.cpp \
    loader/FormSubmission.cpp \
    loader/FrameLoadRequest.cpp \
    loader/FrameLoader.cpp \
    loader/FrameLoaderStateMachine.cpp \
    loader/HistoryController.cpp \
    loader/FTPDirectoryParser.cpp \
    loader/icon/IconController.cpp \
    loader/icon/IconDatabaseBase.cpp \
    loader/icon/IconLoader.cpp \
    loader/ImageLoader.cpp \
    loader/LinkLoader.cpp \
    loader/LoaderStrategy.cpp \
    loader/MixedContentChecker.cpp \
    loader/NavigationAction.cpp \
    loader/NetscapePlugInStreamLoader.cpp \
    loader/PingLoader.cpp \
    loader/PlaceholderDocument.cpp \
    loader/PolicyCallback.cpp \
    loader/PolicyChecker.cpp \
    loader/ProgressTracker.cpp \
    loader/NavigationScheduler.cpp \
    loader/ResourceBuffer.cpp \
    loader/ResourceLoader.cpp \
    loader/ResourceLoadNotifier.cpp \
    loader/ResourceLoadScheduler.cpp \
    loader/SinkDocument.cpp \
    loader/SubframeLoader.cpp \
    loader/SubresourceLoader.cpp \
    loader/TextResourceDecoder.cpp \
    loader/ThreadableLoader.cpp \
    page/animation/AnimationBase.cpp \
    page/animation/AnimationController.cpp \
    page/animation/CompositeAnimation.cpp \
    page/animation/CSSPropertyAnimation.cpp \
    page/animation/ImplicitAnimation.cpp \
    page/animation/KeyframeAnimation.cpp \
    page/AutoscrollController.cpp \
    page/BarProp.cpp \
    page/CaptionUserPreferences.cpp \
    page/Chrome.cpp \
    page/Console.cpp \
    page/ContentSecurityPolicy.cpp \
    page/ContextMenuController.cpp \
    page/Crypto.cpp \
    page/DeviceController.cpp \
    page/DiagnosticLoggingKeys.cpp \
    page/DOMSelection.cpp \
    page/DOMTimer.cpp \
    page/DOMWindow.cpp \
    page/DOMWindowExtension.cpp \
    page/DOMWindowProperty.cpp \
    page/DragController.cpp \
    page/EventHandler.cpp \
    page/EventSource.cpp \
    page/FeatureObserver.cpp \
    page/FocusController.cpp \
    page/Frame.cpp \
    page/FrameActionScheduler.cpp \
    page/FrameDestructionObserver.cpp \
    page/FrameTree.cpp \
    page/FrameView.cpp \
    page/GestureTapHighlighter.cpp \
    page/GroupSettings.cpp \
    page/History.cpp \
    page/Location.cpp \
    page/MouseEventWithHitTestResults.cpp \
    page/Navigator.cpp \
    page/NavigatorBase.cpp \
    page/OriginAccessEntry.cpp \
    page/Page.cpp \
    page/PageActivityAssertionToken.cpp \
    page/PageConsole.cpp \
    page/PageGroup.cpp \
    page/PageGroupLoadDeferrer.cpp \
    page/PageThrottler.cpp \
    page/PageVisibilityState.cpp \
    page/Performance.cpp \
    page/PerformanceEntry.cpp \
    page/PerformanceEntryList.cpp \
    page/PerformanceNavigation.cpp \
    page/PerformanceResourceTiming.cpp \
    page/PerformanceTiming.cpp \
    page/PrintContext.cpp \
    page/Screen.cpp \
    page/scrolling/ScrollingConstraints.cpp \
    page/scrolling/ScrollingCoordinator.cpp \
    page/SecurityOrigin.cpp \
    page/SecurityPolicy.cpp \
    page/Settings.cpp \
    page/SpatialNavigation.cpp \
    page/TouchAdjustment.cpp \
    page/SuspendableTimer.cpp \
    page/UserContentURLPattern.cpp \
    page/WindowFeatures.cpp \
    page/WindowFocusAllowedIndicator.cpp \
    plugins/PluginData.cpp \
    plugins/DOMPluginArray.cpp \
    plugins/DOMPlugin.cpp \
    plugins/PluginMainThreadScheduler.cpp \
    plugins/DOMMimeType.cpp \
    plugins/DOMMimeTypeArray.cpp \
    platform/animation/Animation.cpp \
    platform/animation/AnimationList.cpp \
    platform/Arena.cpp \
    platform/text/BidiContext.cpp \
    platform/text/DateTimeFormat.cpp \
    platform/text/Hyphenation.cpp \
    platform/text/LocaleNone.cpp \
    platform/text/LocaleToScriptMappingDefault.cpp \
    platform/text/PlatformLocale.cpp \
    platform/text/QuotedPrintable.cpp \
    platform/CalculationValue.cpp \
    platform/Clock.cpp \
    platform/ClockGeneric.cpp \
    platform/ContentType.cpp \
    platform/CrossThreadCopier.cpp \
    platform/DatabaseStrategy.cpp \
    platform/DateComponents.cpp \
    platform/Decimal.cpp \
    platform/DragData.cpp \
    platform/DragImage.cpp \
    platform/FileChooser.cpp \
    platform/FileStream.cpp \
    platform/FileSystem.cpp \
    platform/HistogramSupport.cpp \
    platform/graphics/FontDescription.cpp \
    platform/graphics/FontGenericFamilies.cpp \
    platform/graphics/FontGlyphs.cpp \
    platform/graphics/FontFeatureSettings.cpp \
    platform/graphics/BitmapImage.cpp \
    platform/graphics/Color.cpp \
    platform/graphics/CrossfadeGeneratedImage.cpp \
    platform/graphics/FloatPoint3D.cpp \
    platform/graphics/FloatPoint.cpp \
    platform/graphics/FloatPolygon.cpp \
    platform/graphics/FloatQuad.cpp \
    platform/graphics/FloatRect.cpp \
    platform/graphics/FloatSize.cpp \
    platform/graphics/FontData.cpp \
    platform/graphics/Font.cpp \
    platform/graphics/FontCache.cpp \
    platform/graphics/FontFastPath.cpp \
    platform/graphics/LayoutBoxExtent.cpp \
    platform/graphics/LayoutRect.cpp \
    platform/graphics/GeneratedImage.cpp \
    platform/graphics/GeneratorGeneratedImage.cpp \
    platform/graphics/GlyphPageTreeNode.cpp \
    platform/graphics/Gradient.cpp \
    platform/graphics/GraphicsContext.cpp \
    platform/graphics/GraphicsLayer.cpp \
    platform/graphics/GraphicsLayerAnimation.cpp \
    platform/graphics/GraphicsLayerUpdater.cpp \
    platform/graphics/GraphicsLayerTransform.cpp \
    platform/graphics/GraphicsTypes.cpp \
    platform/graphics/Image.cpp \
    platform/graphics/ImageBuffer.cpp \
    platform/graphics/ImageOrientation.cpp \
    platform/graphics/ImageSource.cpp \
    platform/graphics/IntRect.cpp \
    platform/graphics/Path.cpp \
    platform/graphics/PathTraversalState.cpp \
    platform/graphics/Pattern.cpp \
    platform/graphics/qt/FontQt.cpp \
    platform/graphics/Region.cpp \
    platform/graphics/RoundedRect.cpp \
    platform/graphics/SegmentedFontData.cpp \
    platform/graphics/ShadowBlur.cpp \
    platform/graphics/SVGGlyph.cpp \
    platform/graphics/SimpleFontData.cpp \
    platform/graphics/StringTruncator.cpp \
    platform/graphics/surfaces/GraphicsSurface.cpp \
    platform/graphics/surfaces/qt/GraphicsSurfaceQt.cpp \
    platform/graphics/SurrogatePairAwareTextIterator.cpp \
    platform/graphics/TextRun.cpp \
    platform/graphics/TiledBackingStore.cpp \
    platform/graphics/transforms/AffineTransform.cpp \
    platform/graphics/transforms/TransformationMatrix.cpp \
    platform/graphics/transforms/MatrixTransformOperation.cpp \
    platform/graphics/transforms/Matrix3DTransformOperation.cpp \
    platform/graphics/transforms/PerspectiveTransformOperation.cpp \
    platform/graphics/transforms/RotateTransformOperation.cpp \
    platform/graphics/transforms/ScaleTransformOperation.cpp \
    platform/graphics/transforms/SkewTransformOperation.cpp \
    platform/graphics/transforms/TransformOperations.cpp \
    platform/graphics/transforms/TransformState.cpp \
    platform/graphics/transforms/TranslateTransformOperation.cpp \
    platform/graphics/WidthIterator.cpp \
    platform/image-decoders/ImageDecoder.cpp \
    platform/image-decoders/bmp/BMPImageDecoder.cpp \
    platform/image-decoders/bmp/BMPImageReader.cpp \
    platform/image-decoders/gif/GIFImageDecoder.cpp \
    platform/image-decoders/gif/GIFImageReader.cpp\
    platform/KillRingNone.cpp \
    platform/KURL.cpp \
    platform/Language.cpp \
    platform/Length.cpp \
    platform/LengthBox.cpp \
    platform/text/LineEnding.cpp \
    platform/leveldb/LevelDBDatabase.cpp \
    platform/leveldb/LevelDBTransaction.cpp \
    platform/leveldb/LevelDBWriteBatch.cpp \
    platform/LinkHash.cpp \
    platform/Logging.cpp \
    platform/MemoryPressureHandler.cpp \
    platform/MIMETypeRegistry.cpp \
    platform/mock/DeviceMotionClientMock.cpp \
    platform/mock/DeviceOrientationClientMock.cpp \
    platform/mock/GeolocationClientMock.cpp \
    platform/mock/PlatformSpeechSynthesizerMock.cpp \
    platform/mock/ScrollbarThemeMock.cpp \
    platform/network/AuthenticationChallengeBase.cpp \
    platform/network/BlobData.cpp \
    platform/network/BlobRegistry.cpp \
    platform/network/BlobRegistryImpl.cpp \
    platform/network/BlobResourceHandle.cpp \
    platform/network/Credential.cpp \
    platform/network/CredentialStorage.cpp \
    platform/network/FormData.cpp \
    platform/network/FormDataBuilder.cpp \
    platform/network/HTTPHeaderMap.cpp \
    platform/network/HTTPParsers.cpp \
    platform/network/MIMEHeader.cpp \
    platform/network/NetworkStateNotifier.cpp \
    platform/network/NetworkStorageSessionStub.cpp \
    platform/network/ParsedContentType.cpp \
    platform/network/ProtectionSpace.cpp \
    platform/network/ProxyServer.cpp \
    platform/network/ResourceErrorBase.cpp \
    platform/network/ResourceHandle.cpp \
    platform/network/ResourceHandleClient.cpp \
    platform/network/ResourceLoadTiming.cpp \
    platform/network/ResourceRequestBase.cpp \
    platform/network/ResourceResponseBase.cpp \
    platform/NotImplemented.cpp \
    platform/text/RegularExpression.cpp \
    platform/PlatformEvent.cpp \
    platform/PlatformInstrumentation.cpp \
    platform/RuntimeApplicationChecks.cpp \
    platform/RunLoop.cpp \
    platform/SchemeRegistry.cpp \
    platform/ScrollableArea.cpp \
    platform/ScrollAnimator.cpp \
    platform/Scrollbar.cpp \
    platform/ScrollbarTheme.cpp \
    platform/ScrollbarThemeComposite.cpp \
    platform/ScrollView.cpp \
    platform/SharedBuffer.cpp \
    platform/SharedBufferChunkReader.cpp \
    platform/sql/SQLiteAuthorizer.cpp \
    platform/sql/SQLiteDatabase.cpp \
    platform/sql/SQLiteFileSystem.cpp \
    platform/sql/SQLiteStatement.cpp \
    platform/sql/SQLiteTransaction.cpp \
    platform/sql/SQLValue.cpp \
    platform/text/SegmentedString.cpp \
    platform/text/TextBoundaries.cpp \
    platform/text/TextBreakIterator.cpp \
    platform/text/TextCodec.cpp \
    platform/text/TextCodecLatin1.cpp \
    platform/text/TextCodecUserDefined.cpp \
    platform/text/TextCodecUTF16.cpp \
    platform/text/TextCodecUTF8.cpp \
    platform/text/TextCodecICU.cpp \
    platform/text/TextEncoding.cpp \
    platform/text/TextEncodingDetectorICU.cpp \
    platform/text/TextEncodingRegistry.cpp \
    platform/text/TextStream.cpp \
    platform/ThreadGlobalData.cpp \
    platform/ThreadTimers.cpp \
    platform/Timer.cpp \
    platform/text/UnicodeRange.cpp \
    platform/text/transcoder/FontTranscoder.cpp \
    platform/UUID.cpp \
    platform/Widget.cpp \
    platform/PlatformStrategies.cpp \
    plugins/IFrameShimSupport.cpp \
    plugins/PluginDatabase.cpp \
    plugins/PluginDebug.cpp \
    plugins/PluginPackage.cpp \
    plugins/PluginStream.cpp \
    plugins/PluginView.cpp \
    rendering/AutoTableLayout.cpp \
    rendering/break_lines.cpp \
    rendering/BidiRun.cpp \
    rendering/CounterNode.cpp \
    rendering/EllipsisBox.cpp \
    rendering/FilterEffectRenderer.cpp \
    rendering/FixedTableLayout.cpp \
    rendering/FlowThreadController.cpp \
    rendering/HitTestingTransformState.cpp \
    rendering/HitTestLocation.cpp \
    rendering/HitTestResult.cpp \
    rendering/InlineBox.cpp \
    rendering/InlineFlowBox.cpp \
    rendering/InlineTextBox.cpp \
    rendering/LayoutState.cpp \
    rendering/LayoutRepainter.cpp \
    rendering/RenderApplet.cpp \
    rendering/RenderArena.cpp \
    rendering/RenderBlock.cpp \
    rendering/RenderBlockLineLayout.cpp \
    rendering/RenderBox.cpp \
    rendering/RenderBoxModelObject.cpp \
    rendering/RenderBR.cpp \
    rendering/RenderButton.cpp \
    rendering/RenderCombineText.cpp \
    rendering/RenderCounter.cpp \
    rendering/RenderDeprecatedFlexibleBox.cpp \
    rendering/RenderDetailsMarker.cpp \
    rendering/RenderDialog.cpp \
    rendering/RenderEmbeddedObject.cpp \
    rendering/RenderFieldset.cpp \
    rendering/RenderFileUploadControl.cpp \
    rendering/RenderFlexibleBox.cpp \
    rendering/RenderFlowThread.cpp \
    rendering/RenderFrame.cpp \
    rendering/RenderFrameBase.cpp \
    rendering/RenderFrameSet.cpp \
    rendering/RenderGeometryMap.cpp \
    rendering/RenderGrid.cpp \
    rendering/RenderHTMLCanvas.cpp \
    rendering/RenderIFrame.cpp \
    rendering/RenderImage.cpp \
    rendering/RenderImageResource.cpp \
    rendering/RenderImageResourceStyleImage.cpp \
    rendering/RenderInline.cpp \
    rendering/RenderLayer.cpp \
    rendering/RenderLayerBacking.cpp \
    rendering/RenderLayerCompositor.cpp \
    rendering/RenderLayerFilterInfo.cpp \
    rendering/RenderLayerModelObject.cpp \
    rendering/RenderLineBoxList.cpp \
    rendering/RenderListBox.cpp \
    rendering/RenderListItem.cpp \
    rendering/RenderListMarker.cpp \
    rendering/RenderMarquee.cpp \
    rendering/RenderMenuList.cpp \
    rendering/RenderMeter.cpp \
    rendering/RenderMultiColumnBlock.cpp \
    rendering/RenderMultiColumnFlowThread.cpp \
    rendering/RenderMultiColumnSet.cpp \
    rendering/RenderNamedFlowThread.cpp \
    rendering/RenderObject.cpp \
    rendering/RenderObjectChildList.cpp \
    rendering/RenderPart.cpp \
    rendering/RenderProgress.cpp \
    rendering/RenderQuote.cpp \
    rendering/RenderRegion.cpp \
    rendering/RenderRegionSet.cpp \
    rendering/RenderReplaced.cpp \
    rendering/RenderReplica.cpp \
    rendering/RenderRuby.cpp \
    rendering/RenderRubyBase.cpp \
    rendering/RenderRubyRun.cpp \
    rendering/RenderRubyText.cpp \
    rendering/RenderScrollbar.cpp \
    rendering/RenderScrollbarPart.cpp \
    rendering/RenderScrollbarTheme.cpp \
    rendering/RenderSearchField.cpp \
    rendering/RenderSlider.cpp \
    rendering/RenderSnapshottedPlugIn.cpp \
    rendering/RenderTable.cpp \
    rendering/RenderTableCaption.cpp \
    rendering/RenderTableCell.cpp \
    rendering/RenderTableCol.cpp \
    rendering/RenderTableRow.cpp \
    rendering/RenderTableSection.cpp \
    rendering/RenderText.cpp \
    rendering/RenderTextControl.cpp \
    rendering/RenderTextControlMultiLine.cpp \
    rendering/RenderTextControlSingleLine.cpp \
    rendering/RenderTextFragment.cpp \
    rendering/RenderTheme.cpp \
    rendering/RenderTreeAsText.cpp \
    rendering/RenderView.cpp \
    rendering/RenderWidget.cpp \
    rendering/RenderWordBreak.cpp \
    rendering/RootInlineBox.cpp \
    rendering/ScrollBehavior.cpp \
    rendering/shapes/PolygonShape.cpp \
    rendering/shapes/RectangleShape.cpp \
    rendering/shapes/Shape.cpp \
    rendering/shapes/ShapeInfo.cpp \
    rendering/shapes/ShapeInsideInfo.cpp \
    rendering/shapes/ShapeInterval.cpp \
    rendering/shapes/ShapeOutsideInfo.cpp \
    rendering/style/BasicShapes.cpp \
    rendering/style/ContentData.cpp \
    rendering/style/CounterDirectives.cpp \
    rendering/style/FillLayer.cpp \
    rendering/style/KeyframeList.cpp \
    rendering/style/NinePieceImage.cpp \
    rendering/style/QuotesData.cpp \
    rendering/style/RenderStyle.cpp \
    rendering/style/ShadowData.cpp \
    rendering/style/StyleBackgroundData.cpp \
    rendering/style/StyleBoxData.cpp \
    rendering/style/StyleCachedImage.cpp \
    rendering/style/StyleCachedImageSet.cpp \
    rendering/style/StyleCachedShader.cpp \
    rendering/style/StyleCustomFilterProgram.cpp \
    rendering/style/StyleCustomFilterProgramCache.cpp \
    rendering/style/StyleDeprecatedFlexibleBoxData.cpp \
    rendering/style/StyleFilterData.cpp \
    rendering/style/StyleFlexibleBoxData.cpp \
    rendering/style/StyleGeneratedImage.cpp \
    rendering/style/StyleGridData.cpp \
    rendering/style/StyleGridItemData.cpp \
    rendering/style/StyleInheritedData.cpp \
    rendering/style/StyleMarqueeData.cpp \
    rendering/style/StyleMultiColData.cpp \
    rendering/style/StyleRareInheritedData.cpp \
    rendering/style/StyleRareNonInheritedData.cpp \
    rendering/style/StyleSurroundData.cpp \
    rendering/style/StyleTransformData.cpp \
    rendering/style/StyleVisualData.cpp \
    storage/StorageThread.cpp \
    storage/Storage.cpp \
    storage/StorageAreaImpl.cpp \
    storage/StorageAreaSync.cpp \
    storage/StorageEvent.cpp \
    storage/StorageEventDispatcher.cpp \
    storage/StorageMap.cpp \
    storage/StorageNamespace.cpp \
    storage/StorageNamespaceImpl.cpp \
    storage/StorageSyncManager.cpp \
    storage/StorageStrategy.cpp \
    storage/StorageTracker.cpp \
    testing/Internals.cpp \
    testing/InternalSettings.cpp \
    xml/DOMParser.cpp \
    xml/NativeXPathNSResolver.cpp \
    xml/XMLHttpRequest.cpp \
    xml/XMLHttpRequestException.cpp \
    xml/XMLHttpRequestProgressEventThrottle.cpp \
    xml/XMLHttpRequestUpload.cpp \
    xml/XMLErrors.cpp \
    xml/XMLSerializer.cpp \
    xml/XPathEvaluator.cpp \
    xml/XPathException.cpp \
    xml/XPathExpression.cpp \
    xml/XPathExpressionNode.cpp \
    xml/XPathFunctions.cpp \
    xml/XPathNodeSet.cpp \
    xml/XPathNSResolver.cpp \
    xml/XPathParser.cpp \
    xml/XPathPath.cpp \
    xml/XPathPredicate.cpp \
    xml/XPathResult.cpp \
    xml/XPathStep.cpp \
    xml/XPathUtil.cpp \
    xml/XPathValue.cpp \
    xml/XPathVariableReference.cpp \
    xml/parser/XMLDocumentParser.cpp \

HEADERS += \
    accessibility/AccessibilityARIAGridCell.h \
    accessibility/AccessibilityARIAGrid.h \
    accessibility/AccessibilityARIAGridRow.h \
    accessibility/AccessibilityImageMapLink.h \
    accessibility/AccessibilityListBox.h \
    accessibility/AccessibilityListBoxOption.h \
    accessibility/AccessibilityList.h \
    accessibility/AccessibilityMediaControls.h \
    accessibility/AccessibilityObject.h \
    accessibility/AccessibilityProgressIndicator.h \
    accessibility/AccessibilityRenderObject.h \
    accessibility/AccessibilityScrollbar.h \
    accessibility/AccessibilityScrollView.h \
    accessibility/AccessibilitySlider.h \
    accessibility/AccessibilitySpinButton.h \
    accessibility/AccessibilityTableCell.h \
    accessibility/AccessibilityTableColumn.h \
    accessibility/AccessibilityTable.h \
    accessibility/AccessibilityTableHeaderContainer.h \
    accessibility/AccessibilityTableRow.h \
    accessibility/AXObjectCache.h \
    bindings/ScriptControllerBase.h \
    bindings/generic/ActiveDOMCallback.h \
    bindings/generic/BindingSecurity.h \
    bindings/generic/RuntimeEnabledFeatures.h

HEADERS += \
    bindings/js/BindingState.h \
    bindings/js/CachedScriptSourceProvider.h \
    bindings/js/CallbackFunction.h \
    bindings/js/GCController.h \
    bindings/js/DOMObjectHashTableMap.h \
    bindings/js/DOMWrapperWorld.h \
    bindings/js/JSArrayBufferViewHelper.h \
    bindings/js/JSCSSStyleDeclarationCustom.h \
    bindings/js/JSCallbackData.h \
    bindings/js/JSCustomXPathNSResolver.h \
    bindings/js/JSDictionary.h \
    bindings/js/JSDOMBinding.h \
    bindings/js/JSDOMGlobalObject.h \
    bindings/js/JSDOMWindowBase.h \
    bindings/js/JSDOMWindowCustom.h \
    bindings/js/JSDOMWindowShell.h \
    bindings/js/JSDOMWrapper.h \
    bindings/js/JSErrorHandler.h \
    bindings/js/JSEventListener.h \
    bindings/js/JSHTMLInputElementCustom.h \
    bindings/js/JSHTMLSelectElementCustom.h \
    bindings/js/JSImageConstructor.h \
    bindings/js/JSLazyEventListener.h \
    bindings/js/JSMessagePortCustom.h \
    bindings/js/JSMutationCallback.h \
    bindings/js/JSNodeCustom.h \
    bindings/js/JSNodeFilterCondition.h \
    bindings/js/JSPluginElementFunctions.h \
    bindings/js/JSWorkerGlobalScopeBase.h \
    bindings/js/JavaScriptCallFrame.h \
    bindings/js/PageScriptDebugServer.h \
    bindings/js/ScheduledAction.h \
    bindings/js/ScriptCachedFrameData.h \
    bindings/js/ScriptController.h \
    bindings/js/ScriptDebugServer.h \
    bindings/js/ScriptEventListener.h \
    bindings/js/ScriptFunctionCall.h \
    bindings/js/ScriptGCEvent.h \
    bindings/js/ScriptHeapSnapshot.h \
    bindings/js/ScriptObject.h \
    bindings/js/ScriptProfile.h \
    bindings/js/ScriptProfileNode.h \
    bindings/js/ScriptProfiler.h \
    bindings/js/ScriptSourceCode.h \
    bindings/js/ScriptState.h \
    bindings/js/ScriptValue.h \
    bindings/js/ScriptWrappable.h \
    bindings/js/SerializedScriptValue.h \
    bindings/js/WebCoreJSClientData.h \
    bindings/js/WorkerScriptController.h \
    bindings/js/WorkerScriptDebugServer.h \
    bridge/Bridge.h \
    bridge/c/CRuntimeObject.h \
    bridge/c/c_class.h \
    bridge/c/c_instance.h \
    bridge/c/c_runtime.h \
    bridge/c/c_utility.h \
    bridge/jsc/BridgeJSC.h \
    bridge/IdentifierRep.h \
    bridge/NP_jsobject.h \
    bridge/qt/qt_class.h \
    bridge/qt/qt_instance.h \
    bridge/qt/qt_runtime.h \
    bridge/qt/qt_pixmapruntime.h \
    bridge/runtime_array.h \
    bridge/runtime_method.h \
    bridge/runtime_object.h \
    bridge/runtime_root.h \
    plugins/npruntime.h

HEADERS += \
    Modules/geolocation/Coordinates.h \
    Modules/geolocation/Geolocation.h \
    Modules/geolocation/GeolocationController.h \
    Modules/geolocation/GeolocationError.h \
    Modules/geolocation/GeolocationPosition.h \
    Modules/geolocation/Geoposition.h \
    Modules/geolocation/PositionCallback.h \
    Modules/geolocation/PositionError.h \
    Modules/geolocation/PositionErrorCallback.h \
    Modules/geolocation/PositionOptions.h \
    \
    Modules/notifications/DOMWindowNotifications.h \
    Modules/notifications/Notification.h \
    Modules/notifications/NotificationCenter.h \
    Modules/notifications/NotificationClient.h \
    Modules/notifications/NotificationController.h \
    Modules/notifications/WorkerGlobalScopeNotifications.h \
    \
    Modules/proximity/DeviceProximityClient.h \
    Modules/proximity/DeviceProximityController.h \
    Modules/proximity/DeviceProximityEvent.h \
    \
    Modules/webdatabase/AbstractDatabaseServer.h \
    Modules/webdatabase/AbstractSQLStatement.h \
    Modules/webdatabase/AbstractSQLStatementBackend.h \
    Modules/webdatabase/ChangeVersionData.h \
    Modules/webdatabase/ChangeVersionWrapper.h \
    Modules/webdatabase/DOMWindowWebDatabase.h \
    Modules/webdatabase/DatabaseAuthorizer.h \
    Modules/webdatabase/Database.h \
    Modules/webdatabase/DatabaseBackend.h \
    Modules/webdatabase/DatabaseBackendBase.h \
    Modules/webdatabase/DatabaseBackendContext.h \
    Modules/webdatabase/DatabaseBackendSync.h \
    Modules/webdatabase/DatabaseBase.h \
    Modules/webdatabase/DatabaseBasicTypes.h \
    Modules/webdatabase/DatabaseCallback.h \
    Modules/webdatabase/DatabaseError.h \
    Modules/webdatabase/DatabaseManager.h \
    Modules/webdatabase/DatabaseServer.h \
    Modules/webdatabase/DatabaseSync.h \
    Modules/webdatabase/DatabaseTask.h \
    Modules/webdatabase/DatabaseThread.h \
    Modules/webdatabase/DatabaseTracker.h \
    Modules/webdatabase/OriginLock.h \
    Modules/webdatabase/SQLCallbackWrapper.h \
    Modules/webdatabase/SQLResultSet.h \
    Modules/webdatabase/SQLResultSetRowList.h \
    Modules/webdatabase/SQLStatement.h \
    Modules/webdatabase/SQLStatementBackend.h \
    Modules/webdatabase/SQLStatementSync.h \
    Modules/webdatabase/SQLTransaction.h \
    Modules/webdatabase/SQLTransactionBackend.h \
    Modules/webdatabase/SQLTransactionBackendSync.h \
    Modules/webdatabase/SQLTransactionClient.h \
    Modules/webdatabase/SQLTransactionCoordinator.h \
    Modules/webdatabase/SQLTransactionState.h \
    Modules/webdatabase/SQLTransactionStateMachine.h \
    Modules/webdatabase/SQLTransactionSync.h \
    Modules/webdatabase/SQLTransactionSyncCallback.h \
    Modules/webdatabase/WorkerGlobalScopeWebDatabase.h \
    \
    css/BasicShapeFunctions.h \
    css/CSSAspectRatioValue.h \
    css/CSSBasicShapes.h \
    css/CSSBorderImageSliceValue.h \
    css/CSSBorderImage.h \
    css/CSSCalculationValue.h \
    css/CSSCanvasValue.h \
    css/CSSCharsetRule.h \
    css/CSSComputedStyleDeclaration.h \
    css/CSSCrossfadeValue.h \
    css/CSSCursorImageValue.h \
    css/CSSFontFace.h \
    css/CSSFontFaceLoadEvent.h \
    css/CSSFontFaceRule.h \
    css/CSSFontFaceSource.h \
    css/CSSFontFaceSrcValue.h \
    css/CSSFontSelector.h \
    css/CSSFunctionValue.h \
    css/CSSGradientValue.h \
    css/CSSGroupingRule.h \
    css/CSSHelper.h \
    css/CSSImageGeneratorValue.h \
    css/CSSImageValue.h \
    css/CSSImportRule.h \
    css/CSSInheritedValue.h \
    css/CSSInitialValue.h \
    css/CSSMediaRule.h \
    css/CSSOMUtils.h \
    css/CSSPageRule.h \
    css/CSSParser.h \
    css/CSSParserMode.h \
    css/CSSParserValues.h \
    css/CSSPrimitiveValue.h \
    css/CSSProperty.h \
    css/CSSReflectValue.h \
    css/CSSRule.h \
    css/CSSRuleList.h \
    css/CSSSegmentedFontFace.h \
    css/CSSSelector.h \
    css/CSSSelectorList.h \
    css/CSSStyleDeclaration.h \
    css/CSSStyleRule.h \
    css/CSSStyleSheet.h \
    css/CSSSupportsRule.h \
    css/CSSTimingFunctionValue.h \
    css/CSSToStyleMap.h \
    css/CSSUnicodeRangeValue.h \
    css/CSSValue.cpp \
    css/CSSValue.h \
    css/CSSValueList.h \
    css/CSSValuePool.h \
    css/CSSVariableValue.h \
    css/DeprecatedStyleBuilder.h \
    css/DOMWindowCSS.h \
    css/FontFeatureValue.h \
    css/FontLoader.h \
    css/FontValue.h \
    css/LengthFunctions.h \
    css/MediaFeatureNames.h \
    css/MediaList.h \
    css/MediaQuery.h \
    css/MediaQueryEvaluator.h \
    css/MediaQueryExp.h \
    css/MediaQueryList.h \
    css/MediaQueryListListener.h \
    css/MediaQueryMatcher.h \
    css/RGBColor.h \
    css/SelectorChecker.h \
    css/ShadowValue.h \
    css/StyleMedia.h \
    css/StyleInvalidationAnalysis.h \
    css/StylePropertySet.h \
    css/StylePropertyShorthand.h \
    css/StyleResolver.h \
    css/StyleRule.h \
    css/StyleRuleImport.h \
    css/StyleSheet.h \
    css/StyleSheetContents.h \
    css/StyleSheetList.h \
    css/TransformFunctions.h \
    css/ViewportStyleResolver.h \
    css/WebKitCSSArrayFunctionValue.h \
    css/WebKitCSSFilterRule.h \
    css/WebKitCSSFilterValue.h \
    css/WebKitCSSKeyframeRule.h \
    css/WebKitCSSKeyframesRule.h \
    css/WebKitCSSMatrix.h \
    css/WebKitCSSMatFunctionValue.h \
    css/WebKitCSSMixFunctionValue.h \
    css/WebKitCSSRegionRule.h \
    css/WebKitCSSSVGDocumentValue.h \
    css/WebKitCSSShaderValue.h \
    css/WebKitCSSTransformValue.h \
    css/WebKitCSSViewportRule.h \
    dom/ActiveDOMObject.h \
    dom/Attr.h \
    dom/Attribute.h \
    dom/BeforeTextInsertedEvent.h \
    dom/BeforeUnloadEvent.h \
    dom/CDATASection.h \
    dom/CharacterData.h \
    dom/CheckedRadioButtons.h \
    dom/ChildNodeList.h \
    dom/ClassNodeList.h \
    dom/ClientRect.h \
    dom/ClientRectList.h \
    dom/ClipboardEvent.h \
    dom/Clipboard.h \
    dom/Comment.h \
    dom/ComposedShadowTreeWalker.h \
    dom/ContainerNode.h \
    dom/ContainerNodeAlgorithms.h \
    dom/ContextFeatures.h \
    dom/CustomEvent.h \
    dom/default/PlatformMessagePortChannel.h \
    dom/DeviceMotionClient.h \
    dom/DeviceMotionController.h \
    dom/DeviceMotionData.h \
    dom/DeviceMotionEvent.h \
    dom/DeviceOrientationClient.h \
    dom/DeviceOrientationController.h \
    dom/DeviceOrientationData.h \
    dom/DeviceOrientationEvent.h \
    dom/Document.h \
    dom/DocumentFragment.h \
    dom/DocumentMarker.h \
    dom/DocumentMarkerController.h \
    dom/DocumentOrderedMap.h \
    dom/DocumentSharedObjectPool.h \
    dom/DocumentStyleSheetCollection.h \
    dom/DocumentType.h \
    dom/DOMError.h \
    dom/DOMImplementation.h \
    dom/DOMStringList.h \
    dom/DOMStringMap.h \
    dom/DOMTimeStamp.h \
    dom/DatasetDOMStringMap.h \
    dom/Element.h \
    dom/ElementShadow.h \
    dom/Entity.h \
    dom/EntityReference.h \
    dom/Event.h \
    dom/EventDispatchMediator.h \
    dom/EventListenerMap.h \
    dom/EventNames.h \
    dom/EventPathWalker.h \
    dom/EventQueue.h \
    dom/EventSender.h \
    dom/EventTarget.h \
    dom/ExceptionBase.h \
    dom/ExceptionCode.h \
    dom/FocusEvent.h \
    dom/FragmentScriptingPermission.h \
    dom/GestureEvent.h \
    dom/IdTargetObserver.h \
    dom/IdTargetObserverRegistry.h \
    dom/LiveNodeList.h \
    dom/KeyboardEvent.h \
    dom/MessageChannel.h \
    dom/MessageEvent.h \
    dom/MessagePortChannel.h \
    dom/MessagePort.h \
    dom/MicroDataItemList.h \
    dom/MouseEvent.h \
    dom/MouseRelatedEvent.h \
    dom/MutationCallback.h \
    dom/MutationEvent.h \
    dom/MutationObserver.h \
    dom/MutationObserverRegistration.h \
    dom/MutationRecord.h \
    dom/NamedFlowCollection.h \
    dom/NamedNodeMap.h \
    dom/NameNodeList.h \
    dom/NodeFilterCondition.h \
    dom/NodeFilter.h \
    dom/Node.h \
    dom/NodeIterator.h \
    dom/NodeRareData.h \
    dom/NodeRenderingContext.h \
    dom/NodeRenderingTraversal.h \
    dom/NodeTraversal.h \
    dom/Notation.h \
    dom/OverflowEvent.h \
    dom/PageTransitionEvent.h \
    dom/Position.h \
    dom/PositionIterator.h \
    dom/ProcessingInstruction.h \
    dom/ProgressEvent.h \
    dom/PropertyNodeList.h \
    dom/PseudoElement.h \
    dom/QualifiedName.h \
    dom/Range.h \
    dom/RegisteredEventListener.h \
    dom/RenderedDocumentMarker.h \
    dom/UserActionElementSet.h \
    dom/ScriptedAnimationController.h \
    dom/ScriptElement.h \
    dom/ScriptExecutionContext.h \
    dom/SelectorQuery.h \
    dom/ShadowRoot.h \
    dom/SimulatedClickOptions.h \
    dom/SpaceSplitString.h \
    dom/StaticNodeList.h \
    dom/StyledElement.h \
    dom/StyleElement.h \
    dom/TagNodeList.h \
    dom/TemplateContentDocumentFragment.h \
    dom/TextEvent.h \
    dom/TextEventInputType.h \
    dom/Text.h \
    dom/Touch.h \
    dom/TouchEvent.h \
    dom/TouchList.h \
    dom/TransformSource.h \
    dom/TransitionEvent.h \
    dom/Traversal.h \
    dom/TreeDepthLimit.h \
    dom/TreeScope.h \
    dom/TreeScopeAdopter.h \
    dom/TreeWalker.h \
    dom/UIEvent.h \
    dom/UIEventWithKeyState.h \
    dom/UserGestureIndicator.h \
    dom/ViewportArguments.h \
    dom/WebKitAnimationEvent.h \
    dom/WebKitNamedFlow.h \
    dom/WebKitTransitionEvent.h \
    dom/WheelEvent.h \
    editing/AlternativeTextController.h \
    editing/AppendNodeCommand.h \
    editing/ApplyBlockElementCommand.h \
    editing/ApplyStyleCommand.h \
    editing/BreakBlockquoteCommand.h \
    editing/CompositeEditCommand.h \
    editing/CreateLinkCommand.h \
    editing/DeleteButtonController.h \
    editing/DeleteButton.h \
    editing/DeleteFromTextNodeCommand.h \
    editing/DeleteSelectionCommand.h \
    editing/DictationAlternative.h \
    editing/DictationCommand.h \
    editing/EditCommand.h \
    editing/EditingStyle.h \
    editing/EditingBehavior.h \
    editing/EditingBoundary.h \
    editing/Editor.h \
    editing/FindOptions.h \
    editing/FormatBlockCommand.h \
    editing/FrameSelection.h \
    editing/htmlediting.h \
    editing/HTMLInterchange.h \
    editing/IndentOutdentCommand.h \
    editing/InsertIntoTextNodeCommand.h \
    editing/InsertLineBreakCommand.h \
    editing/InsertListCommand.h \
    editing/InsertNodeBeforeCommand.h \
    editing/InsertParagraphSeparatorCommand.h \
    editing/InsertTextCommand.h \
    editing/markup.h \
    editing/MergeIdenticalElementsCommand.h \
    editing/ModifySelectionListLevel.h \
    editing/MoveSelectionCommand.h \
    editing/RemoveCSSPropertyCommand.h \
    editing/RemoveFormatCommand.h \
    editing/RemoveNodeCommand.h \
    editing/RemoveNodePreservingChildrenCommand.h \
    editing/RenderedPosition.h \
    editing/ReplaceNodeWithSpanCommand.h \
    editing/ReplaceSelectionCommand.h \
    editing/SetNodeAttributeCommand.h \
    editing/SimplifyMarkupCommand.h \
    editing/SmartReplace.h \
    editing/SpellingCorrectionCommand.h \
    editing/SplitElementCommand.h \
    editing/SplitTextNodeCommand.h \
    editing/SplitTextNodeContainingElementCommand.h \
    editing/TextInsertionBaseCommand.h \
    editing/TextIterator.h \
    editing/TypingCommand.h \
    editing/UndoStep.h \
    editing/UnlinkCommand.h \
    editing/VisiblePosition.h \
    editing/VisibleSelection.h \
    editing/VisibleUnits.h \
    editing/WrapContentsInDummySpanCommand.h \
    fileapi/Blob.h \
    fileapi/BlobURL.h \
    fileapi/File.h \
    fileapi/FileError.h \
    fileapi/FileException.h \
    fileapi/FileList.h \
    fileapi/FileReader.h \
    fileapi/FileReaderLoader.h \
    fileapi/FileReaderLoaderClient.h \
    fileapi/FileReaderSync.h \
    fileapi/FileThread.h \
    fileapi/FileThreadTask.h \
    fileapi/WebKitBlobBuilder.h \
    history/BackForwardController.h \
    history/BackForwardListImpl.h \
    history/BackForwardList.h \
    history/CachedFrame.h \
    history/CachedPage.h \
    history/HistoryItem.h \
    history/PageCache.h \
    html/canvas/CanvasGradient.h \
    html/canvas/CanvasPathMethods.h \
    html/canvas/CanvasPattern.h \
    html/canvas/CanvasProxy.h \
    html/canvas/CanvasRenderingContext.h \
    html/canvas/CanvasRenderingContext2D.h \
    html/canvas/CanvasStyle.h \
    html/canvas/DataView.h \
    html/canvas/DOMPath.h \
    html/ClassList.h \
    html/DOMFormData.h \
    html/DOMSettableTokenList.h \
    html/DOMTokenList.h \
    html/DOMURL.h \
    html/FormAssociatedElement.h \
    html/FormController.h \
    html/FormDataList.h \
    html/FTPDirectoryDocument.h \
    html/HTMLAllCollection.h \
    html/HTMLAnchorElement.h \
    html/HTMLAppletElement.h \
    html/HTMLAreaElement.h \
    html/HTMLAudioElement.h \
    html/HTMLBaseElement.h \
    html/HTMLBaseFontElement.h \
    html/HTMLBodyElement.h \
    html/HTMLBDIElement.h \
    html/HTMLBRElement.h \
    html/HTMLButtonElement.h \
    html/HTMLCanvasElement.h \
    html/HTMLCollection.h \
    html/HTMLDialogElement.h \
    html/HTMLDirectoryElement.h \
    html/HTMLDetailsElement.h \
    html/HTMLDivElement.h \
    html/HTMLDListElement.h \
    html/HTMLDocument.h \
    html/HTMLElement.h \
    html/HTMLEmbedElement.h \
    html/HTMLFieldSetElement.h \
    html/HTMLFontElement.h \
    html/HTMLFormControlsCollection.h \
    html/HTMLFormControlElement.h \
    html/HTMLFormControlElementWithState.h \
    html/HTMLFormElement.h \
    html/HTMLFrameElementBase.h \
    html/HTMLFrameElement.h \
    html/HTMLFrameOwnerElement.h \
    html/HTMLFrameSetElement.h \
    html/HTMLHeadElement.h \
    html/HTMLHeadingElement.h \
    html/HTMLHRElement.h \
    html/HTMLHtmlElement.h \
    html/HTMLIFrameElement.h \
    html/HTMLImageElement.h \
    html/HTMLImageLoader.h \
    html/HTMLInputElement.h \
    html/HTMLKeygenElement.h \
    html/HTMLLabelElement.h \
    html/HTMLLegendElement.h \
    html/HTMLLIElement.h \
    html/HTMLLinkElement.h \
    html/HTMLMapElement.h \
    html/HTMLMarqueeElement.h \
    html/HTMLMediaElement.h \
    html/HTMLMenuElement.h \
    html/HTMLMetaElement.h \
    html/HTMLMeterElement.h \
    html/HTMLModElement.h \
    html/HTMLNameCollection.h \
    html/HTMLObjectElement.h \
    html/HTMLOListElement.h \
    html/HTMLOptGroupElement.h \
    html/HTMLOptionElement.h \
    html/HTMLOptionsCollection.h \
    html/HTMLOutputElement.h \
    html/HTMLParagraphElement.h \
    html/HTMLParamElement.h \
    html/HTMLParserErrorCodes.h \
    html/HTMLPlugInElement.h \
    html/HTMLPlugInImageElement.h \
    html/HTMLPreElement.h \
    html/HTMLProgressElement.h \
    html/HTMLPropertiesCollection.h \
    html/HTMLQuoteElement.h \
    html/HTMLScriptElement.h \
    html/HTMLSelectElement.h \
    html/HTMLSourceElement.h \
    html/HTMLSpanElement.h \
    html/HTMLStyleElement.h \
    html/HTMLSummaryElement.h \
    html/HTMLTableCaptionElement.h \
    html/HTMLTableCellElement.h \
    html/HTMLTableColElement.h \
    html/HTMLTableElement.h \
    html/HTMLTablePartElement.h \
    html/HTMLTableRowElement.h \
    html/HTMLTableRowsCollection.h \
    html/HTMLTableSectionElement.h \
    html/HTMLTemplateElement.h \
    html/HTMLTextAreaElement.h \
    html/HTMLTextFormControlElement.h \
    html/HTMLTitleElement.h \
    html/HTMLUListElement.h \
    html/HTMLVideoElement.h \
    html/HTMLViewSourceDocument.h \
    html/ImageData.h \
    html/ImageDocument.h \
    html/LabelableElement.h \
    html/LabelsNodeList.h \
    html/LinkRelAttribute.h \
    html/MediaController.h \
    html/MediaDocument.h \
    html/MediaFragmentURIParser.h \
    html/MicroDataAttributeTokenList.h \
    html/MicroDataItemValue.h \
    html/PluginDocument.h \
    html/PublicURLManager.h \
    html/RadioNodeList.h \
    html/StepRange.h \
    html/TextDocument.h \
    html/TimeRanges.h \
    html/TypeAhead.h \
    html/ValidityState.h \
    html/parser/AtomicHTMLToken.h \
    html/parser/CSSPreloadScanner.h \
    html/parser/CompactHTMLToken.h \
    html/parser/HTMLConstructionSite.h \
    html/parser/HTMLDocumentParser.h \
    html/parser/HTMLElementStack.h \
    html/parser/HTMLEntityParser.h \
    html/parser/HTMLEntitySearch.h \
    html/parser/HTMLEntityTable.h \
    html/parser/HTMLFormattingElementList.h \
    html/parser/HTMLParserScheduler.h \
    html/parser/HTMLPreloadScanner.h \
    html/parser/HTMLResourcePreloader.h \
    html/parser/HTMLScriptRunner.h \
    html/parser/HTMLScriptRunnerHost.h \
    html/parser/HTMLToken.h \
    html/parser/HTMLTokenizer.h \
    html/parser/HTMLTreeBuilder.h \
    html/parser/HTMLViewSourceParser.h \
    html/parser/InputStreamPreprocessor.h \
    html/parser/XSSAuditor.h \
    html/parser/XSSAuditorDelegate.h \
    html/shadow/ContentDistributor.h \
    html/shadow/HTMLContentElement.h \
    html/shadow/MediaControlElementTypes.h \
    html/shadow/MediaControlElements.h \
    html/shadow/MediaControls.h \
    html/shadow/MediaControlsApple.h \
    html/shadow/DetailsMarkerControl.h \
    inspector/BindingVisitors.h \
    inspector/ConsoleAPITypes.h \
    inspector/ConsoleMessage.h \
    inspector/ContentSearchUtils.h \
    inspector/DOMEditor.h \
    inspector/DOMPatchSupport.h \
    inspector/IdentifiersFactory.h \
    inspector/InjectedScript.h \
    inspector/InjectedScriptBase.h \
    inspector/InjectedScriptCanvasModule.h \
    inspector/InjectedScriptHost.h \
    inspector/InjectedScriptManager.h \
    inspector/InjectedScriptModule.h \
    inspector/InspectorAgent.h \
    inspector/InspectorApplicationCacheAgent.h \
    inspector/InspectorBaseAgent.h \
    inspector/InspectorCanvasAgent.h \
    inspector/InspectorCanvasInstrumentation.h \
    inspector/InspectorConsoleAgent.h \
    inspector/InspectorConsoleInstrumentation.h \
    inspector/InspectorController.h \
    inspector/InspectorCounters.h \
    inspector/InspectorCSSAgent.h \
    inspector/InspectorDatabaseAgent.h \
    inspector/InspectorDatabaseInstrumentation.h \
    inspector/InspectorDatabaseResource.h \
    inspector/InspectorDebuggerAgent.h \
    inspector/InspectorDOMDebuggerAgent.h \
    inspector/InspectorDOMStorageAgent.h \
    inspector/InspectorFrontendChannel.h \
    inspector/InspectorFrontendClient.h \
    inspector/InspectorFrontendClientLocal.h \
    inspector/InspectorFrontendHost.h \
    inspector/InspectorHeapProfilerAgent.h \
    inspector/InspectorHistory.h \
    inspector/InspectorInstrumentation.h \
    inspector/InspectorLayerTreeAgent.h \
    inspector/InspectorMemoryAgent.h \
    inspector/InspectorOverlay.h \
    inspector/InspectorPageAgent.h \
    inspector/InspectorProfilerAgent.h \
    inspector/InspectorResourceAgent.h \
    inspector/InspectorRuntimeAgent.h \
    inspector/InspectorState.h \
    inspector/InspectorStyleSheet.h \
    inspector/InspectorStyleTextEditor.h \
    inspector/InspectorTimelineAgent.h \
    inspector/InspectorWorkerAgent.h \
    inspector/InstrumentingAgents.h \
    inspector/NetworkResourcesData.h \
    inspector/PageConsoleAgent.h \
    inspector/PageDebuggerAgent.h \
    inspector/PageRuntimeAgent.h \
    inspector/ScriptGCEventListener.h \
    inspector/TimelineRecordFactory.h \
    inspector/WorkerConsoleAgent.h \
    inspector/WorkerDebuggerAgent.h \
    inspector/WorkerRuntimeAgent.h \
    loader/appcache/ApplicationCacheGroup.h \
    loader/appcache/ApplicationCacheHost.h \
    loader/appcache/ApplicationCache.h \
    loader/appcache/ApplicationCacheResource.h \
    loader/appcache/ApplicationCacheStorage.h \
    loader/appcache/DOMApplicationCache.h \
    loader/appcache/ManifestParser.h \
    loader/archive/ArchiveResourceCollection.h \
    loader/archive/ArchiveResource.h \
    loader/cache/CachedCSSStyleSheet.h \
    loader/cache/CachedFont.h \
    loader/cache/CachedImage.h \
    loader/cache/CachedRawResource.h \
    loader/cache/CachedRawResourceClient.h \
    loader/cache/CachedResourceClientWalker.h \
    loader/cache/CachedResource.h \
    loader/cache/CachedResourceHandle.h \
    loader/cache/CachedScript.h \
    loader/cache/CachedShader.h \
    loader/cache/CachedSVGDocument.h \
    loader/cache/CachedXSLStyleSheet.h \
    loader/cache/MemoryCache.h \
    loader/CookieJar.h \
    loader/CrossOriginAccessControl.h \
    loader/CrossOriginPreflightResultCache.h \
    loader/cache/CachedResourceLoader.h \
    loader/cache/CachedResourceRequest.h \
    loader/cache/CachedResourceRequestInitiators.h \
    loader/DocumentLoader.h \
    loader/DocumentThreadableLoader.h \
    loader/FormState.h \
    loader/FrameLoader.h \
    loader/FrameLoaderStateMachine.h \
    loader/FTPDirectoryParser.h \
    loader/icon/IconController.h \
    loader/icon/IconDatabase.h \
    loader/icon/IconDatabaseBase.h \
    loader/icon/IconLoader.h \
    loader/icon/IconRecord.h \
    loader/icon/PageURLRecord.h \
    loader/ImageLoader.h \
    loader/LinkLoader.h \
    loader/LinkLoaderClient.h \
    loader/LoaderStrategy.h \
    loader/MixedContentChecker.h \
    loader/NavigationAction.h \
    loader/NetscapePlugInStreamLoader.h \
    loader/PlaceholderDocument.h \
    loader/ProgressTracker.h \
    loader/ResourceBuffer.h \
    loader/ResourceLoader.h \
    loader/ResourceLoaderTypes.h \
    loader/SubresourceLoader.h \
    loader/SubstituteData.h \
    loader/TextResourceDecoder.h \
    loader/ThreadableLoader.h \
    loader/WorkerThreadableLoader.h \
    mathml/MathMLElement.h \
    mathml/MathMLInlineContainerElement.h \
    mathml/MathMLMathElement.h \
    mathml/MathMLTextElement.h \
    page/animation/AnimationBase.h \
    page/animation/AnimationController.h \
    page/animation/CompositeAnimation.h \
    page/animation/ImplicitAnimation.h \
    page/animation/KeyframeAnimation.h \
    page/AdjustViewSizeOrNot.h \
    page/AutoscrollController.h \
    page/BarProp.h \
    page/CaptionUserPreferences.h \
    page/Chrome.h \
    page/Console.h \
    page/ConsoleTypes.h \
    page/ContextMenuController.h \
    page/ContextMenuProvider.h \
    page/DeviceClient.h \
    page/DeviceController.h \
    page/DiagnosticLoggingKeys.h \
    page/DOMSelection.h \
    page/DOMTimer.h \
    page/DOMWindow.h \
    page/DOMWindowExtension.h \
    page/DragController.h \
    page/DragState.h \
    page/EventHandler.h \
    page/EventSource.h \
    page/EditorClient.h \
    page/FocusController.h \
    page/Frame.h \
    page/FrameTree.h \
    page/FrameView.h \
    page/GestureTapHighlighter.h\
    page/GroupSettings.h \
    page/History.h \
    page/LayoutMilestones.h \
    page/Location.h \
    page/MouseEventWithHitTestResults.h \
    page/NavigatorBase.h \
    page/Navigator.h \
    page/PageGroup.h \
    page/PageGroupLoadDeferrer.h \
    page/Page.h \
    page/PageActivityAssertionToken.h \
    page/PageConsole.h \
    page/PageThrottler.h \
    page/PageVisibilityState.h \
    page/PlugInClient.h \
    page/PopupOpeningObserver.h \
    page/PrintContext.h \
    page/Screen.h \
    page/SecurityOrigin.h \
    page/Settings.h \
    page/SpatialNavigation.h \
    page/SpeechInput.h \
    page/SpeechInputClient.h \
    page/SpeechInputEvent.h \
    page/SpeechInputListener.h \
    page/SpeechInputResult.h \
    page/SpeechInputResultList.h \
    page/TouchAdjustment.h \
    page/ValidationMessageClient.h \
    page/WindowFeatures.h \
    page/WindowFocusAllowedIndicator.h \
    page/WorkerNavigator.h \
    platform/animation/Animation.h \
    platform/animation/AnimationList.h \
    platform/animation/AnimationUtilities.h \
    platform/Arena.h \
    platform/CalculationValue.h \
    platform/Clock.h \
    platform/ClockGeneric.h \
    platform/ContentType.h \
    platform/ContextMenu.h \
    platform/ContextMenuItem.h \
    platform/CrossThreadCopier.h \
    platform/DateComponents.h \
    platform/Decimal.h \
    platform/DragData.h \
    platform/DragImage.h \
    platform/FileChooser.h \
    platform/FileStream.h \
    platform/FileStreamClient.h \
    platform/FileSystem.h \
    platform/HistogramSupport.h \
    platform/InitializeLogging.h \
    platform/image-decoders/ImageDecoder.h \
    platform/mock/DeviceMotionClientMock.h \
    platform/mock/DeviceOrientationClientMock.h \
    platform/mock/GeolocationClientMock.cpp \
    platform/mock/PlatformSpeechSynthesizerMock.h \
    platform/mock/ScrollbarThemeMock.h \
    platform/graphics/BitmapImage.h \
    platform/graphics/Color.h \
    platform/graphics/cpu/arm/filters/NEONHelpers.h \
    platform/graphics/cpu/arm/filters/FEBlendNEON.h \
    platform/graphics/cpu/arm/filters/FECompositeArithmeticNEON.h \
    platform/graphics/cpu/arm/filters/FEGaussianBlurNEON.h \
    platform/graphics/cpu/arm/filters/FELightingNEON.h \
    platform/graphics/CrossfadeGeneratedImage.h \
    platform/graphics/filters/texmap/TextureMapperPlatformCompiledProgram.h \
    platform/graphics/filters/CustomFilterArrayParameter.h \
    platform/graphics/filters/CustomFilterColorParameter.h \
    platform/graphics/filters/CustomFilterConstants.h \
    platform/graphics/filters/CustomFilterGlobalContext.h \
    platform/graphics/filters/CustomFilterMesh.h \
    platform/graphics/filters/CustomFilterMeshGenerator.h \
    platform/graphics/filters/CustomFilterNumberParameter.h \
    platform/graphics/filters/CustomFilterCompiledProgram.h \
    platform/graphics/filters/CustomFilterOperation.h \
    platform/graphics/filters/ValidatedCustomFilterOperation.h \
    platform/graphics/filters/CustomFilterParameter.h \
    platform/graphics/filters/CustomFilterParameterList.h \
    platform/graphics/filters/CustomFilterProgram.h \
    platform/graphics/filters/CustomFilterProgramInfo.h \
    platform/graphics/filters/CustomFilterRenderer.h \
    platform/graphics/filters/CustomFilterTransformParameter.h \
    platform/graphics/filters/CustomFilterValidatedProgram.h \
    platform/graphics/filters/FEBlend.h \
    platform/graphics/filters/FEColorMatrix.h \
    platform/graphics/filters/FEComponentTransfer.h \
    platform/graphics/filters/FEComposite.h \
    platform/graphics/filters/FEConvolveMatrix.h \
    platform/graphics/filters/FECustomFilter.h \
    platform/graphics/filters/FEDiffuseLighting.h \
    platform/graphics/filters/FEDisplacementMap.h \
    platform/graphics/filters/FEDropShadow.h \
    platform/graphics/filters/FEFlood.h \
    platform/graphics/filters/FEGaussianBlur.h \
    platform/graphics/filters/FELighting.h \
    platform/graphics/filters/FEMerge.h \
    platform/graphics/filters/FEMorphology.h \
    platform/graphics/filters/FEOffset.h \
    platform/graphics/filters/FESpecularLighting.h \
    platform/graphics/filters/FETile.h \
    platform/graphics/filters/FETurbulence.h \
    platform/graphics/filters/FilterEffect.h \
    platform/graphics/filters/FilterOperation.h \
    platform/graphics/filters/FilterOperations.h \
    platform/graphics/filters/LightSource.h \
    platform/graphics/filters/SourceAlpha.h \
    platform/graphics/filters/SourceGraphic.h \
    platform/graphics/FloatPoint3D.h \
    platform/graphics/FloatPoint.h \
    platform/graphics/FloatPolygon.h \
    platform/graphics/FloatQuad.h \
    platform/graphics/FloatRect.h \
    platform/graphics/FloatSize.h \
    platform/graphics/FontData.h \
    platform/graphics/FontDescription.h \
    platform/graphics/FontFeatureSettings.h \
    platform/graphics/FontMetrics.h \
    platform/graphics/Font.h \
    platform/graphics/GeneratorGeneratedImage.h \
    platform/graphics/GeneratedImage.h \
    platform/graphics/GlyphPageTreeNode.h \
    platform/graphics/Gradient.h \
    platform/graphics/GraphicsContext.h \
    platform/graphics/GraphicsLayer.h \
    platform/graphics/GraphicsLayerAnimation.h \
    platform/graphics/GraphicsLayerClient.h \
    platform/graphics/GraphicsLayerTransform.h \
    platform/graphics/GraphicsTypes.h \
    platform/graphics/GraphicsTypes3D.h \
    platform/graphics/Image.h \
    platform/graphics/ImageOrientation.h \
    platform/graphics/ImageSource.h \
    platform/graphics/IntPoint.h \
    platform/graphics/IntPointHash.h \
    platform/graphics/IntRect.h \
    platform/graphics/IntRectExtent.h \
    platform/graphics/Latin1TextIterator.h \
    platform/graphics/MediaPlayer.h \
    platform/graphics/NativeImagePtr.h \
    platform/graphics/opentype/OpenTypeVerticalData.h \
    platform/graphics/Path.h \
    platform/graphics/PathTraversalState.h \
    platform/graphics/Pattern.h \
    platform/graphics/PlatformLayer.h \
    platform/graphics/Region.h \
    platform/graphics/RoundedRect.h \
    platform/graphics/qt/FontCustomPlatformData.h \
    platform/graphics/qt/NativeImageQt.h \
    platform/graphics/qt/StillImageQt.h \
    platform/graphics/qt/TransparencyLayer.h \
    platform/graphics/SegmentedFontData.h \
    platform/graphics/ShadowBlur.h \
    platform/graphics/SimpleFontData.h \
    platform/graphics/surfaces/GraphicsSurface.h \
    platform/graphics/surfaces/GraphicsSurfaceToken.h \
    platform/graphics/SurrogatePairAwareTextIterator.h \
    platform/graphics/texmap/GraphicsLayerTextureMapper.h \
    platform/graphics/texmap/TextureMapper.h \
    platform/graphics/texmap/TextureMapperBackingStore.h \
    platform/graphics/texmap/TextureMapperFPSCounter.h \
    platform/graphics/texmap/TextureMapperImageBuffer.h \
    platform/graphics/texmap/TextureMapperLayer.h \
    platform/graphics/texmap/TextureMapperPlatformLayer.h \
    platform/graphics/texmap/TextureMapperSurfaceBackingStore.h \
    platform/graphics/texmap/TextureMapperTile.h \
    platform/graphics/texmap/TextureMapperTiledBackingStore.h \
    platform/graphics/Tile.h \
    platform/graphics/TiledBackingStore.h \
    platform/graphics/TiledBackingStoreClient.h \
    platform/graphics/transforms/Matrix3DTransformOperation.h \
    platform/graphics/transforms/MatrixTransformOperation.h \
    platform/graphics/transforms/PerspectiveTransformOperation.h \
    platform/graphics/transforms/RotateTransformOperation.h \
    platform/graphics/transforms/ScaleTransformOperation.h \
    platform/graphics/transforms/SkewTransformOperation.h \
    platform/graphics/transforms/TransformationMatrix.h \
    platform/graphics/transforms/TransformOperations.h \
    platform/graphics/transforms/TransformState.h \
    platform/graphics/transforms/TranslateTransformOperation.h \
    platform/graphics/WidthIterator.h \
    platform/graphics/WidthCache.h \
    platform/image-decoders/bmp/BMPImageDecoder.h \
    platform/image-decoders/bmp/BMPImageReader.h \
    platform/image-decoders/ico/ICOImageDecoder.h \
    platform/image-decoders/gif/GIFImageDecoder.h \
    platform/image-decoders/gif/GIFImageReader.h \
    platform/image-decoders/png/PNGImageDecoder.h \
    platform/KillRing.h \
    platform/KURL.h \
    platform/Length.h \
    platform/LengthBox.h \
    platform/leveldb/LevelDBComparator.h \
    platform/leveldb/LevelDBDatabase.h \
    platform/leveldb/LevelDBIterator.h \
    platform/leveldb/LevelDBSlice.h \
    platform/leveldb/LevelDBTransaction.h \
    platform/leveldb/LevelDBWriteBatch.h \
    platform/text/BidiRunList.h \
    platform/text/LineEnding.h \
    platform/text/LocaleToScriptMapping.h \
    platform/text/TextCheckerClient.h \
    platform/text/TextChecking.h \
    platform/text/UnicodeBidi.h \
    platform/LinkHash.h \
    platform/Logging.h \
    platform/Language.h \
    platform/MemoryPressureHandler.h \
    platform/MainThreadTask.h \
    platform/MIMETypeRegistry.h \
    platform/network/AuthenticationChallengeBase.h \
    platform/network/AuthenticationClient.h \
    platform/network/BlobData.h \
    platform/network/BlobRegistry.h \
    platform/network/BlobRegistryImpl.h \
    platform/network/BlobResourceHandle.h \
    platform/network/BlobStorageData.h \
    platform/network/CookieStorage.h \
    platform/network/Credential.h \
    platform/network/CredentialStorage.h \
    platform/network/DNSResolveQueue.h \
    platform/network/FormDataBuilder.h \
    platform/network/FormData.h \
    platform/network/HTTPHeaderMap.h \
    platform/network/HTTPParsers.h \
    platform/network/HTTPStatusCodes.h \
    platform/network/MIMESniffing.h \
    platform/network/NetworkStorageSession.h \
    platform/network/NetworkingContext.h \
    platform/network/NetworkStateNotifier.h \
    platform/network/ParsedContentType.h \
    platform/network/PlatformCookieJar.h \
    platform/network/ProtectionSpace.h \
    platform/network/ProxyServer.h \
    platform/network/qt/QtMIMETypeSniffer.h \
    platform/network/qt/QNetworkReplyHandler.h \
    platform/network/ResourceErrorBase.h \
    platform/network/ResourceHandle.h \
    platform/network/ResourceHandleTypes.h \
    platform/network/ResourceLoadPriority.h \
    platform/network/ResourceLoadTiming.h \
    platform/network/ResourceRequestBase.h \
    platform/network/ResourceResponseBase.h \
    platform/network/qt/NetworkStateNotifierPrivate.h \
    platform/network/qt/CookieJarQt.h \
    platform/PlatformExportMacros.h \
    platform/PlatformTouchEvent.h \
    platform/PlatformTouchPoint.h \
    platform/PopupMenu.h \
    platform/ReferrerPolicy.h \
    platform/qt/QWebPageClient.h \
    platform/qt/QStyleFacade.h \
    platform/qt/RenderThemeQStyle.h \
    platform/qt/RenderThemeQt.h \
    platform/qt/RenderThemeQtMobile.h \
    platform/qt/ScrollbarThemeQStyle.h \
    platform/qt/UserAgentQt.h \
    platform/ScrollableArea.h \
    platform/ScrollAnimator.h \
    platform/Scrollbar.h \
    platform/ScrollbarThemeClient.h \
    platform/ScrollbarThemeComposite.h \
    platform/ScrollView.h \
    platform/SearchPopupMenu.h \
    platform/SharedBuffer.h \
    platform/SharedBufferChunkReader.h \
    platform/sql/SQLiteDatabase.h \
    platform/sql/SQLiteFileSystem.h \
    platform/sql/SQLiteStatement.h \
    platform/sql/SQLiteTransaction.h \
    platform/sql/SQLValue.h \
    platform/text/BidiContext.h \
    platform/text/DateTimeFormat.h \
    platform/text/DecodeEscapeSequences.h \
    platform/text/Hyphenation.h \
    platform/text/QuotedPrintable.h \
    platform/text/RegularExpression.h \
    platform/text/SegmentedString.h \
    platform/text/TextBoundaries.h \
    platform/text/TextCodec.h \
    platform/text/TextCodecASCIIFastPath.h \
    platform/text/TextCodecLatin1.h \
    platform/text/TextCodecUserDefined.h \
    platform/text/TextCodecUTF16.h \
    platform/text/TextCodecUTF8.h \
    platform/text/TextEncoding.h \
    platform/text/TextEncodingRegistry.h \
    platform/text/TextStream.h \
    platform/text/UnicodeRange.h \
    platform/text/transcoder/FontTranscoder.h \
    platform/ThreadGlobalData.h \
    platform/ThreadTimers.h \
    platform/Timer.h \
    platform/Widget.h \
    platform/PlatformStrategies.h \
    platform/LocalizedStrings.h \
    plugins/DOMMimeTypeArray.h \
    plugins/DOMMimeType.h \
    plugins/DOMPluginArray.h \
    plugins/PluginDatabase.h \
    plugins/PluginData.h \
    plugins/PluginDebug.h \
    plugins/DOMPlugin.h \
    plugins/IFrameShimSupport.h \
    plugins/PluginMainThreadScheduler.h \
    plugins/PluginPackage.h \
    plugins/PluginStream.h \
    plugins/PluginView.h \
    plugins/win/PluginMessageThrottlerWin.h \
    rendering/AutoTableLayout.h \
    rendering/break_lines.h \
    rendering/CounterNode.h \
    rendering/EllipsisBox.h \
    rendering/FilterEffectRenderer.h \
    rendering/FixedTableLayout.h \
    rendering/HitTestingTransformState.h \
    rendering/HitTestLocation.h \
    rendering/HitTestResult.h \
    rendering/InlineBox.h \
    rendering/InlineFlowBox.h \
    rendering/InlineTextBox.h \
    rendering/LayoutRepainter.h \
    rendering/LayoutState.h \
    rendering/LogicalSelectionOffsetCaches.h \
    rendering/mathml/RenderMathMLBlock.h \
    rendering/mathml/RenderMathMLFenced.h \
    rendering/mathml/RenderMathMLFraction.h \
    rendering/mathml/RenderMathMLMath.h \
    rendering/mathml/RenderMathMLOperator.h \
    rendering/mathml/RenderMathMLRoot.h \
    rendering/mathml/RenderMathMLRow.h \
    rendering/mathml/RenderMathMLSpace.h \
    rendering/mathml/RenderMathMLSquareRoot.h \
    rendering/mathml/RenderMathMLSubSup.h \
    rendering/mathml/RenderMathMLUnderOver.h \
    rendering/Pagination.h \
    rendering/PaintInfo.h \
    rendering/PaintPhase.h \
    rendering/PointerEventsHitRules.h \
    rendering/RegionOversetState.h \
    rendering/RenderApplet.h \
    rendering/RenderArena.h \
    rendering/RenderBlock.h \
    rendering/RenderBox.h \
    rendering/RenderBoxModelObject.h \
    rendering/RenderBR.h \
    rendering/RenderButton.h \
    rendering/RenderCombineText.h \
    rendering/RenderCounter.h \
    rendering/RenderDeprecatedFlexibleBox.h \
    rendering/RenderDetailsMarker.h \
    rendering/RenderEmbeddedObject.h \
    rendering/RenderFieldset.h \
    rendering/RenderFileUploadControl.h \
    rendering/RenderFlexibleBox.h \
    rendering/RenderFrame.h \
    rendering/RenderFrameBase.h \
    rendering/RenderFrameSet.h \
    rendering/RenderGeometryMap.h \
    rendering/RenderGrid.h \
    rendering/RenderHTMLCanvas.h \
    rendering/RenderIFrame.h \
    rendering/RenderImageResource.h \
    rendering/RenderImageResourceStyleImage.h \
    rendering/RenderImage.h \
    rendering/RenderInline.h \
    rendering/RenderInputSpeech.h \
    rendering/RenderLayer.h \
    rendering/RenderLayerBacking.h \
    rendering/RenderLayerCompositor.h \
    rendering/RenderLayerModelObject.h \
    rendering/RenderLineBoxList.h \
    rendering/RenderListBox.h \
    rendering/RenderListItem.h \
    rendering/RenderListMarker.h \
    rendering/RenderMarquee.h \
    rendering/RenderMediaControlElements.h \
    rendering/RenderMediaControls.h \
    rendering/RenderMedia.h \
    rendering/RenderMenuList.h \
    rendering/RenderMeter.h \
    rendering/RenderMultiColumnBlock.h \
    rendering/RenderObjectChildList.h \
    rendering/RenderObject.h \
    rendering/RenderPart.h \
    rendering/RenderProgress.h \
    rendering/RenderQuote.h \
    rendering/RenderReplaced.h \
    rendering/RenderReplica.h \
    rendering/RenderRuby.h \
    rendering/RenderRubyBase.h \
    rendering/RenderRubyRun.h \
    rendering/RenderRubyText.h \
    rendering/RenderScrollbar.h \
    rendering/RenderScrollbarPart.h \
    rendering/RenderScrollbarTheme.h \
    rendering/RenderSearchField.h \
    rendering/RenderSlider.h \
    rendering/RenderSnapshottedPlugIn.h \
    rendering/RenderTableCaption.h \
    rendering/RenderTableCell.h \
    rendering/RenderTableCol.h \
    rendering/RenderTable.h \
    rendering/RenderTableRow.h \
    rendering/RenderTableSection.h \
    rendering/RenderTextControl.h \
    rendering/RenderTextControlMultiLine.h \
    rendering/RenderTextControlSingleLine.h \
    rendering/RenderTextFragment.h \
    rendering/RenderText.h \
    rendering/RenderTheme.h \
    rendering/RenderTreeAsText.h \
    rendering/RenderVideo.h \
    rendering/RenderView.h \
    rendering/RenderWidget.h \
    rendering/RenderWordBreak.h \
    rendering/RootInlineBox.h \
    rendering/ScrollBehavior.h \
    rendering/shapes/PolygonShape.h \
    rendering/shapes/RectangleShape.h \
    rendering/shapes/Shape.h \
    rendering/shapes/ShapeInfo.h \
    rendering/shapes/ShapeInsideInfo.h \
    rendering/shapes/ShapeInterval.h \
    rendering/shapes/ShapeOutsideInfo.h \
    rendering/style/BasicShapes.h \
    rendering/style/ContentData.h \
    rendering/style/CounterDirectives.h \
    rendering/style/CursorData.h \
    rendering/style/CursorList.h \
    rendering/style/FillLayer.h \
    rendering/style/KeyframeList.h \
    rendering/style/NinePieceImage.h \
    rendering/style/QuotesData.h \
    rendering/style/RenderStyle.h \
    rendering/style/ShadowData.h \
    rendering/style/ShapeValue.h \
    rendering/style/StyleBackgroundData.h \
    rendering/style/StyleBoxData.h \
    rendering/style/StyleCachedImage.h \
    rendering/style/StyleCachedShader.h \
    rendering/style/StyleCustomFilterProgram.h \
    rendering/style/StyleCustomFilterProgramCache.h \
    rendering/style/StyleDeprecatedFlexibleBoxData.h \
    rendering/style/StyleFilterData.h \
    rendering/style/StyleFlexibleBoxData.h \
    rendering/style/StyleGeneratedImage.h \
    rendering/style/StyleInheritedData.h \
    rendering/style/StyleMarqueeData.h \
    rendering/style/StyleMultiColData.h \
    rendering/style/StylePendingShader.h \
    rendering/style/StyleRareInheritedData.h \
    rendering/style/StyleRareNonInheritedData.h \
    rendering/style/StyleReflection.h \
    rendering/style/StyleShader.h \
    rendering/style/StyleSurroundData.h \
    rendering/style/StyleTransformData.h \
    rendering/style/StyleVariableData.h \
    rendering/style/StyleVisualData.h \
    rendering/style/GridTrackSize.h \
    rendering/style/SVGRenderStyleDefs.h \
    rendering/style/SVGRenderStyle.h \
    rendering/svg/RenderSVGBlock.h \
    rendering/svg/RenderSVGContainer.h \
    rendering/svg/RenderSVGEllipse.h \
    rendering/svg/RenderSVGForeignObject.h \
    rendering/svg/RenderSVGGradientStop.h \
    rendering/svg/RenderSVGHiddenContainer.h \
    rendering/svg/RenderSVGImage.h \
    rendering/svg/RenderSVGInline.h \
    rendering/svg/RenderSVGInlineText.h \
    rendering/svg/RenderSVGModelObject.h \
    rendering/svg/RenderSVGPath.h \
    rendering/svg/RenderSVGRect.h \
    rendering/svg/RenderSVGResource.h \
    rendering/svg/RenderSVGResourceClipper.h \
    rendering/svg/RenderSVGResourceContainer.h \
    rendering/svg/RenderSVGResourceFilter.h \
    rendering/svg/RenderSVGResourceFilterPrimitive.h \
    rendering/svg/RenderSVGResourceGradient.h \
    rendering/svg/RenderSVGResourceLinearGradient.h \
    rendering/svg/RenderSVGResourceMarker.h \
    rendering/svg/RenderSVGResourceMasker.h \
    rendering/svg/RenderSVGResourcePattern.h \
    rendering/svg/RenderSVGResourceRadialGradient.h \
    rendering/svg/RenderSVGResourceSolidColor.h \
    rendering/svg/RenderSVGRoot.h \
    rendering/svg/RenderSVGShape.h \
    rendering/svg/RenderSVGTSpan.h \
    rendering/svg/RenderSVGText.h \
    rendering/svg/RenderSVGTextPath.h \
    rendering/svg/RenderSVGTransformableContainer.h \
    rendering/svg/RenderSVGViewportContainer.h \
    rendering/svg/SVGInlineFlowBox.h \
    rendering/svg/SVGInlineTextBox.h \
    rendering/svg/SVGMarkerData.h \
    rendering/svg/SVGPathData.h \
    rendering/svg/SVGRenderSupport.h \
    rendering/svg/SVGRenderTreeAsText.h \
    rendering/svg/SVGRenderingContext.h \
    rendering/svg/SVGResources.h \
    rendering/svg/SVGResourcesCache.h \
    rendering/svg/SVGResourcesCycleSolver.h \
    rendering/svg/SVGRootInlineBox.h \
    rendering/svg/SVGTextChunk.h \
    rendering/svg/SVGTextChunkBuilder.h \
    rendering/svg/SVGTextFragment.h \
    rendering/svg/SVGTextLayoutAttributes.h \
    rendering/svg/SVGTextLayoutAttributesBuilder.h \
    rendering/svg/SVGTextLayoutEngine.h \
    rendering/svg/SVGTextLayoutEngineBaseline.h \
    rendering/svg/SVGTextLayoutEngineSpacing.h \
    rendering/svg/SVGTextMetrics.h \
    rendering/svg/SVGTextMetricsBuilder.h \
    rendering/svg/SVGTextQuery.h \
    rendering/svg/SVGTextRunRenderingContext.h \
    storage/Storage.h \
    storage/StorageArea.h \
    storage/StorageAreaImpl.h \
    storage/StorageAreaSync.h \
    storage/StorageEvent.h \
    storage/StorageEventDispatcher.h \
    storage/StorageMap.h \
    storage/StorageNamespace.h \
    storage/StorageNamespaceImpl.h \
    storage/StorageSyncManager.h \
    storage/StorageThread.h \
    storage/StorageTracker.h \
    storage/StorageTrackerClient.h \
    svg/animation/SMILTimeContainer.h \
    svg/animation/SMILTime.h \
    svg/animation/SVGSMILElement.h \
    svg/ColorDistance.h \
    svg/graphics/filters/SVGFEImage.h \
    svg/graphics/filters/SVGFilterBuilder.h \
    svg/graphics/filters/SVGFilter.h \
    svg/graphics/SVGImage.h \
    svg/graphics/SVGImageCache.h \
    svg/graphics/SVGImageForContainer.h \
    svg/properties/SVGAnimatedProperty.h \
    svg/properties/SVGAttributeToPropertyMap.h \
    svg/properties/SVGAnimatedEnumerationPropertyTearOff.h \
    svg/properties/SVGAnimatedListPropertyTearOff.h \
    svg/properties/SVGAnimatedPathSegListPropertyTearOff.h \
    svg/properties/SVGAnimatedProperty.h \
    svg/properties/SVGAnimatedPropertyDescription.h \
    svg/properties/SVGAnimatedPropertyMacros.h \
    svg/properties/SVGAnimatedPropertyTearOff.h \
    svg/properties/SVGAnimatedStaticPropertyTearOff.h \
    svg/properties/SVGAnimatedTransformListPropertyTearOff.h \
    svg/properties/SVGListProperty.h \
    svg/properties/SVGListPropertyTearOff.h \
    svg/properties/SVGPathSegListPropertyTearOff.h \
    svg/properties/SVGProperty.h \
    svg/properties/SVGPropertyInfo.h \
    svg/properties/SVGPropertyTearOff.h \
    svg/properties/SVGPropertyTraits.h \
    svg/properties/SVGStaticListPropertyTearOff.h \
    svg/properties/SVGStaticPropertyTearOff.h \
    svg/properties/SVGStaticPropertyWithParentTearOff.h \
    svg/properties/SVGTransformListPropertyTearOff.h \
    svg/SVGAElement.h \
    svg/SVGAltGlyphDefElement.h \
    svg/SVGAltGlyphElement.h \
    svg/SVGAltGlyphItemElement.h \
    svg/SVGAngle.h \
    svg/SVGAnimateColorElement.h \
    svg/SVGAnimatedAngle.h \
    svg/SVGAnimatedBoolean.h \
    svg/SVGAnimatedColor.h \
    svg/SVGAnimatedEnumeration.h \
    svg/SVGAnimatedInteger.h \
    svg/SVGAnimatedIntegerOptionalInteger.h \
    svg/SVGAnimatedLength.h \
    svg/SVGAnimatedLengthList.h \
    svg/SVGAnimatedNumber.h \
    svg/SVGAnimatedNumberList.h \
    svg/SVGAnimatedNumberOptionalNumber.h \
    svg/SVGAnimatedPath.h \
    svg/SVGAnimatedPreserveAspectRatio.h \
    svg/SVGAnimatedPointList.h \
    svg/SVGAnimatedRect.h \
    svg/SVGAnimatedString.h \
    svg/SVGAnimatedTransformList.h \
    svg/SVGAnimatedType.h \
    svg/SVGAnimatedTypeAnimator.h \
    svg/SVGAnimateElement.h \
    svg/SVGAnimateMotionElement.h \
    svg/SVGAnimateTransformElement.h \
    svg/SVGAnimationElement.h \
    svg/SVGAnimatorFactory.h \
    svg/SVGCircleElement.h \
    svg/SVGClipPathElement.h \
    svg/SVGColor.h \
    svg/SVGComponentTransferFunctionElement.h \
    svg/SVGCursorElement.h \
    svg/SVGDefsElement.h \
    svg/SVGDescElement.h \
    svg/SVGDocumentExtensions.h \
    svg/SVGDocument.h \
    svg/SVGElement.h \
    svg/SVGElementInstance.h \
    svg/SVGElementInstanceList.h \
    svg/SVGElementRareData.h \
    svg/SVGEllipseElement.h \
    svg/SVGExternalResourcesRequired.h \
    svg/SVGFEBlendElement.h \
    svg/SVGFEColorMatrixElement.h \
    svg/SVGFEComponentTransferElement.h \
    svg/SVGFECompositeElement.h \
    svg/SVGFEConvolveMatrixElement.h \
    svg/SVGFEDiffuseLightingElement.h \
    svg/SVGFEDisplacementMapElement.h \
    svg/SVGFEDistantLightElement.h \
    svg/SVGFEDropShadowElement.h \
    svg/SVGFEFloodElement.h \
    svg/SVGFEFuncAElement.h \
    svg/SVGFEFuncBElement.h \
    svg/SVGFEFuncGElement.h \
    svg/SVGFEFuncRElement.h \
    svg/SVGFEGaussianBlurElement.h \
    svg/SVGFEImageElement.h \
    svg/SVGFELightElement.h \
    svg/SVGFEMergeElement.h \
    svg/SVGFEMergeNodeElement.h \
    svg/SVGFEMorphologyElement.h \
    svg/SVGFEOffsetElement.h \
    svg/SVGFEPointLightElement.h \
    svg/SVGFESpecularLightingElement.h \
    svg/SVGFESpotLightElement.h \
    svg/SVGFETileElement.h \
    svg/SVGFETurbulenceElement.h \
    svg/SVGFilterElement.h \
    svg/SVGFilterPrimitiveStandardAttributes.h \
    svg/SVGFitToViewBox.h \
    svg/SVGFontData.h \
    svg/SVGFontElement.h \
    svg/SVGFontFaceElement.h \
    svg/SVGFontFaceFormatElement.h \
    svg/SVGFontFaceNameElement.h \
    svg/SVGFontFaceSrcElement.h \
    svg/SVGFontFaceUriElement.h \
    svg/SVGForeignObjectElement.h \
    svg/SVGGElement.h \
    svg/SVGGlyphElement.h \
    svg/SVGGlyphRefElement.h \
    svg/SVGGradientElement.h \
    svg/SVGGraphicsElement.h \
    svg/SVGHKernElement.h \
    svg/SVGImageElement.h \
    svg/SVGImageLoader.h \
    svg/SVGLangSpace.h \
    svg/SVGLength.h \
    svg/SVGLengthContext.h \
    svg/SVGLengthList.h \
    svg/SVGLinearGradientElement.h \
    svg/SVGLineElement.h \
    svg/SVGLocatable.h \
    svg/SVGMarkerElement.h \
    svg/SVGMaskElement.h \
    svg/SVGMatrix.h \
    svg/SVGMetadataElement.h \
    svg/SVGMissingGlyphElement.h \
    svg/SVGMPathElement.h \
    svg/SVGNumberList.h \
    svg/SVGPaint.h \
    svg/SVGParserUtilities.h \
    svg/SVGParsingError.h \
    svg/SVGPathBuilder.h \
    svg/SVGPathConsumer.h \
    svg/SVGPathElement.h \
    svg/SVGPathParser.h \
    svg/SVGPathSegArc.h \
    svg/SVGPathSegArcAbs.h \
    svg/SVGPathSegArcRel.h \
    svg/SVGPathSegClosePath.h \
    svg/SVGPathSegCurvetoCubic.h \
    svg/SVGPathSegCurvetoCubicAbs.h \
    svg/SVGPathSegCurvetoCubicRel.h \
    svg/SVGPathSegCurvetoCubicSmooth.h \
    svg/SVGPathSegCurvetoCubicSmoothAbs.h \
    svg/SVGPathSegCurvetoCubicSmoothRel.h \
    svg/SVGPathSegCurvetoQuadratic.h \
    svg/SVGPathSegCurvetoQuadraticAbs.h \
    svg/SVGPathSegCurvetoQuadraticRel.h \
    svg/SVGPathSegCurvetoQuadraticSmoothAbs.h \
    svg/SVGPathSegCurvetoQuadraticSmoothRel.h \
    svg/SVGPathSegLinetoAbs.h \
    svg/SVGPathSegLinetoRel.h \
    svg/SVGPathSegLinetoHorizontal.h \
    svg/SVGPathSegLinetoHorizontalAbs.h \
    svg/SVGPathSegLinetoHorizontalRel.h \
    svg/SVGPathSegLinetoVertical.h \
    svg/SVGPathSegLinetoVerticalAbs.h \
    svg/SVGPathSegLinetoVerticalRel.h \
    svg/SVGPathSegList.h \
    svg/SVGPathSegListBuilder.h \
    svg/SVGPathSegMovetoAbs.h \
    svg/SVGPathSegMovetoRel.h \
    svg/SVGPatternElement.h \
    svg/SVGPointList.h \
    svg/SVGPolyElement.h \
    svg/SVGPolygonElement.h \
    svg/SVGPolylineElement.h \
    svg/SVGPreserveAspectRatio.h \
    svg/SVGRadialGradientElement.h \
    svg/SVGRect.h \
    svg/SVGRectElement.h \
    svg/SVGScriptElement.h \
    svg/SVGSetElement.h \
    svg/SVGStopElement.h \
    svg/SVGStringList.h \
    svg/SVGStyleElement.h \
    svg/SVGStyledElement.h \
    svg/SVGSVGElement.h \
    svg/SVGSwitchElement.h \
    svg/SVGSymbolElement.h \
    svg/SVGTests.h \
    svg/SVGTextContentElement.h \
    svg/SVGTextElement.h \
    svg/SVGTextPathElement.h \
    svg/SVGTextPositioningElement.h \
    svg/SVGTitleElement.h \
    svg/SVGTransformable.h \
    svg/SVGTransformDistance.h \
    svg/SVGTransform.h \
    svg/SVGTransformList.h \
    svg/SVGTRefElement.h \
    svg/SVGTSpanElement.h \
    svg/SVGURIReference.h \
    svg/SVGUseElement.h \
    svg/SVGViewElement.h \
    svg/SVGViewSpec.h \
    svg/SVGVKernElement.h \
    svg/SVGZoomAndPan.h \
    svg/SVGZoomEvent.h \
    testing/Internals.h \
    testing/InternalSettings.h \
    testing/MallocStatistics.h \
    testing/MemoryInfo.h \
    testing/TypeConversions.h \
    workers/AbstractWorker.h \
    workers/DedicatedWorkerGlobalScope.h \
    workers/DedicatedWorkerThread.h \
    workers/SharedWorker.h \
    workers/WorkerGlobalScope.h \
    workers/WorkerEventQueue.h \
    workers/Worker.h \
    workers/WorkerLocation.h \
    workers/WorkerMessagingProxy.h \
    workers/WorkerRunLoop.h \
    workers/WorkerScriptLoader.h \
    workers/WorkerThread.h \
    xml/parser/CharacterReferenceParserInlines.h \
    xml/parser/MarkupTokenizerInlines.h \
    xml/parser/XMLDocumentParser.h \
    xml/DOMParser.h \
    xml/NativeXPathNSResolver.h \
    xml/XMLHttpRequest.h \
    xml/XMLHttpRequestUpload.h \
    xml/XMLErrors.h \
    xml/XMLSerializer.h \
    xml/XPathEvaluator.h \
    xml/XPathExpression.h \
    xml/XPathExpressionNode.h \
    xml/XPathFunctions.h \
    xml/XPathNodeSet.h \
    xml/XPathNSResolver.h \
    xml/XPathParser.h \
    xml/XPathPath.h \
    xml/XPathPredicate.h \
    xml/XPathResult.h \
    xml/XPathStep.h \
    xml/XPathUtil.h \
    xml/XPathValue.h \
    xml/XPathVariableReference.h \
    xml/XSLImportRule.h \
    xml/XSLStyleSheet.h \
    xml/XSLTExtensions.h \
    xml/XSLTProcessor.h \
    xml/XSLTUnicodeSort.h

SOURCES += \
    accessibility/qt/AccessibilityObjectQt.cpp \
    page/qt/DragControllerQt.cpp \
    page/qt/EventHandlerQt.cpp \
    platform/graphics/qt/TransformationMatrixQt.cpp \
    platform/graphics/qt/ColorQt.cpp \
    platform/graphics/qt/FontPlatformDataQt.cpp \
    platform/graphics/qt/FloatPointQt.cpp \
    platform/graphics/qt/FloatRectQt.cpp \
    platform/graphics/qt/FloatSizeQt.cpp \
    platform/graphics/qt/GradientQt.cpp \
    platform/graphics/qt/GraphicsContextQt.cpp \
    platform/graphics/qt/IconQt.cpp \
    platform/graphics/qt/ImageBufferQt.cpp \
    platform/graphics/qt/ImageDecoderQt.cpp \
    platform/graphics/qt/ImageQt.cpp \
    platform/graphics/qt/IntPointQt.cpp \
    platform/graphics/qt/IntRectQt.cpp \
    platform/graphics/qt/IntSizeQt.cpp \
    platform/graphics/qt/PathQt.cpp \
    platform/graphics/qt/PatternQt.cpp \
    platform/graphics/qt/StillImageQt.cpp \
    platform/graphics/texmap/GraphicsLayerTextureMapper.cpp \
    platform/graphics/texmap/TextureMapper.cpp \
    platform/graphics/texmap/TextureMapperBackingStore.cpp \
    platform/graphics/texmap/TextureMapperFPSCounter.cpp \
    platform/graphics/texmap/TextureMapperImageBuffer.cpp \
    platform/graphics/texmap/TextureMapperLayer.cpp \
    platform/graphics/texmap/TextureMapperSurfaceBackingStore.cpp \
    platform/graphics/texmap/TextureMapperTile.cpp \
    platform/graphics/texmap/TextureMapperTiledBackingStore.cpp \
    platform/network/DNSResolveQueue.cpp \
    platform/network/MIMESniffing.cpp \
    platform/network/qt/CookieJarQt.cpp \
    platform/network/qt/CredentialStorageQt.cpp \
    platform/network/qt/ResourceHandleQt.cpp \
    platform/network/qt/ResourceRequestQt.cpp \
    platform/network/qt/DNSQt.cpp \
    platform/network/qt/NetworkStateNotifierQt.cpp \
    platform/network/qt/ProxyServerQt.cpp \
    platform/network/qt/QtMIMETypeSniffer.cpp \
    platform/network/qt/QNetworkReplyHandler.cpp \
    platform/Cursor.cpp \
    platform/ContextMenu.cpp \
    platform/ContextMenuItem.cpp \
    platform/qt/ClipboardQt.cpp \
    platform/ContextMenuItemNone.cpp \
    platform/ContextMenuNone.cpp \
    platform/qt/CursorQt.cpp \
    platform/qt/DragDataQt.cpp \
    platform/qt/DragImageQt.cpp \
    platform/qt/EventLoopQt.cpp \
    platform/qt/FileSystemQt.cpp \
    platform/qt/RunLoopQt.cpp \
    platform/qt/SharedBufferQt.cpp \
    platform/qt/ThirdPartyCookiesQt.cpp \
    platform/qt/UserAgentQt.cpp \
    platform/graphics/qt/FontCacheQt.cpp \
    platform/graphics/qt/FontCustomPlatformDataQt.cpp \
    platform/graphics/qt/GlyphPageTreeNodeQt.cpp \
    platform/graphics/qt/SimpleFontDataQt.cpp \
    platform/graphics/qt/TileQt.cpp \
    platform/qt/KURLQt.cpp \
    platform/qt/MIMETypeRegistryQt.cpp \
    platform/qt/PasteboardQt.cpp \
    platform/qt/PlatformKeyboardEventQt.cpp \
    platform/qt/PlatformScreenQt.cpp \
    platform/qt/RenderThemeQStyle.cpp \
    platform/qt/RenderThemeQt.cpp \
    platform/qt/RenderThemeQtMobile.cpp \
    platform/qt/ScrollbarThemeQStyle.cpp \
    platform/qt/ScrollbarThemeQt.cpp \
    platform/qt/ScrollViewQt.cpp \
    platform/qt/SharedTimerQt.cpp \
    platform/qt/SoundQt.cpp \
    platform/qt/LoggingQt.cpp \
    platform/qt/LanguageQt.cpp \
    platform/qt/LocalizedStringsQt.cpp \
    platform/qt/TemporaryLinkStubsQt.cpp \
    platform/text/qt/TextBoundariesQt.cpp \
    platform/text/qt/TextBreakIteratorInternalICUQt.cpp \
    platform/qt/WidgetQt.cpp

use?(LIBXML2) {
    HEADERS += xml/parser/XMLDocumentParserScope.h
    SOURCES += \
            xml/parser/XMLDocumentParserLibxml2.cpp \
            xml/parser/XMLDocumentParserScope.cpp
} else {
    SOURCES += xml/parser/XMLDocumentParserQt.cpp
}

enable?(SMOOTH_SCROLLING) {
    HEADERS += platform/ScrollAnimatorNone.h
    SOURCES += platform/ScrollAnimatorNone.cpp
}

win32-*|wince* {
    HEADERS += platform/win/SystemInfo.h
    SOURCES += \
        platform/win/SystemInfo.cpp \
        platform/graphics/win/TransformationMatrixWin.cpp
}

mac {
    SOURCES += \
        platform/cf/SharedBufferCF.cpp \
        platform/text/cf/AtomicStringCF.cpp \
        platform/text/cf/StringCF.cpp \
        platform/text/cf/StringImplCF.cpp
}

contains(QT_CONFIG,icu)|mac: SOURCES += platform/text/TextBreakIteratorICU.cpp
mac {
    # For Mac we use the same SmartReplace implementation as the Apple port.
    SOURCES += editing/SmartReplaceCF.cpp
    INCLUDEPATH += $$PWD/icu
} else {
    SOURCES += editing/SmartReplaceICU.cpp
}

enable?(NETSCAPE_PLUGIN_API) {

    SOURCES += plugins/npapi.cpp

    unix {
        mac {
            SOURCES += \
                plugins/mac/PluginPackageMac.cpp
            OBJECTIVE_SOURCES += \
                platform/text/mac/StringImplMac.mm \
                platform/mac/WebCoreNSStringExtras.mm \
                plugins/mac/PluginViewMac.mm
        } else {
            SOURCES += \
                plugins/qt/PluginPackageQt.cpp \
                plugins/qt/PluginViewQt.cpp
        }
    }

    win32-* {
        INCLUDEPATH += $$PWD/plugins/win \
                       $$PWD/platform/win \
                       $$PWD/platform/graphics/win

        SOURCES += plugins/win/PluginDatabaseWin.cpp \
                   plugins/win/PluginPackageWin.cpp \
                   plugins/win/PluginMessageThrottlerWin.cpp \
                   plugins/win/PluginViewWin.cpp \
                   platform/win/BitmapInfo.cpp \
                   platform/win/WebCoreInstanceHandle.cpp
    }

} else {
    SOURCES += \
        plugins/PluginPackageNone.cpp \
        plugins/PluginViewNone.cpp
}

use?(PLUGIN_BACKEND_XLIB) {
    SOURCES += plugins/qt/QtX11ImageConversion.cpp
    HEADERS += plugins/qt/QtX11ImageConversion.h
}

enable?(SQL_DATABASE) {
    SOURCES += \
        Modules/webdatabase/ChangeVersionWrapper.cpp \
        Modules/webdatabase/DatabaseBackend.cpp \
        Modules/webdatabase/DatabaseBackendContext.cpp \
        Modules/webdatabase/DatabaseBackendSync.cpp \
        Modules/webdatabase/DatabaseBase.cpp \
        Modules/webdatabase/DatabaseManager.cpp \
        Modules/webdatabase/DatabaseTask.cpp \
        Modules/webdatabase/DatabaseThread.cpp \
        Modules/webdatabase/DatabaseTracker.cpp \
        Modules/webdatabase/OriginLock.cpp \
        Modules/webdatabase/SQLException.cpp \
        Modules/webdatabase/SQLResultSet.cpp \
        Modules/webdatabase/SQLResultSetRowList.cpp \
        Modules/webdatabase/SQLStatement.cpp \
        Modules/webdatabase/SQLStatementBackend.cpp \
        Modules/webdatabase/SQLStatementSync.cpp \
        Modules/webdatabase/SQLTransaction.cpp \
        Modules/webdatabase/SQLTransactionBackend.cpp \
        Modules/webdatabase/SQLTransactionBackendSync.cpp \
        Modules/webdatabase/SQLTransactionClient.cpp \
        Modules/webdatabase/SQLTransactionCoordinator.cpp \
        Modules/webdatabase/SQLTransactionStateMachine.cpp \
        Modules/webdatabase/SQLTransactionSync.cpp \

    SOURCES += \
        bindings/js/JSCustomSQLStatementErrorCallback.cpp \
        bindings/js/JSSQLResultSetRowListCustom.cpp \
        bindings/js/JSSQLTransactionCustom.cpp \
        bindings/js/JSSQLTransactionSyncCustom.cpp
}

enable?(INDEXED_DATABASE) {
    HEADERS += \
        bindings/js/IDBBindingUtilities.h \

    HEADERS += \
        Modules/indexeddb/IDBAny.h \
        Modules/indexeddb/IDBBackingStore.h \
        Modules/indexeddb/IDBCallbacks.h \
        Modules/indexeddb/IDBCursor.h \
        Modules/indexeddb/IDBCursorBackendImpl.h \
        Modules/indexeddb/IDBCursorBackendInterface.h \
        Modules/indexeddb/IDBDatabase.h \
        Modules/indexeddb/IDBDatabaseBackendImpl.h \
        Modules/indexeddb/IDBDatabaseBackendInterface.h \
        Modules/indexeddb/IDBDatabaseError.h \
        Modules/indexeddb/IDBDatabaseException.h \
        Modules/indexeddb/IDBEventDispatcher.h \
        Modules/indexeddb/IDBFactory.h \
        Modules/indexeddb/IDBFactoryBackendImpl.h \
        Modules/indexeddb/IDBFactoryBackendInterface.h \
        Modules/indexeddb/IDBHistograms.h \
        Modules/indexeddb/IDBIndex.h \
        Modules/indexeddb/IDBKey.h \
        Modules/indexeddb/IDBKeyPath.h \
        Modules/indexeddb/IDBKeyRange.h \
        Modules/indexeddb/IDBLevelDBCoding.h \
        Modules/indexeddb/IDBObjectStore.h \
        Modules/indexeddb/IDBObjectStoreBackendImpl.h \
        Modules/indexeddb/IDBOpenDBRequest.h \
        Modules/indexeddb/IDBRequest.h \
        Modules/indexeddb/IDBTransaction.h \
        Modules/indexeddb/IDBTransactionBackendImpl.h \
        Modules/indexeddb/IDBTransactionCoordinator.h \
        Modules/indexeddb/IDBVersionChangeEvent.h \
        Modules/indexeddb/IndexedDB.h

    SOURCES += \
        bindings/js/IDBBindingUtilities.cpp \
        bindings/js/JSIDBAnyCustom.cpp \
        bindings/js/JSIDBDatabaseCustom.cpp \
        bindings/js/JSIDBObjectStoreCustom.cpp

    SOURCES += \
        inspector/InspectorIndexedDBAgent.cpp

    SOURCES += \
        Modules/indexeddb/DOMWindowIndexedDatabase.cpp \
        Modules/indexeddb/IDBAny.cpp \
        Modules/indexeddb/IDBBackingStore.cpp \
        Modules/indexeddb/IDBCursor.cpp \
        Modules/indexeddb/IDBCursorBackendImpl.cpp \
        Modules/indexeddb/IDBCursorWithValue.cpp \
        Modules/indexeddb/IDBDatabase.cpp \
        Modules/indexeddb/IDBDatabaseBackendImpl.cpp \
        Modules/indexeddb/IDBDatabaseCallbacksImpl.cpp \
        Modules/indexeddb/IDBDatabaseException.cpp \
        Modules/indexeddb/IDBEventDispatcher.cpp \
        Modules/indexeddb/IDBFactory.cpp \
        Modules/indexeddb/IDBFactoryBackendInterface.cpp \
        Modules/indexeddb/IDBFactoryBackendImpl.cpp \
        Modules/indexeddb/IDBIndex.cpp \
        Modules/indexeddb/IDBKey.cpp \
        Modules/indexeddb/IDBKeyPath.cpp \
        Modules/indexeddb/IDBKeyRange.cpp \
        Modules/indexeddb/IDBLevelDBCoding.cpp \
        Modules/indexeddb/IDBObjectStore.cpp \
        Modules/indexeddb/IDBObjectStoreBackendImpl.cpp \
        Modules/indexeddb/IDBOpenDBRequest.cpp \
        Modules/indexeddb/IDBPendingTransactionMonitor.cpp \
        Modules/indexeddb/IDBRequest.cpp \
        Modules/indexeddb/IDBTransaction.cpp \
        Modules/indexeddb/IDBTransactionBackendImpl.cpp \
        Modules/indexeddb/IDBTransactionCoordinator.cpp \
        Modules/indexeddb/IDBVersionChangeEvent.cpp \
        Modules/indexeddb/PageGroupIndexedDatabase.cpp \
        Modules/indexeddb/WorkerGlobalScopeIndexedDatabase.cpp

    use?(leveldb):!use?(system_leveldb): WEBKIT += leveldb

}

enable?(DATA_TRANSFER_ITEMS) {
    HEADERS += \
        dom/DataTransferItem.h \
        dom/DataTransferItemList.h \
        dom/StringCallback.h \
        platform/qt/DataTransferItemQt.h \
        platform/qt/DataTransferItemListQt.h
    SOURCES += \
        dom/DataTransferItem.cpp \
        dom/StringCallback.cpp \
        platform/qt/DataTransferItemQt.cpp \
        platform/qt/DataTransferItemListQt.cpp
}

enable?(FILE_SYSTEM) {
    HEADERS += \
        Modules/filesystem/AsyncFileWriter.h \
        Modules/filesystem/DOMFilePath.h \
        Modules/filesystem/DOMFileSystem.h \
        Modules/filesystem/DOMFileSystemBase.h \
        Modules/filesystem/DOMFileSystemSync.h \
        Modules/filesystem/DirectoryEntry.h \
        Modules/filesystem/DirectoryEntrySync.h \
        Modules/filesystem/DirectoryReader.h \
        Modules/filesystem/DirectoryReaderBase.h \
        Modules/filesystem/DirectoryReaderSync.h \
        Modules/filesystem/EntriesCallback.h \
        Modules/filesystem/Entry.h \
        Modules/filesystem/EntryArray.h \
        Modules/filesystem/EntryArraySync.h \
        Modules/filesystem/EntryBase.h \
        Modules/filesystem/EntryCallback.h \
        Modules/filesystem/EntrySync.h \
        Modules/filesystem/ErrorCallback.h \
        Modules/filesystem/FileCallback.h \
        Modules/filesystem/FileEntry.h \
        Modules/filesystem/FileEntrySync.h \
        Modules/filesystem/FileSystemCallback.h \
        Modules/filesystem/FileSystemCallbacks.h \
        Modules/filesystem/FileSystemFlags.h \
        Modules/filesystem/FileWriter.h \
        Modules/filesystem/FileWriterBase.h \
        Modules/filesystem/FileWriterBaseCallback.h \
        Modules/filesystem/FileWriterCallback.h \
        Modules/filesystem/FileWriterClient.h \
        Modules/filesystem/FileWriterSync.h \
        Modules/filesystem/LocalFileSystem.h \
        Modules/filesystem/Metadata.h \
        Modules/filesystem/MetadataCallback.h \
        platform/AsyncFileSystem.h \
        platform/AsyncFileSystemCallbacks.h \
        platform/FileMetadata.h

    SOURCES += \
        bindings/js/JSEntryCustom.cpp \
        bindings/js/JSEntrySyncCustom.cpp \
        platform/AsyncFileSystem.cpp
}

enable?(MEDIA_SOURCE) {
    HEADERS += \
        Modules/mediasource/MediaSource.h \
        Modules/mediasource/MediaSourceRegistry.h \
        Modules/mediasource/SourceBuffer.h \
        Modules/mediasource/SourceBufferList.h
    SOURCES += \
        Modules/mediasource/MediaSource.cpp \
        Modules/mediasource/MediaSourceRegistry.cpp \
        Modules/mediasource/SourceBuffer.cpp \
        Modules/mediasource/SourceBufferList.cpp
}

enable?(ICONDATABASE) {
    SOURCES += \
        loader/icon/IconDatabase.cpp \
        loader/icon/IconRecord.cpp \
        loader/icon/PageURLRecord.cpp
}

enable?(WORKERS) {
    SOURCES += \
        bindings/js/JSDedicatedWorkerGlobalScopeCustom.cpp \
        bindings/js/JSWorkerGlobalScopeBase.cpp \
        bindings/js/JSWorkerGlobalScopeCustom.cpp \
        bindings/js/JSWorkerCustom.cpp \
        bindings/js/WorkerScriptController.cpp \
        bindings/js/WorkerScriptDebugServer.cpp

    SOURCES += \
        loader/WorkerThreadableLoader.cpp \
        page/WorkerNavigator.cpp \
        workers/AbstractWorker.cpp \
        workers/DedicatedWorkerGlobalScope.cpp \
        workers/DedicatedWorkerThread.cpp \
        workers/Worker.cpp \
        workers/WorkerGlobalScope.cpp \
        workers/WorkerEventQueue.cpp \
        workers/WorkerLocation.cpp \
        workers/WorkerMessagingProxy.cpp \
        workers/WorkerRunLoop.cpp \
        workers/WorkerThread.cpp \
        workers/WorkerScriptLoader.cpp
}

enable?(SHARED_WORKERS) {
    SOURCES += \
        bindings/js/JSSharedWorkerCustom.cpp

    SOURCES += \
        workers/DefaultSharedWorkerRepository.cpp \
        workers/SharedWorker.cpp \
        workers/SharedWorkerGlobalScope.cpp \
        workers/SharedWorkerRepository.cpp \
        workers/SharedWorkerThread.cpp
}

enable?(INPUT_SPEECH) {
    SOURCES += \
        page/SpeechInput.cpp \
        page/SpeechInputEvent.cpp \
        page/SpeechInputResult.cpp \
        page/SpeechInputResultList.cpp \
        rendering/RenderInputSpeech.cpp
}

enable?(SCRIPTED_SPEECH) {
    SOURCES += # FIXME!
}

enable?(QUOTA) {
    HEADERS += \
        Modules/quota/DOMWindowQuota.idl \
        Modules/quota/NavigatorStorageQuota.idl \
        Modules/quota/StorageErrorCallback.h \
        Modules/quota/StorageInfo.h \
        Modules/quota/StorageQuota.h \
        Modules/quota/StorageQuotaCallback.h \
        Modules/quota/StorageUsageCallback.h

    SOURCES += \
        Modules/quota/DOMWindowQuota.cpp \
        Modules/quota/NavigatorStorageQuota.cpp \
        Modules/quota/StorageErrorCallback.cpp \
        Modules/quota/StorageInfo.cpp \
        Modules/quota/StorageQuota.cpp
    enable?(WORKERS) {
        HEADERS += \
            Modules/quota/NavigatorStorageQuota.idl \
            Modules/quota/WorkerNavigatorStorageQuota.h
        SOURCES += \
            Modules/quota/WorkerNavigatorStorageQuota.h
    }
}

enable?(GAMEPAD) {
    HEADERS += \
        Modules/gamepad/Gamepad.h\
        Modules/gamepad/GamepadList.h \
        Modules/gamepad/NavigatorGamepad.h \
        platform/linux/GamepadDeviceLinux.h \
        platform/Gamepads.h

    SOURCES += \
        Modules/gamepad/Gamepad.cpp \
        Modules/gamepad/GamepadList.cpp \
        Modules/gamepad/NavigatorGamepad.cpp \
        platform/linux/GamepadDeviceLinux.cpp \
        platform/qt/GamepadsQt.cpp
}

use?(GSTREAMER) {
    HEADERS += \
            platform/graphics/gstreamer/GRefPtrGStreamer.h \
            platform/graphics/gstreamer/GStreamerUtilities.h \
            platform/graphics/gstreamer/GStreamerVersioning.h

    SOURCES += \
            platform/graphics/gstreamer/GRefPtrGStreamer.cpp \
            platform/graphics/gstreamer/GStreamerUtilities.cpp \
            platform/graphics/gstreamer/GStreamerVersioning.cpp
}

enable?(VIDEO) {
    SOURCES += \
        html/HTMLAudioElement.cpp \
        html/HTMLMediaElement.cpp \
        html/HTMLSourceElement.cpp \
        html/HTMLVideoElement.cpp \
        html/MediaController.cpp \
        html/MediaFragmentURIParser.cpp \
        html/shadow/MediaControlElementTypes.cpp \
        html/shadow/MediaControlElements.cpp \
        html/TimeRanges.cpp \
        platform/graphics/MediaPlayer.cpp \
        rendering/RenderVideo.cpp \
        rendering/RenderMedia.cpp \
        rendering/RenderMediaControls.cpp \
        rendering/RenderMediaControlElements.cpp

    use?(GSTREAMER) {
        HEADERS += \
            platform/graphics/gstreamer/GStreamerGWorld.h \
            platform/graphics/gstreamer/MediaPlayerPrivateGStreamerBase.h \
            platform/graphics/gstreamer/MediaPlayerPrivateGStreamer.h \
            platform/graphics/gstreamer/VideoSinkGStreamer.h \
            platform/graphics/gstreamer/WebKitWebSourceGStreamer.h \
            platform/graphics/gstreamer/ImageGStreamer.h
        SOURCES += \
            platform/graphics/gstreamer/GStreamerGWorld.cpp \
            platform/graphics/gstreamer/MediaPlayerPrivateGStreamerBase.cpp \
            platform/graphics/gstreamer/MediaPlayerPrivateGStreamer.cpp \
            platform/graphics/gstreamer/VideoSinkGStreamer.cpp \
            platform/graphics/gstreamer/WebKitWebSourceGStreamer.cpp \
            platform/graphics/gstreamer/ImageGStreamerQt.cpp

        use?(NATIVE_FULLSCREEN_VIDEO) {
            HEADERS += \
                platform/graphics/gstreamer/FullscreenVideoControllerGStreamer.h \
                platform/graphics/gstreamer/PlatformVideoWindow.h \
                platform/graphics/gstreamer/PlatformVideoWindowPrivate.h
            SOURCES += \
                platform/graphics/gstreamer/FullscreenVideoControllerGStreamer.cpp \
                platform/graphics/gstreamer/PlatformVideoWindowQt.cpp
        }

    } else:use?(QT_MULTIMEDIA) {
        HEADERS += \
            platform/graphics/qt/MediaPlayerPrivateQt.h

        SOURCES += \
            platform/graphics/qt/MediaPlayerPrivateQt.cpp
    }
}

enable?(WEB_AUDIO) {
    HEADERS += \
        Modules/webaudio/AsyncAudioDecoder.h \
        Modules/webaudio/AudioBasicInspectorNode.h \
        Modules/webaudio/AudioBasicProcessorNode.h \
        Modules/webaudio/AudioBufferCallback.h \
        Modules/webaudio/AudioBuffer.h \
        Modules/webaudio/AudioBufferSourceNode.h \
        Modules/webaudio/ChannelMergerNode.h \
        Modules/webaudio/ChannelSplitterNode.h \
        Modules/webaudio/AudioContext.h \
        Modules/webaudio/AudioDestinationNode.h \
        Modules/webaudio/GainNode.h \
        Modules/webaudio/AudioListener.h \
        Modules/webaudio/AudioNode.h \
        Modules/webaudio/AudioNodeInput.h \
        Modules/webaudio/AudioNodeOutput.h \
        Modules/webaudio/PannerNode.h \
        Modules/webaudio/AudioParam.h \
        Modules/webaudio/AudioParamTimeline.h \
        Modules/webaudio/AudioProcessingEvent.h \
        Modules/webaudio/AudioScheduledSourceNode.h \
        Modules/webaudio/AudioSummingJunction.h \
        Modules/webaudio/BiquadDSPKernel.h \
        Modules/webaudio/BiquadFilterNode.h \
        Modules/webaudio/BiquadProcessor.h \
        Modules/webaudio/ConvolverNode.h \
        Modules/webaudio/DefaultAudioDestinationNode.h \
        Modules/webaudio/DelayDSPKernel.h \
        Modules/webaudio/DelayNode.h \
        Modules/webaudio/DelayProcessor.h \
        Modules/webaudio/DynamicsCompressorNode.h \
        Modules/webaudio/ScriptProcessorNode.h \
        Modules/webaudio/MediaElementAudioSourceNode.h \
        Modules/webaudio/MediaStreamAudioSourceNode.h \
        Modules/webaudio/OfflineAudioCompletionEvent.h \
        Modules/webaudio/OfflineAudioContext.h \
        Modules/webaudio/OfflineAudioDestinationNode.h \
        Modules/webaudio/OscillatorNode.h \
        Modules/webaudio/RealtimeAnalyser.h \
        Modules/webaudio/AnalyserNode.h \
        Modules/webaudio/WaveShaperDSPKernel.h \
        Modules/webaudio/WaveShaperNode.h \
        Modules/webaudio/WaveShaperProcessor.h \
        Modules/webaudio/PeriodicWave.h \
        platform/audio/AudioArray.h \
        platform/audio/AudioBus.h \
        platform/audio/AudioChannel.h \
        platform/audio/AudioDestination.h \
        platform/audio/AudioDSPKernel.h \
        platform/audio/AudioDSPKernelProcessor.h \
        platform/audio/AudioFIFO.h \
        platform/audio/AudioFileReader.h \
        platform/audio/AudioIOCallback.h \
        platform/audio/AudioProcessor.h \
        platform/audio/AudioPullFIFO.h \
        platform/audio/AudioResampler.h \
        platform/audio/AudioResamplerKernel.h \
        platform/audio/AudioSourceProviderClient.h \
        platform/audio/AudioSourceProvider.h \
        platform/audio/AudioUtilities.h \
        platform/audio/Biquad.h \
        platform/audio/Cone.h \
        platform/audio/DenormalDisabler.h \
        platform/audio/DirectConvolver.h \
        platform/audio/Distance.h \
        platform/audio/DownSampler.h \
        platform/audio/DynamicsCompressor.h \
        platform/audio/DynamicsCompressorKernel.h \
        platform/audio/EqualPowerPanner.h \
        platform/audio/FFTConvolver.h \
        platform/audio/FFTFrame.h \
        platform/audio/HRTFDatabase.h \
        platform/audio/HRTFDatabaseLoader.h \
        platform/audio/HRTFElevation.h \
        platform/audio/HRTFKernel.h \
        platform/audio/HRTFPanner.h \
        platform/audio/MultiChannelResampler.h \
        platform/audio/Panner.h \
        platform/audio/ReverbAccumulationBuffer.h \
        platform/audio/ReverbConvolver.h \
        platform/audio/ReverbConvolverStage.h \
        platform/audio/Reverb.h \
        platform/audio/ReverbInputBuffer.h \
        platform/audio/SincResampler.h \
        platform/audio/UpSampler.h \
        platform/audio/VectorMath.h \
        platform/audio/ZeroPole.h

    SOURCES += \
        bindings/js/JSAudioBufferSourceNodeCustom.cpp \
        bindings/js/JSAudioContextCustom.cpp \
        bindings/js/JSBiquadFilterNodeCustom.cpp \
        bindings/js/JSOscillatorNodeCustom.cpp \
        bindings/js/JSPannerNodeCustom.cpp \
        Modules/webaudio/AsyncAudioDecoder.cpp \
        Modules/webaudio/AudioBasicInspectorNode.cpp \
        Modules/webaudio/AudioBasicProcessorNode.cpp \
        Modules/webaudio/AudioBuffer.cpp \
        Modules/webaudio/AudioBufferSourceNode.cpp \
        Modules/webaudio/ChannelMergerNode.cpp \
        Modules/webaudio/ChannelSplitterNode.cpp \
        Modules/webaudio/AudioContext.cpp \
        Modules/webaudio/AudioDestinationNode.cpp \
        Modules/webaudio/GainNode.cpp \
        Modules/webaudio/AudioListener.cpp \
        Modules/webaudio/AudioNode.cpp \
        Modules/webaudio/AudioNodeInput.cpp \
        Modules/webaudio/AudioNodeOutput.cpp \
        Modules/webaudio/PannerNode.cpp \
        Modules/webaudio/AudioParam.cpp \
        Modules/webaudio/AudioParamTimeline.cpp \
        Modules/webaudio/AudioProcessingEvent.cpp \
        Modules/webaudio/AudioScheduledSourceNode.cpp \
        Modules/webaudio/AudioSummingJunction.cpp \
        Modules/webaudio/BiquadDSPKernel.cpp \
        Modules/webaudio/BiquadFilterNode.cpp \
        Modules/webaudio/BiquadProcessor.cpp \
        Modules/webaudio/ConvolverNode.cpp \
        Modules/webaudio/DefaultAudioDestinationNode.cpp \
        Modules/webaudio/DelayDSPKernel.cpp \
        Modules/webaudio/DelayNode.cpp \
        Modules/webaudio/DelayProcessor.cpp \
        Modules/webaudio/DynamicsCompressorNode.cpp \
        Modules/webaudio/ScriptProcessorNode.cpp \
        Modules/webaudio/MediaElementAudioSourceNode.cpp \
        Modules/webaudio/MediaStreamAudioSourceNode.cpp \
        Modules/webaudio/OfflineAudioCompletionEvent.cpp \
        Modules/webaudio/OfflineAudioContext.cpp \
        Modules/webaudio/OfflineAudioDestinationNode.cpp \
        Modules/webaudio/OscillatorNode.cpp \
        Modules/webaudio/RealtimeAnalyser.cpp \
        Modules/webaudio/AnalyserNode.cpp \
        Modules/webaudio/WaveShaperDSPKernel.cpp \
        Modules/webaudio/WaveShaperNode.cpp \
        Modules/webaudio/WaveShaperProcessor.cpp \
        Modules/webaudio/PeriodicWave.cpp \
        platform/audio/AudioBus.cpp \
        platform/audio/AudioChannel.cpp \
        platform/audio/AudioDSPKernelProcessor.cpp \
        platform/audio/AudioFIFO.cpp \
        platform/audio/AudioPullFIFO.cpp \
        platform/audio/AudioResampler.cpp \
        platform/audio/AudioResamplerKernel.cpp \
        platform/audio/AudioUtilities.cpp \
        platform/audio/Biquad.cpp \
        platform/audio/Cone.cpp \
        platform/audio/DirectConvolver.cpp \
        platform/audio/Distance.cpp \
        platform/audio/DownSampler.cpp \
        platform/audio/DynamicsCompressor.cpp \
        platform/audio/DynamicsCompressorKernel.cpp \
        platform/audio/EqualPowerPanner.cpp \
        platform/audio/FFTConvolver.cpp \
        platform/audio/FFTFrame.cpp \
        platform/audio/FFTFrameStub.cpp \
        platform/audio/HRTFDatabase.cpp \
        platform/audio/HRTFDatabaseLoader.cpp \
        platform/audio/HRTFElevation.cpp \
        platform/audio/HRTFKernel.cpp \
        platform/audio/HRTFPanner.cpp \
        platform/audio/MultiChannelResampler.cpp \
        platform/audio/Panner.cpp \
        platform/audio/qt/AudioBusQt.cpp \
        platform/audio/ReverbAccumulationBuffer.cpp \
        platform/audio/ReverbConvolver.cpp \
        platform/audio/ReverbConvolverStage.cpp \
        platform/audio/Reverb.cpp \
        platform/audio/ReverbInputBuffer.cpp \
        platform/audio/SincResampler.cpp \
        platform/audio/UpSampler.cpp \
        platform/audio/VectorMath.cpp \
        platform/audio/ZeroPole.cpp

    use?(GSTREAMER) {
        HEADERS += \
            platform/audio/gstreamer/AudioDestinationGStreamer.h \
            platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.h
        SOURCES += \
            platform/audio/gstreamer/AudioDestinationGStreamer.cpp \
            platform/audio/gstreamer/AudioFileReaderGStreamer.cpp \
            platform/audio/gstreamer/FFTFrameGStreamer.cpp \
            platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp
    }
}

enable?(FULLSCREEN_API) {
    SOURCES += \
        rendering/RenderFullScreen.cpp
    HEADERS += \
        rendering/RenderFullScreen.h
}

enable?(XSLT) {
    SOURCES += \
        bindings/js/JSXSLTProcessorCustom.cpp

    SOURCES += xml/XMLTreeViewer.cpp
    HEADERS += xml/XMLTreeViewer.h

    use?(LIBXML2) {
        SOURCES += \
            xml/XSLTProcessor.cpp \
            xml/XSLTProcessorLibxslt.cpp \
            dom/TransformSourceLibxslt.cpp \
            xml/XSLStyleSheetLibxslt.cpp \
            xml/XSLImportRule.cpp \
            xml/XSLTExtensions.cpp \
            xml/XSLImportRule.cpp \
            xml/XSLTUnicodeSort.cpp

            HEADERS += \
                xml/XSLImportRule.h \
                xml/XSLTExtensions.h \
                xml/XSLImportRule.h \
                xml/XSLTUnicodeSort.h

    } else {
        SOURCES += \
            dom/TransformSourceQt.cpp \
            xml/XSLStyleSheetQt.cpp \
            xml/XSLTProcessor.cpp \
            xml/XSLTProcessorQt.cpp
    }
}

enable?(FILTERS) {
    SOURCES += \
        platform/graphics/cpu/arm/filters/FELightingNEON.cpp \
        platform/graphics/filters/texmap/CustomFilterValidatedProgramTextureMapper.cpp \
        platform/graphics/filters/CustomFilterGlobalContext.cpp \
        platform/graphics/filters/CustomFilterOperation.cpp \
        platform/graphics/filters/CustomFilterParameterList.cpp \
        platform/graphics/filters/ValidatedCustomFilterOperation.cpp \
        platform/graphics/filters/CustomFilterProgram.cpp \
        platform/graphics/filters/CustomFilterProgramInfo.cpp \
        platform/graphics/filters/CustomFilterCompiledProgram.cpp \
        platform/graphics/filters/CustomFilterMesh.cpp \
        platform/graphics/filters/CustomFilterMeshGenerator.cpp \
        platform/graphics/filters/CustomFilterRenderer.cpp \
        platform/graphics/filters/CustomFilterValidatedProgram.cpp \
        platform/graphics/filters/DistantLightSource.cpp \
        platform/graphics/filters/FEBlend.cpp \
        platform/graphics/filters/FEColorMatrix.cpp \
        platform/graphics/filters/FEComponentTransfer.cpp \
        platform/graphics/filters/FEComposite.cpp \
        platform/graphics/filters/FEConvolveMatrix.cpp \
        platform/graphics/filters/FECustomFilter.cpp \
        platform/graphics/filters/FEDiffuseLighting.cpp \
        platform/graphics/filters/FEDisplacementMap.cpp \
        platform/graphics/filters/FEDropShadow.cpp \
        platform/graphics/filters/FEFlood.cpp \
        platform/graphics/filters/FEGaussianBlur.cpp \
        platform/graphics/filters/FELighting.cpp \
        platform/graphics/filters/FEMerge.cpp \
        platform/graphics/filters/FEMorphology.cpp \
        platform/graphics/filters/FEOffset.cpp \
        platform/graphics/filters/FESpecularLighting.cpp \
        platform/graphics/filters/FETile.cpp \
        platform/graphics/filters/FETurbulence.cpp \
        platform/graphics/filters/FilterOperations.cpp \
        platform/graphics/filters/FilterOperation.cpp \
        platform/graphics/filters/FilterEffect.cpp \
        platform/graphics/filters/PointLightSource.cpp \
        platform/graphics/filters/SpotLightSource.cpp \
        platform/graphics/filters/SourceAlpha.cpp \
        platform/graphics/filters/SourceGraphic.cpp \
}

enable?(MATHML) {
    SOURCES += \
        mathml/MathMLElement.cpp \
        mathml/MathMLInlineContainerElement.cpp \
        mathml/MathMLMathElement.cpp \
        mathml/MathMLTextElement.cpp \
        rendering/mathml/RenderMathMLBlock.cpp \
        rendering/mathml/RenderMathMLFenced.cpp \
        rendering/mathml/RenderMathMLFraction.cpp \
        rendering/mathml/RenderMathMLMath.cpp \
        rendering/mathml/RenderMathMLOperator.cpp \
        rendering/mathml/RenderMathMLRoot.cpp \
        rendering/mathml/RenderMathMLRow.cpp \
        rendering/mathml/RenderMathMLSpace.cpp \
        rendering/mathml/RenderMathMLSquareRoot.cpp \
        rendering/mathml/RenderMathMLSubSup.cpp \
        rendering/mathml/RenderMathMLUnderOver.cpp

    ALL_IN_ONE_SOURCES +=
        mathml/MathMLAllInOne.cpp
}

enable?(TEXT_AUTOSIZING) {
    SOURCES += # FIXME!
}

have?(qtsensors):enable?(DEVICE_ORIENTATION) {
    HEADERS += \
        platform/qt/DeviceMotionClientQt.h \
        platform/qt/DeviceMotionProviderQt.h \
        platform/qt/DeviceOrientationClientQt.h \
        platform/qt/DeviceOrientationProviderQt.h
    SOURCES += \
        platform/qt/DeviceMotionClientQt.cpp \
        platform/qt/DeviceMotionProviderQt.cpp \
        platform/qt/DeviceOrientationClientQt.cpp \
        platform/qt/DeviceOrientationProviderQt.cpp
}

enable?(SVG) {
    SOURCES += \
# TODO: this-one-is-not-auto-added! FIXME! tmp/SVGElementFactory.cpp \
        bindings/js/JSSVGElementInstanceCustom.cpp \
        bindings/js/JSSVGLengthCustom.cpp \
        bindings/js/JSSVGPathSegCustom.cpp

    SOURCES += \
        css/SVGCSSComputedStyleDeclaration.cpp \
        css/SVGCSSParser.cpp \
        css/SVGCSSStyleSelector.cpp \
        rendering/style/SVGRenderStyle.cpp \
        rendering/style/SVGRenderStyleDefs.cpp \
        rendering/PointerEventsHitRules.cpp \
        rendering/svg/RenderSVGEllipse.cpp \
        rendering/svg/RenderSVGPath.cpp \
        rendering/svg/RenderSVGRect.cpp \
        rendering/svg/RenderSVGShape.cpp \
        rendering/svg/RenderSVGBlock.cpp \
        rendering/svg/RenderSVGContainer.cpp \
        rendering/svg/RenderSVGForeignObject.cpp \
        rendering/svg/RenderSVGGradientStop.cpp \
        rendering/svg/RenderSVGHiddenContainer.cpp \
        rendering/svg/RenderSVGImage.cpp \
        rendering/svg/RenderSVGInline.cpp \
        rendering/svg/RenderSVGInlineText.cpp \
        rendering/svg/RenderSVGModelObject.cpp \
        rendering/svg/RenderSVGResource.cpp \
        rendering/svg/RenderSVGResourceClipper.cpp \
        rendering/svg/RenderSVGResourceContainer.cpp \
        rendering/svg/RenderSVGResourceFilter.cpp \
        rendering/svg/RenderSVGResourceFilterPrimitive.cpp \
        rendering/svg/RenderSVGResourceGradient.cpp \
        rendering/svg/RenderSVGResourceLinearGradient.cpp \
        rendering/svg/RenderSVGResourceMarker.cpp \
        rendering/svg/RenderSVGResourceMasker.cpp \
        rendering/svg/RenderSVGResourcePattern.cpp \
        rendering/svg/RenderSVGResourceRadialGradient.cpp \
        rendering/svg/RenderSVGResourceSolidColor.cpp \
        rendering/svg/RenderSVGRoot.cpp \
        rendering/svg/RenderSVGTSpan.cpp \
        rendering/svg/RenderSVGText.cpp \
        rendering/svg/RenderSVGTextPath.cpp \
        rendering/svg/RenderSVGTransformableContainer.cpp \
        rendering/svg/RenderSVGViewportContainer.cpp \
        rendering/svg/SVGInlineFlowBox.cpp \
        rendering/svg/SVGInlineTextBox.cpp \
        rendering/svg/SVGPathData.cpp \
        rendering/svg/SVGRenderSupport.cpp \
        rendering/svg/SVGRenderTreeAsText.cpp \
        rendering/svg/SVGRenderingContext.cpp \
        rendering/svg/SVGResources.cpp \
        rendering/svg/SVGResourcesCache.cpp \
        rendering/svg/SVGResourcesCycleSolver.cpp \
        rendering/svg/SVGRootInlineBox.cpp \
        rendering/svg/SVGTextChunk.cpp \
        rendering/svg/SVGTextChunkBuilder.cpp \
        rendering/svg/SVGTextLayoutAttributes.cpp \
        rendering/svg/SVGTextLayoutAttributesBuilder.cpp \
        rendering/svg/SVGTextLayoutEngine.cpp \
        rendering/svg/SVGTextLayoutEngineBaseline.cpp \
        rendering/svg/SVGTextLayoutEngineSpacing.cpp \
        rendering/svg/SVGTextMetrics.cpp \
        rendering/svg/SVGTextMetricsBuilder.cpp \
        rendering/svg/SVGTextQuery.cpp \
        rendering/svg/SVGTextRunRenderingContext.cpp \
        svg/animation/SMILTime.cpp \
        svg/animation/SMILTimeContainer.cpp \
        svg/animation/SVGSMILElement.cpp \
        svg/graphics/filters/SVGFEImage.cpp \
        svg/graphics/filters/SVGFilter.cpp \
        svg/graphics/filters/SVGFilterBuilder.cpp \
        svg/graphics/SVGImage.cpp \
        svg/graphics/SVGImageCache.cpp \
        svg/graphics/SVGImageForContainer.cpp \
        svg/properties/SVGAnimatedProperty.cpp \
        svg/properties/SVGAttributeToPropertyMap.cpp \
        svg/properties/SVGPathSegListPropertyTearOff.cpp \
        svg/SVGDocumentExtensions.cpp \
        svg/ColorDistance.cpp \
        svg/SVGAElement.cpp \
        svg/SVGAltGlyphDefElement.cpp \
        svg/SVGAltGlyphElement.cpp \
        svg/SVGAltGlyphItemElement.cpp \
        svg/SVGAngle.cpp \
        svg/SVGAnimateColorElement.cpp \
        svg/SVGAnimatedAngle.cpp \
        svg/SVGAnimatedBoolean.cpp \
        svg/SVGAnimatedColor.cpp \
        svg/SVGAnimatedEnumeration.cpp \
        svg/SVGAnimatedInteger.cpp \
        svg/SVGAnimatedIntegerOptionalInteger.cpp \
        svg/SVGAnimatedLength.cpp \
        svg/SVGAnimatedLengthList.cpp \
        svg/SVGAnimatedNumber.cpp \
        svg/SVGAnimatedNumberList.cpp \
        svg/SVGAnimatedNumberOptionalNumber.cpp \
        svg/SVGAnimatedPath.cpp \
        svg/SVGAnimatedPointList.cpp \
        svg/SVGAnimatedPreserveAspectRatio.cpp \
        svg/SVGAnimatedRect.cpp \
        svg/SVGAnimatedString.cpp \
        svg/SVGAnimatedTransformList.cpp \
        svg/SVGAnimatedType.cpp \
        svg/SVGAnimatedTypeAnimator.cpp \
        svg/SVGAnimateElement.cpp \
        svg/SVGAnimateMotionElement.cpp \
        svg/SVGAnimateTransformElement.cpp \
        svg/SVGAnimationElement.cpp \
        svg/SVGCircleElement.cpp \
        svg/SVGClipPathElement.cpp \
        svg/SVGColor.cpp \
        svg/SVGComponentTransferFunctionElement.cpp \
        svg/SVGCursorElement.cpp \
        svg/SVGDefsElement.cpp \
        svg/SVGDescElement.cpp \
        svg/SVGDocument.cpp \
        svg/SVGElement.cpp \
        svg/SVGElementInstance.cpp \
        svg/SVGElementInstanceList.cpp \
        svg/SVGEllipseElement.cpp \
        svg/SVGException.cpp \
        svg/SVGExternalResourcesRequired.cpp \
        svg/SVGFEBlendElement.cpp \
        svg/SVGFEColorMatrixElement.cpp \
        svg/SVGFEComponentTransferElement.cpp \
        svg/SVGFECompositeElement.cpp \
        svg/SVGFEConvolveMatrixElement.cpp \
        svg/SVGFEDiffuseLightingElement.cpp \
        svg/SVGFEDisplacementMapElement.cpp \
        svg/SVGFEDistantLightElement.cpp \
        svg/SVGFEDropShadowElement.cpp \
        svg/SVGFEFloodElement.cpp \
        svg/SVGFEFuncAElement.cpp \
        svg/SVGFEFuncBElement.cpp \
        svg/SVGFEFuncGElement.cpp \
        svg/SVGFEFuncRElement.cpp \
        svg/SVGFEGaussianBlurElement.cpp \
        svg/SVGFEImageElement.cpp \
        svg/SVGFELightElement.cpp \
        svg/SVGFEMergeElement.cpp \
        svg/SVGFEMergeNodeElement.cpp \
        svg/SVGFEMorphologyElement.cpp \
        svg/SVGFEOffsetElement.cpp \
        svg/SVGFEPointLightElement.cpp \
        svg/SVGFESpecularLightingElement.cpp \
        svg/SVGFESpotLightElement.cpp \
        svg/SVGFETileElement.cpp \
        svg/SVGFETurbulenceElement.cpp \
        svg/SVGFilterElement.cpp \
        svg/SVGFilterPrimitiveStandardAttributes.cpp \
        svg/SVGFitToViewBox.cpp \
        svg/SVGFontData.cpp \
        svg/SVGFontElement.cpp \
        svg/SVGFontFaceElement.cpp \
        svg/SVGFontFaceFormatElement.cpp \
        svg/SVGFontFaceNameElement.cpp \
        svg/SVGFontFaceSrcElement.cpp \
        svg/SVGFontFaceUriElement.cpp \
        svg/SVGForeignObjectElement.cpp \
        svg/SVGGElement.cpp \
        svg/SVGGlyphElement.cpp \
        svg/SVGGlyphRefElement.cpp \
        svg/SVGGradientElement.cpp \
        svg/SVGGraphicsElement.cpp \
        svg/SVGHKernElement.cpp \
        svg/SVGImageElement.cpp \
        svg/SVGImageLoader.cpp \
        svg/SVGLangSpace.cpp \
        svg/SVGLength.cpp \
        svg/SVGLengthContext.cpp \
        svg/SVGLengthList.cpp \
        svg/SVGLinearGradientElement.cpp \
        svg/SVGLineElement.cpp \
        svg/SVGLocatable.cpp \
        svg/SVGMarkerElement.cpp \
        svg/SVGMaskElement.cpp \
        svg/SVGMetadataElement.cpp \
        svg/SVGMissingGlyphElement.cpp \
        svg/SVGMPathElement.cpp \
        svg/SVGNumberList.cpp \
        svg/SVGPaint.cpp \
        svg/SVGParserUtilities.cpp \
        svg/SVGPathBlender.cpp \
        svg/SVGPathBuilder.cpp \
        svg/SVGPathByteStreamBuilder.cpp \
        svg/SVGPathByteStreamSource.cpp \
        svg/SVGPathElement.cpp \
        svg/SVGPathParser.cpp \
        svg/SVGPathSegList.cpp \
        svg/SVGPathSegListBuilder.cpp \
        svg/SVGPathSegListSource.cpp \
        svg/SVGPathStringBuilder.cpp \
        svg/SVGPathStringSource.cpp \
        svg/SVGPathTraversalStateBuilder.cpp \
        svg/SVGPathUtilities.cpp \
        svg/SVGPatternElement.cpp \
        svg/SVGPointList.cpp \
        svg/SVGPolyElement.cpp \
        svg/SVGPolygonElement.cpp \
        svg/SVGPolylineElement.cpp \
        svg/SVGPreserveAspectRatio.cpp \
        svg/SVGRadialGradientElement.cpp \
        svg/SVGRectElement.cpp \
        svg/SVGSVGElement.cpp \
        svg/SVGScriptElement.cpp \
        svg/SVGSetElement.cpp \
        svg/SVGStopElement.cpp \
        svg/SVGStringList.cpp \
        svg/SVGStyleElement.cpp \
        svg/SVGStyledElement.cpp \
        svg/SVGSwitchElement.cpp \
        svg/SVGSymbolElement.cpp \
        svg/SVGTRefElement.cpp \
        svg/SVGTSpanElement.cpp \
        svg/SVGTests.cpp \
        svg/SVGTextContentElement.cpp \
        svg/SVGTextElement.cpp \
        svg/SVGTextPathElement.cpp \
        svg/SVGTextPositioningElement.cpp \
        svg/SVGTitleElement.cpp \
        svg/SVGTransform.cpp \
        svg/SVGTransformDistance.cpp \
        svg/SVGTransformList.cpp \
        svg/SVGTransformable.cpp \
        svg/SVGURIReference.cpp \
        svg/SVGUseElement.cpp \
        svg/SVGVKernElement.cpp \
        svg/SVGViewElement.cpp \
        svg/SVGViewSpec.cpp \
        svg/SVGZoomAndPan.cpp \
        svg/SVGZoomEvent.cpp

    ALL_IN_ONE_SOURCES += \
        rendering/svg/RenderSVGAllInOne.cpp \
        svg/SVGAllInOne.cpp
}

enable?(JAVASCRIPT_DEBUGGER) {
    SOURCES += \
        bindings/js/JSJavaScriptCallFrameCustom.cpp \
        bindings/js/ScriptProfiler.cpp \
        bindings/js/JavaScriptCallFrame.cpp
}


enable?(VIDEO_TRACK) {
    HEADERS += \
        bindings/js/JSTrackCustom.h \
        html/HTMLTrackElement.h \
        html/track/AudioTrack.h \
        html/track/AudioTrackList.h \
        html/track/InbandTextTrack.h \
        html/track/LoadableTextTrack.h \
        html/track/TextTrack.h \
        html/track/TextTrackCue.h \
        html/track/TextTrackCueGeneric.h \
        html/track/TextTrackCueList.h \
        html/track/TextTrackList.h \
        html/track/TrackBase.h \
        html/track/TrackEvent.h \
        html/track/TrackListBase.h \
        html/track/VideoTrack.h \
        html/track/VideoTrackList.h \
        html/track/WebVTTParser.h \
        html/track/WebVTTToken.h \
        html/track/WebVTTTokenizer.h \
        loader/TextTrackLoader.h \
        platform/graphics/AudioTrackPrivate.h \
        platform/graphics/InbandTextTrackPrivate.h \
        platform/graphics/InbandTextTrackPrivateClient.h
        platform/graphics/VideoTrackPrivate.h \

    SOURCES += \
        bindings/js/JSAudioTrackCustom.cpp \
        bindings/js/JSAudioTrackListCustom.cpp \
        bindings/js/JSTextTrackCueCustom.cpp \
        bindings/js/JSTextTrackCustom.cpp \
        bindings/js/JSTrackCustom.cpp \
        bindings/js/JSTrackEventCustom.cpp \
        bindings/js/JSTextTrackListCustom.cpp \
        bindings/js/JSVideoTrackCustom.cpp \
        bindings/js/JSVideoTrackListCustom.cpp \
        html/HTMLTrackElement.cpp \
        html/track/AudioTrack.cpp \
        html/track/AudioTrackList.cpp \
        html/track/InbandTextTrack.cpp \
        html/track/LoadableTextTrack.cpp \
        html/track/TextTrack.cpp \
        html/track/TextTrackCue.cpp \
        html/track/TextTrackCueGeneric.cpp \
        html/track/TextTrackCueList.cpp \
        html/track/TextTrackList.cpp \
        html/track/TrackBase.cpp \
        html/track/TrackEvent.cpp \
        html/track/TrackListBase.cpp \
        html/track/VideoTrack.cpp \
        html/track/VideoTrackList.cpp \
        html/track/WebVTTElement.cpp \
        html/track/WebVTTParser.cpp \
        html/track/WebVTTTokenizer.cpp \
        loader/cache/CachedTextTrack.cpp \
        loader/TextTrackLoader.cpp \
        platform/graphics/TextTrackRepresentation.cpp \
        rendering/RenderTextTrackCue.cpp
}

enable?(WEB_SOCKETS) {
    HEADERS += \
        Modules/websockets/CloseEvent.h \
        Modules/websockets/ThreadableWebSocketChannel.h \
        Modules/websockets/ThreadableWebSocketChannelClientWrapper.h \
        Modules/websockets/WebSocket.h \
        Modules/websockets/WebSocketChannel.h \
        Modules/websockets/WebSocketChannelClient.h \
        Modules/websockets/WebSocketDeflateFramer.h \
        Modules/websockets/WebSocketDeflater.h \
        Modules/websockets/WebSocketExtensionDispatcher.h \
        Modules/websockets/WebSocketExtensionParser.h \
        Modules/websockets/WebSocketExtensionProcessor.h \
        Modules/websockets/WebSocketFrame.h \
        Modules/websockets/WebSocketHandshake.h \
        Modules/websockets/WorkerThreadableWebSocketChannel.h \
        platform/network/qt/SocketStreamHandlePrivate.h

    SOURCES += \
        Modules/websockets/WebSocket.cpp \
        Modules/websockets/WebSocketChannel.cpp \
        Modules/websockets/WebSocketDeflateFramer.cpp \
        Modules/websockets/WebSocketDeflater.cpp \
        Modules/websockets/WebSocketExtensionDispatcher.cpp \
        Modules/websockets/WebSocketExtensionParser.cpp \
        Modules/websockets/WebSocketFrame.cpp \
        Modules/websockets/WebSocketHandshake.cpp \
        Modules/websockets/WorkerThreadableWebSocketChannel.cpp \
        Modules/websockets/ThreadableWebSocketChannel.cpp \
        Modules/websockets/ThreadableWebSocketChannelClientWrapper.cpp \
        platform/network/SocketStreamErrorBase.cpp \
        platform/network/SocketStreamHandleBase.cpp \
        platform/network/qt/SocketStreamHandleQt.cpp

    enable?(WORKERS) {
        HEADERS += \
            Modules/websockets/WorkerThreadableWebSocketChannel.h

        SOURCES += \
            Modules/websockets/WorkerThreadableWebSocketChannel.cpp
    }
}

enable?(WEBGL) {
    HEADERS += \
        html/canvas/CanvasContextAttributes.h \
        html/canvas/WebGLObject.h \
        html/canvas/WebGLActiveInfo.h \
        html/canvas/WebGLBuffer.h \
        html/canvas/WebGLCompressedTextureATC.h \
        html/canvas/WebGLCompressedTexturePVRTC.h \
        html/canvas/WebGLCompressedTextureS3TC.h \
        html/canvas/WebGLContextAttributes.h \
        html/canvas/WebGLContextEvent.h \
        html/canvas/WebGLContextGroup.h \
        html/canvas/WebGLContextObject.h \
        html/canvas/WebGLDebugRendererInfo.h \
        html/canvas/WebGLDebugShaders.h \
        html/canvas/WebGLDepthTexture.h \
        html/canvas/WebGLExtension.h \
        html/canvas/WebGLFramebuffer.h \
        html/canvas/WebGLGetInfo.h \
        html/canvas/WebGLLoseContext.h \
        html/canvas/WebGLProgram.h \
        html/canvas/WebGLRenderbuffer.h \
        html/canvas/WebGLRenderingContext.h \
        html/canvas/WebGLShader.h \
        html/canvas/WebGLShaderPrecisionFormat.h \
        html/canvas/WebGLSharedObject.h \
        html/canvas/EXTDrawBuffers.h \
        html/canvas/EXTTextureFilterAnisotropic.h \
        html/canvas/OESStandardDerivatives.h \
        html/canvas/OESTextureFloat.h \
        html/canvas/OESTextureHalfFloat.h \
        html/canvas/OESVertexArrayObject.h \
        html/canvas/OESElementIndexUint.h \
        html/canvas/WebGLTexture.h \
        html/canvas/WebGLUniformLocation.h \
        html/canvas/WebGLVertexArrayObjectOES.h \

    SOURCES += \
        bindings/js/JSWebGLRenderingContextCustom.cpp

    SOURCES += \
        html/canvas/CanvasContextAttributes.cpp \
        html/canvas/WebGLObject.cpp \
        html/canvas/WebGLBuffer.cpp \
        html/canvas/WebGLCompressedTextureATC.cpp \
        html/canvas/WebGLCompressedTexturePVRTC.cpp \
        html/canvas/WebGLCompressedTextureS3TC.cpp \
        html/canvas/WebGLContextAttributes.cpp \
        html/canvas/WebGLContextEvent.cpp \
        html/canvas/WebGLContextGroup.cpp \
        html/canvas/WebGLContextObject.cpp \
        html/canvas/WebGLDebugRendererInfo.cpp \
        html/canvas/WebGLDebugShaders.cpp \
        html/canvas/WebGLDepthTexture.cpp \
        html/canvas/WebGLExtension.cpp \
        html/canvas/WebGLFramebuffer.cpp \
        html/canvas/WebGLGetInfo.cpp \
        html/canvas/WebGLLoseContext.cpp \
        html/canvas/WebGLProgram.cpp \
        html/canvas/WebGLRenderbuffer.cpp \
        html/canvas/WebGLRenderingContext.cpp \
        html/canvas/WebGLShader.cpp \
        html/canvas/WebGLShaderPrecisionFormat.cpp \
        html/canvas/WebGLSharedObject.cpp \
        html/canvas/EXTDrawBuffers.cpp \
        html/canvas/EXTTextureFilterAnisotropic.cpp \
        html/canvas/OESStandardDerivatives.cpp \
        html/canvas/OESTextureFloat.cpp \
        html/canvas/OESTextureHalfFloat.cpp \
        html/canvas/OESVertexArrayObject.cpp \
        html/canvas/OESElementIndexUint.cpp \
        html/canvas/WebGLTexture.cpp \
        html/canvas/WebGLUniformLocation.cpp \
        html/canvas/WebGLVertexArrayObjectOES.cpp
}

enable?(CANVAS_PROXY) {
    HEADERS += \
        html/canvas/CanvasProxy.h

    SOURCES += \
        html/canvas/CanvasProxy.cpp
}

use?(3D_GRAPHICS) {
    HEADERS += \
        page/scrolling/ScrollingConstraints.h \
        page/scrolling/ScrollingCoordinator.h \
        page/scrolling/ScrollingStateFixedNode.h \
        page/scrolling/ScrollingStateNode.h \
        page/scrolling/ScrollingStateScrollingNode.h \
        page/scrolling/ScrollingStateStickyNode.h \
        page/scrolling/ScrollingStateTree.h \
        page/scrolling/coordinatedgraphics/ScrollingCoordinatorCoordinatedGraphics.h \
        platform/graphics/cpu/arm/GraphicsContext3DNEON.h \
        platform/graphics/ANGLEWebKitBridge.h \
        platform/graphics/Extensions3D.h \
        platform/graphics/GraphicsContext3D.h \
        platform/graphics/gpu/DrawingBuffer.h \
        platform/graphics/gpu/Texture.h \
        platform/graphics/gpu/TilingData.h \
        platform/graphics/opengl/Extensions3DOpenGL.h \
        platform/graphics/texmap/TextureMapperGL.h \
        platform/graphics/texmap/TextureMapperShaderProgram.h \
        platform/graphics/texmap/coordinated/AreaAllocator.h \
        platform/graphics/texmap/coordinated/CompositingCoordinator.h \
        platform/graphics/texmap/coordinated/CoordinatedBackingStore.h \
        platform/graphics/texmap/coordinated/CoordinatedCustomFilterOperation.h \
        platform/graphics/texmap/coordinated/CoordinatedCustomFilterProgram.h \
        platform/graphics/texmap/coordinated/CoordinatedGraphicsLayer.h \
        platform/graphics/texmap/coordinated/CoordinatedGraphicsScene.h \
        platform/graphics/texmap/coordinated/CoordinatedGraphicsState.h \
        platform/graphics/texmap/coordinated/CoordinatedImageBacking.h \
        platform/graphics/texmap/coordinated/CoordinatedSurface.h \
        platform/graphics/texmap/coordinated/CoordinatedTile.h \
        platform/graphics/texmap/coordinated/SurfaceUpdateInfo.h \
        platform/graphics/texmap/coordinated/UpdateAtlas.h

    SOURCES += \
        page/scrolling/ScrollingConstraints.cpp \
        page/scrolling/ScrollingCoordinator.cpp \
        page/scrolling/ScrollingStateFixedNode.cpp \
        page/scrolling/ScrollingStateNode.cpp \
        page/scrolling/ScrollingStateScrollingNode.cpp \
        page/scrolling/ScrollingStateStickyNode.cpp \
        page/scrolling/ScrollingStateTree.cpp \
        page/scrolling/coordinatedgraphics/ScrollingCoordinatorCoordinatedGraphics.cpp \
        page/scrolling/coordinatedgraphics/ScrollingStateNodeCoordinatedGraphics.cpp \
        page/scrolling/coordinatedgraphics/ScrollingStateScrollingNodeCoordinatedGraphics.cpp \
        platform/graphics/ANGLEWebKitBridge.cpp \
        platform/graphics/GraphicsContext3D.cpp \
        platform/graphics/gpu/DrawingBuffer.cpp \
        platform/graphics/gpu/qt/DrawingBufferQt.cpp \
        platform/graphics/gpu/Texture.cpp \
        platform/graphics/gpu/TilingData.cpp \
        platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp \
        platform/graphics/opengl/Extensions3DOpenGLCommon.cpp \
        platform/graphics/qt/GraphicsContext3DQt.cpp \
        platform/graphics/texmap/TextureMapperGL.cpp \
        platform/graphics/texmap/TextureMapperShaderProgram.cpp \
        platform/graphics/texmap/coordinated/AreaAllocator.cpp \
        platform/graphics/texmap/coordinated/CompositingCoordinator.cpp \
        platform/graphics/texmap/coordinated/CoordinatedBackingStore.cpp \
        platform/graphics/texmap/coordinated/CoordinatedGraphicsLayer.cpp \
        platform/graphics/texmap/coordinated/CoordinatedGraphicsScene.cpp \
        platform/graphics/texmap/coordinated/CoordinatedImageBacking.cpp \
        platform/graphics/texmap/coordinated/CoordinatedSurface.cpp \
        platform/graphics/texmap/coordinated/CoordinatedTile.cpp \
        platform/graphics/texmap/coordinated/UpdateAtlas.cpp

    INCLUDEPATH += $$PWD/platform/graphics/gpu

    contains(QT_CONFIG, opengl) | contains(QT_CONFIG, opengles2) {
        !contains(QT_CONFIG, opengles2) {
            SOURCES += \
               platform/graphics/opengl/GraphicsContext3DOpenGL.cpp \
               platform/graphics/opengl/Extensions3DOpenGL.cpp
        } else {
            SOURCES += \
               platform/graphics/opengl/GraphicsContext3DOpenGLES.cpp \
               platform/graphics/opengl/Extensions3DOpenGLES.cpp
        }

        HEADERS += platform/graphics/opengl/Extensions3DOpenGL.h

        SOURCES += \
            platform/graphics/opengl/Extensions3DOpenGLCommon.cpp \
            platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
    }

    WEBKIT += angle

    CONFIG += opengl-shims
    INCLUDEPATH += platform/graphics/gpu
}


enable?(MHTML) {

    INCLUDEPATH += $$PWD/loader/archive/mhtml

    HEADERS += \
        loader/archive/Archive.h \
        page/PageSerializer.h

    SOURCES += \
        loader/archive/Archive.cpp \
        loader/archive/ArchiveFactory.cpp \
        loader/archive/mhtml/MHTMLArchive.cpp \
        loader/archive/mhtml/MHTMLParser.cpp \
        page/PageSerializer.cpp
}

use?(LIBPNG) {
    SOURCES += platform/image-decoders/ico/ICOImageDecoder.cpp \
               platform/image-decoders/png/PNGImageDecoder.cpp
}

use?(LIBJPEG) {
    HEADERS += platform/image-decoders/jpeg/JPEGImageDecoder.h
    SOURCES += platform/image-decoders/jpeg/JPEGImageDecoder.cpp
}

use?(WEBP) {
    HEADERS += platform/image-decoders/webp/WEBPImageDecoder.h
    SOURCES += platform/image-decoders/webp/WEBPImageDecoder.cpp
}

use?(ZLIB) {
    HEADERS += platform/graphics/WOFFFileFormat.h
    SOURCES += platform/graphics/WOFFFileFormat.cpp
}

!have?(sqlite3):exists($${SQLITE3SRCDIR}/sqlite3.c) {
    # Build sqlite3 into WebCore from source
    # somewhat copied from $$QT_SOURCE_TREE/src/plugins/sqldrivers/sqlite/sqlite.pro
    SOURCES += $${SQLITE3SRCDIR}/sqlite3.c
}

win32:!mingw:contains(QMAKE_HOST.arch, x86_64):{
    asm_compiler.commands = ml64 /c
    asm_compiler.commands +=  /Fo ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
    asm_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    asm_compiler.input = ASM_SOURCES
    asm_compiler.variable_out = OBJECTS
    asm_compiler.name = compiling[asm] ${QMAKE_FILE_IN}
    silent:asm_compiler.commands = @echo compiling[asm] ${QMAKE_FILE_IN} && $$asm_compiler.commands
    QMAKE_EXTRA_COMPILERS += asm_compiler

    ASM_SOURCES += \
        plugins/win/PaintHooks.asm
   if(win32-msvc2005|win32-msvc2008):equals(TEMPLATE_PREFIX, "vc") {
        SOURCES += \
            plugins/win/PaintHooks.asm
   }
}

contains(CONFIG, opengl-shims) {
    HEADERS += platform/graphics/OpenGLShims.h
    SOURCES += platform/graphics/OpenGLShims.cpp
    DEFINES += QT_OPENGL_SHIMS=1
}

enable?(opencl) {
    HEADERS += \
        platform/graphics/gpu/opencl/OpenCLHandle.h \
        platform/graphics/gpu/opencl/FilterContextOpenCL.h
    SOURCES += \
        platform/graphics/gpu/opencl/FilterContextOpenCL.cpp \
        platform/graphics/gpu/opencl/OpenCLFEColorMatrix.cpp \
        platform/graphics/gpu/opencl/OpenCLFEFlood.cpp \
        platform/graphics/gpu/opencl/OpenCLFEImage.cpp \
        platform/graphics/gpu/opencl/OpenCLFEMerge.cpp \
        platform/graphics/gpu/opencl/OpenCLFESourceAlpha.cpp \
        platform/graphics/gpu/opencl/OpenCLFESourceGraphic.cpp \
        platform/graphics/gpu/opencl/OpenCLFETurbulence.cpp
}

use?(GRAPHICS_SURFACE) {
    mac {
        SOURCES += platform/graphics/surfaces/mac/GraphicsSurfaceMac.cpp
    }
    win32 {
        SOURCES += platform/graphics/surfaces/win/GraphicsSurfaceWin.cpp
    }
    use?(glx) {
        HEADERS += \
            platform/graphics/surfaces/glx/X11Helper.h \
            platform/graphics/surfaces/glx/GLXConfigSelector.h
        SOURCES += \
            platform/graphics/surfaces/glx/X11Helper.cpp \
            platform/graphics/surfaces/glx/GraphicsSurfaceGLX.cpp
    }
}

build?(qttestsupport) {
    HEADERS += platform/qt/QtTestSupport.h
    SOURCES += platform/qt/QtTestSupport.cpp
}

ALL_IN_ONE_SOURCES += \
    accessibility/AccessibilityAllInOne.cpp \
    inspector/InspectorAllInOne.cpp \
    loader/appcache/ApplicationCacheAllInOne.cpp \
    platform/text/TextAllInOne.cpp \
    rendering/style/StyleAllInOne.cpp \
    html/HTMLElementsAllInOne.cpp \
    editing/EditingAllInOne.cpp \
    rendering/RenderingAllInOne.cpp \
    css/CSSAllInOne.cpp \
    css/MediaAllInOne.cpp \
    dom/DOMAllInOne.cpp \
    bindings/js/JSBindingsAllInOne.cpp

# Make sure the derived sources are built
include(DerivedSources.pri)
