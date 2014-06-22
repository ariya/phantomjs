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

#if ENABLE(INSPECTOR)

#include "InspectorInstrumentation.h"

#include "CSSRule.h"
#include "CSSStyleRule.h"
#include "ConsoleAPITypes.h"
#include "ConsoleTypes.h"
#include "DOMWindow.h"
#include "DOMWrapperWorld.h"
#include "Database.h"
#include "DeviceOrientationData.h"
#include "DocumentLoader.h"
#include "Event.h"
#include "EventContext.h"
#include "InspectorAgent.h"
#include "InspectorApplicationCacheAgent.h"
#include "InspectorCSSAgent.h"
#include "InspectorCanvasAgent.h"
#include "InspectorConsoleAgent.h"
#include "InspectorController.h"
#include "InspectorDOMAgent.h"
#include "InspectorDOMDebuggerAgent.h"
#include "InspectorDOMStorageAgent.h"
#include "InspectorDatabaseAgent.h"
#include "InspectorDebuggerAgent.h"
#include "InspectorHeapProfilerAgent.h"
#include "InspectorLayerTreeAgent.h"
#include "InspectorPageAgent.h"
#include "InspectorProfilerAgent.h"
#include "InspectorResourceAgent.h"
#include "InspectorTimelineAgent.h"
#include "InspectorWorkerAgent.h"
#include "InstrumentingAgents.h"
#include "PageDebuggerAgent.h"
#include "PageRuntimeAgent.h"
#include "RenderObject.h"
#include "ScriptArguments.h"
#include "ScriptCallStack.h"
#include "ScriptController.h"
#include "ScriptProfile.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "WorkerGlobalScope.h"
#include "WorkerInspectorController.h"
#include "WorkerRuntimeAgent.h"
#include "WorkerThread.h"
#include "XMLHttpRequest.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

namespace WebCore {

static const char* const requestAnimationFrameEventName = "requestAnimationFrame";
static const char* const cancelAnimationFrameEventName = "cancelAnimationFrame";
static const char* const animationFrameFiredEventName = "animationFrameFired";
static const char* const setTimerEventName = "setTimer";
static const char* const clearTimerEventName = "clearTimer";
static const char* const timerFiredEventName = "timerFired";

namespace {
static HashSet<InstrumentingAgents*>* instrumentingAgentsSet = 0;
}

int InspectorInstrumentation::s_frontendCounter = 0;

static bool eventHasListeners(const AtomicString& eventType, DOMWindow* window, Node* node, const EventPath& eventPath)
{
    if (window && window->hasEventListeners(eventType))
        return true;

    if (node->hasEventListeners(eventType))
        return true;

    for (size_t i = 0; i < eventPath.size(); i++) {
        if (eventPath[i]->node()->hasEventListeners(eventType))
            return true;
    }

    return false;
}

static Frame* frameForScriptExecutionContext(ScriptExecutionContext* context)
{
    Frame* frame = 0;
    if (context->isDocument())
        frame = toDocument(context)->frame();
    return frame;
}

InspectorInstrumentationCookie::InspectorInstrumentationCookie()
    : m_instrumentingAgents(0)
    , m_timelineAgentId(0)
{
}

InspectorInstrumentationCookie::InspectorInstrumentationCookie(InstrumentingAgents* agents, int timelineAgentId)
    : m_instrumentingAgents(agents)
    , m_timelineAgentId(timelineAgentId)
{
}

InspectorInstrumentationCookie::InspectorInstrumentationCookie(const InspectorInstrumentationCookie& other)
    : m_instrumentingAgents(other.m_instrumentingAgents)
    , m_timelineAgentId(other.m_timelineAgentId)
{
}

InspectorInstrumentationCookie& InspectorInstrumentationCookie::operator=(const InspectorInstrumentationCookie& other)
{
    if (this != &other) {
        m_instrumentingAgents = other.m_instrumentingAgents;
        m_timelineAgentId = other.m_timelineAgentId;
    }
    return *this;
}

InspectorInstrumentationCookie::~InspectorInstrumentationCookie()
{
}

void InspectorInstrumentation::didClearWindowObjectInWorldImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, DOMWrapperWorld* world)
{
    InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent();
    if (pageAgent)
        pageAgent->didClearWindowObjectInWorld(frame, world);
    if (InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent())
        inspectorAgent->didClearWindowObjectInWorld(frame, world);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (PageDebuggerAgent* debuggerAgent = instrumentingAgents->pageDebuggerAgent()) {
        if (pageAgent && world == mainThreadNormalWorld() && frame == pageAgent->mainFrame())
            debuggerAgent->didClearMainFrameWindowObject();
    }
#endif
    if (PageRuntimeAgent* pageRuntimeAgent = instrumentingAgents->pageRuntimeAgent()) {
        if (world == mainThreadNormalWorld())
            pageRuntimeAgent->didCreateMainWorldContext(frame);
    }
}

