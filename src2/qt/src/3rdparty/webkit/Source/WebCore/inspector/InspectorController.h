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

#ifndef InspectorController_h
#define InspectorController_h

#include "PlatformString.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebCore {

class DOMWrapperWorld;
class Frame;
class GraphicsContext;
class InjectedScriptManager;
class InspectorAgent;
class InspectorBackendDispatcher;
class InspectorClient;
class InspectorFrontend;
class InspectorFrontendClient;
class Page;
class PostWorkerNotificationToFrontendTask;
class Node;

class InspectorController {
    WTF_MAKE_NONCOPYABLE(InspectorController);
    WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorController(Page*, InspectorClient*);
    ~InspectorController();

    bool enabled() const;
    Page* inspectedPage() const;

    void show();
    void close();

    void setInspectorFrontendClient(PassOwnPtr<InspectorFrontendClient>);
    bool hasInspectorFrontendClient() const;
    void didClearWindowObjectInWorld(Frame*, DOMWrapperWorld*);
    void setInspectorExtensionAPI(const String& source);

    void dispatchMessageFromFrontend(const String& message);

    bool hasFrontend() const { return m_inspectorFrontend; }
    void connectFrontend();
    void disconnectFrontend();
    void restoreInspectorStateFromCookie(const String& inspectorCookie);

    void showConsole();
    void inspect(Node*);
    void drawNodeHighlight(GraphicsContext&) const;
    void hideHighlight();
    Node* highlightedNode() const;

    void evaluateForTestInFrontend(long callId, const String& script);

    void startTimelineProfiler();
    void stopTimelineProfiler();
    bool timelineProfilerEnabled();

#if ENABLE(JAVASCRIPT_DEBUGGER)
    bool profilerEnabled();
    void enableProfiler();
    void startUserInitiatedProfiling();
    bool isRecordingUserInitiatedProfile() const;
    void stopUserInitiatedProfiling();
    void disableProfiler();
    void showAndEnableDebugger();
    bool debuggerEnabled();
    void disableDebugger();
    void resume();
#endif

private:
    friend class PostWorkerNotificationToFrontendTask;

    OwnPtr<InjectedScriptManager> m_injectedScriptManager;
    OwnPtr<InspectorAgent> m_inspectorAgent;
    OwnPtr<InspectorBackendDispatcher> m_inspectorBackendDispatcher;
    OwnPtr<InspectorFrontendClient> m_inspectorFrontendClient;
    OwnPtr<InspectorFrontend> m_inspectorFrontend;
    InspectorClient* m_inspectorClient;
    bool m_openingFrontend;
    bool m_startUserInitiatedDebuggingWhenFrontedIsConnected;
};

}

#endif // !defined(InspectorController_h)
