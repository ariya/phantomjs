/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "InspectorCanvasAgent.h"

#include "BindingVisitors.h"
#include "DOMWindow.h"
#include "Frame.h"
#include "HTMLCanvasElement.h"
#include "HTMLNames.h"
#include "InjectedScript.h"
#include "InjectedScriptCanvasModule.h"
#include "InjectedScriptManager.h"
#include "InspectorFrontend.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InstrumentingAgents.h"
#include "Page.h"
#include "ScriptObject.h"
#include "ScriptProfiler.h"
#include "ScriptState.h"

using WebCore::TypeBuilder::Array;
using WebCore::TypeBuilder::Canvas::ResourceId;
using WebCore::TypeBuilder::Canvas::ResourceInfo;
using WebCore::TypeBuilder::Canvas::ResourceState;
using WebCore::TypeBuilder::Canvas::TraceLog;
using WebCore::TypeBuilder::Canvas::TraceLogId;
using WebCore::TypeBuilder::Network::FrameId;

namespace WebCore {

namespace CanvasAgentState {
static const char canvasAgentEnabled[] = "canvasAgentEnabled";
};

InspectorCanvasAgent::InspectorCanvasAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state, InspectorPageAgent* pageAgent, InjectedScriptManager* injectedScriptManager)
    : InspectorBaseAgent<InspectorCanvasAgent>("Canvas", instrumentingAgents, state)
    , m_pageAgent(pageAgent)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
    , m_enabled(false)
{
}

InspectorCanvasAgent::~InspectorCanvasAgent()
{
}

void InspectorCanvasAgent::setFrontend(InspectorFrontend* frontend)
{
    ASSERT(frontend);
    m_frontend = frontend->canvas();
}

void InspectorCanvasAgent::clearFrontend()
{
    m_frontend = 0;
    disable(0);
}

void InspectorCanvasAgent::restore()
{
    if (m_state->getBoolean(CanvasAgentState::canvasAgentEnabled)) {
        ErrorString error;
        enable(&error);
    }
}

void InspectorCanvasAgent::enable(ErrorString*)
{
    if (m_enabled)
        return;
    m_enabled = true;
    m_state->setBoolean(CanvasAgentState::canvasAgentEnabled, m_enabled);
    m_instrumentingAgents->setInspectorCanvasAgent(this);
    findFramesWithUninstrumentedCanvases();
}

void InspectorCanvasAgent::disable(ErrorString*)
{
    m_enabled = false;
    m_state->setBoolean(CanvasAgentState::canvasAgentEnabled, m_enabled);
    m_instrumentingAgents->setInspectorCanvasAgent(0);
    m_framesWithUninstrumentedCanvases.clear();
}

void InspectorCanvasAgent::dropTraceLog(ErrorString* errorString, const TraceLogId& traceLogId)
{
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(errorString, traceLogId);
    if (!module.hasNoValue())
        module.dropTraceLog(errorString, traceLogId);
}

void InspectorCanvasAgent::hasUninstrumentedCanvases(ErrorString* errorString, bool* result)
{
    if (!checkIsEnabled(errorString))
        return;
    for (FramesWithUninstrumentedCanvases::iterator it = m_framesWithUninstrumentedCanvases.begin(); it != m_framesWithUninstrumentedCanvases.end(); ++it) {
        if (it->value) {
            *result = true;
            return;
        }
    }
    *result = false;
}

void InspectorCanvasAgent::captureFrame(ErrorString* errorString, const FrameId* frameId, TraceLogId* traceLogId)
{
    Frame* frame = frameId ? m_pageAgent->assertFrame(errorString, *frameId) : m_pageAgent->mainFrame();
    if (!frame)
        return;
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(errorString, mainWorldScriptState(frame));
    if (!module.hasNoValue())
        module.captureFrame(errorString, traceLogId);
}

