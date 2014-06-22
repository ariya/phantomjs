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

#include "PageConsoleAgent.h"

#include "DOMWindow.h"
#include "InjectedScriptHost.h"
#include "InjectedScriptManager.h"
#include "InspectorAgent.h"
#include "InspectorDOMAgent.h"
#include "Node.h"
#include "ScriptObject.h"

namespace WebCore {

PageConsoleAgent::PageConsoleAgent(InstrumentingAgents* instrumentingAgents, InspectorAgent* inspectorAgent, InspectorCompositeState* state, InjectedScriptManager* injectedScriptManager, InspectorDOMAgent* domAgent)
    : InspectorConsoleAgent(instrumentingAgents, state, injectedScriptManager)
    , m_inspectorAgent(inspectorAgent)
    , m_inspectorDOMAgent(domAgent)
{
}

PageConsoleAgent::~PageConsoleAgent()
{
    m_inspectorAgent = 0;
    m_inspectorDOMAgent = 0;
}

void PageConsoleAgent::clearMessages(ErrorString* errorString)
{
    m_inspectorDOMAgent->releaseDanglingNodes();
    InspectorConsoleAgent::clearMessages(errorString);
}

class InspectableNode : public InjectedScriptHost::InspectableObject {
public:
    explicit InspectableNode(Node* node) : m_node(node) { }
    virtual ScriptValue get(ScriptState* state)
    {
        return InjectedScriptHost::nodeAsScriptValue(state, m_node);
    }
private:
    Node* m_node;
};

void PageConsoleAgent::addInspectedNode(ErrorString* errorString, int nodeId)
{
    Node* node = m_inspectorDOMAgent->nodeForId(nodeId);
    if (!node || node->isInShadowTree()) {
        *errorString = "nodeId is not valid";
        return;
    }
    m_injectedScriptManager->injectedScriptHost()->addInspectedObject(adoptPtr(new InspectableNode(node)));
}

bool PageConsoleAgent::developerExtrasEnabled()
{
    return m_inspectorAgent->developerExtrasEnabled();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
