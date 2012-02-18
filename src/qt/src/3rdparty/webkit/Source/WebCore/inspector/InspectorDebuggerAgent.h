/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010-2011 Google Inc. All rights reserved.
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

#ifndef InspectorDebuggerAgent_h
#define InspectorDebuggerAgent_h

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
#include "InjectedScript.h"
#include "InspectorFrontend.h"
#include "ScriptBreakpoint.h"
#include "ScriptDebugListener.h"
#include "ScriptState.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class InjectedScriptManager;
class InspectorFrontend;
class InspectorArray;
class InspectorObject;
class InspectorState;
class InspectorValue;
class InstrumentingAgents;
class ScriptDebugServer;
class ScriptValue;

typedef String ErrorString;

enum DebuggerEventType {
    JavaScriptPauseEventType,
    JavaScriptBreakpointEventType,
    NativeBreakpointDebuggerEventType
};

class InspectorDebuggerAgent : public ScriptDebugListener {
    WTF_MAKE_NONCOPYABLE(InspectorDebuggerAgent); WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~InspectorDebuggerAgent();

    void enable(ErrorString*) { enable(false); }
    void disable(ErrorString*) { disable(); }
    void disable();
    bool enabled();
    void restore();
    void setFrontend(InspectorFrontend*);
    void clearFrontend();

    void inspectedURLChanged(const String& url);

    // Part of the protocol.
    void setBreakpointsActive(ErrorString*, bool active);

    void setBreakpointByUrl(ErrorString*, const String& url, int lineNumber, const int* const optionalColumnNumber, const String* const optionalCondition, String* breakpointId, RefPtr<InspectorArray>* locations);
    void setBreakpoint(ErrorString*, PassRefPtr<InspectorObject> location, const String* const optionalCondition, String* breakpointId, RefPtr<InspectorObject>* actualLocation);
    void removeBreakpoint(ErrorString*, const String& breakpointId);
    void continueToLocation(ErrorString*, PassRefPtr<InspectorObject> location);

    void editScriptSource(ErrorString*, const String& sourceID, const String& newContent, RefPtr<InspectorArray>* newCallFrames);
    void getScriptSource(ErrorString*, const String& sourceID, String* scriptSource);
    void schedulePauseOnNextStatement(DebuggerEventType type, PassRefPtr<InspectorValue> data);
    void cancelPauseOnNextStatement();
    void breakProgram(DebuggerEventType type, PassRefPtr<InspectorValue> data);
    void pause(ErrorString*);
    void resume(ErrorString*);
    void stepOver(ErrorString*);
    void stepInto(ErrorString*);
    void stepOut(ErrorString*);
    void setPauseOnExceptions(ErrorString*, const String& pauseState);
    void evaluateOnCallFrame(ErrorString*, const String& callFrameId, const String& expression, const String* const objectGroup, const bool* const includeCommandLineAPI, RefPtr<InspectorObject>* result, bool* wasThrown);

    class Listener {
    public:
        virtual ~Listener() { }
        virtual void debuggerWasEnabled() = 0;
        virtual void debuggerWasDisabled() = 0;
    };
    void setListener(Listener* listener) { m_listener = listener; }

    virtual ScriptDebugServer& scriptDebugServer() = 0;

protected:
    InspectorDebuggerAgent(InstrumentingAgents*, InspectorState*, InjectedScriptManager*);

    virtual void startListeningScriptDebugServer() = 0;
    virtual void stopListeningScriptDebugServer() = 0;

private:
    void enable(bool restoringFromState);

    PassRefPtr<InspectorArray> currentCallFrames();

    virtual void didParseSource(const String& sourceID, const String& url, const String& data, int startLine, int startColumn, int endLine, int endColumn, bool isContentScript);
    virtual void failedToParseSource(const String& url, const String& data, int firstLine, int errorLine, const String& errorMessage);
    virtual void didPause(ScriptState*, const ScriptValue& callFrames, const ScriptValue& exception);
    virtual void didContinue();

    PassRefPtr<InspectorObject> resolveBreakpoint(const String& breakpointId, const String& sourceId, const ScriptBreakpoint&);
    void clear();

    class Script {
    public:
        Script()
            : startLine(0)
            , startColumn(0)
            , endLine(0)
            , endColumn(0)
        {
        }

        Script(const String& url, const String& data, int startLine, int startColumn, int endLine, int endColumn)
            : url(url)
            , data(data)
            , startLine(startLine)
            , startColumn(startColumn)
            , endLine(endLine)
            , endColumn(endColumn)
        {
        }

        String url;
        String data;
        int startLine;
        int startColumn;
        int endLine;
        int endColumn;
        int linesCount;
    };

    typedef HashMap<String, Script> ScriptsMap;
    typedef HashMap<String, Vector<String> > BreakpointIdToDebugServerBreakpointIdsMap;

    InstrumentingAgents* m_instrumentingAgents;
    InspectorState* m_inspectorState;
    InjectedScriptManager* m_injectedScriptManager;
    InspectorFrontend::Debugger* m_frontend;
    ScriptState* m_pausedScriptState;
    ScriptValue m_currentCallStack;
    ScriptsMap m_scripts;
    BreakpointIdToDebugServerBreakpointIdsMap m_breakpointIdToDebugServerBreakpointIds;
    String m_continueToLocationBreakpointId;
    RefPtr<InspectorObject> m_breakProgramDetails;
    bool m_javaScriptPauseScheduled;
    Listener* m_listener;
};

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)

#endif // !defined(InspectorDebuggerAgent_h)
