/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorAgent.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "InjectedScriptHost.h"
#include "InjectedScriptManager.h"
#include "InspectorController.h"
#include "InspectorFrontend.h"
#include "InspectorInstrumentation.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "Page.h"
#include "ResourceRequest.h"
#include "ScriptController.h"
#include "ScriptFunctionCall.h"
#include "ScriptObject.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

namespace InspectorAgentState {
static const char inspectorAgentEnabled[] = "inspectorAgentEnabled";
}

InspectorAgent::InspectorAgent(Page* page, InjectedScriptManager* injectedScriptManager, InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state)
    : InspectorBaseAgent<InspectorAgent>("Inspector", instrumentingAgents, state)
    , m_inspectedPage(page)
    , m_frontend(0)
    , m_injectedScriptManager(injectedScriptManager)
{
    ASSERT_ARG(page, page);
    m_instrumentingAgents->setInspectorAgent(this);
}

InspectorAgent::~InspectorAgent()
{
    m_instrumentingAgents->setInspectorAgent(0);
}

void InspectorAgent::didClearWindowObjectInWorld(Frame* frame, DOMWrapperWorld* world)
{
    if (world != mainThreadNormalWorld())
        return;

    if (m_injectedScriptForOrigin.isEmpty())
        return;

    String origin = frame->document()->securityOrigin()->toRawString();
    String script = m_injectedScriptForOrigin.get(origin);
    if (script.isEmpty())
        return;
    int injectedScriptId = m_injectedScriptManager->injectedScriptIdFor(mainWorldScriptState(frame));
    StringBuilder scriptSource;
    scriptSource.append(script);
    scriptSource.append("(");
    scriptSource.appendNumber(injectedScriptId);
    scriptSource.append(")");
    frame->script()->executeScript(scriptSource.toString());
}

void InspectorAgent::setFrontend(InspectorFrontend* inspectorFrontend)
{
    m_frontend = inspectorFrontend;
}

void InspectorAgent::clearFrontend()
{
    m_pendingEvaluateTestCommands.clear();
    m_frontend = 0;
    m_injectedScriptManager->discardInjectedScripts();
    ErrorString error;
    disable(&error);
}

void InspectorAgent::didCommitLoad()
{
    m_injectedScriptManager->discardInjectedScripts();
}

void InspectorAgent::enable(ErrorString*)
{
    m_state->setBoolean(InspectorAgentState::inspectorAgentEnabled, true);

    if (m_pendingInspectData.first)
        inspect(m_pendingInspectData.first, m_pendingInspectData.second);

    for (Vector<pair<long, String> >::iterator it = m_pendingEvaluateTestCommands.begin(); m_frontend && it != m_pendingEvaluateTestCommands.end(); ++it)
        m_frontend->inspector()->evaluateForTestInFrontend(static_cast<int>((*it).first), (*it).second);
    m_pendingEvaluateTestCommands.clear();
}

void InspectorAgent::disable(ErrorString*)
{
    m_state->setBoolean(InspectorAgentState::inspectorAgentEnabled, false);
}

void InspectorAgent::domContentLoadedEventFired()
{
    m_injectedScriptManager->injectedScriptHost()->clearInspectedObjects();
}

bool InspectorAgent::isMainResourceLoader(DocumentLoader* loader, const KURL& requestUrl)
{
    return loader->frame() == m_inspectedPage->mainFrame() && requestUrl == loader->requestURL();
}

void InspectorAgent::evaluateForTestInFrontend(long callId, const String& script)
{
    if (m_state->getBoolean(InspectorAgentState::inspectorAgentEnabled))
        m_frontend->inspector()->evaluateForTestInFrontend(static_cast<int>(callId), script);
    else
        m_pendingEvaluateTestCommands.append(pair<long, String>(callId, script));
}

void InspectorAgent::setInjectedScriptForOrigin(const String& origin, const String& source)
{
    m_injectedScriptForOrigin.set(origin, source);
}

void InspectorAgent::inspect(PassRefPtr<TypeBuilder::Runtime::RemoteObject> objectToInspect, PassRefPtr<InspectorObject> hints)
{
    if (m_state->getBoolean(InspectorAgentState::inspectorAgentEnabled) && m_frontend) {
        m_frontend->inspector()->inspect(objectToInspect, hints);
        m_pendingInspectData.first = 0;
        m_pendingInspectData.second = 0;
        return;
    }
    m_pendingInspectData.first = objectToInspect;
    m_pendingInspectData.second = hints;
}

KURL InspectorAgent::inspectedURL() const
{
    return m_inspectedPage->mainFrame()->document()->url();
}

KURL InspectorAgent::inspectedURLWithoutFragment() const
{
    KURL url = inspectedURL();
    url.removeFragmentIdentifier();
    return url;
}

bool InspectorAgent::developerExtrasEnabled() const
{
    if (!m_inspectedPage)
        return false;
    return m_inspectedPage->settings()->developerExtrasEnabled();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