bool InspectorInstrumentation::isDebuggerPausedImpl(InstrumentingAgents* instrumentingAgents)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        return debuggerAgent->isPaused();
#endif
    return false;
}

void InspectorInstrumentation::willInsertDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* parent)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willInsertDOMNode(parent);
#endif
}

void InspectorInstrumentation::didInsertDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didInsertDOMNode(node);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didInsertDOMNode(node);
#endif
}

void InspectorInstrumentation::willRemoveDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willRemoveDOMNode(node);
#endif
}

void InspectorInstrumentation::didRemoveDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didRemoveDOMNode(node);
#endif
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didRemoveDOMNode(node);
}

void InspectorInstrumentation::willModifyDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& oldValue, const AtomicString& newValue)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willModifyDOMAttr(element);
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->willModifyDOMAttr(element, oldValue, newValue);
#endif
}

void InspectorInstrumentation::didModifyDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& name, const AtomicString& value)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didModifyDOMAttr(element, name, value);
}

void InspectorInstrumentation::didRemoveDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& name)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didRemoveDOMAttr(element, name);
}

void InspectorInstrumentation::didInvalidateStyleAttrImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didInvalidateStyleAttr(node);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didInvalidateStyleAttr(node);
#endif
}

void InspectorInstrumentation::frameWindowDiscardedImpl(InstrumentingAgents* instrumentingAgents, DOMWindow* window)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->frameWindowDiscarded(window);
}

void InspectorInstrumentation::mediaQueryResultChangedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->mediaQueryResultChanged();
}

void InspectorInstrumentation::didPushShadowRootImpl(InstrumentingAgents* instrumentingAgents, Element* host, ShadowRoot* root)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didPushShadowRoot(host, root);
}

void InspectorInstrumentation::willPopShadowRootImpl(InstrumentingAgents* instrumentingAgents, Element* host, ShadowRoot* root)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->willPopShadowRoot(host, root);
}

void InspectorInstrumentation::didCreateNamedFlowImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didCreateNamedFlow(document, namedFlow);
}

void InspectorInstrumentation::willRemoveNamedFlowImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->willRemoveNamedFlow(document, namedFlow);
}

void InspectorInstrumentation::didUpdateRegionLayoutImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didUpdateRegionLayout(document, namedFlow);
}

void InspectorInstrumentation::didChangeRegionOversetImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didChangeRegionOverset(document, namedFlow);
}

void InspectorInstrumentation::mouseDidMoveOverElementImpl(InstrumentingAgents* instrumentingAgents, const HitTestResult& result, unsigned modifierFlags)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->mouseDidMoveOverElement(result, modifierFlags);
}

void InspectorInstrumentation::didScrollImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didScroll();
}

bool InspectorInstrumentation::handleTouchEventImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        return domAgent->handleTouchEvent(node);
    return false;
}

bool InspectorInstrumentation::handleMousePressImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        return domAgent->handleMousePress();
    return false;
}

bool InspectorInstrumentation::forcePseudoStateImpl(InstrumentingAgents* instrumentingAgents, Element* element, CSSSelector::PseudoType pseudoState)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        return cssAgent->forcePseudoState(element, pseudoState);
    return false;
}

void InspectorInstrumentation::characterDataModifiedImpl(InstrumentingAgents* instrumentingAgents, CharacterData* characterData)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->characterDataModified(characterData);
}

void InspectorInstrumentation::willSendXMLHttpRequestImpl(InstrumentingAgents* instrumentingAgents, const String& url)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willSendXMLHttpRequest(url);
#endif
}

void InspectorInstrumentation::didScheduleResourceRequestImpl(InstrumentingAgents* instrumentingAgents, const String& url, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScheduleResourceRequest(url, frame);
}

void InspectorInstrumentation::didInstallTimerImpl(InstrumentingAgents* instrumentingAgents, int timerId, int timeout, bool singleShot, ScriptExecutionContext* context)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, setTimerEventName, true);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didInstallTimer(timerId, timeout, singleShot, frameForScriptExecutionContext(context));
}

void InspectorInstrumentation::didRemoveTimerImpl(InstrumentingAgents* instrumentingAgents, int timerId, ScriptExecutionContext* context)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, clearTimerEventName, true);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didRemoveTimer(timerId, frameForScriptExecutionContext(context));
}

