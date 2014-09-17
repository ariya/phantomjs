// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef InspectorBackendDispatcher_h
#define InspectorBackendDispatcher_h

#include <PlatformString.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class InspectorAgent;
class InspectorApplicationCacheAgent;
class InspectorArray;
class InspectorCSSAgent;
class InspectorConsoleAgent;
class InspectorDOMAgent;
class InspectorDOMDebuggerAgent;
class InspectorDOMStorageAgent;
class InspectorDatabaseAgent;
class InspectorDebuggerAgent;
class InspectorFrontendChannel;
class InspectorObject;
class InspectorPageAgent;
class InspectorProfilerAgent;
class InspectorResourceAgent;
class InspectorRuntimeAgent;
class InspectorTimelineAgent;
class InspectorWorkerAgent;

typedef String ErrorString;

class InspectorBackendDispatcher {
public:
    InspectorBackendDispatcher(InspectorFrontendChannel* inspectorFrontendChannel, InspectorApplicationCacheAgent* applicationCacheAgent, InspectorCSSAgent* cssAgent, InspectorConsoleAgent* consoleAgent, InspectorDOMAgent* domAgent, InspectorDOMDebuggerAgent* domDebuggerAgent, InspectorDOMStorageAgent* domStorageAgent, InspectorDatabaseAgent* databaseAgent, InspectorDebuggerAgent* debuggerAgent, InspectorResourceAgent* resourceAgent, InspectorPageAgent* pageAgent, InspectorProfilerAgent* profilerAgent, InspectorRuntimeAgent* runtimeAgent, InspectorTimelineAgent* timelineAgent, InspectorWorkerAgent* workerAgent)
        : m_inspectorFrontendChannel(inspectorFrontendChannel)
        , m_applicationCacheAgent(applicationCacheAgent)
        , m_cssAgent(cssAgent)
        , m_consoleAgent(consoleAgent)
        , m_domAgent(domAgent)
        , m_domDebuggerAgent(domDebuggerAgent)
        , m_domStorageAgent(domStorageAgent)
        , m_databaseAgent(databaseAgent)
        , m_debuggerAgent(debuggerAgent)
        , m_resourceAgent(resourceAgent)
        , m_pageAgent(pageAgent)
        , m_profilerAgent(profilerAgent)
        , m_runtimeAgent(runtimeAgent)
        , m_timelineAgent(timelineAgent)
        , m_workerAgent(workerAgent)
    { }

    enum CommonErrorCode {
        ParseError = 0,
        InvalidRequest,
        MethodNotFound,
        InvalidParams,
        InternalError,
        ServerError,
        LastEntry,
    };

