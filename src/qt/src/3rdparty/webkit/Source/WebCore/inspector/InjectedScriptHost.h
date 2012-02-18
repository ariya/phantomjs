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

#include "Console.h"
#include "InspectorAgent.h"
#include "PlatformString.h"
#include "ScriptState.h"

#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class Database;
class InjectedScript;
class InspectorAgent;
class InspectorConsoleAgent;
class InspectorDOMStorageAgent;
class InspectorDatabaseAgent;
class InspectorFrontend;
class InspectorObject;
class Node;
class ScriptObject;
class ScriptValue;
class Storage;

class InjectedScriptHost : public RefCounted<InjectedScriptHost>
{
public:
    static PassRefPtr<InjectedScriptHost> create();
    ~InjectedScriptHost();

    void init(InspectorAgent* inspectorAgent
            , InspectorConsoleAgent* consoleAgent
#if ENABLE(DATABASE)
            , InspectorDatabaseAgent* databaseAgent
#endif
#if ENABLE(DOM_STORAGE)
            , InspectorDOMStorageAgent* domStorageAgent
#endif
        )
    {
        m_inspectorAgent = inspectorAgent;
        m_consoleAgent = consoleAgent;
#if ENABLE(DATABASE)
        m_databaseAgent = databaseAgent;
#endif
#if ENABLE(DOM_STORAGE)
        m_domStorageAgent = domStorageAgent;
#endif
    }
    void setFrontend(InspectorFrontend* frontend) { m_frontend = frontend; }
    void clearFrontend() { m_frontend = 0; }

    static Node* scriptValueAsNode(ScriptValue);
    static ScriptValue nodeAsScriptValue(ScriptState*, Node*);

    void disconnect();

    void addInspectedNode(Node*);
    void clearInspectedNodes();

    void inspectImpl(PassRefPtr<InspectorValue> objectToInspect, PassRefPtr<InspectorValue> hints);
    void clearConsoleMessages();
    void copyText(const String& text);
    Node* inspectedNode(unsigned int num);
#if ENABLE(DATABASE)
    int databaseIdImpl(Database*);
#endif
#if ENABLE(DOM_STORAGE)
    int storageIdImpl(Storage*);
#endif
#if ENABLE(WORKERS)
    long nextWorkerId();
    void didCreateWorker(long id, const String& url, bool isSharedWorker);
    void didDestroyWorker(long id);
#endif

private:
    InjectedScriptHost();

    InspectorAgent* m_inspectorAgent;
    InspectorConsoleAgent* m_consoleAgent;
#if ENABLE(DATABASE)
    InspectorDatabaseAgent* m_databaseAgent;
#endif
#if ENABLE(DOM_STORAGE)
    InspectorDOMStorageAgent* m_domStorageAgent;
#endif
    InspectorFrontend* m_frontend;
    long m_lastWorkerId;
    Vector<RefPtr<Node> > m_inspectedNodes;
};

} // namespace WebCore

#endif // !defined(InjectedScriptHost_h)
