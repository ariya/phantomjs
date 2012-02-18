LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/bindings/js"
    "${WEBCORE_DIR}/bridge/jsc"
)

LIST(APPEND WebCore_IDL_INCLUDES
    bindings/js
)

LIST(APPEND WebCore_SOURCES
    bindings/js/DOMObjectHashTableMap.cpp
    bindings/js/DOMWrapperWorld.cpp
    bindings/js/GCController.cpp
    bindings/js/IDBBindingUtilities.cpp
    bindings/js/JSAttrCustom.cpp
    bindings/js/JSArrayBufferCustom.cpp
    bindings/js/JSDataViewCustom.cpp
    bindings/js/JSCDATASectionCustom.cpp
    bindings/js/JSCSSFontFaceRuleCustom.cpp
    bindings/js/JSCSSImportRuleCustom.cpp
    bindings/js/JSCSSMediaRuleCustom.cpp
    bindings/js/JSCSSPageRuleCustom.cpp
    bindings/js/JSCSSRuleCustom.cpp
    bindings/js/JSCSSRuleListCustom.cpp
    bindings/js/JSCSSStyleDeclarationCustom.cpp
    bindings/js/JSCSSStyleRuleCustom.cpp
    bindings/js/JSCSSValueCustom.cpp
    bindings/js/JSCallbackData.cpp
    bindings/js/JSCanvasRenderingContext2DCustom.cpp
    bindings/js/JSCanvasRenderingContextCustom.cpp
    bindings/js/JSClipboardCustom.cpp
    bindings/js/JSConsoleCustom.cpp
    bindings/js/JSCoordinatesCustom.cpp
    bindings/js/JSCustomPositionCallback.cpp
    bindings/js/JSCustomPositionErrorCallback.cpp
    bindings/js/JSCustomSQLStatementErrorCallback.cpp
    bindings/js/JSCustomVoidCallback.cpp
    bindings/js/JSCustomXPathNSResolver.cpp
    bindings/js/JSDOMApplicationCacheCustom.cpp
    bindings/js/JSDOMBinding.cpp
    bindings/js/JSDOMFormDataCustom.cpp
    bindings/js/JSDOMGlobalObject.cpp
    bindings/js/JSDOMImplementationCustom.cpp
    bindings/js/JSDOMMimeTypeArrayCustom.cpp
    bindings/js/JSDOMPluginArrayCustom.cpp
    bindings/js/JSDOMPluginCustom.cpp
    bindings/js/JSDOMStringMapCustom.cpp
    bindings/js/JSDOMTokenListCustom.cpp
    bindings/js/JSDOMWindowBase.cpp
    bindings/js/JSDOMWindowCustom.cpp
    bindings/js/JSDOMWindowShell.cpp
    bindings/js/JSDOMWrapper.cpp
    bindings/js/JSDedicatedWorkerContextCustom.cpp
    bindings/js/JSDeviceMotionEventCustom.cpp
    bindings/js/JSDeviceOrientationEventCustom.cpp
    bindings/js/JSDocumentCustom.cpp
    bindings/js/JSElementCustom.cpp
    bindings/js/JSErrorHandler.cpp
    bindings/js/JSEventCustom.cpp
    bindings/js/JSEventListener.cpp
    bindings/js/JSEventSourceCustom.cpp
    bindings/js/JSEventTarget.cpp
    bindings/js/JSExceptionBase.cpp
    bindings/js/JSFileReaderCustom.cpp
    bindings/js/JSFloat32ArrayCustom.cpp
    bindings/js/JSGeolocationCustom.cpp
    bindings/js/JSHTMLAllCollectionCustom.cpp
    bindings/js/JSHTMLAppletElementCustom.cpp
    bindings/js/JSHTMLCanvasElementCustom.cpp
    bindings/js/JSHTMLCollectionCustom.cpp
    bindings/js/JSHTMLDocumentCustom.cpp
    bindings/js/JSHTMLElementCustom.cpp
    bindings/js/JSHTMLEmbedElementCustom.cpp
    bindings/js/JSHTMLFormElementCustom.cpp
    bindings/js/JSHTMLFrameElementCustom.cpp
    bindings/js/JSHTMLFrameSetElementCustom.cpp
    bindings/js/JSHTMLInputElementCustom.cpp
    bindings/js/JSHTMLLinkElementCustom.cpp
    bindings/js/JSHTMLObjectElementCustom.cpp
    bindings/js/JSHTMLOptionsCollectionCustom.cpp
    bindings/js/JSHTMLOutputElementCustom.cpp
    bindings/js/JSHTMLSelectElementCustom.cpp
    bindings/js/JSHTMLStyleElementCustom.cpp
    bindings/js/JSHistoryCustom.cpp
    bindings/js/JSIDBAnyCustom.cpp
    bindings/js/JSIDBKeyCustom.cpp
    bindings/js/JSImageConstructor.cpp
    bindings/js/JSImageDataCustom.cpp
    bindings/js/JSInt16ArrayCustom.cpp
    bindings/js/JSInt32ArrayCustom.cpp
    bindings/js/JSInt8ArrayCustom.cpp
    bindings/js/JSInjectedScriptHostCustom.cpp
    bindings/js/JSInjectedScriptManager.cpp
    bindings/js/JSInspectorFrontendHostCustom.cpp
    bindings/js/JSJavaScriptCallFrameCustom.cpp
    bindings/js/JSLazyEventListener.cpp
    bindings/js/JSLocationCustom.cpp
    bindings/js/JSMainThreadExecState.cpp
    bindings/js/JSMediaListCustom.cpp
    bindings/js/JSMemoryInfoCustom.cpp
    bindings/js/JSMessageChannelCustom.cpp
    bindings/js/JSMessageEventCustom.cpp
    bindings/js/JSMessagePortCustom.cpp
    bindings/js/JSNamedNodeMapCustom.cpp
    bindings/js/JSNavigatorCustom.cpp
    bindings/js/JSNodeCustom.cpp
    bindings/js/JSNodeFilterCondition.cpp
    bindings/js/JSNodeFilterCustom.cpp
    bindings/js/JSNodeIteratorCustom.cpp
    bindings/js/JSNodeListCustom.cpp
    bindings/js/JSOptionConstructor.cpp
    bindings/js/JSPluginElementFunctions.cpp
    bindings/js/JSProcessingInstructionCustom.cpp
    bindings/js/JSSQLResultSetRowListCustom.cpp
    bindings/js/JSSQLTransactionCustom.cpp
    bindings/js/JSSQLTransactionSyncCustom.cpp
    bindings/js/JSScriptProfileNodeCustom.cpp
    bindings/js/JSSharedWorkerCustom.cpp
    bindings/js/JSStorageCustom.cpp
    bindings/js/JSStyleSheetCustom.cpp
    bindings/js/JSStyleSheetListCustom.cpp
    bindings/js/JSTextCustom.cpp
    bindings/js/JSTouchCustom.cpp
    bindings/js/JSTouchListCustom.cpp
    bindings/js/JSTreeWalkerCustom.cpp
    bindings/js/JSUint16ArrayCustom.cpp 
    bindings/js/JSUint32ArrayCustom.cpp
    bindings/js/JSUint8ArrayCustom.cpp
    bindings/js/JSWebKitAnimationCustom.cpp
    bindings/js/JSWebKitAnimationListCustom.cpp
    bindings/js/JSWebKitCSSKeyframeRuleCustom.cpp
    bindings/js/JSWebKitCSSKeyframesRuleCustom.cpp
    bindings/js/JSWebKitCSSMatrixCustom.cpp
    bindings/js/JSWebKitPointCustom.cpp
    bindings/js/JSWebSocketCustom.cpp
    bindings/js/JSWorkerContextBase.cpp
    bindings/js/JSWorkerContextCustom.cpp
    bindings/js/JSWorkerCustom.cpp
    bindings/js/JSXMLHttpRequestCustom.cpp
    bindings/js/JSXMLHttpRequestUploadCustom.cpp
    bindings/js/JSXSLTProcessorCustom.cpp
    bindings/js/JavaScriptCallFrame.cpp
    bindings/js/PageScriptDebugServer.cpp
    bindings/js/ScheduledAction.cpp
    bindings/js/ScriptCachedFrameData.cpp
    bindings/js/ScriptCallStackFactory.cpp
    bindings/js/ScriptController.cpp
    bindings/js/ScriptDebugServer.cpp
    bindings/js/ScriptEventListener.cpp
    bindings/js/ScriptFunctionCall.cpp
    bindings/js/ScriptGCEvent.cpp
    bindings/js/ScriptObject.cpp
    bindings/js/ScriptProfile.cpp
    bindings/js/ScriptProfiler.cpp
    bindings/js/ScriptState.cpp
    bindings/js/ScriptValue.cpp
    bindings/js/SerializedScriptValue.cpp
    bindings/js/WorkerScriptController.cpp
    bindings/js/WorkerScriptDebugServer.cpp

    bridge/IdentifierRep.cpp
    bridge/NP_jsobject.cpp
    bridge/npruntime.cpp
    bridge/runtime_array.cpp
    bridge/runtime_method.cpp
    bridge/runtime_object.cpp
    bridge/runtime_root.cpp

    bridge/c/CRuntimeObject.cpp
    bridge/c/c_class.cpp
    bridge/c/c_instance.cpp
    bridge/c/c_runtime.cpp
    bridge/c/c_utility.cpp

    bridge/jsc/BridgeJSC.cpp
)