void InspectorCanvasAgent::startCapturing(ErrorString* errorString, const FrameId* frameId, TraceLogId* traceLogId)
{
    Frame* frame = frameId ? m_pageAgent->assertFrame(errorString, *frameId) : m_pageAgent->mainFrame();
    if (!frame)
        return;
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(errorString, mainWorldScriptState(frame));
    if (!module.hasNoValue())
        module.startCapturing(errorString, traceLogId);
}

void InspectorCanvasAgent::stopCapturing(ErrorString* errorString, const TraceLogId& traceLogId)
{
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(errorString, traceLogId);
    if (!module.hasNoValue())
        module.stopCapturing(errorString, traceLogId);
}

void InspectorCanvasAgent::getTraceLog(ErrorString* errorString, const TraceLogId& traceLogId, const int* startOffset, const int* maxLength, RefPtr<TraceLog>& traceLog)
{
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(errorString, traceLogId);
    if (!module.hasNoValue())
        module.traceLog(errorString, traceLogId, startOffset, maxLength, &traceLog);
}

void InspectorCanvasAgent::replayTraceLog(ErrorString* errorString, const TraceLogId& traceLogId, int stepNo, RefPtr<ResourceState>& result)
{
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(errorString, traceLogId);
    if (!module.hasNoValue())
        module.replayTraceLog(errorString, traceLogId, stepNo, &result);
}

void InspectorCanvasAgent::getResourceInfo(ErrorString* errorString, const ResourceId& resourceId, RefPtr<ResourceInfo>& result)
{
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(errorString, resourceId);
    if (!module.hasNoValue())
        module.resourceInfo(errorString, resourceId, &result);
}

void InspectorCanvasAgent::getResourceState(ErrorString* errorString, const TraceLogId& traceLogId, const ResourceId& resourceId, RefPtr<ResourceState>& result)
{
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(errorString, traceLogId);
    if (!module.hasNoValue())
        module.resourceState(errorString, traceLogId, resourceId, &result);
}

ScriptObject InspectorCanvasAgent::wrapCanvas2DRenderingContextForInstrumentation(const ScriptObject& context)
{
    ErrorString error;
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(&error, context);
    if (module.hasNoValue())
        return ScriptObject();
    return notifyRenderingContextWasWrapped(module.wrapCanvas2DContext(context));
}

#if ENABLE(WEBGL)
ScriptObject InspectorCanvasAgent::wrapWebGLRenderingContextForInstrumentation(const ScriptObject& glContext)
{
    ErrorString error;
    InjectedScriptCanvasModule module = injectedScriptCanvasModule(&error, glContext);
    if (module.hasNoValue())
        return ScriptObject();
    return notifyRenderingContextWasWrapped(module.wrapWebGLContext(glContext));
}
#endif

ScriptObject InspectorCanvasAgent::notifyRenderingContextWasWrapped(const ScriptObject& wrappedContext)
{
    ASSERT(m_frontend);
    ScriptState* scriptState = wrappedContext.scriptState();
    DOMWindow* domWindow = scriptState ? domWindowFromScriptState(scriptState) : 0;
    Frame* frame = domWindow ? domWindow->frame() : 0;
    if (frame && !m_framesWithUninstrumentedCanvases.contains(frame))
        m_framesWithUninstrumentedCanvases.set(frame, false);
    String frameId = m_pageAgent->frameId(frame);
    if (!frameId.isEmpty())
        m_frontend->contextCreated(frameId);
    return wrappedContext;
}

InjectedScriptCanvasModule InspectorCanvasAgent::injectedScriptCanvasModule(ErrorString* errorString, ScriptState* scriptState)
{
    if (!checkIsEnabled(errorString))
        return InjectedScriptCanvasModule();
    InjectedScriptCanvasModule module = InjectedScriptCanvasModule::moduleForState(m_injectedScriptManager, scriptState);
    if (module.hasNoValue()) {
        ASSERT_NOT_REACHED();
        *errorString = "Internal error: no Canvas module";
    }
    return module;
}

