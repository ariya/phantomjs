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

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)

#include "PageDebuggerAgent.h"

#include "InspectorOverlay.h"
#include "InspectorPageAgent.h"
#include "InstrumentingAgents.h"
#include "Page.h"
#include "PageConsole.h"
#include "PageScriptDebugServer.h"

namespace WebCore {

PassOwnPtr<PageDebuggerAgent> PageDebuggerAgent::create(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* inspectorState, InspectorPageAgent* pageAgent, InjectedScriptManager* injectedScriptManager, InspectorOverlay* overlay)
{
    return adoptPtr(new PageDebuggerAgent(instrumentingAgents, inspectorState, pageAgent, injectedScriptManager, overlay));
}

PageDebuggerAgent::PageDebuggerAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* inspectorState, InspectorPageAgent* pageAgent, InjectedScriptManager* injectedScriptManager, InspectorOverlay* overlay)
    : InspectorDebuggerAgent(instrumentingAgents, inspectorState, injectedScriptManager)
    , m_pageAgent(pageAgent)
    , m_overlay(overlay)
{
}

PageDebuggerAgent::~PageDebuggerAgent()
{
}

void PageDebuggerAgent::enable()
{
    InspectorDebuggerAgent::enable();
    m_instrumentingAgents->setPageDebuggerAgent(this);
}

void PageDebuggerAgent::disable()
{
    InspectorDebuggerAgent::disable();
    m_instrumentingAgents->setPageDebuggerAgent(0);
}

void PageDebuggerAgent::startListeningScriptDebugServer()
{
    scriptDebugServer().addListener(this, m_pageAgent->page());
}

void PageDebuggerAgent::stopListeningScriptDebugServer()
{
    scriptDebugServer().removeListener(this, m_pageAgent->page());
}

PageScriptDebugServer& PageDebuggerAgent::scriptDebugServer()
{
    return PageScriptDebugServer::shared();
}

void PageDebuggerAgent::muteConsole()
{
    PageConsole::mute();
}

void PageDebuggerAgent::unmuteConsole()
{
    PageConsole::unmute();
}

InjectedScript PageDebuggerAgent::injectedScriptForEval(ErrorString* errorString, const int* executionContextId)
{
    if (!executionContextId) {
        ScriptState* scriptState = mainWorldScriptState(m_pageAgent->mainFrame());
        return injectedScriptManager()->injectedScriptFor(scriptState);
    }
    InjectedScript injectedScript = injectedScriptManager()->injectedScriptForId(*executionContextId);
    if (injectedScript.hasNoValue())
        *errorString = "Execution context with given id not found.";
    return injectedScript;
}

void PageDebuggerAgent::setOverlayMessage(ErrorString*, const String* message)
{
    m_overlay->setPausedInDebuggerMessage(message);
}

void PageDebuggerAgent::didClearMainFrameWindowObject()
{
    reset();
    scriptDebugServer().setScriptPreprocessor(m_pageAgent->scriptPreprocessor());
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