LIST(APPEND SCRIPTS_BINDINGS
    ${WEBCORE_DIR}/bindings/scripts/CodeGenerator.pm
)

SET(IDL_INCLUDES "")
FOREACH (_include ${WebCore_IDL_INCLUDES})
    LIST(APPEND IDL_INCLUDES --include=${WEBCORE_DIR}/${_include})
ENDFOREACH ()

SET(FEATURE_DEFINES_JAVASCRIPT "LANGUAGE_JAVASCRIPT=1")
FOREACH (_feature ${FEATURE_DEFINES})
    SET(FEATURE_DEFINES_JAVASCRIPT "${FEATURE_DEFINES_JAVASCRIPT} ${_feature}")
ENDFOREACH ()

# Create JavaScript C++ code given an IDL input
FOREACH (_file ${WebCore_IDL_FILES})
    GET_FILENAME_COMPONENT (_name ${_file} NAME_WE)
    ADD_CUSTOM_COMMAND(
        OUTPUT  ${DERIVED_SOURCES_WEBCORE_DIR}/JS${_name}.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/JS${_name}.h
        MAIN_DEPENDENCY ${_file}
        DEPENDS ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl ${SCRIPTS_BINDINGS} ${WEBCORE_DIR}/bindings/scripts/CodeGeneratorJS.pm ${_file}
        COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl --defines "${FEATURE_DEFINES_JAVASCRIPT}" --generator JS ${IDL_INCLUDES} --outputDir "${DERIVED_SOURCES_WEBCORE_DIR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" ${WEBCORE_DIR}/${_file}
        VERBATIM)
    LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/JS${_name}.cpp)
ENDFOREACH ()