InjectedScriptCanvasModule InspectorCanvasAgent::injectedScriptCanvasModule(ErrorString* errorString, const ScriptObject& scriptObject)
{
    if (!checkIsEnabled(errorString))
        return InjectedScriptCanvasModule();
    if (scriptObject.hasNoValue()) {
        ASSERT_NOT_REACHED();
        *errorString = "Internal error: original ScriptObject has no value";
        return InjectedScriptCanvasModule();
    }
    return injectedScriptCanvasModule(errorString, scriptObject.scriptState());
}

InjectedScriptCanvasModule InspectorCanvasAgent::injectedScriptCanvasModule(ErrorString* errorString, const String& objectId)
{
    if (!checkIsEnabled(errorString))
        return InjectedScriptCanvasModule();
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return InjectedScriptCanvasModule();
    }
    return injectedScriptCanvasModule(errorString, injectedScript.scriptState());
}

void InspectorCanvasAgent::findFramesWithUninstrumentedCanvases()
{
    class NodeVisitor : public WrappedNodeVisitor {
    public:
        NodeVisitor(Page* page, FramesWithUninstrumentedCanvases& result)
            : m_page(page)
            , m_framesWithUninstrumentedCanvases(result)
        {
        }

        virtual void visitNode(Node* node) OVERRIDE
        {
            if (!node->hasTagName(HTMLNames::canvasTag) || !node->document() || !node->document()->frame())
                return;
            
            Frame* frame = node->document()->frame();
            if (frame->page() != m_page)
                return;

            HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(node);
            if (canvas->renderingContext())
                m_framesWithUninstrumentedCanvases.set(frame, true);
        }

    private:
        Page* m_page;
        FramesWithUninstrumentedCanvases& m_framesWithUninstrumentedCanvases;
    } nodeVisitor(m_pageAgent->page(), m_framesWithUninstrumentedCanvases);

    m_framesWithUninstrumentedCanvases.clear();
    ScriptProfiler::visitNodeWrappers(&nodeVisitor);

    for (FramesWithUninstrumentedCanvases::iterator it = m_framesWithUninstrumentedCanvases.begin(); it != m_framesWithUninstrumentedCanvases.end(); ++it) {
        String frameId = m_pageAgent->frameId(it->key);
        if (!frameId.isEmpty())
            m_frontend->contextCreated(frameId);
    }
}

bool InspectorCanvasAgent::checkIsEnabled(ErrorString* errorString) const
{
    if (m_enabled)
        return true;
    *errorString = "Canvas agent is not enabled";
    return false;
}

void InspectorCanvasAgent::frameNavigated(Frame* frame)
{
    if (!m_enabled)
        return;
    if (frame == m_pageAgent->mainFrame()) {
        for (FramesWithUninstrumentedCanvases::iterator it = m_framesWithUninstrumentedCanvases.begin(); it != m_framesWithUninstrumentedCanvases.end(); ++it)
            m_framesWithUninstrumentedCanvases.set(it->key, false);
        m_frontend->traceLogsRemoved(0, 0);
    } else {
        while (frame) {
            if (m_framesWithUninstrumentedCanvases.contains(frame))
                m_framesWithUninstrumentedCanvases.set(frame, false);
            if (m_pageAgent->hasIdForFrame(frame)) {
                String frameId = m_pageAgent->frameId(frame);
                m_frontend->traceLogsRemoved(&frameId, 0);
            }
            frame = frame->tree()->traverseNext();
        }
    }
}

void InspectorCanvasAgent::frameDetached(Frame* frame)
{
    if (m_enabled)
        m_framesWithUninstrumentedCanvases.remove(frame);
}

void InspectorCanvasAgent::didBeginFrame()
{
    if (!m_enabled)
        return;
    ErrorString error;
    for (FramesWithUninstrumentedCanvases::iterator it = m_framesWithUninstrumentedCanvases.begin(); it != m_framesWithUninstrumentedCanvases.end(); ++it) {
        InjectedScriptCanvasModule module = injectedScriptCanvasModule(&error, mainWorldScriptState(it->key));
        if (!module.hasNoValue())
            module.markFrameEnd();
    }
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