    void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorText) const;
    void reportProtocolError(const long* const callId, CommonErrorCode, PassRefPtr<InspectorArray> data) const;
    void dispatch(const String& message);
    static bool getCommandName(const String& message, String* result);

    static const char* Page_addScriptToEvaluateOnLoadCmd;
    static const char* Page_removeAllScriptsToEvaluateOnLoadCmd;
    static const char* Page_reloadCmd;
    static const char* Page_openCmd;
    static const char* Page_getCookiesCmd;
    static const char* Page_deleteCookieCmd;
    static const char* Page_getResourceTreeCmd;
    static const char* Page_getResourceContentCmd;
    static const char* Runtime_evaluateCmd;
    static const char* Runtime_evaluateOnCmd;
    static const char* Runtime_getPropertiesCmd;
    static const char* Runtime_setPropertyValueCmd;
    static const char* Runtime_releaseObjectCmd;
    static const char* Runtime_releaseObjectGroupCmd;
    static const char* Console_enableCmd;
    static const char* Console_disableCmd;
    static const char* Console_clearConsoleMessagesCmd;
    static const char* Console_setMonitoringXHREnabledCmd;
    static const char* Console_addInspectedNodeCmd;
    static const char* Network_enableCmd;
    static const char* Network_disableCmd;
    static const char* Network_setUserAgentOverrideCmd;
    static const char* Network_setExtraHeadersCmd;
    static const char* Database_enableCmd;
    static const char* Database_disableCmd;
    static const char* Database_getDatabaseTableNamesCmd;
    static const char* Database_executeSQLCmd;
    static const char* DOMStorage_enableCmd;
    static const char* DOMStorage_disableCmd;
    static const char* DOMStorage_getDOMStorageEntriesCmd;
    static const char* DOMStorage_setDOMStorageItemCmd;
    static const char* DOMStorage_removeDOMStorageItemCmd;
    static const char* ApplicationCache_getApplicationCachesCmd;
    static const char* DOM_getDocumentCmd;
    static const char* DOM_getChildNodesCmd;
    static const char* DOM_querySelectorCmd;
    static const char* DOM_querySelectorAllCmd;
    static const char* DOM_setNodeNameCmd;
    static const char* DOM_setNodeValueCmd;
    static const char* DOM_removeNodeCmd;
    static const char* DOM_setAttributeCmd;
    static const char* DOM_removeAttributeCmd;
    static const char* DOM_getEventListenersForNodeCmd;
    static const char* DOM_copyNodeCmd;
    static const char* DOM_getOuterHTMLCmd;
    static const char* DOM_setOuterHTMLCmd;
    static const char* DOM_performSearchCmd;
    static const char* DOM_cancelSearchCmd;
    static const char* DOM_pushNodeToFrontendCmd;
    static const char* DOM_setInspectModeEnabledCmd;
    static const char* DOM_highlightNodeCmd;
    static const char* DOM_hideNodeHighlightCmd;
    static const char* DOM_highlightFrameCmd;
    static const char* DOM_hideFrameHighlightCmd;
    static const char* DOM_pushNodeByPathToFrontendCmd;
    static const char* DOM_resolveNodeCmd;
    static const char* CSS_getStylesForNodeCmd;
    static const char* CSS_getComputedStyleForNodeCmd;
    static const char* CSS_getInlineStyleForNodeCmd;
    static const char* CSS_getAllStyleSheetsCmd;
    static const char* CSS_getStyleSheetCmd;
    static const char* CSS_getStyleSheetTextCmd;
    static const char* CSS_setStyleSheetTextCmd;
    static const char* CSS_setPropertyTextCmd;
    static const char* CSS_togglePropertyCmd;
    static const char* CSS_setRuleSelectorCmd;
    static const char* CSS_addRuleCmd;
    static const char* CSS_getSupportedCSSPropertiesCmd;
    static const char* Timeline_startCmd;
    static const char* Timeline_stopCmd;
    static const char* Debugger_enableCmd;
    static const char* Debugger_disableCmd;
    static const char* Debugger_setBreakpointsActiveCmd;
    static const char* Debugger_setBreakpointByUrlCmd;
    static const char* Debugger_setBreakpointCmd;
    static const char* Debugger_removeBreakpointCmd;
    static const char* Debugger_continueToLocationCmd;
    static const char* Debugger_stepOverCmd;
    static const char* Debugger_stepIntoCmd;
    static const char* Debugger_stepOutCmd;
    static const char* Debugger_pauseCmd;
    static const char* Debugger_resumeCmd;
    static const char* Debugger_editScriptSourceCmd;
    static const char* Debugger_getScriptSourceCmd;
    static const char* Debugger_setPauseOnExceptionsCmd;
    static const char* Debugger_evaluateOnCallFrameCmd;
    static const char* DOMDebugger_setDOMBreakpointCmd;
    static const char* DOMDebugger_removeDOMBreakpointCmd;
    static const char* DOMDebugger_setEventListenerBreakpointCmd;
    static const char* DOMDebugger_removeEventListenerBreakpointCmd;
    static const char* DOMDebugger_setXHRBreakpointCmd;
    static const char* DOMDebugger_removeXHRBreakpointCmd;
    static const char* Profiler_enableCmd;
    static const char* Profiler_disableCmd;
    static const char* Profiler_isEnabledCmd;
    static const char* Profiler_startCmd;
    static const char* Profiler_stopCmd;
    static const char* Profiler_getProfileHeadersCmd;
    static const char* Profiler_getProfileCmd;
    static const char* Profiler_removeProfileCmd;
    static const char* Profiler_clearProfilesCmd;
    static const char* Profiler_takeHeapSnapshotCmd;
    static const char* Profiler_collectGarbageCmd;
    static const char* Worker_sendMessageToWorkerCmd;

