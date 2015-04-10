/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef InjectedScriptHost_h
#define InjectedScriptHost_h

#include "ConsoleTypes.h"
#include "InspectorAgent.h"
#include "ScriptState.h"
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class Database;
class InjectedScript;
class InspectorAgent;
class InspectorConsoleAgent;
class InspectorDOMAgent;
class InspectorDOMStorageAgent;
class InspectorDatabaseAgent;
class InspectorDebuggerAgent;
class InspectorFrontend;
class InspectorObject;
class InspectorValue;
class Node;
class ScriptDebugServer;
class ScriptObject;
class ScriptValue;
class Storage;

struct EventListenerInfo;

class InjectedScriptHost : public RefCounted<InjectedScriptHost> {
public:
    static PassRefPtr<InjectedScriptHost> create();
    ~InjectedScriptHost();

    void init(InspectorAgent* inspectorAgent
            , InspectorConsoleAgent* consoleAgent
#if ENABLE(SQL_DATABASE)
            , InspectorDatabaseAgent* databaseAgent
#endif
            , InspectorDOMStorageAgent* domStorageAgent
            , InspectorDOMAgent* domAgent
#if ENABLE(JAVASCRIPT_DEBUGGER)
            , InspectorDebuggerAgent* debuggerAgent
#endif
        )
    {
        m_inspectorAgent = inspectorAgent;
        m_consoleAgent = consoleAgent;
#if ENABLE(SQL_DATABASE)
        m_databaseAgent = databaseAgent;
#endif
        m_domStorageAgent = domStorageAgent;
        m_domAgent = domAgent;
#if ENABLE(JAVASCRIPT_DEBUGGER)
        m_debuggerAgent = debuggerAgent;
#endif
    }

    static Node* scriptValueAsNode(ScriptValue);
    static ScriptValue nodeAsScriptValue(ScriptState*, Node*);

    void disconnect();

    class InspectableObject {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        virtual ScriptValue get(ScriptState*);
        virtual ~InspectableObject() { }
    };
    void addInspectedObject(PassOwnPtr<InspectableObject>);
    void clearInspectedObjects();
    InspectableObject* inspectedObject(unsigned int num);

    void inspectImpl(PassRefPtr<InspectorValue> objectToInspect, PassRefPtr<InspectorValue> hints);
    void getEventListenersImpl(Node*, Vector<EventListenerInfo>& listenersArray);

    void clearConsoleMessages();
    void copyText(const String& text);
#if ENABLE(SQL_DATABASE)
    String databaseIdImpl(Database*);
#endif
    String storageIdImpl(Storage*);

#if ENABLE(JAVASCRIPT_DEBUGGER)
    ScriptDebugServer& scriptDebugServer();
#endif

private:
    InjectedScriptHost();

    InspectorAgent* m_inspectorAgent;
    InspectorConsoleAgent* m_consoleAgent;
#if ENABLE(SQL_DATABASE)
    InspectorDatabaseAgent* m_databaseAgent;
#endif
    InspectorDOMStorageAgent* m_domStorageAgent;
    InspectorDOMAgent* m_domAgent;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    InspectorDebuggerAgent* m_debuggerAgent;
#endif
    Vector<OwnPtr<InspectableObject> > m_inspectedObjects;
    OwnPtr<InspectableObject> m_defaultInspectableObject;
};

} // namespace WebCore

#endif // !defined(InjectedScriptHost_h)