InspectorInstrumentationCookie InspectorInstrumentation::willCallFunctionImpl(InstrumentingAgents* instrumentingAgents, const String& scriptName, int scriptLine, ScriptExecutionContext* context)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willCallFunction(scriptName, scriptLine, frameForScriptExecutionContext(context));
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didCallFunctionImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didCallFunction();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchXHRReadyStateChangeEventImpl(InstrumentingAgents* instrumentingAgents, XMLHttpRequest* request, ScriptExecutionContext* context)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent && request->hasEventListeners(eventNames().readystatechangeEvent)) {
        timelineAgent->willDispatchXHRReadyStateChangeEvent(request->url().string(), request->readyState(), frameForScriptExecutionContext(context));
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didDispatchXHRReadyStateChangeEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchXHRReadyStateChangeEvent();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEventImpl(InstrumentingAgents* instrumentingAgents, const Event& event, DOMWindow* window, Node* node, const EventPath& eventPath, Document* document)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent && eventHasListeners(event.type(), window, node, eventPath)) {
        timelineAgent->willDispatchEvent(event, document->frame());
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

InspectorInstrumentationCookie InspectorInstrumentation::willHandleEventImpl(InstrumentingAgents* instrumentingAgents, Event* event)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, true, event->type(), false);
    return InspectorInstrumentationCookie(instrumentingAgents, 0);
}

void InspectorInstrumentation::didHandleEventImpl(const InspectorInstrumentationCookie& cookie)
{
    cancelPauseOnNativeEvent(cookie.instrumentingAgents());
}

void InspectorInstrumentation::didDispatchEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchEvent();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEventOnWindowImpl(InstrumentingAgents* instrumentingAgents, const Event& event, DOMWindow* window)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent && window->hasEventListeners(event.type())) {
        timelineAgent->willDispatchEvent(event, window ? window->frame() : 0);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didDispatchEventOnWindowImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchEvent();
}

InspectorInstrumentationCookie InspectorInstrumentation::willEvaluateScriptImpl(InstrumentingAgents* instrumentingAgents, const String& url, int lineNumber, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willEvaluateScript(url, lineNumber, frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didEvaluateScriptImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didEvaluateScript();
}

void InspectorInstrumentation::scriptsEnabledImpl(InstrumentingAgents* instrumentingAgents, bool isEnabled)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->scriptsEnabled(isEnabled);
}

void InspectorInstrumentation::didCreateIsolatedContextImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, ScriptState* scriptState, SecurityOrigin* origin)
{
    if (PageRuntimeAgent* runtimeAgent = instrumentingAgents->pageRuntimeAgent())
        runtimeAgent->didCreateIsolatedContext(frame, scriptState, origin);
}

InspectorInstrumentationCookie InspectorInstrumentation::willFireTimerImpl(InstrumentingAgents* instrumentingAgents, int timerId, ScriptExecutionContext* context)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, timerFiredEventName, false);

    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willFireTimer(timerId, frameForScriptExecutionContext(context));
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didFireTimerImpl(const InspectorInstrumentationCookie& cookie)
{
    cancelPauseOnNativeEvent(cookie.instrumentingAgents());

    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didFireTimer();
}

void InspectorInstrumentation::didInvalidateLayoutImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didInvalidateLayout(frame);
}

InspectorInstrumentationCookie InspectorInstrumentation::willLayoutImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willLayout(frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didLayoutImpl(const InspectorInstrumentationCookie& cookie, RenderObject* root)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didLayout(root);

    if (InspectorPageAgent* pageAgent = cookie.instrumentingAgents()->inspectorPageAgent())
        pageAgent->didLayout();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchXHRLoadEventImpl(InstrumentingAgents* instrumentingAgents, XMLHttpRequest* request, ScriptExecutionContext* context)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent && request->hasEventListeners(eventNames().loadEvent)) {
        timelineAgent->willDispatchXHRLoadEvent(request->url(), frameForScriptExecutionContext(context));
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didDispatchXHRLoadEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchXHRLoadEvent();
}

void InspectorInstrumentation::willPaintImpl(InstrumentingAgents* instrumentingAgents, RenderObject* renderer)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willPaint(renderer->frame());
}

void InspectorInstrumentation::didPaintImpl(InstrumentingAgents*  instrumentingAgents, RenderObject* renderer, GraphicsContext* context, const LayoutRect& rect)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didPaint(renderer, rect);
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didPaint(context, rect);
}

void InspectorInstrumentation::willScrollLayerImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willScroll(frame);
}

void InspectorInstrumentation::didScrollLayerImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScroll();
}

InspectorInstrumentationCookie InspectorInstrumentation::willRecalculateStyleImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willRecalculateStyle(frame);
        timelineAgentId = timelineAgent->id();
    }
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willRecalculateStyle();
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didRecalculateStyleImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didRecalculateStyle();
    InstrumentingAgents* instrumentingAgents = cookie.instrumentingAgents();
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didRecalculateStyle();
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didRecalculateStyle();
}

void InspectorInstrumentation::didScheduleStyleRecalculationImpl(InstrumentingAgents* instrumentingAgents, Document* document)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScheduleStyleRecalculation(document->frame());
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didScheduleStyleRecalculation(document);
}

