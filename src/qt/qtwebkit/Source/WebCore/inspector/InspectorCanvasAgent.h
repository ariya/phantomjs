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

#ifndef InspectorCanvasAgent_h
#define InspectorCanvasAgent_h

#if ENABLE(INSPECTOR)

#include "InspectorBaseAgent.h"
#include "InspectorFrontend.h"
#include "InspectorTypeBuilder.h"
#include "ScriptState.h"
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Frame;
class InjectedScriptCanvasModule;
class InjectedScriptManager;
class InspectorPageAgent;
class InspectorState;
class InstrumentingAgents;
class ScriptObject;

typedef String ErrorString;

class InspectorCanvasAgent : public InspectorBaseAgent<InspectorCanvasAgent>, public InspectorBackendDispatcher::CanvasCommandHandler {
public:
    static PassOwnPtr<InspectorCanvasAgent> create(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state, InspectorPageAgent* pageAgent, InjectedScriptManager* injectedScriptManager)
    {
        return adoptPtr(new InspectorCanvasAgent(instrumentingAgents, state, pageAgent, injectedScriptManager));
    }
    ~InspectorCanvasAgent();

    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();
    virtual void restore();

    void frameNavigated(Frame*);
    void frameDetached(Frame*);
    void didBeginFrame();

    // Called from InspectorCanvasInstrumentation.
    ScriptObject wrapCanvas2DRenderingContextForInstrumentation(const ScriptObject&);
#if ENABLE(WEBGL)
    ScriptObject wrapWebGLRenderingContextForInstrumentation(const ScriptObject&);
#endif

    // Called from the front-end.
    virtual void enable(ErrorString*);
    virtual void disable(ErrorString*);
    virtual void dropTraceLog(ErrorString*, const TypeBuilder::Canvas::TraceLogId&);
    virtual void hasUninstrumentedCanvases(ErrorString*, bool*);
    virtual void captureFrame(ErrorString*, const TypeBuilder::Network::FrameId*, TypeBuilder::Canvas::TraceLogId*);
    virtual void startCapturing(ErrorString*, const TypeBuilder::Network::FrameId*, TypeBuilder::Canvas::TraceLogId*);
    virtual void stopCapturing(ErrorString*, const TypeBuilder::Canvas::TraceLogId&);
    virtual void getTraceLog(ErrorString*, const TypeBuilder::Canvas::TraceLogId&, const int*, const int*, RefPtr<TypeBuilder::Canvas::TraceLog>&);
    virtual void replayTraceLog(ErrorString*, const TypeBuilder::Canvas::TraceLogId&, int, RefPtr<TypeBuilder::Canvas::ResourceState>&);
    virtual void getResourceInfo(ErrorString*, const TypeBuilder::Canvas::ResourceId&, RefPtr<TypeBuilder::Canvas::ResourceInfo>&);
    virtual void getResourceState(ErrorString*, const TypeBuilder::Canvas::TraceLogId&, const TypeBuilder::Canvas::ResourceId&, RefPtr<TypeBuilder::Canvas::ResourceState>&);

private:
    InspectorCanvasAgent(InstrumentingAgents*, InspectorCompositeState*, InspectorPageAgent*, InjectedScriptManager*);

    InjectedScriptCanvasModule injectedScriptCanvasModule(ErrorString*, ScriptState*);
    InjectedScriptCanvasModule injectedScriptCanvasModule(ErrorString*, const ScriptObject&);
    InjectedScriptCanvasModule injectedScriptCanvasModule(ErrorString*, const String&);

    void findFramesWithUninstrumentedCanvases();
    bool checkIsEnabled(ErrorString*) const;
    ScriptObject notifyRenderingContextWasWrapped(const ScriptObject&);

    InspectorPageAgent* m_pageAgent;
    InjectedScriptManager* m_injectedScriptManager;
    InspectorFrontend::Canvas* m_frontend;
    bool m_enabled;
    // Contains all frames with canvases, value is true only for frames that have an uninstrumented canvas.
    typedef HashMap<Frame*, bool> FramesWithUninstrumentedCanvases;
    FramesWithUninstrumentedCanvases m_framesWithUninstrumentedCanvases;
};

} // namespace WebCore

#endif // ENABLE(INSPECTOR)

#endif // !defined(InspectorCanvasAgent_h)