private:
    int getInt(InspectorObject* object, const String& name, bool optional, InspectorArray* protocolErrors);
    String getString(InspectorObject* object, const String& name, bool optional, InspectorArray* protocolErrors);
    bool getBoolean(InspectorObject* object, const String& name, bool optional, InspectorArray* protocolErrors);
    PassRefPtr<InspectorObject> getObject(InspectorObject* object, const String& name, bool optional, InspectorArray* protocolErrors);
    PassRefPtr<InspectorArray> getArray(InspectorObject* object, const String& name, bool optional, InspectorArray* protocolErrors);

    void Page_addScriptToEvaluateOnLoad(long callId, InspectorObject* requestMessageObject);
    void Page_removeAllScriptsToEvaluateOnLoad(long callId, InspectorObject* requestMessageObject);
    void Page_reload(long callId, InspectorObject* requestMessageObject);
    void Page_open(long callId, InspectorObject* requestMessageObject);
    void Page_getCookies(long callId, InspectorObject* requestMessageObject);
    void Page_deleteCookie(long callId, InspectorObject* requestMessageObject);
    void Page_getResourceTree(long callId, InspectorObject* requestMessageObject);
    void Page_getResourceContent(long callId, InspectorObject* requestMessageObject);
    void Runtime_evaluate(long callId, InspectorObject* requestMessageObject);
    void Runtime_evaluateOn(long callId, InspectorObject* requestMessageObject);
    void Runtime_getProperties(long callId, InspectorObject* requestMessageObject);
    void Runtime_setPropertyValue(long callId, InspectorObject* requestMessageObject);
    void Runtime_releaseObject(long callId, InspectorObject* requestMessageObject);
    void Runtime_releaseObjectGroup(long callId, InspectorObject* requestMessageObject);
    void Console_enable(long callId, InspectorObject* requestMessageObject);
    void Console_disable(long callId, InspectorObject* requestMessageObject);
    void Console_clearConsoleMessages(long callId, InspectorObject* requestMessageObject);
    void Console_setMonitoringXHREnabled(long callId, InspectorObject* requestMessageObject);
    void Console_addInspectedNode(long callId, InspectorObject* requestMessageObject);
    void Network_enable(long callId, InspectorObject* requestMessageObject);
    void Network_disable(long callId, InspectorObject* requestMessageObject);
    void Network_setUserAgentOverride(long callId, InspectorObject* requestMessageObject);
    void Network_setExtraHeaders(long callId, InspectorObject* requestMessageObject);
    void Database_enable(long callId, InspectorObject* requestMessageObject);
    void Database_disable(long callId, InspectorObject* requestMessageObject);
    void Database_getDatabaseTableNames(long callId, InspectorObject* requestMessageObject);
    void Database_executeSQL(long callId, InspectorObject* requestMessageObject);
    void DOMStorage_enable(long callId, InspectorObject* requestMessageObject);
    void DOMStorage_disable(long callId, InspectorObject* requestMessageObject);
    void DOMStorage_getDOMStorageEntries(long callId, InspectorObject* requestMessageObject);
    void DOMStorage_setDOMStorageItem(long callId, InspectorObject* requestMessageObject);
    void DOMStorage_removeDOMStorageItem(long callId, InspectorObject* requestMessageObject);
    void ApplicationCache_getApplicationCaches(long callId, InspectorObject* requestMessageObject);
    void DOM_getDocument(long callId, InspectorObject* requestMessageObject);
    void DOM_getChildNodes(long callId, InspectorObject* requestMessageObject);
    void DOM_querySelector(long callId, InspectorObject* requestMessageObject);
    void DOM_querySelectorAll(long callId, InspectorObject* requestMessageObject);
    void DOM_setNodeName(long callId, InspectorObject* requestMessageObject);
    void DOM_setNodeValue(long callId, InspectorObject* requestMessageObject);
    void DOM_removeNode(long callId, InspectorObject* requestMessageObject);
    void DOM_setAttribute(long callId, InspectorObject* requestMessageObject);
    void DOM_removeAttribute(long callId, InspectorObject* requestMessageObject);
    void DOM_getEventListenersForNode(long callId, InspectorObject* requestMessageObject);
    void DOM_copyNode(long callId, InspectorObject* requestMessageObject);
    void DOM_getOuterHTML(long callId, InspectorObject* requestMessageObject);
    void DOM_setOuterHTML(long callId, InspectorObject* requestMessageObject);
    void DOM_performSearch(long callId, InspectorObject* requestMessageObject);
    void DOM_cancelSearch(long callId, InspectorObject* requestMessageObject);
    void DOM_pushNodeToFrontend(long callId, InspectorObject* requestMessageObject);
    void DOM_setInspectModeEnabled(long callId, InspectorObject* requestMessageObject);
    void DOM_highlightNode(long callId, InspectorObject* requestMessageObject);
    void DOM_hideNodeHighlight(long callId, InspectorObject* requestMessageObject);
    void DOM_highlightFrame(long callId, InspectorObject* requestMessageObject);
    void DOM_hideFrameHighlight(long callId, InspectorObject* requestMessageObject);
    void DOM_pushNodeByPathToFrontend(long callId, InspectorObject* requestMessageObject);
    void DOM_resolveNode(long callId, InspectorObject* requestMessageObject);
    void CSS_getStylesForNode(long callId, InspectorObject* requestMessageObject);
    void CSS_getComputedStyleForNode(long callId, InspectorObject* requestMessageObject);
    void CSS_getInlineStyleForNode(long callId, InspectorObject* requestMessageObject);
    void CSS_getAllStyleSheets(long callId, InspectorObject* requestMessageObject);
    void CSS_getStyleSheet(long callId, InspectorObject* requestMessageObject);
    void CSS_getStyleSheetText(long callId, InspectorObject* requestMessageObject);
    void CSS_setStyleSheetText(long callId, InspectorObject* requestMessageObject);
    void CSS_setPropertyText(long callId, InspectorObject* requestMessageObject);
    void CSS_toggleProperty(long callId, InspectorObject* requestMessageObject);
    void CSS_setRuleSelector(long callId, InspectorObject* requestMessageObject);
    void CSS_addRule(long callId, InspectorObject* requestMessageObject);
    void CSS_getSupportedCSSProperties(long callId, InspectorObject* requestMessageObject);
    void Timeline_start(long callId, InspectorObject* requestMessageObject);
    void Timeline_stop(long callId, InspectorObject* requestMessageObject);
    void Debugger_enable(long callId, InspectorObject* requestMessageObject);
    void Debugger_disable(long callId, InspectorObject* requestMessageObject);
    void Debugger_setBreakpointsActive(long callId, InspectorObject* requestMessageObject);
    void Debugger_setBreakpointByUrl(long callId, InspectorObject* requestMessageObject);
    void Debugger_setBreakpoint(long callId, InspectorObject* requestMessageObject);
    void Debugger_removeBreakpoint(long callId, InspectorObject* requestMessageObject);
    void Debugger_continueToLocation(long callId, InspectorObject* requestMessageObject);
    void Debugger_stepOver(long callId, InspectorObject* requestMessageObject);
    void Debugger_stepInto(long callId, InspectorObject* requestMessageObject);
    void Debugger_stepOut(long callId, InspectorObject* requestMessageObject);
    void Debugger_pause(long callId, InspectorObject* requestMessageObject);
    void Debugger_resume(long callId, InspectorObject* requestMessageObject);
    void Debugger_editScriptSource(long callId, InspectorObject* requestMessageObject);
    void Debugger_getScriptSource(long callId, InspectorObject* requestMessageObject);
    void Debugger_setPauseOnExceptions(long callId, InspectorObject* requestMessageObject);
    void Debugger_evaluateOnCallFrame(long callId, InspectorObject* requestMessageObject);
    void DOMDebugger_setDOMBreakpoint(long callId, InspectorObject* requestMessageObject);
    void DOMDebugger_removeDOMBreakpoint(long callId, InspectorObject* requestMessageObject);
    void DOMDebugger_setEventListenerBreakpoint(long callId, InspectorObject* requestMessageObject);
    void DOMDebugger_removeEventListenerBreakpoint(long callId, InspectorObject* requestMessageObject);
    void DOMDebugger_setXHRBreakpoint(long callId, InspectorObject* requestMessageObject);
    void DOMDebugger_removeXHRBreakpoint(long callId, InspectorObject* requestMessageObject);
    void Profiler_enable(long callId, InspectorObject* requestMessageObject);
    void Profiler_disable(long callId, InspectorObject* requestMessageObject);
    void Profiler_isEnabled(long callId, InspectorObject* requestMessageObject);
    void Profiler_start(long callId, InspectorObject* requestMessageObject);
    void Profiler_stop(long callId, InspectorObject* requestMessageObject);
    void Profiler_getProfileHeaders(long callId, InspectorObject* requestMessageObject);
    void Profiler_getProfile(long callId, InspectorObject* requestMessageObject);
    void Profiler_removeProfile(long callId, InspectorObject* requestMessageObject);
    void Profiler_clearProfiles(long callId, InspectorObject* requestMessageObject);
    void Profiler_takeHeapSnapshot(long callId, InspectorObject* requestMessageObject);
    void Profiler_collectGarbage(long callId, InspectorObject* requestMessageObject);
    void Worker_sendMessageToWorker(long callId, InspectorObject* requestMessageObject);

    InspectorFrontendChannel* m_inspectorFrontendChannel;
    InspectorApplicationCacheAgent* m_applicationCacheAgent;
    InspectorCSSAgent* m_cssAgent;
    InspectorConsoleAgent* m_consoleAgent;
    InspectorDOMAgent* m_domAgent;
    InspectorDOMDebuggerAgent* m_domDebuggerAgent;
    InspectorDOMStorageAgent* m_domStorageAgent;
    InspectorDatabaseAgent* m_databaseAgent;
    InspectorDebuggerAgent* m_debuggerAgent;
    InspectorResourceAgent* m_resourceAgent;
    InspectorPageAgent* m_pageAgent;
    InspectorProfilerAgent* m_profilerAgent;
    InspectorRuntimeAgent* m_runtimeAgent;
    InspectorTimelineAgent* m_timelineAgent;
    InspectorWorkerAgent* m_workerAgent;
};

} // namespace WebCore
#endif // !defined(InspectorBackendDispatcher_h)