InspectorInstrumentationCookie InspectorInstrumentation::willMatchRuleImpl(InstrumentingAgents* instrumentingAgents, StyleRule* rule, InspectorCSSOMWrappers& inspectorCSSOMWrappers, DocumentStyleSheetCollection* sheetCollection)
{
    InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent();
    if (cssAgent) {
        cssAgent->willMatchRule(rule, inspectorCSSOMWrappers, sheetCollection);
        return InspectorInstrumentationCookie(instrumentingAgents, 1);
    }

    return InspectorInstrumentationCookie();
}

void InspectorInstrumentation::didMatchRuleImpl(const InspectorInstrumentationCookie& cookie, bool matched)
{
    InspectorCSSAgent* cssAgent = cookie.instrumentingAgents()->inspectorCSSAgent();
    if (cssAgent)
        cssAgent->didMatchRule(matched);
}

InspectorInstrumentationCookie InspectorInstrumentation::willProcessRuleImpl(InstrumentingAgents* instrumentingAgents, StyleRule* rule, StyleResolver* styleResolver)
{
    InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent();
    if (cssAgent) {
        cssAgent->willProcessRule(rule, styleResolver);
        return InspectorInstrumentationCookie(instrumentingAgents, 1);
    }

    return InspectorInstrumentationCookie();
}

void InspectorInstrumentation::didProcessRuleImpl(const InspectorInstrumentationCookie& cookie)
{
    InspectorCSSAgent* cssAgent = cookie.instrumentingAgents()->inspectorCSSAgent();
    if (cssAgent)
        cssAgent->didProcessRule();
}

void InspectorInstrumentation::applyUserAgentOverrideImpl(InstrumentingAgents* instrumentingAgents, String* userAgent)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->applyUserAgentOverride(userAgent);
}

void InspectorInstrumentation::applyScreenWidthOverrideImpl(InstrumentingAgents* instrumentingAgents, long* width)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->applyScreenWidthOverride(width);
}

void InspectorInstrumentation::applyScreenHeightOverrideImpl(InstrumentingAgents* instrumentingAgents, long* height)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->applyScreenHeightOverride(height);
}

bool InspectorInstrumentation::shouldApplyScreenWidthOverrideImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent()) {
        long width = 0;
        pageAgent->applyScreenWidthOverride(&width);
        return !!width;
    }
    return false;
}

bool InspectorInstrumentation::shouldApplyScreenHeightOverrideImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent()) {
        long height = 0;
        pageAgent->applyScreenHeightOverride(&height);
        return !!height;
    }
    return false;
}

void InspectorInstrumentation::applyEmulatedMediaImpl(InstrumentingAgents* instrumentingAgents, String* media)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->applyEmulatedMedia(media);
}

void InspectorInstrumentation::willSendRequestImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willSendResourceRequest(identifier, request, loader->frame());
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willSendRequest(identifier, loader, request, redirectResponse);
}

void InspectorInstrumentation::continueAfterPingLoaderImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& response)
{
    willSendRequestImpl(instrumentingAgents, identifier, loader, request, response);
}

void InspectorInstrumentation::markResourceAsCachedImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->markResourceAsCached(identifier);
}

void InspectorInstrumentation::didLoadResourceFromMemoryCacheImpl(InstrumentingAgents* instrumentingAgents, DocumentLoader* loader, CachedResource* cachedResource)
{
    InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent();
    if (!inspectorAgent || !inspectorAgent->developerExtrasEnabled())
        return;
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didLoadResourceFromMemoryCache(loader, cachedResource);
}

InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceDataImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, Frame* frame, int length)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willReceiveResourceData(identifier, frame, length);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didReceiveResourceDataImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didReceiveResourceData();
}

InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const ResourceResponse& response, Frame* frame)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent) {
        timelineAgent->willReceiveResourceResponse(identifier, response, frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didReceiveResourceResponseImpl(const InspectorInstrumentationCookie& cookie, unsigned long identifier, DocumentLoader* loader, const ResourceResponse& response, ResourceLoader* resourceLoader)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didReceiveResourceResponse();
    if (!loader)
        return;
    InstrumentingAgents* instrumentingAgents = cookie.instrumentingAgents();
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveResponse(identifier, loader, response, resourceLoader);
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->didReceiveResponse(identifier, response); // This should come AFTER resource notification, front-end relies on this.
}

void InspectorInstrumentation::didReceiveResourceResponseButCanceledImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceResponse(frame, identifier, r);
    InspectorInstrumentation::didReceiveResourceResponse(cookie, identifier, loader, r, 0);
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

void InspectorInstrumentation::didReceiveDataImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const char* data, int dataLength, int encodedDataLength)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveData(identifier, data, dataLength, encodedDataLength);
}

