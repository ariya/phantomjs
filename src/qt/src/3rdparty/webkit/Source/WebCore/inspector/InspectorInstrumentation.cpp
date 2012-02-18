/*
* Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"
#include "InspectorInstrumentation.h"

#if ENABLE(INSPECTOR)

#include "DOMWindow.h"
#include "Database.h"
#include "DocumentLoader.h"
#include "Event.h"
#include "EventContext.h"
#include "InspectorAgent.h"
#include "InspectorApplicationCacheAgent.h"
#include "InspectorDOMDebuggerAgent.h"
#include "InspectorCSSAgent.h"
#include "InspectorConsoleAgent.h"
#include "InspectorDatabaseAgent.h"
#include "InspectorDOMAgent.h"
#include "InspectorDOMStorageAgent.h"
#include "InspectorDebuggerAgent.h"
#include "InspectorPageAgent.h"
#include "InspectorProfilerAgent.h"
#include "InspectorResourceAgent.h"
#include "InspectorTimelineAgent.h"
#include "InspectorWorkerAgent.h"
#include "InstrumentingAgents.h"
#include "ScriptArguments.h"
#include "ScriptCallStack.h"
#include "XMLHttpRequest.h"
#include <wtf/text/CString.h>

namespace WebCore {

static const char* const listenerEventCategoryType = "listener";
static const char* const instrumentationEventCategoryType = "instrumentation";

static const char* const setTimerEventName = "setTimer";
static const char* const clearTimerEventName = "clearTimer";
static const char* const timerFiredEventName = "timerFired";

HashMap<Page*, InspectorAgent*>& InspectorInstrumentation::inspectorAgents()
{
    static HashMap<Page*, InspectorAgent*>& agents = *new HashMap<Page*, InspectorAgent*>;
    return agents;
}

int InspectorInstrumentation::s_frontendCounter = 0;

static bool eventHasListeners(const AtomicString& eventType, DOMWindow* window, Node* node, const Vector<EventContext>& ancestors)
{
    if (window && window->hasEventListeners(eventType))
        return true;

    if (node->hasEventListeners(eventType))
        return true;

    for (size_t i = 0; i < ancestors.size(); i++) {
        Node* ancestor = ancestors[i].node();
        if (ancestor->hasEventListeners(eventType))
            return true;
    }

    return false;
}

void InspectorInstrumentation::didClearWindowObjectInWorldImpl(InspectorAgent* inspectorAgent, Frame* frame, DOMWrapperWorld* world)
{
    if (InspectorPageAgent* pageAgent = inspectorAgent->instrumentingAgents()->inspectorPageAgent())
        pageAgent->didClearWindowObjectInWorld(frame, world);
    inspectorAgent->didClearWindowObjectInWorld(frame, world);
}

void InspectorInstrumentation::inspectedPageDestroyedImpl(InspectorAgent* inspectorAgent)
{
    inspectorAgent->inspectedPageDestroyed();
}

void InspectorInstrumentation::willInsertDOMNodeImpl(InspectorAgent* inspectorAgent, Node* node, Node* parent)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = inspectorAgent->instrumentingAgents()->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willInsertDOMNode(node, parent);
#endif
}

void InspectorInstrumentation::didInsertDOMNodeImpl(InspectorAgent* inspectorAgent, Node* node)
{
    if (InspectorDOMAgent* domAgent = inspectorAgent->domAgent())
        domAgent->didInsertDOMNode(node);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = inspectorAgent->instrumentingAgents()->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didInsertDOMNode(node);
#endif
}

void InspectorInstrumentation::willRemoveDOMNodeImpl(InspectorAgent* inspectorAgent, Node* node)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = inspectorAgent->instrumentingAgents()->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willRemoveDOMNode(node);
#endif
}

void InspectorInstrumentation::didRemoveDOMNodeImpl(InspectorAgent* inspectorAgent, Node* node)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = inspectorAgent->instrumentingAgents()->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didRemoveDOMNode(node);
#endif
    if (InspectorDOMAgent* domAgent = inspectorAgent->domAgent())
        domAgent->didRemoveDOMNode(node);
}

void InspectorInstrumentation::willModifyDOMAttrImpl(InspectorAgent* inspectorAgent, Element* element)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = inspectorAgent->instrumentingAgents()->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willModifyDOMAttr(element);
#endif
}

void InspectorInstrumentation::didModifyDOMAttrImpl(InspectorAgent* inspectorAgent, Element* element)
{
    if (InspectorDOMAgent* domAgent = inspectorAgent->domAgent())
        domAgent->didModifyDOMAttr(element);
}

void InspectorInstrumentation::didInvalidateStyleAttrImpl(InspectorAgent* inspectorAgent, Node* node)
{
    if (InspectorDOMAgent* domAgent = inspectorAgent->domAgent())
        domAgent->didInvalidateStyleAttr(node);
}

void InspectorInstrumentation::mouseDidMoveOverElementImpl(InspectorAgent* inspectorAgent, const HitTestResult& result, unsigned modifierFlags)
{
    if (InspectorDOMAgent* domAgent = inspectorAgent->instrumentingAgents()->inspectorDOMAgent())
        domAgent->mouseDidMoveOverElement(result, modifierFlags);
}

bool InspectorInstrumentation::handleMousePressImpl(InspectorAgent* inspectorAgent)
{
    if (InspectorDOMAgent* domAgent = inspectorAgent->instrumentingAgents()->inspectorDOMAgent())
        return domAgent->handleMousePress();
    return false;
}

void InspectorInstrumentation::characterDataModifiedImpl(InspectorAgent* inspectorAgent, CharacterData* characterData)
{
    if (InspectorDOMAgent* domAgent = inspectorAgent->domAgent())
        domAgent->characterDataModified(characterData);
}

void InspectorInstrumentation::willSendXMLHttpRequestImpl(InspectorAgent* inspectorAgent, const String& url)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = inspectorAgent->instrumentingAgents()->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willSendXMLHttpRequest(url);
#endif
}

void InspectorInstrumentation::didScheduleResourceRequestImpl(InspectorAgent* inspectorAgent, const String& url)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent))
        timelineAgent->didScheduleResourceRequest(url);
}

void InspectorInstrumentation::didInstallTimerImpl(InspectorAgent* inspectorAgent, int timerId, int timeout, bool singleShot)
{
    pauseOnNativeEventIfNeeded(inspectorAgent, instrumentationEventCategoryType, setTimerEventName, true);
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent))
        timelineAgent->didInstallTimer(timerId, timeout, singleShot);
}

void InspectorInstrumentation::didRemoveTimerImpl(InspectorAgent* inspectorAgent, int timerId)
{
    pauseOnNativeEventIfNeeded(inspectorAgent, instrumentationEventCategoryType, clearTimerEventName, true);
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent))
        timelineAgent->didRemoveTimer(timerId);
}

InspectorInstrumentationCookie InspectorInstrumentation::willCallFunctionImpl(InspectorAgent* inspectorAgent, const String& scriptName, int scriptLine)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willCallFunction(scriptName, scriptLine);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didCallFunctionImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didCallFunction();
}

InspectorInstrumentationCookie InspectorInstrumentation::willChangeXHRReadyStateImpl(InspectorAgent* inspectorAgent, XMLHttpRequest* request)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent && request->hasEventListeners(eventNames().readystatechangeEvent)) {
        timelineAgent->willChangeXHRReadyState(request->url().string(), request->readyState());
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didChangeXHRReadyStateImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didChangeXHRReadyState();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEventImpl(InspectorAgent* inspectorAgent, const Event& event, DOMWindow* window, Node* node, const Vector<EventContext>& ancestors)
{
    pauseOnNativeEventIfNeeded(inspectorAgent, listenerEventCategoryType, event.type(), false);

    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent && eventHasListeners(event.type(), window, node, ancestors)) {
        timelineAgent->willDispatchEvent(event);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didDispatchEventImpl(const InspectorInstrumentationCookie& cookie)
{
    cancelPauseOnNativeEvent(cookie.first);

    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchEvent();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEventOnWindowImpl(InspectorAgent* inspectorAgent, const Event& event, DOMWindow* window)
{
    pauseOnNativeEventIfNeeded(inspectorAgent, listenerEventCategoryType, event.type(), false);

    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent && window->hasEventListeners(event.type())) {
        timelineAgent->willDispatchEvent(event);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didDispatchEventOnWindowImpl(const InspectorInstrumentationCookie& cookie)
{
    cancelPauseOnNativeEvent(cookie.first);

    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchEvent();
}

InspectorInstrumentationCookie InspectorInstrumentation::willEvaluateScriptImpl(InspectorAgent* inspectorAgent, const String& url, int lineNumber)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willEvaluateScript(url, lineNumber);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didEvaluateScriptImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didEvaluateScript();
}

InspectorInstrumentationCookie InspectorInstrumentation::willFireTimerImpl(InspectorAgent* inspectorAgent, int timerId)
{
    pauseOnNativeEventIfNeeded(inspectorAgent, instrumentationEventCategoryType, timerFiredEventName, false);

    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willFireTimer(timerId);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didFireTimerImpl(const InspectorInstrumentationCookie& cookie)
{
    cancelPauseOnNativeEvent(cookie.first);

    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didFireTimer();
}

InspectorInstrumentationCookie InspectorInstrumentation::willLayoutImpl(InspectorAgent* inspectorAgent)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willLayout();
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didLayoutImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didLayout();
}

InspectorInstrumentationCookie InspectorInstrumentation::willLoadXHRImpl(InspectorAgent* inspectorAgent, XMLHttpRequest* request)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent && request->hasEventListeners(eventNames().loadEvent)) {
        timelineAgent->willLoadXHR(request->url());
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didLoadXHRImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didLoadXHR();
}

InspectorInstrumentationCookie InspectorInstrumentation::willPaintImpl(InspectorAgent* inspectorAgent, const IntRect& rect)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willPaint(rect);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didPaintImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didPaint();
}

InspectorInstrumentationCookie InspectorInstrumentation::willRecalculateStyleImpl(InspectorAgent* inspectorAgent)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willRecalculateStyle();
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didRecalculateStyleImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didRecalculateStyle();
}

void InspectorInstrumentation::applyUserAgentOverrideImpl(InspectorAgent* inspectorAgent, String* userAgent)
{
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->applyUserAgentOverride(userAgent);
}

void InspectorInstrumentation::willSendRequestImpl(InspectorAgent* inspectorAgent, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent))
        timelineAgent->willSendResourceRequest(identifier, request);
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->willSendRequest(identifier, loader, request, redirectResponse);
}

void InspectorInstrumentation::continueAfterPingLoaderImpl(InspectorAgent* inspectorAgent, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& response)
{
    willSendRequestImpl(inspectorAgent, identifier, loader, request, response);
}

void InspectorInstrumentation::markResourceAsCachedImpl(InspectorAgent* inspectorAgent, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->markResourceAsCached(identifier);
}

void InspectorInstrumentation::didLoadResourceFromMemoryCacheImpl(InspectorAgent* inspectorAgent, DocumentLoader* loader, const CachedResource* cachedResource)
{
    if (!inspectorAgent->enabled())
        return;
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->didLoadResourceFromMemoryCache(loader, cachedResource);
}

InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceDataImpl(InspectorAgent* inspectorAgent, unsigned long identifier)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willReceiveResourceData(identifier);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didReceiveResourceDataImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didReceiveResourceData();
}

InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceResponseImpl(InspectorAgent* inspectorAgent, unsigned long identifier, const ResourceResponse& response)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willReceiveResourceResponse(identifier, response);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didReceiveResourceResponseImpl(const InspectorInstrumentationCookie& cookie, unsigned long identifier, DocumentLoader* loader, const ResourceResponse& response)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didReceiveResourceResponse();
    if (!loader)
        return;
    if (InspectorAgent* inspectorAgent = inspectorAgentForFrame(loader->frame())) {
        if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
            resourceAgent->didReceiveResponse(identifier, loader, response);
        inspectorAgent->consoleAgent()->didReceiveResponse(identifier, response); // This should come AFTER resource notification, front-end relies on this.
    }
}

void InspectorInstrumentation::didReceiveResourceResponseButCanceledImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceResponse(frame, identifier, r);
    InspectorInstrumentation::didReceiveResourceResponse(cookie, identifier, loader, r);
}

void InspectorInstrumentation::continueAfterXFrameOptionsDeniedImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void InspectorInstrumentation::continueWithPolicyDownloadImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void InspectorInstrumentation::continueWithPolicyIgnoreImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void InspectorInstrumentation::didReceiveContentLengthImpl(InspectorAgent* inspectorAgent, unsigned long identifier, int dataLength, int encodedDataLength)
{
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->didReceiveContentLength(identifier, dataLength, encodedDataLength);
}

void InspectorInstrumentation::didFinishLoadingImpl(InspectorAgent* inspectorAgent, unsigned long identifier, double finishTime)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent))
        timelineAgent->didFinishLoadingResource(identifier, false, finishTime);
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->didFinishLoading(identifier, finishTime);
}

void InspectorInstrumentation::didFailLoadingImpl(InspectorAgent* inspectorAgent, unsigned long identifier, const ResourceError& error)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent))
        timelineAgent->didFinishLoadingResource(identifier, true, 0);
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->didFailLoading(identifier, error);
    inspectorAgent->consoleAgent()->didFailLoading(identifier, error); // This should come AFTER resource notification, front-end relies on this.
}

void InspectorInstrumentation::resourceRetrievedByXMLHttpRequestImpl(InspectorAgent* inspectorAgent, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber)
{
    inspectorAgent->consoleAgent()->resourceRetrievedByXMLHttpRequest(url, sendURL, sendLineNumber);
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->setInitialXHRContent(identifier, sourceString);
}

void InspectorInstrumentation::scriptImportedImpl(InspectorAgent* inspectorAgent, unsigned long identifier, const String& sourceString)
{
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->setInitialScriptContent(identifier, sourceString);
}

void InspectorInstrumentation::domContentLoadedEventFiredImpl(InspectorAgent* inspectorAgent, Frame* frame, const KURL& url)
{
    DocumentLoader* documentLoader = frame->loader()->documentLoader();
    ASSERT(documentLoader);

    if (frame->page()->mainFrame() != frame || url != documentLoader->requestURL())
        return;

    inspectorAgent->domContentLoadedEventFired();

    if (InspectorDOMAgent* domAgent = inspectorAgent->instrumentingAgents()->inspectorDOMAgent())
        domAgent->mainFrameDOMContentLoaded();

    if (InspectorTimelineAgent* timelineAgent = inspectorAgent->instrumentingAgents()->inspectorTimelineAgent())
        timelineAgent->didMarkDOMContentEvent();

    if (InspectorPageAgent* pageAgent = inspectorAgent->instrumentingAgents()->inspectorPageAgent())
        pageAgent->domContentEventFired();
}

void InspectorInstrumentation::loadEventFiredImpl(InspectorAgent* inspectorAgent, Frame* frame, const KURL& url)
{
    DocumentLoader* documentLoader = frame->loader()->documentLoader();
    ASSERT(documentLoader);

    if (InspectorDOMAgent* domAgent = inspectorAgent->instrumentingAgents()->inspectorDOMAgent())
        domAgent->loadEventFired(documentLoader->frame()->document());

    if (frame->page()->mainFrame() != frame || url != documentLoader->requestURL())
        return;

    if (InspectorTimelineAgent* timelineAgent = inspectorAgent->instrumentingAgents()->inspectorTimelineAgent())
        timelineAgent->didMarkLoadEvent();

    if (InspectorPageAgent* pageAgent = inspectorAgent->instrumentingAgents()->inspectorPageAgent())
        pageAgent->loadEventFired();
}

void InspectorInstrumentation::frameDetachedFromParentImpl(InspectorAgent* inspectorAgent, Frame* frame)
{
    if (InspectorPageAgent* pageAgent = inspectorAgent->instrumentingAgents()->inspectorPageAgent())
        pageAgent->frameDetached(frame);
}

void InspectorInstrumentation::didCommitLoadImpl(Page* page, InspectorAgent* inspectorAgent, DocumentLoader* loader)
{
    if (!inspectorAgent->enabled())
        return;

    InstrumentingAgents* instrumentingAgents = inspectorAgent->instrumentingAgents();

    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameNavigated(loader);

    Frame* mainFrame = page->mainFrame();
    if (loader->frame() != mainFrame)
        return;

    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->reset();
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent()) {
        KURL url = inspectorAgent->inspectedURLWithoutFragment();
        debuggerAgent->inspectedURLChanged(url);
    }
#endif
#if ENABLE(JAVASCRIPT_DEBUGGER) && USE(JSC)
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent()) {
        profilerAgent->stopUserInitiatedProfiling(true);
        profilerAgent->resetState();
    }
#endif
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->reset();
#if ENABLE(DATABASE)
    if (InspectorDatabaseAgent* databaseAgent = instrumentingAgents->inspectorDatabaseAgent())
        databaseAgent->clearResources();
#endif
#if ENABLE(DOM_STORAGE)
    if (InspectorDOMStorageAgent* domStorageAgent = instrumentingAgents->inspectorDOMStorageAgent())
        domStorageAgent->clearResources();
#endif
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->setDocument(mainFrame->document());

    inspectorAgent->didCommitLoad();
}

InspectorInstrumentationCookie InspectorInstrumentation::willWriteHTMLImpl(InspectorAgent* inspectorAgent, unsigned int length, unsigned int startLine)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent);
    if (timelineAgent) {
        timelineAgent->willWriteHTML(length, startLine);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(inspectorAgent, timelineAgentId);
}

void InspectorInstrumentation::didWriteHTMLImpl(const InspectorInstrumentationCookie& cookie, unsigned int endLine)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didWriteHTML(endLine);
}

void InspectorInstrumentation::addMessageToConsoleImpl(InspectorAgent* inspectorAgent, MessageSource source, MessageType type, MessageLevel level, const String& message, PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    inspectorAgent->consoleAgent()->addMessageToConsole(source, type, level, message, arguments, callStack);
}

void InspectorInstrumentation::addMessageToConsoleImpl(InspectorAgent* inspectorAgent, MessageSource source, MessageType type, MessageLevel level, const String& message, unsigned lineNumber, const String& sourceID)
{
    inspectorAgent->consoleAgent()->addMessageToConsole(source, type, level, message, lineNumber, sourceID);
}

void InspectorInstrumentation::consoleCountImpl(InspectorAgent* inspectorAgent, PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> stack)
{
    inspectorAgent->consoleAgent()->count(arguments, stack);
}

void InspectorInstrumentation::startConsoleTimingImpl(InspectorAgent* inspectorAgent, const String& title)
{
    inspectorAgent->consoleAgent()->startTiming(title);
}

void InspectorInstrumentation::stopConsoleTimingImpl(InspectorAgent* inspectorAgent, const String& title, PassRefPtr<ScriptCallStack> stack)
{
    inspectorAgent->consoleAgent()->stopTiming(title, stack);
}

void InspectorInstrumentation::consoleMarkTimelineImpl(InspectorAgent* inspectorAgent, PassRefPtr<ScriptArguments> arguments)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(inspectorAgent)) {
        String message;
        arguments->getFirstArgumentAsString(message);
        timelineAgent->didMarkTimeline(message);
     }
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
void InspectorInstrumentation::addStartProfilingMessageToConsoleImpl(InspectorAgent* inspectorAgent, const String& title, unsigned lineNumber, const String& sourceURL)
{
    if (InspectorProfilerAgent* profilerAgent = inspectorAgent->profilerAgent())
        profilerAgent->addStartProfilingMessageToConsole(title, lineNumber, sourceURL);
}

void InspectorInstrumentation::addProfileImpl(InspectorAgent* inspectorAgent, RefPtr<ScriptProfile> profile, PassRefPtr<ScriptCallStack> callStack)
{
    if (InspectorProfilerAgent* profilerAgent = inspectorAgent->profilerAgent()) {
        const ScriptCallFrame& lastCaller = callStack->at(0);
        profilerAgent->addProfile(profile, lastCaller.lineNumber(), lastCaller.sourceURL());
    }
}

String InspectorInstrumentation::getCurrentUserInitiatedProfileNameImpl(InspectorAgent* inspectorAgent, bool incrementProfileNumber)
{
    if (InspectorProfilerAgent* profilerAgent = inspectorAgent->profilerAgent())
        return profilerAgent->getCurrentUserInitiatedProfileName(incrementProfileNumber);
    return "";
}

bool InspectorInstrumentation::profilerEnabledImpl(InspectorAgent* inspectorAgent)
{
    return inspectorAgent->instrumentingAgents()->inspectorProfilerAgent()->enabled();
}
#endif

#if ENABLE(DATABASE)
void InspectorInstrumentation::didOpenDatabaseImpl(InspectorAgent* inspectorAgent, PassRefPtr<Database> database, const String& domain, const String& name, const String& version)
{
    if (!inspectorAgent->enabled())
        return;
    if (InspectorDatabaseAgent* dbAgent = inspectorAgent->instrumentingAgents()->inspectorDatabaseAgent())
        dbAgent->didOpenDatabase(database, domain, name, version);
}
#endif

#if ENABLE(DOM_STORAGE)
void InspectorInstrumentation::didUseDOMStorageImpl(InspectorAgent* inspectorAgent, StorageArea* storageArea, bool isLocalStorage, Frame* frame)
{
    if (!inspectorAgent->enabled())
        return;
    if (InspectorDOMStorageAgent* domStorageAgent = inspectorAgent->instrumentingAgents()->inspectorDOMStorageAgent())
        domStorageAgent->didUseDOMStorage(storageArea, isLocalStorage, frame);
}
#endif

#if ENABLE(WORKERS)
void InspectorInstrumentation::didStartWorkerContextImpl(InspectorAgent* inspectorAgent, WorkerContextProxy* workerContextProxy)
{
    if (InspectorWorkerAgent* workerAgent = inspectorAgent->instrumentingAgents()->inspectorWorkerAgent())
        workerAgent->didStartWorkerContext(workerContextProxy);
}

void InspectorInstrumentation::didCreateWorkerImpl(InspectorAgent* inspectorAgent, intptr_t id, const String& url, bool isSharedWorker)
{
    inspectorAgent->didCreateWorker(id, url, isSharedWorker);
}

void InspectorInstrumentation::didDestroyWorkerImpl(InspectorAgent* inspectorAgent, intptr_t id)
{
    inspectorAgent->didDestroyWorker(id);
}
#endif

#if ENABLE(WEB_SOCKETS)
void InspectorInstrumentation::didCreateWebSocketImpl(InspectorAgent* inspectorAgent, unsigned long identifier, const KURL& requestURL, const KURL&)
{
    if (!inspectorAgent->enabled())
        return;
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->didCreateWebSocket(identifier, requestURL);
}

void InspectorInstrumentation::willSendWebSocketHandshakeRequestImpl(InspectorAgent* inspectorAgent, unsigned long identifier, const WebSocketHandshakeRequest& request)
{
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->willSendWebSocketHandshakeRequest(identifier, request);
}

void InspectorInstrumentation::didReceiveWebSocketHandshakeResponseImpl(InspectorAgent* inspectorAgent, unsigned long identifier, const WebSocketHandshakeResponse& response)
{
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->didReceiveWebSocketHandshakeResponse(identifier, response);
}

void InspectorInstrumentation::didCloseWebSocketImpl(InspectorAgent* inspectorAgent, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = retrieveResourceAgent(inspectorAgent))
        resourceAgent->didCloseWebSocket(identifier);
}
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
void InspectorInstrumentation::networkStateChangedImpl(InspectorAgent* inspectorAgent)
{
    if (InspectorApplicationCacheAgent* applicationCacheAgent = inspectorAgent->instrumentingAgents()->inspectorApplicationCacheAgent())
        applicationCacheAgent->networkStateChanged();
}

void InspectorInstrumentation::updateApplicationCacheStatusImpl(InspectorAgent* inspectorAgent, Frame* frame)
{
    if (InspectorApplicationCacheAgent* applicationCacheAgent = inspectorAgent->instrumentingAgents()->inspectorApplicationCacheAgent())
        applicationCacheAgent->updateApplicationCacheStatus(frame);
}
#endif

bool InspectorInstrumentation::hasFrontend(InspectorAgent* inspectorAgent)
{
    return inspectorAgent->hasFrontend();
}

void InspectorInstrumentation::pauseOnNativeEventIfNeeded(InspectorAgent* inspectorAgent, const String& categoryType, const String& eventName, bool synchronous)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = inspectorAgent->instrumentingAgents()->inspectorDOMDebuggerAgent())
        domDebuggerAgent->pauseOnNativeEventIfNeeded(categoryType, eventName, synchronous);
#endif
}

void InspectorInstrumentation::cancelPauseOnNativeEvent(InspectorAgent* inspectorAgent)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDebuggerAgent* debuggerAgent = inspectorAgent->debuggerAgent())
        debuggerAgent->cancelPauseOnNextStatement();
#endif
}

InspectorTimelineAgent* InspectorInstrumentation::retrieveTimelineAgent(InspectorAgent* inspectorAgent)
{
    return inspectorAgent->instrumentingAgents()->inspectorTimelineAgent();
}

InspectorTimelineAgent* InspectorInstrumentation::retrieveTimelineAgent(const InspectorInstrumentationCookie& cookie)
{
    if (!cookie.first)
        return 0;
    InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie.first);
    if (timelineAgent && timelineAgent->id() == cookie.second)
        return timelineAgent;
    return 0;
}

InspectorResourceAgent* InspectorInstrumentation::retrieveResourceAgent(InspectorAgent* inspectorAgent)
{
    return inspectorAgent->instrumentingAgents()->inspectorResourceAgent();
}

InspectorPageAgent* InspectorInstrumentation::retrievePageAgent(InspectorAgent* inspectorAgent)
{
    return inspectorAgent->instrumentingAgents()->inspectorPageAgent();
}

} // namespace WebCore

#endif // !ENABLE(INSPECTOR)
