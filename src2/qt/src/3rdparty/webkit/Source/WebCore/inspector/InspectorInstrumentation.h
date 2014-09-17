/*
* Copyright (C) 2010 Google Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
*     * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following disclaimer
* in the documentation and/or other materials provided with the
* distribution.
*     * Neither the name of Google Inc. nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef InspectorInstrumentation_h
#define InspectorInstrumentation_h

#include "Console.h"
#include "Document.h"
#include "Frame.h"
#include "Page.h"
#include "ScriptExecutionContext.h"
#include <wtf/HashMap.h>

namespace WebCore {

class CharacterData;
class DOMWrapperWorld;
class Database;
class Element;
class EventContext;
class DocumentLoader;
class HitTestResult;
class InspectorAgent;
class InspectorPageAgent;
class InspectorResourceAgent;
class InspectorTimelineAgent;
class KURL;
class Node;
class ResourceRequest;
class ResourceResponse;
class ScriptArguments;
class ScriptCallStack;
class ScriptExecutionContext;
class StorageArea;
class WorkerContextProxy;
class XMLHttpRequest;

#if ENABLE(WEB_SOCKETS)
class WebSocketHandshakeRequest;
class WebSocketHandshakeResponse;
#endif

typedef pair<InspectorAgent*, int> InspectorInstrumentationCookie;

class InspectorInstrumentation {
public:
    static void didClearWindowObjectInWorld(Frame*, DOMWrapperWorld*);
    static void inspectedPageDestroyed(Page*);

    static void willInsertDOMNode(Document*, Node*, Node* parent);
    static void didInsertDOMNode(Document*, Node*);
    static void willRemoveDOMNode(Document*, Node*);
    static void willModifyDOMAttr(Document*, Element*);
    static void didModifyDOMAttr(Document*, Element*);
    static void characterDataModified(Document*, CharacterData*);
    static void didInvalidateStyleAttr(Document*, Node*);

    static void mouseDidMoveOverElement(Page*, const HitTestResult&, unsigned modifierFlags);
    static bool handleMousePress(Page*);

    static void willSendXMLHttpRequest(ScriptExecutionContext*, const String& url);
    static void didScheduleResourceRequest(Document*, const String& url);
    static void didInstallTimer(ScriptExecutionContext*, int timerId, int timeout, bool singleShot);
    static void didRemoveTimer(ScriptExecutionContext*, int timerId);

    static InspectorInstrumentationCookie willCallFunction(Frame*, const String& scriptName, int scriptLine);
    static void didCallFunction(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willChangeXHRReadyState(ScriptExecutionContext*, XMLHttpRequest* request);
    static void didChangeXHRReadyState(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchEvent(Document*, const Event& event, DOMWindow* window, Node* node, const Vector<EventContext>& ancestors);
    static void didDispatchEvent(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchEventOnWindow(Frame*, const Event& event, DOMWindow* window);
    static void didDispatchEventOnWindow(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willEvaluateScript(Frame*, const String& url, int lineNumber);
    static void didEvaluateScript(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willFireTimer(ScriptExecutionContext*, int timerId);
    static void didFireTimer(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willLayout(Frame*);
    static void didLayout(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willLoadXHR(ScriptExecutionContext*, XMLHttpRequest*);
    static void didLoadXHR(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willPaint(Frame*, const IntRect& rect);
    static void didPaint(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willRecalculateStyle(Document*);
    static void didRecalculateStyle(const InspectorInstrumentationCookie&);

    static void applyUserAgentOverride(Frame*, String*);
    static void willSendRequest(Frame*, unsigned long identifier, DocumentLoader*, ResourceRequest&, const ResourceResponse& redirectResponse);
    static void continueAfterPingLoader(Frame*, unsigned long identifier, DocumentLoader*, ResourceRequest&, const ResourceResponse&);
    static void markResourceAsCached(Page*, unsigned long identifier);
    static void didLoadResourceFromMemoryCache(Page*, DocumentLoader*, const CachedResource*);
    static InspectorInstrumentationCookie willReceiveResourceData(Frame*, unsigned long identifier);
    static void didReceiveResourceData(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willReceiveResourceResponse(Frame*, unsigned long identifier, const ResourceResponse&);
    static void didReceiveResourceResponse(const InspectorInstrumentationCookie&, unsigned long identifier, DocumentLoader*, const ResourceResponse&);
    static void continueAfterXFrameOptionsDenied(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueWithPolicyDownload(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueWithPolicyIgnore(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void didReceiveContentLength(Frame*, unsigned long identifier, int dataLength, int encodedDataLength);
    static void didFinishLoading(Frame*, unsigned long identifier, double finishTime);
    static void didFailLoading(Frame*, unsigned long identifier, const ResourceError&);
    static void resourceRetrievedByXMLHttpRequest(ScriptExecutionContext*, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber);
    static void scriptImported(ScriptExecutionContext*, unsigned long identifier, const String& sourceString);
    static void domContentLoadedEventFired(Frame*, const KURL&);
    static void loadEventFired(Frame*, const KURL&);
    static void frameDetachedFromParent(Frame*);
    static void didCommitLoad(Frame*, DocumentLoader*);

    static InspectorInstrumentationCookie willWriteHTML(Document*, unsigned int length, unsigned int startLine);
    static void didWriteHTML(const InspectorInstrumentationCookie&, unsigned int endLine);

    static void addMessageToConsole(Page*, MessageSource, MessageType, MessageLevel, const String& message, PassRefPtr<ScriptArguments>, PassRefPtr<ScriptCallStack>);
    static void addMessageToConsole(Page*, MessageSource, MessageType, MessageLevel, const String& message, unsigned lineNumber, const String&);
    static void consoleCount(Page*, PassRefPtr<ScriptArguments>, PassRefPtr<ScriptCallStack>);
    static void startConsoleTiming(Page*, const String& title);
    static void stopConsoleTiming(Page*, const String& title, PassRefPtr<ScriptCallStack>);
    static void consoleMarkTimeline(Page*, PassRefPtr<ScriptArguments>);

#if ENABLE(JAVASCRIPT_DEBUGGER)
    static void addStartProfilingMessageToConsole(Page*, const String& title, unsigned lineNumber, const String& sourceURL);
    static void addProfile(Page*, RefPtr<ScriptProfile>, PassRefPtr<ScriptCallStack>);
    static String getCurrentUserInitiatedProfileName(Page*, bool incrementProfileNumber);
    static bool profilerEnabled(Page*);
#endif

#if ENABLE(DATABASE)
    static void didOpenDatabase(ScriptExecutionContext*, PassRefPtr<Database>, const String& domain, const String& name, const String& version);
#endif

#if ENABLE(DOM_STORAGE)
    static void didUseDOMStorage(Page*, StorageArea*, bool isLocalStorage, Frame*);
#endif

#if ENABLE(WORKERS)
    static bool willStartWorkerContext(ScriptExecutionContext*, WorkerContextProxy*);
    static void didStartWorkerContext(ScriptExecutionContext*, WorkerContextProxy*, bool paused);
    static void didCreateWorker(ScriptExecutionContext*, intptr_t id, const String& url, bool isSharedWorker);
    static void didDestroyWorker(ScriptExecutionContext*, intptr_t id);
#endif

#if ENABLE(WEB_SOCKETS)
    static void didCreateWebSocket(ScriptExecutionContext*, unsigned long identifier, const KURL& requestURL, const KURL& documentURL);
    static void willSendWebSocketHandshakeRequest(ScriptExecutionContext*, unsigned long identifier, const WebSocketHandshakeRequest&);
    static void didReceiveWebSocketHandshakeResponse(ScriptExecutionContext*, unsigned long identifier, const WebSocketHandshakeResponse&);
    static void didCloseWebSocket(ScriptExecutionContext*, unsigned long identifier);
#endif

    static void networkStateChanged(Page*);

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    static void updateApplicationCacheStatus(Frame*);
#endif

#if ENABLE(INSPECTOR)
    static void bindInspectorAgent(Page* page, InspectorAgent* inspectorAgent) { inspectorAgents().set(page, inspectorAgent); }
    static void unbindInspectorAgent(Page* page) { inspectorAgents().remove(page); }
    static void frontendCreated() { s_frontendCounter += 1; }
    static void frontendDeleted() { s_frontendCounter -= 1; }
    static bool hasFrontends() { return s_frontendCounter; }
    static bool hasFrontend(Page*);
#else
    static bool hasFrontends() { return false; }
    static bool hasFrontend(Page*) { return false; }
#endif

private:
#if ENABLE(INSPECTOR)
    static void didClearWindowObjectInWorldImpl(InspectorAgent*, Frame*, DOMWrapperWorld*);
    static void inspectedPageDestroyedImpl(InspectorAgent*);

    static void willInsertDOMNodeImpl(InspectorAgent*, Node* node, Node* parent);
    static void didInsertDOMNodeImpl(InspectorAgent*, Node*);
    static void willRemoveDOMNodeImpl(InspectorAgent*, Node*);
    static void didRemoveDOMNodeImpl(InspectorAgent*, Node*);
    static void willModifyDOMAttrImpl(InspectorAgent*, Element*);
    static void didModifyDOMAttrImpl(InspectorAgent*, Element*);
    static void characterDataModifiedImpl(InspectorAgent*, CharacterData*);
    static void didInvalidateStyleAttrImpl(InspectorAgent*, Node*);

    static void mouseDidMoveOverElementImpl(InspectorAgent*, const HitTestResult&, unsigned modifierFlags);
    static bool handleMousePressImpl(InspectorAgent*);

    static void willSendXMLHttpRequestImpl(InspectorAgent*, const String& url);
    static void didScheduleResourceRequestImpl(InspectorAgent*, const String& url);
    static void didInstallTimerImpl(InspectorAgent*, int timerId, int timeout, bool singleShot);
    static void didRemoveTimerImpl(InspectorAgent*, int timerId);

    static InspectorInstrumentationCookie willCallFunctionImpl(InspectorAgent*, const String& scriptName, int scriptLine);
    static void didCallFunctionImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willChangeXHRReadyStateImpl(InspectorAgent*, XMLHttpRequest* request);
    static void didChangeXHRReadyStateImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchEventImpl(InspectorAgent*, const Event& event, DOMWindow* window, Node* node, const Vector<EventContext>& ancestors);
    static void didDispatchEventImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchEventOnWindowImpl(InspectorAgent*, const Event& event, DOMWindow* window);
    static void didDispatchEventOnWindowImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willEvaluateScriptImpl(InspectorAgent*, const String& url, int lineNumber);
    static void didEvaluateScriptImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willFireTimerImpl(InspectorAgent*, int timerId);
    static void didFireTimerImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willLayoutImpl(InspectorAgent*);
    static void didLayoutImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willLoadXHRImpl(InspectorAgent*, XMLHttpRequest* request);
    static void didLoadXHRImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willPaintImpl(InspectorAgent*, const IntRect& rect);
    static void didPaintImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willRecalculateStyleImpl(InspectorAgent*);
    static void didRecalculateStyleImpl(const InspectorInstrumentationCookie&);

    static void applyUserAgentOverrideImpl(InspectorAgent*, String*);
    static void willSendRequestImpl(InspectorAgent*, unsigned long identifier, DocumentLoader*, ResourceRequest&, const ResourceResponse& redirectResponse);
    static void continueAfterPingLoaderImpl(InspectorAgent*, unsigned long identifier, DocumentLoader*, ResourceRequest&, const ResourceResponse&);
    static void markResourceAsCachedImpl(InspectorAgent*, unsigned long identifier);
    static void didLoadResourceFromMemoryCacheImpl(InspectorAgent*, DocumentLoader*, const CachedResource*);
    static InspectorInstrumentationCookie willReceiveResourceDataImpl(InspectorAgent*, unsigned long identifier);
    static void didReceiveResourceDataImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willReceiveResourceResponseImpl(InspectorAgent*, unsigned long identifier, const ResourceResponse&);
    static void didReceiveResourceResponseImpl(const InspectorInstrumentationCookie&, unsigned long identifier, DocumentLoader*, const ResourceResponse&);
    static void didReceiveResourceResponseButCanceledImpl(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueAfterXFrameOptionsDeniedImpl(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueWithPolicyDownloadImpl(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueWithPolicyIgnoreImpl(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void didReceiveContentLengthImpl(InspectorAgent*, unsigned long identifier, int dataLength, int encodedDataLength);
    static void didFinishLoadingImpl(InspectorAgent*, unsigned long identifier, double finishTime);
    static void didFailLoadingImpl(InspectorAgent*, unsigned long identifier, const ResourceError&);
    static void resourceRetrievedByXMLHttpRequestImpl(InspectorAgent*, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber);
    static void scriptImportedImpl(InspectorAgent*, unsigned long identifier, const String& sourceString);
    static void domContentLoadedEventFiredImpl(InspectorAgent*, Frame*, const KURL&);
    static void loadEventFiredImpl(InspectorAgent*, Frame*, const KURL&);
    static void frameDetachedFromParentImpl(InspectorAgent*, Frame*);
    static void didCommitLoadImpl(Page*, InspectorAgent*, DocumentLoader*);

    static InspectorInstrumentationCookie willWriteHTMLImpl(InspectorAgent*, unsigned int length, unsigned int startLine);
    static void didWriteHTMLImpl(const InspectorInstrumentationCookie&, unsigned int endLine);

    static void addMessageToConsoleImpl(InspectorAgent*, MessageSource, MessageType, MessageLevel, const String& message, PassRefPtr<ScriptArguments>, PassRefPtr<ScriptCallStack>);
    static void addMessageToConsoleImpl(InspectorAgent*, MessageSource, MessageType, MessageLevel, const String& message, unsigned lineNumber, const String& sourceID);
    static void consoleCountImpl(InspectorAgent*, PassRefPtr<ScriptArguments>, PassRefPtr<ScriptCallStack>);
    static void startConsoleTimingImpl(InspectorAgent*, const String& title);
    static void stopConsoleTimingImpl(InspectorAgent*, const String& title, PassRefPtr<ScriptCallStack>);
    static void consoleMarkTimelineImpl(InspectorAgent*, PassRefPtr<ScriptArguments>);

#if ENABLE(JAVASCRIPT_DEBUGGER)
    static void addStartProfilingMessageToConsoleImpl(InspectorAgent*, const String& title, unsigned lineNumber, const String& sourceURL);
    static void addProfileImpl(InspectorAgent*, RefPtr<ScriptProfile>, PassRefPtr<ScriptCallStack>);
    static String getCurrentUserInitiatedProfileNameImpl(InspectorAgent*, bool incrementProfileNumber);
    static bool profilerEnabledImpl(InspectorAgent*);
#endif

#if ENABLE(DATABASE)
    static void didOpenDatabaseImpl(InspectorAgent*, PassRefPtr<Database>, const String& domain, const String& name, const String& version);
#endif

#if ENABLE(DOM_STORAGE)
    static void didUseDOMStorageImpl(InspectorAgent*, StorageArea*, bool isLocalStorage, Frame*);
#endif

#if ENABLE(WORKERS)
    static void didStartWorkerContextImpl(InspectorAgent*, WorkerContextProxy*);
    static void didCreateWorkerImpl(InspectorAgent*, intptr_t id, const String& url, bool isSharedWorker);
    static void didDestroyWorkerImpl(InspectorAgent*, intptr_t id);
#endif

#if ENABLE(WEB_SOCKETS)
    static void didCreateWebSocketImpl(InspectorAgent*, unsigned long identifier, const KURL& requestURL, const KURL& documentURL);
    static void willSendWebSocketHandshakeRequestImpl(InspectorAgent*, unsigned long identifier, const WebSocketHandshakeRequest&);
    static void didReceiveWebSocketHandshakeResponseImpl(InspectorAgent*, unsigned long identifier, const WebSocketHandshakeResponse&);
    static void didCloseWebSocketImpl(InspectorAgent*, unsigned long identifier);
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    static void networkStateChangedImpl(InspectorAgent*);
    static void updateApplicationCacheStatusImpl(InspectorAgent*, Frame*);
#endif

    static InspectorAgent* inspectorAgentForFrame(Frame*);
    static InspectorAgent* inspectorAgentForContext(ScriptExecutionContext*);
    static InspectorAgent* inspectorAgentForPage(Page*);
    static InspectorAgent* inspectorAgentWithFrontendForContext(ScriptExecutionContext*);
    static InspectorAgent* inspectorAgentWithFrontendForDocument(Document*);
    static InspectorAgent* inspectorAgentWithFrontendForFrame(Frame*);
    static InspectorAgent* inspectorAgentWithFrontendForPage(Page*);

    static bool hasFrontend(InspectorAgent*);
    static void pauseOnNativeEventIfNeeded(InspectorAgent*, const String& categoryType, const String& eventName, bool synchronous);
    static void cancelPauseOnNativeEvent(InspectorAgent*);
    static InspectorTimelineAgent* retrieveTimelineAgent(InspectorAgent*);
    static InspectorTimelineAgent* retrieveTimelineAgent(const InspectorInstrumentationCookie&);
    static InspectorResourceAgent* retrieveResourceAgent(InspectorAgent*);
    static InspectorPageAgent* retrievePageAgent(InspectorAgent*);

    static HashMap<Page*, InspectorAgent*>& inspectorAgents();
    static int s_frontendCounter;
#endif
};

inline void InspectorInstrumentation::didClearWindowObjectInWorld(Frame* frame, DOMWrapperWorld* world)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(frame))
        didClearWindowObjectInWorldImpl(inspectorAgent, frame, world);
#endif
}

inline void InspectorInstrumentation::inspectedPageDestroyed(Page* page)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForPage(page))
        inspectedPageDestroyedImpl(inspectorAgent);
#endif
}

inline void InspectorInstrumentation::willInsertDOMNode(Document* document, Node* node, Node* parent)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        willInsertDOMNodeImpl(inspectorAgent, node, parent);
#endif
}

inline void InspectorInstrumentation::didInsertDOMNode(Document* document, Node* node)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        didInsertDOMNodeImpl(inspectorAgent, node);
#endif
}

inline void InspectorInstrumentation::willRemoveDOMNode(Document* document, Node* node)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document)) {
        willRemoveDOMNodeImpl(inspectorAgent, node);
        didRemoveDOMNodeImpl(inspectorAgent, node);
    }
#endif
}

inline void InspectorInstrumentation::willModifyDOMAttr(Document* document, Element* element)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        willModifyDOMAttrImpl(inspectorAgent, element);
#endif
}

inline void InspectorInstrumentation::didModifyDOMAttr(Document* document, Element* element)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        didModifyDOMAttrImpl(inspectorAgent, element);
#endif
}

inline void InspectorInstrumentation::didInvalidateStyleAttr(Document* document, Node* node)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        didInvalidateStyleAttrImpl(inspectorAgent, node);
#endif
}

inline void InspectorInstrumentation::mouseDidMoveOverElement(Page* page, const HitTestResult& result, unsigned modifierFlags)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForPage(page))
        mouseDidMoveOverElementImpl(inspectorAgent, result, modifierFlags);
#endif
}

inline bool InspectorInstrumentation::handleMousePress(Page* page)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForPage(page))
        return handleMousePressImpl(inspectorAgent);
#endif
    return false;
}

inline void InspectorInstrumentation::characterDataModified(Document* document, CharacterData* characterData)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        characterDataModifiedImpl(inspectorAgent, characterData);
#endif
}

inline void InspectorInstrumentation::willSendXMLHttpRequest(ScriptExecutionContext* context, const String& url)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        willSendXMLHttpRequestImpl(inspectorAgent, url);
#endif
}

inline void InspectorInstrumentation::didScheduleResourceRequest(Document* document, const String& url)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        didScheduleResourceRequestImpl(inspectorAgent, url);
#endif
}

inline void InspectorInstrumentation::didInstallTimer(ScriptExecutionContext* context, int timerId, int timeout, bool singleShot)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        didInstallTimerImpl(inspectorAgent, timerId, timeout, singleShot);
#endif
}

inline void InspectorInstrumentation::didRemoveTimer(ScriptExecutionContext* context, int timerId)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        didRemoveTimerImpl(inspectorAgent, timerId);
#endif
}


inline InspectorInstrumentationCookie InspectorInstrumentation::willCallFunction(Frame* frame, const String& scriptName, int scriptLine)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForFrame(frame))
        return willCallFunctionImpl(inspectorAgent, scriptName, scriptLine);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didCallFunction(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didCallFunctionImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willChangeXHRReadyState(ScriptExecutionContext* context, XMLHttpRequest* request)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        return willChangeXHRReadyStateImpl(inspectorAgent, request);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didChangeXHRReadyState(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didChangeXHRReadyStateImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEvent(Document* document, const Event& event, DOMWindow* window, Node* node, const Vector<EventContext>& ancestors)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        return willDispatchEventImpl(inspectorAgent, event, window, node, ancestors);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didDispatchEvent(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didDispatchEventImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEventOnWindow(Frame* frame, const Event& event, DOMWindow* window)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForFrame(frame))
        return willDispatchEventOnWindowImpl(inspectorAgent, event, window);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didDispatchEventOnWindow(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didDispatchEventOnWindowImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willEvaluateScript(Frame* frame, const String& url, int lineNumber)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForFrame(frame))
        return willEvaluateScriptImpl(inspectorAgent, url, lineNumber);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didEvaluateScript(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didEvaluateScriptImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willFireTimer(ScriptExecutionContext* context, int timerId)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        return willFireTimerImpl(inspectorAgent, timerId);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didFireTimer(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didFireTimerImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willLayout(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForFrame(frame))
        return willLayoutImpl(inspectorAgent);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didLayout(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didLayoutImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willLoadXHR(ScriptExecutionContext* context, XMLHttpRequest* request)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        return willLoadXHRImpl(inspectorAgent, request);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didLoadXHR(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didLoadXHRImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willPaint(Frame* frame, const IntRect& rect)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForFrame(frame))
        return willPaintImpl(inspectorAgent, rect);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didPaint(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didPaintImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willRecalculateStyle(Document* document)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        return willRecalculateStyleImpl(inspectorAgent);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didRecalculateStyle(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didRecalculateStyleImpl(cookie);
#endif
}

inline void InspectorInstrumentation::applyUserAgentOverride(Frame* frame, String* userAgent)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForFrame(frame))
        applyUserAgentOverrideImpl(inspectorAgent, userAgent);
#endif
}

inline void InspectorInstrumentation::willSendRequest(Frame* frame, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* ic = inspectorAgentForFrame(frame))
        willSendRequestImpl(ic, identifier, loader, request, redirectResponse);
#endif
}

inline void InspectorInstrumentation::continueAfterPingLoader(Frame* frame, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& response)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* ic = inspectorAgentForFrame(frame))
        InspectorInstrumentation::continueAfterPingLoaderImpl(ic, identifier, loader, request, response);
#endif
}

inline void InspectorInstrumentation::markResourceAsCached(Page* page, unsigned long identifier)
{
#if ENABLE(INSPECTOR)
    markResourceAsCachedImpl(inspectorAgentForPage(page), identifier); 
#endif
}

inline void InspectorInstrumentation::didLoadResourceFromMemoryCache(Page* page, DocumentLoader* loader, const CachedResource* resource)
{
#if ENABLE(INSPECTOR)
    didLoadResourceFromMemoryCacheImpl(inspectorAgentForPage(page), loader, resource);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceData(Frame* frame, unsigned long identifier)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForFrame(frame))
        return willReceiveResourceDataImpl(inspectorAgent, identifier);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didReceiveResourceData(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didReceiveResourceDataImpl(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceResponse(Frame* frame, unsigned long identifier, const ResourceResponse& response)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(frame))
        return willReceiveResourceResponseImpl(inspectorAgent, identifier, response);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didReceiveResourceResponse(const InspectorInstrumentationCookie& cookie, unsigned long identifier, DocumentLoader* loader, const ResourceResponse& response)
{
#if ENABLE(INSPECTOR)
    // Call this unconditionally so that we're able to log to console with no front-end attached.
    didReceiveResourceResponseImpl(cookie, identifier, loader, response);
#endif
}

inline void InspectorInstrumentation::continueAfterXFrameOptionsDenied(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
#if ENABLE(INSPECTOR)
    if (inspectorAgentWithFrontendForFrame(frame))
        InspectorInstrumentation::continueAfterXFrameOptionsDeniedImpl(frame, loader, identifier, r);
#endif
}

inline void InspectorInstrumentation::continueWithPolicyDownload(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
#if ENABLE(INSPECTOR)
    if (inspectorAgentWithFrontendForFrame(frame))
        InspectorInstrumentation::continueWithPolicyDownloadImpl(frame, loader, identifier, r);
#endif
}

inline void InspectorInstrumentation::continueWithPolicyIgnore(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
#if ENABLE(INSPECTOR)
    if (inspectorAgentWithFrontendForFrame(frame))
        InspectorInstrumentation::continueWithPolicyIgnoreImpl(frame, loader, identifier, r);
#endif
}

inline void InspectorInstrumentation::didReceiveContentLength(Frame* frame, unsigned long identifier, int dataLength, int encodedDataLength)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(frame))
        didReceiveContentLengthImpl(inspectorAgent, identifier, dataLength, encodedDataLength);
#endif
}

inline void InspectorInstrumentation::didFinishLoading(Frame* frame, unsigned long identifier, double finishTime)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(frame))
        didFinishLoadingImpl(inspectorAgent, identifier, finishTime);
#endif
}

inline void InspectorInstrumentation::didFailLoading(Frame* frame, unsigned long identifier, const ResourceError& error)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(frame))
        didFailLoadingImpl(inspectorAgent, identifier, error);
#endif
}

inline void InspectorInstrumentation::resourceRetrievedByXMLHttpRequest(ScriptExecutionContext* context, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForContext(context))
        resourceRetrievedByXMLHttpRequestImpl(inspectorAgent, identifier, sourceString, url, sendURL, sendLineNumber);
#endif
}

inline void InspectorInstrumentation::scriptImported(ScriptExecutionContext* context, unsigned long identifier, const String& sourceString)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForContext(context))
        scriptImportedImpl(inspectorAgent, identifier, sourceString);
#endif
}

inline void InspectorInstrumentation::domContentLoadedEventFired(Frame* frame, const KURL& url)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(frame))
        domContentLoadedEventFiredImpl(inspectorAgent, frame, url);
#endif
}

inline void InspectorInstrumentation::loadEventFired(Frame* frame, const KURL& url)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(frame))
        loadEventFiredImpl(inspectorAgent, frame, url);
#endif
}

inline void InspectorInstrumentation::frameDetachedFromParent(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(frame))
        frameDetachedFromParentImpl(inspectorAgent, frame);
#endif
}

inline void InspectorInstrumentation::didCommitLoad(Frame* frame, DocumentLoader* loader)
{
#if ENABLE(INSPECTOR)
    if (!frame)
        return;
    Page* page = frame->page();
    if (!page)
        return;
    if (InspectorAgent* inspectorAgent = inspectorAgentForPage(page))
        didCommitLoadImpl(page, inspectorAgent, loader);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willWriteHTML(Document* document, unsigned int length, unsigned int startLine)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForDocument(document))
        return willWriteHTMLImpl(inspectorAgent, length, startLine);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didWriteHTML(const InspectorInstrumentationCookie& cookie, unsigned int endLine)
{
#if ENABLE(INSPECTOR)
    if (hasFrontends() && cookie.first)
        didWriteHTMLImpl(cookie, endLine);
#endif
}

#if ENABLE(DOM_STORAGE)
inline void InspectorInstrumentation::didUseDOMStorage(Page* page, StorageArea* storageArea, bool isLocalStorage, Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForPage(page))
        didUseDOMStorageImpl(inspectorAgent, storageArea, isLocalStorage, frame);
#endif
}
#endif

#if ENABLE(WORKERS)
inline bool InspectorInstrumentation::willStartWorkerContext(ScriptExecutionContext* context, WorkerContextProxy*)
{
#if ENABLE(INSPECTOR)
    return inspectorAgentWithFrontendForContext(context);
#else
    return false;
#endif
}

inline void InspectorInstrumentation::didStartWorkerContext(ScriptExecutionContext* context, WorkerContextProxy* proxy, bool paused)
{
#if ENABLE(INSPECTOR)
    if (!paused)
        return;
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        didStartWorkerContextImpl(inspectorAgent, proxy);
#endif
}

inline void InspectorInstrumentation::didCreateWorker(ScriptExecutionContext* context, intptr_t id, const String& url, bool isSharedWorker)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        didCreateWorkerImpl(inspectorAgent, id, url, isSharedWorker);
#endif
}

inline void InspectorInstrumentation::didDestroyWorker(ScriptExecutionContext* context, intptr_t id)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForContext(context))
        didDestroyWorkerImpl(inspectorAgent, id);
#endif
}
#endif


#if ENABLE(WEB_SOCKETS)
inline void InspectorInstrumentation::didCreateWebSocket(ScriptExecutionContext* context, unsigned long identifier, const KURL& requestURL, const KURL& documentURL)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForContext(context))
        didCreateWebSocketImpl(inspectorAgent, identifier, requestURL, documentURL);
#endif
}

inline void InspectorInstrumentation::willSendWebSocketHandshakeRequest(ScriptExecutionContext* context, unsigned long identifier, const WebSocketHandshakeRequest& request)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForContext(context))
        willSendWebSocketHandshakeRequestImpl(inspectorAgent, identifier, request);
#endif
}

inline void InspectorInstrumentation::didReceiveWebSocketHandshakeResponse(ScriptExecutionContext* context, unsigned long identifier, const WebSocketHandshakeResponse& response)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForContext(context))
        didReceiveWebSocketHandshakeResponseImpl(inspectorAgent, identifier, response);
#endif
}

inline void InspectorInstrumentation::didCloseWebSocket(ScriptExecutionContext* context, unsigned long identifier)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentForContext(context))
        didCloseWebSocketImpl(inspectorAgent, identifier);
#endif
}
#endif

inline void InspectorInstrumentation::networkStateChanged(Page* page)
{
#if ENABLE(INSPECTOR) && ENABLE(OFFLINE_WEB_APPLICATIONS)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForPage(page))
        networkStateChangedImpl(inspectorAgent);
#endif
}

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
inline void InspectorInstrumentation::updateApplicationCacheStatus(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InspectorAgent* inspectorAgent = inspectorAgentWithFrontendForFrame(frame))
        updateApplicationCacheStatusImpl(inspectorAgent, frame);
#endif
}
#endif

#if ENABLE(INSPECTOR)
inline bool InspectorInstrumentation::hasFrontend(Page* page)
{
    return inspectorAgentWithFrontendForPage(page);
}
#endif


#if ENABLE(INSPECTOR)
inline InspectorAgent* InspectorInstrumentation::inspectorAgentForContext(ScriptExecutionContext* context)
{
    if (context && context->isDocument())
        return inspectorAgentForPage(static_cast<Document*>(context)->page());
    return 0;
}

inline InspectorAgent* InspectorInstrumentation::inspectorAgentForFrame(Frame* frame)
{
    if (frame)
        return inspectorAgentForPage(frame->page());
    return 0;
}

inline InspectorAgent* InspectorInstrumentation::inspectorAgentForPage(Page* page)
{
    if (!page)
        return 0;
    return inspectorAgents().get(page);
}

inline InspectorAgent* InspectorInstrumentation::inspectorAgentWithFrontendForContext(ScriptExecutionContext* context)
{
    if (hasFrontends() && context && context->isDocument())
        return inspectorAgentWithFrontendForPage(static_cast<Document*>(context)->page());
    return 0;
}

inline InspectorAgent* InspectorInstrumentation::inspectorAgentWithFrontendForDocument(Document* document)
{
    if (hasFrontends() && document)
        return inspectorAgentWithFrontendForPage(document->page());
    return 0;
}

inline InspectorAgent* InspectorInstrumentation::inspectorAgentWithFrontendForFrame(Frame* frame)
{
    if (hasFrontends() && frame)
        return inspectorAgentWithFrontendForPage(frame->page());
    return 0;
}

inline InspectorAgent* InspectorInstrumentation::inspectorAgentWithFrontendForPage(Page* page)
{
    if (page) {
        if (InspectorAgent* inspectorAgent = inspectorAgentForPage(page)) {
            if (hasFrontend(inspectorAgent))
                return inspectorAgent;
        }
    }
    return 0;
}

#endif

} // namespace WebCore

#endif // !defined(InspectorInstrumentation_h)