void InspectorInstrumentation::didFinishLoadingImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, double monotonicFinishTime)
{
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent();
    if (!timelineAgent && !resourceAgent)
        return;

    double finishTime = 0.0;
    // FIXME: Expose all of the timing details to inspector and have it calculate finishTime.
    if (monotonicFinishTime)
        finishTime = loader->timing()->monotonicTimeToPseudoWallTime(monotonicFinishTime);

    if (timelineAgent)
        timelineAgent->didFinishLoadingResource(identifier, false, finishTime, loader->frame());
    if (resourceAgent)
        resourceAgent->didFinishLoading(identifier, loader, finishTime);
}

void InspectorInstrumentation::didFailLoadingImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, const ResourceError& error)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didFinishLoadingResource(identifier, true, 0, loader->frame());
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFailLoading(identifier, loader, error);
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->didFailLoading(identifier, error); // This should come AFTER resource notification, front-end relies on this.
}

void InspectorInstrumentation::documentThreadableLoaderStartedLoadingForClientImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, ThreadableLoaderClient* client)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->documentThreadableLoaderStartedLoadingForClient(identifier, client);
}

void InspectorInstrumentation::willLoadXHRImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client, const String& method, const KURL& url, bool async, PassRefPtr<FormData> formData, const HTTPHeaderMap& headers, bool includeCredentials)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willLoadXHR(client, method, url, async, formData, headers, includeCredentials);
}

void InspectorInstrumentation::didFailXHRLoadingImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFailXHRLoading(client);
}

void InspectorInstrumentation::didFinishXHRLoadingImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->didFinishXHRLoading(identifier, url, sendURL, sendLineNumber);
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFinishXHRLoading(client, identifier, sourceString);
}

void InspectorInstrumentation::didReceiveXHRResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveXHRResponse(identifier);
}

void InspectorInstrumentation::willLoadXHRSynchronouslyImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willLoadXHRSynchronously();
}

void InspectorInstrumentation::didLoadXHRSynchronouslyImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didLoadXHRSynchronously();
}

void InspectorInstrumentation::scriptImportedImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const String& sourceString)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->setInitialScriptContent(identifier, sourceString);
}

void InspectorInstrumentation::scriptExecutionBlockedByCSPImpl(InstrumentingAgents* instrumentingAgents, const String& directiveText)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->scriptExecutionBlockedByCSP(directiveText);
#endif
}

void InspectorInstrumentation::didReceiveScriptResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveScriptResponse(identifier);
}

void InspectorInstrumentation::domContentLoadedEventFiredImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didMarkDOMContentEvent(frame);

    if (frame->page()->mainFrame() != frame)
        return;

    if (InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent())
        inspectorAgent->domContentLoadedEventFired();

    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->mainFrameDOMContentLoaded();

    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->domContentEventFired();
}

void InspectorInstrumentation::loadEventFiredImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->loadEventFired(frame->document());

    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didMarkLoadEvent(frame);

    if (frame->page()->mainFrame() != frame)
        return;

    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->loadEventFired();
}

void InspectorInstrumentation::frameDetachedFromParentImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorCanvasAgent* canvasAgent = instrumentingAgents->inspectorCanvasAgent())
        canvasAgent->frameDetached(frame);
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameDetached(frame);
}

void InspectorInstrumentation::didCommitLoadImpl(InstrumentingAgents* instrumentingAgents, Page* page, DocumentLoader* loader)
{
    InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent();
    if (!inspectorAgent || !inspectorAgent->developerExtrasEnabled())
        return;

    Frame* mainFrame = page->mainFrame();
    if (loader->frame() == mainFrame) {
        if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
            consoleAgent->reset();

        if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
            resourceAgent->mainFrameNavigated(loader);
#if ENABLE(JAVASCRIPT_DEBUGGER)
        if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent())
            profilerAgent->resetState();
        if (InspectorHeapProfilerAgent* heapProfilerAgent = instrumentingAgents->inspectorHeapProfilerAgent())
            heapProfilerAgent->resetState();
#endif
        if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
            cssAgent->reset();
#if ENABLE(SQL_DATABASE)
        if (InspectorDatabaseAgent* databaseAgent = instrumentingAgents->inspectorDatabaseAgent())
            databaseAgent->clearResources();
#endif
        if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
            domAgent->setDocument(mainFrame->document());
#if USE(ACCELERATED_COMPOSITING)
        if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
            layerTreeAgent->reset();
#endif
        inspectorAgent->didCommitLoad();
    }
    if (InspectorCanvasAgent* canvasAgent = instrumentingAgents->inspectorCanvasAgent())
        canvasAgent->frameNavigated(loader->frame());
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameNavigated(loader);
}

void InspectorInstrumentation::frameDocumentUpdatedImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent();
    if (!inspectorAgent || !inspectorAgent->developerExtrasEnabled())
        return;

    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->frameDocumentUpdated(frame);
}

void InspectorInstrumentation::loaderDetachedFromFrameImpl(InstrumentingAgents* instrumentingAgents, DocumentLoader* loader)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents->inspectorPageAgent())
        inspectorPageAgent->loaderDetachedFromFrame(loader);
}

