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

#include "InspectorDOMDebuggerAgent.h"

#if ENABLE(INSPECTOR) && ENABLE(JAVASCRIPT_DEBUGGER)

#include "HTMLElement.h"
#include "InspectorAgent.h"
#include "InspectorDOMAgent.h"
#include "InspectorDebuggerAgent.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include <wtf/text/StringConcatenate.h>

namespace {

enum DOMBreakpointType {
    SubtreeModified = 0,
    AttributeModified,
    NodeRemoved,
    DOMBreakpointTypesCount
};

static const char* const domNativeBreakpointType = "DOM";
static const char* const eventListenerNativeBreakpointType = "EventListener";
static const char* const xhrNativeBreakpointType = "XHR";

const uint32_t inheritableDOMBreakpointTypesMask = (1 << SubtreeModified);
const int domBreakpointDerivedTypeShift = 16;

}

namespace WebCore {

namespace DOMDebuggerAgentState {
static const char eventListenerBreakpoints[] = "eventListenerBreakpoints";
static const char pauseOnAllXHRs[] = "pauseOnAllXHRs";
static const char xhrBreakpoints[] = "xhrBreakpoints";
}

PassOwnPtr<InspectorDOMDebuggerAgent> InspectorDOMDebuggerAgent::create(InstrumentingAgents* instrumentingAgents, InspectorState* inspectorState, InspectorDOMAgent* domAgent, InspectorDebuggerAgent* debuggerAgent, InspectorAgent* inspectorAgent)
{
    return adoptPtr(new InspectorDOMDebuggerAgent(instrumentingAgents, inspectorState, domAgent, debuggerAgent, inspectorAgent));
}

InspectorDOMDebuggerAgent::InspectorDOMDebuggerAgent(InstrumentingAgents* instrumentingAgents, InspectorState* inspectorState, InspectorDOMAgent* domAgent, InspectorDebuggerAgent* debuggerAgent, InspectorAgent* inspectorAgent)
    : m_instrumentingAgents(instrumentingAgents)
    , m_inspectorState(inspectorState)
    , m_domAgent(domAgent)
    , m_debuggerAgent(debuggerAgent)
    , m_inspectorAgent(inspectorAgent)
{
    m_debuggerAgent->setListener(this);
}

InspectorDOMDebuggerAgent::~InspectorDOMDebuggerAgent()
{
    m_debuggerAgent->setListener(0);
    ASSERT(!m_instrumentingAgents->inspectorDOMDebuggerAgent());
}

// Browser debugger agent enabled only when JS debugger is enabled.
void InspectorDOMDebuggerAgent::debuggerWasEnabled()
{
    m_instrumentingAgents->setInspectorDOMDebuggerAgent(this);
}

void InspectorDOMDebuggerAgent::debuggerWasDisabled()
{
    disable();
}

void InspectorDOMDebuggerAgent::disable()
{
    m_instrumentingAgents->setInspectorDOMDebuggerAgent(0);
    clear();
}

void InspectorDOMDebuggerAgent::clearFrontend()
{
    disable();
}

void InspectorDOMDebuggerAgent::discardBindings()
{
    m_domBreakpoints.clear();
}

void InspectorDOMDebuggerAgent::setEventListenerBreakpoint(ErrorString* error, const String& eventName)
{
    if (eventName.isEmpty()) {
        *error = "Event name is empty";
        return;
    }

    RefPtr<InspectorObject> eventListenerBreakpoints = m_inspectorState->getObject(DOMDebuggerAgentState::eventListenerBreakpoints);
    eventListenerBreakpoints->setBoolean(eventName, true);
    m_inspectorState->setObject(DOMDebuggerAgentState::eventListenerBreakpoints, eventListenerBreakpoints);
}

void InspectorDOMDebuggerAgent::removeEventListenerBreakpoint(ErrorString* error, const String& eventName)
{
    if (eventName.isEmpty()) {
        *error = "Event name is empty";
        return;
    }

    RefPtr<InspectorObject> eventListenerBreakpoints = m_inspectorState->getObject(DOMDebuggerAgentState::eventListenerBreakpoints);
    eventListenerBreakpoints->remove(eventName);
    m_inspectorState->setObject(DOMDebuggerAgentState::eventListenerBreakpoints, eventListenerBreakpoints);
}

void InspectorDOMDebuggerAgent::didInsertDOMNode(Node* node)
{
    if (m_domBreakpoints.size()) {
        uint32_t mask = m_domBreakpoints.get(InspectorDOMAgent::innerParentNode(node));
        uint32_t inheritableTypesMask = (mask | (mask >> domBreakpointDerivedTypeShift)) & inheritableDOMBreakpointTypesMask;
        if (inheritableTypesMask)
            updateSubtreeBreakpoints(node, inheritableTypesMask, true);
    }
}

void InspectorDOMDebuggerAgent::didRemoveDOMNode(Node* node)
{
    if (m_domBreakpoints.size()) {
        // Remove subtree breakpoints.
        m_domBreakpoints.remove(node);
        Vector<Node*> stack(1, InspectorDOMAgent::innerFirstChild(node));
        do {
            Node* node = stack.last();
            stack.removeLast();
            if (!node)
                continue;
            m_domBreakpoints.remove(node);
            stack.append(InspectorDOMAgent::innerFirstChild(node));
            stack.append(InspectorDOMAgent::innerNextSibling(node));
        } while (!stack.isEmpty());
    }
}

void InspectorDOMDebuggerAgent::setDOMBreakpoint(ErrorString*, int nodeId, int type)
{
    Node* node = m_domAgent->nodeForId(nodeId);
    if (!node)
        return;

    uint32_t rootBit = 1 << type;
    m_domBreakpoints.set(node, m_domBreakpoints.get(node) | rootBit);
    if (rootBit & inheritableDOMBreakpointTypesMask) {
        for (Node* child = InspectorDOMAgent::innerFirstChild(node); child; child = InspectorDOMAgent::innerNextSibling(child))
            updateSubtreeBreakpoints(child, rootBit, true);
    }
}

void InspectorDOMDebuggerAgent::removeDOMBreakpoint(ErrorString*, int nodeId, int type)
{
    Node* node = m_domAgent->nodeForId(nodeId);
    if (!node)
        return;

    uint32_t rootBit = 1 << type;
    uint32_t mask = m_domBreakpoints.get(node) & ~rootBit;
    if (mask)
        m_domBreakpoints.set(node, mask);
    else
        m_domBreakpoints.remove(node);

    if ((rootBit & inheritableDOMBreakpointTypesMask) && !(mask & (rootBit << domBreakpointDerivedTypeShift))) {
        for (Node* child = InspectorDOMAgent::innerFirstChild(node); child; child = InspectorDOMAgent::innerNextSibling(child))
            updateSubtreeBreakpoints(child, rootBit, false);
    }
}

void InspectorDOMDebuggerAgent::willInsertDOMNode(Node*, Node* parent)
{
    InspectorDebuggerAgent* debuggerAgent = m_debuggerAgent;
    if (!debuggerAgent)
        return;

    if (hasBreakpoint(parent, SubtreeModified)) {
        RefPtr<InspectorObject> eventData = InspectorObject::create();
        descriptionForDOMEvent(parent, SubtreeModified, true, eventData.get());
        eventData->setString("breakpointType", domNativeBreakpointType);
        debuggerAgent->breakProgram(NativeBreakpointDebuggerEventType, eventData.release());
    }
}

void InspectorDOMDebuggerAgent::willRemoveDOMNode(Node* node)
{
    InspectorDebuggerAgent* debuggerAgent = m_debuggerAgent;
    if (!debuggerAgent)
        return;

    Node* parentNode = InspectorDOMAgent::innerParentNode(node);
    if (hasBreakpoint(node, NodeRemoved)) {
        RefPtr<InspectorObject> eventData = InspectorObject::create();
        descriptionForDOMEvent(node, NodeRemoved, false, eventData.get());
        eventData->setString("breakpointType", domNativeBreakpointType);
        debuggerAgent->breakProgram(NativeBreakpointDebuggerEventType, eventData.release());
    } else if (parentNode && hasBreakpoint(parentNode, SubtreeModified)) {
        RefPtr<InspectorObject> eventData = InspectorObject::create();
        descriptionForDOMEvent(node, SubtreeModified, false, eventData.get());
        eventData->setString("breakpointType", domNativeBreakpointType);
        debuggerAgent->breakProgram(NativeBreakpointDebuggerEventType, eventData.release());
    }
}

void InspectorDOMDebuggerAgent::willModifyDOMAttr(Element* element)
{
    InspectorDebuggerAgent* debuggerAgent = m_debuggerAgent;
    if (!debuggerAgent)
        return;

    if (hasBreakpoint(element, AttributeModified)) {
        RefPtr<InspectorObject> eventData = InspectorObject::create();
        descriptionForDOMEvent(element, AttributeModified, false, eventData.get());
        eventData->setString("breakpointType", domNativeBreakpointType);
        debuggerAgent->breakProgram(NativeBreakpointDebuggerEventType, eventData.release());
    }
}

void InspectorDOMDebuggerAgent::descriptionForDOMEvent(Node* target, int breakpointType, bool insertion, InspectorObject* description)
{
    ASSERT(hasBreakpoint(target, breakpointType));

    Node* breakpointOwner = target;
    if ((1 << breakpointType) & inheritableDOMBreakpointTypesMask) {
        // For inheritable breakpoint types, target node isn't always the same as the node that owns a breakpoint.
        // Target node may be unknown to frontend, so we need to push it first.
        RefPtr<InspectorObject> targetNodeObject = m_domAgent->resolveNode(target);
        description->setObject("targetNode", targetNodeObject);

        // Find breakpoint owner node.
        if (!insertion)
            breakpointOwner = InspectorDOMAgent::innerParentNode(target);
        ASSERT(breakpointOwner);
        while (!(m_domBreakpoints.get(breakpointOwner) & (1 << breakpointType))) {
            breakpointOwner = InspectorDOMAgent::innerParentNode(breakpointOwner);
            ASSERT(breakpointOwner);
        }

        if (breakpointType == SubtreeModified)
            description->setBoolean("insertion", insertion);
    }

    int breakpointOwnerNodeId = m_domAgent->boundNodeId(breakpointOwner);
    ASSERT(breakpointOwnerNodeId);
    description->setNumber("nodeId", breakpointOwnerNodeId);
    description->setNumber("type", breakpointType);
}

bool InspectorDOMDebuggerAgent::hasBreakpoint(Node* node, int type)
{
    uint32_t rootBit = 1 << type;
    uint32_t derivedBit = rootBit << domBreakpointDerivedTypeShift;
    return m_domBreakpoints.get(node) & (rootBit | derivedBit);
}

void InspectorDOMDebuggerAgent::updateSubtreeBreakpoints(Node* node, uint32_t rootMask, bool set)
{
    uint32_t oldMask = m_domBreakpoints.get(node);
    uint32_t derivedMask = rootMask << domBreakpointDerivedTypeShift;
    uint32_t newMask = set ? oldMask | derivedMask : oldMask & ~derivedMask;
    if (newMask)
        m_domBreakpoints.set(node, newMask);
    else
        m_domBreakpoints.remove(node);

    uint32_t newRootMask = rootMask & ~newMask;
    if (!newRootMask)
        return;

    for (Node* child = InspectorDOMAgent::innerFirstChild(node); child; child = InspectorDOMAgent::innerNextSibling(child))
        updateSubtreeBreakpoints(child, newRootMask, set);
}

void InspectorDOMDebuggerAgent::pauseOnNativeEventIfNeeded(const String& categoryType, const String& eventName, bool synchronous)
{
    InspectorDebuggerAgent* debuggerAgent = m_debuggerAgent;
    if (!debuggerAgent)
        return;

    String fullEventName = makeString(categoryType, ":", eventName);
    RefPtr<InspectorObject> eventListenerBreakpoints = m_inspectorState->getObject(DOMDebuggerAgentState::eventListenerBreakpoints);
    if (eventListenerBreakpoints->find(fullEventName) == eventListenerBreakpoints->end())
        return;

    RefPtr<InspectorObject> eventData = InspectorObject::create();
    eventData->setString("breakpointType", eventListenerNativeBreakpointType);
    eventData->setString("eventName", fullEventName);
    if (synchronous)
        debuggerAgent->breakProgram(NativeBreakpointDebuggerEventType, eventData.release());
    else
        debuggerAgent->schedulePauseOnNextStatement(NativeBreakpointDebuggerEventType, eventData.release());
}

void InspectorDOMDebuggerAgent::setXHRBreakpoint(ErrorString*, const String& url)
{
    if (url.isEmpty()) {
        m_inspectorState->setBoolean(DOMDebuggerAgentState::pauseOnAllXHRs, true);
        return;
    }

    RefPtr<InspectorObject> xhrBreakpoints = m_inspectorState->getObject(DOMDebuggerAgentState::xhrBreakpoints);
    xhrBreakpoints->setBoolean(url, true);
    m_inspectorState->setObject(DOMDebuggerAgentState::xhrBreakpoints, xhrBreakpoints);
}

void InspectorDOMDebuggerAgent::removeXHRBreakpoint(ErrorString*, const String& url)
{
    if (url.isEmpty()) {
        m_inspectorState->setBoolean(DOMDebuggerAgentState::pauseOnAllXHRs, false);
        return;
    }

    RefPtr<InspectorObject> xhrBreakpoints = m_inspectorState->getObject(DOMDebuggerAgentState::xhrBreakpoints);
    xhrBreakpoints->remove(url);
    m_inspectorState->setObject(DOMDebuggerAgentState::xhrBreakpoints, xhrBreakpoints);
}

void InspectorDOMDebuggerAgent::willSendXMLHttpRequest(const String& url)
{
    InspectorDebuggerAgent* debuggerAgent = m_debuggerAgent;
    if (!debuggerAgent)
        return;

    String breakpointURL;
    if (m_inspectorState->getBoolean(DOMDebuggerAgentState::pauseOnAllXHRs))
        breakpointURL = "";
    else {
        RefPtr<InspectorObject> xhrBreakpoints = m_inspectorState->getObject(DOMDebuggerAgentState::xhrBreakpoints);
        for (InspectorObject::iterator it = xhrBreakpoints->begin(); it != xhrBreakpoints->end(); ++it) {
            if (url.contains(it->first)) {
                breakpointURL = it->first;
                break;
            }
        }
    }

    if (breakpointURL.isNull())
        return;

    RefPtr<InspectorObject> eventData = InspectorObject::create();
    eventData->setString("breakpointType", xhrNativeBreakpointType);
    eventData->setString("breakpointURL", breakpointURL);
    eventData->setString("url", url);
    debuggerAgent->breakProgram(NativeBreakpointDebuggerEventType, eventData.release());
}

void InspectorDOMDebuggerAgent::clear()
{
    m_domBreakpoints.clear();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR) && ENABLE(JAVASCRIPT_DEBUGGER)
