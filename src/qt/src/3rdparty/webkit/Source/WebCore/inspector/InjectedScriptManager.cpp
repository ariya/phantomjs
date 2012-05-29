/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
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

#include "config.h"
#include "InjectedScriptManager.h"

#if ENABLE(INSPECTOR)

#include "InjectedScript.h"
#include "InjectedScriptHost.h"
#include "InjectedScriptSource.h"
#include "InspectorValues.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

PassOwnPtr<InjectedScriptManager> InjectedScriptManager::createForPage()
{
    return adoptPtr(new InjectedScriptManager(&InjectedScriptManager::canAccessInspectedWindow));
}

PassOwnPtr<InjectedScriptManager> InjectedScriptManager::createForWorker()
{
    return adoptPtr(new InjectedScriptManager(&InjectedScriptManager::canAccessInspectedWorkerContext));
}

InjectedScriptManager::InjectedScriptManager(InspectedStateAccessCheck accessCheck)
    : m_nextInjectedScriptId(1)
    , m_injectedScriptHost(InjectedScriptHost::create())
    , m_inspectedStateAccessCheck(accessCheck)
{
}

InjectedScriptManager::~InjectedScriptManager()
{
}

void InjectedScriptManager::disconnect()
{
    m_injectedScriptHost->disconnect();
    m_injectedScriptHost.clear();
}

InjectedScriptHost* InjectedScriptManager::injectedScriptHost()
{
    return m_injectedScriptHost.get();
}

InjectedScript InjectedScriptManager::injectedScriptForId(long id)
{
    return m_idToInjectedScript.get(id);
}

InjectedScript InjectedScriptManager::injectedScriptForObjectId(const String& objectId)
{
    RefPtr<InspectorValue> parsedObjectId = InspectorValue::parseJSON(objectId);
    if (parsedObjectId && parsedObjectId->type() == InspectorValue::TypeObject) {
        long injectedScriptId = 0;
        bool success = parsedObjectId->asObject()->getNumber("injectedScriptId", &injectedScriptId);
        if (success)
            return injectedScriptForId(injectedScriptId);
    }
    return InjectedScript();
}

void InjectedScriptManager::discardInjectedScripts()
{
    IdToInjectedScriptMap::iterator end = m_idToInjectedScript.end();
    for (IdToInjectedScriptMap::iterator it = m_idToInjectedScript.begin(); it != end; ++it)
        discardInjectedScript(it->second.scriptState());
    m_idToInjectedScript.clear();
}

bool InjectedScriptManager::canAccessInspectedWorkerContext(ScriptState*)
{
    return true;
}

void InjectedScriptManager::releaseObjectGroup(const String& objectGroup)
{
    for (IdToInjectedScriptMap::iterator it = m_idToInjectedScript.begin(); it != m_idToInjectedScript.end(); ++it)
        it->second.releaseObjectGroup(objectGroup);
}

String InjectedScriptManager::injectedScriptSource()
{
    return String(reinterpret_cast<const char*>(InjectedScriptSource_js), sizeof(InjectedScriptSource_js));
}

pair<long, ScriptObject> InjectedScriptManager::injectScript(const String& source, ScriptState* scriptState)
{
    long id = m_nextInjectedScriptId++;
    return std::make_pair(id, createInjectedScript(source, scriptState, id));
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