void InspectorInstrumentation::frameStartedLoadingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents->inspectorPageAgent())
        inspectorPageAgent->frameStartedLoading(frame);
}

void InspectorInstrumentation::frameStoppedLoadingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents->inspectorPageAgent())
        inspectorPageAgent->frameStoppedLoading(frame);
}

void InspectorInstrumentation::frameScheduledNavigationImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, double delay)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents->inspectorPageAgent())
        inspectorPageAgent->frameScheduledNavigation(frame, delay);
}

void InspectorInstrumentation::frameClearedScheduledNavigationImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents->inspectorPageAgent())
        inspectorPageAgent->frameClearedScheduledNavigation(frame);
}

InspectorInstrumentationCookie InspectorInstrumentation::willRunJavaScriptDialogImpl(InstrumentingAgents* instrumentingAgents, const String& message)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents->inspectorPageAgent())
        inspectorPageAgent->willRunJavaScriptDialog(message);
    return InspectorInstrumentationCookie(instrumentingAgents, 0);
}

void InspectorInstrumentation::didRunJavaScriptDialogImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorPageAgent* inspectorPageAgent = cookie.instrumentingAgents()->inspectorPageAgent())
        inspectorPageAgent->didRunJavaScriptDialog();
}

void InspectorInstrumentation::willDestroyCachedResourceImpl(CachedResource* cachedResource)
{
    if (!instrumentingAgentsSet)
        return;
    HashSet<InstrumentingAgents*>::iterator end = instrumentingAgentsSet->end();
    for (HashSet<InstrumentingAgents*>::iterator it = instrumentingAgentsSet->begin(); it != end; ++it) {
        InstrumentingAgents* instrumentingAgents = *it;
        if (InspectorResourceAgent* inspectorResourceAgent = instrumentingAgents->inspectorResourceAgent())
            inspectorResourceAgent->willDestroyCachedResource(cachedResource);
    }
}

InspectorInstrumentationCookie InspectorInstrumentation::willWriteHTMLImpl(InstrumentingAgents* instrumentingAgents, unsigned startLine, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willWriteHTML(startLine, frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didWriteHTMLImpl(const InspectorInstrumentationCookie& cookie, unsigned endLine)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didWriteHTML(endLine);
}

// FIXME: Drop this once we no longer generate stacks outside of Inspector.
void InspectorInstrumentation::addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, ScriptCallStack* callStack, unsigned long requestIdentifier)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, callStack, requestIdentifier);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->addMessageToConsole(source, type);
#endif
}

void InspectorInstrumentation::addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, ScriptState* state, ScriptArguments* arguments, unsigned long requestIdentifier)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, state, arguments, requestIdentifier);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->addMessageToConsole(source, type);
#endif
}

void InspectorInstrumentation::addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, const String& scriptId, unsigned lineNumber, unsigned columnNumber, ScriptState* state, unsigned long requestIdentifier)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, scriptId, lineNumber, columnNumber, state, requestIdentifier);
}

void InspectorInstrumentation::consoleCountImpl(InstrumentingAgents* instrumentingAgents, ScriptState* state, PassRefPtr<ScriptArguments> arguments)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->count(state, arguments);
}

void InspectorInstrumentation::startConsoleTimingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, const String& title)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->time(frame, title);
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->startTiming(title);
}

void InspectorInstrumentation::stopConsoleTimingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, const String& title, PassRefPtr<ScriptCallStack> stack)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->stopTiming(title, stack);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->timeEnd(frame, title);
}

void InspectorInstrumentation::consoleTimeStampImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, PassRefPtr<ScriptArguments> arguments)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        String message;
        arguments->getFirstArgumentAsString(message);
        timelineAgent->didTimeStamp(frame, message);
     }
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
void InspectorInstrumentation::addStartProfilingMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, const String& title, unsigned lineNumber, unsigned columnNumber, const String& sourceURL)
{
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent())
        profilerAgent->addStartProfilingMessageToConsole(title, lineNumber, columnNumber, sourceURL);
}

void InspectorInstrumentation::addProfileImpl(InstrumentingAgents* instrumentingAgents, RefPtr<ScriptProfile> profile, PassRefPtr<ScriptCallStack> callStack)
{
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent()) {
        const ScriptCallFrame& lastCaller = callStack->at(0);
        profilerAgent->addProfile(profile, lastCaller.lineNumber(), lastCaller.columnNumber(), lastCaller.sourceURL());
    }
}

String InspectorInstrumentation::getCurrentUserInitiatedProfileNameImpl(InstrumentingAgents* instrumentingAgents, bool incrementProfileNumber)
{
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent())
        return profilerAgent->getCurrentUserInitiatedProfileName(incrementProfileNumber);
    return "";
}

bool InspectorInstrumentation::profilerEnabledImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent())
        return profilerAgent->enabled();
    return false;
}
#endif

#if ENABLE(SQL_DATABASE)
void InspectorInstrumentation::didOpenDatabaseImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<Database> database, const String& domain, const String& name, const String& version)
{
    InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent();
    if (!inspectorAgent || !inspectorAgent->developerExtrasEnabled())
        return;
    if (InspectorDatabaseAgent* dbAgent = instrumentingAgents->inspectorDatabaseAgent())
        dbAgent->didOpenDatabase(database, domain, name, version);
}
#endif

void InspectorInstrumentation::didDispatchDOMStorageEventImpl(InstrumentingAgents* instrumentingAgents, const String& key, const String& oldValue, const String& newValue, StorageType storageType, SecurityOrigin* securityOrigin, Page* page)
{
    if (InspectorDOMStorageAgent* domStorageAgent = instrumentingAgents->inspectorDOMStorageAgent())
        domStorageAgent->didDispatchDOMStorageEvent(key, oldValue, newValue, storageType, securityOrigin, page);
}

#if ENABLE(WORKERS)
bool InspectorInstrumentation::shouldPauseDedicatedWorkerOnStartImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        return workerAgent->shouldPauseDedicatedWorkerOnStart();
    return false;
}

void InspectorInstrumentation::didStartWorkerGlobalScopeImpl(InstrumentingAgents* instrumentingAgents, WorkerGlobalScopeProxy* workerGlobalScopeProxy, const KURL& url)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        workerAgent->didStartWorkerGlobalScope(workerGlobalScopeProxy, url);
}

void InspectorInstrumentation::willEvaluateWorkerScript(WorkerGlobalScope* workerGlobalScope, int workerThreadStartMode)
{
    if (workerThreadStartMode != PauseWorkerGlobalScopeOnStart)
        return;
    InstrumentingAgents* instrumentingAgents = instrumentationForWorkerGlobalScope(workerGlobalScope);
    if (!instrumentingAgents)
        return;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (WorkerRuntimeAgent* runtimeAgent = instrumentingAgents->workerRuntimeAgent())
        runtimeAgent->pauseWorkerGlobalScope(workerGlobalScope);
#endif
}

void InspectorInstrumentation::workerGlobalScopeTerminatedImpl(InstrumentingAgents* instrumentingAgents, WorkerGlobalScopeProxy* proxy)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        workerAgent->workerGlobalScopeTerminated(proxy);
}
#endif

#if ENABLE(WEB_SOCKETS)
void InspectorInstrumentation::didCreateWebSocketImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const KURL& requestURL, const KURL&, const String& protocol, Document* document)
{
    InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent();
    if (!inspectorAgent || !inspectorAgent->developerExtrasEnabled())
        return;
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didCreateWebSocket(identifier, requestURL);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didCreateWebSocket(identifier, requestURL, protocol, document->frame());
}

void InspectorInstrumentation::willSendWebSocketHandshakeRequestImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const ResourceRequest& request, Document* document)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willSendWebSocketHandshakeRequest(identifier, request);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willSendWebSocketHandshakeRequest(identifier, document->frame());
}

void InspectorInstrumentation::didReceiveWebSocketHandshakeResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const ResourceResponse& response, Document* document)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketHandshakeResponse(identifier, response);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didReceiveWebSocketHandshakeResponse(identifier, document->frame());
}

void InspectorInstrumentation::didCloseWebSocketImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, Document* document)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didCloseWebSocket(identifier);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didDestroyWebSocket(identifier, document->frame());
}

void InspectorInstrumentation::didReceiveWebSocketFrameImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const WebSocketFrame& frame)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketFrame(identifier, frame);
}
void InspectorInstrumentation::didReceiveWebSocketFrameErrorImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const String& errorMessage)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketFrameError(identifier, errorMessage);
}
void InspectorInstrumentation::didSendWebSocketFrameImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const WebSocketFrame& frame)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didSendWebSocketFrame(identifier, frame);
}
#endif

void InspectorInstrumentation::networkStateChangedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorApplicationCacheAgent* applicationCacheAgent = instrumentingAgents->inspectorApplicationCacheAgent())
        applicationCacheAgent->networkStateChanged();
}

void InspectorInstrumentation::updateApplicationCacheStatusImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorApplicationCacheAgent* applicationCacheAgent = instrumentingAgents->inspectorApplicationCacheAgent())
        applicationCacheAgent->updateApplicationCacheStatus(frame);
}

bool InspectorInstrumentation::collectingHTMLParseErrors(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent())
        return inspectorAgent->hasFrontend();
    return false;
}

bool InspectorInstrumentation::canvasAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(scriptExecutionContext);
    return instrumentingAgents && instrumentingAgents->inspectorCanvasAgent();
}

bool InspectorInstrumentation::consoleAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(scriptExecutionContext);
    InspectorConsoleAgent* consoleAgent = instrumentingAgents ? instrumentingAgents->inspectorConsoleAgent() : 0;
    return consoleAgent && consoleAgent->enabled();
}

bool InspectorInstrumentation::timelineAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(scriptExecutionContext);
    return instrumentingAgents && instrumentingAgents->inspectorTimelineAgent();
}

void InspectorInstrumentation::pauseOnNativeEventIfNeeded(InstrumentingAgents* instrumentingAgents, bool isDOMEvent, const String& eventName, bool synchronous)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->pauseOnNativeEventIfNeeded(isDOMEvent, eventName, synchronous);
#endif
}

void InspectorInstrumentation::cancelPauseOnNativeEvent(InstrumentingAgents* instrumentingAgents)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->cancelPauseOnNextStatement();
#endif
}

void InspectorInstrumentation::didRequestAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, int callbackId, Frame* frame)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, requestAnimationFrameEventName, true);

    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didRequestAnimationFrame(callbackId, frame);
}

void InspectorInstrumentation::didCancelAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, int callbackId, Frame* frame)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, cancelAnimationFrameEventName, true);

    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didCancelAnimationFrame(callbackId, frame);
}

InspectorInstrumentationCookie InspectorInstrumentation::willFireAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, int callbackId, Frame* frame)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, animationFrameFiredEventName, false);

    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willFireAnimationFrame(callbackId, frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didFireAnimationFrameImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didFireAnimationFrame();
}

void InspectorInstrumentation::registerInstrumentingAgents(InstrumentingAgents* instrumentingAgents)
{
    if (!instrumentingAgentsSet)
        instrumentingAgentsSet = new HashSet<InstrumentingAgents*>();
    instrumentingAgentsSet->add(instrumentingAgents);
}

void InspectorInstrumentation::unregisterInstrumentingAgents(InstrumentingAgents* instrumentingAgents)
{
    if (!instrumentingAgentsSet)
        return;
    instrumentingAgentsSet->remove(instrumentingAgents);
    if (instrumentingAgentsSet->isEmpty()) {
        delete instrumentingAgentsSet;
        instrumentingAgentsSet = 0;
    }
}

InspectorTimelineAgent* InspectorInstrumentation::retrieveTimelineAgent(const InspectorInstrumentationCookie& cookie)
{
    if (!cookie.instrumentingAgents())
        return 0;
    InspectorTimelineAgent* timelineAgent = cookie.instrumentingAgents()->inspectorTimelineAgent();
    if (timelineAgent && cookie.hasMatchingTimelineAgentId(timelineAgent->id()))
        return timelineAgent;
    return 0;
}

InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForPage(Page* page)
{
    if (!page)
        return 0;
    return instrumentationForPage(page);
}

InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForRenderer(RenderObject* renderer)
{
    return instrumentingAgentsForFrame(renderer->frame());
}

#if ENABLE(WORKERS)
InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForWorkerGlobalScope(WorkerGlobalScope* workerGlobalScope)
{
    if (!workerGlobalScope)
        return 0;
    return instrumentationForWorkerGlobalScope(workerGlobalScope);
}

InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForNonDocumentContext(ScriptExecutionContext* context)
{
    if (context->isWorkerGlobalScope())
        return instrumentationForWorkerGlobalScope(static_cast<WorkerGlobalScope*>(context));
    return 0;
}
#endif

#if ENABLE(GEOLOCATION)
GeolocationPosition* InspectorInstrumentation::overrideGeolocationPositionImpl(InstrumentingAgents* instrumentingAgents, GeolocationPosition* position)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        position = pageAgent->overrideGeolocationPosition(position);
    return position;
}
#endif

DeviceOrientationData* InspectorInstrumentation::overrideDeviceOrientationImpl(InstrumentingAgents* instrumentingAgents, DeviceOrientationData* deviceOrientation)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        deviceOrientation = pageAgent->overrideDeviceOrientation(deviceOrientation);
    return deviceOrientation;
}

#if USE(ACCELERATED_COMPOSITING)
void InspectorInstrumentation::layerTreeDidChangeImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->layerTreeDidChange();
}

void InspectorInstrumentation::renderLayerDestroyedImpl(InstrumentingAgents* instrumentingAgents, const RenderLayer* renderLayer)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->renderLayerDestroyed(renderLayer);
}

void InspectorInstrumentation::pseudoElementDestroyedImpl(InstrumentingAgents* instrumentingAgents, PseudoElement* pseudoElement)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->pseudoElementDestroyed(pseudoElement);
}
#endif

namespace InstrumentationEvents {
const char PaintLayer[] = "PaintLayer";
const char RasterTask[] = "RasterTask";
const char Paint[] = "Paint";
const char Layer[] = "Layer";
const char BeginFrame[] = "BeginFrame";
};

namespace InstrumentationEventArguments {
const char LayerId[] = "layerId";
const char PageId[] = "pageId";
};

} // namespace WebCore

#endif // !ENABLE(INSPECTOR)
