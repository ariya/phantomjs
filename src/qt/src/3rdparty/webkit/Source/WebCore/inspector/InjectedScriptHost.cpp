/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "InjectedScriptHost.h"

#if ENABLE(INSPECTOR)

#include "Element.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLFrameOwnerElement.h"
#include "InjectedScript.h"
#include "InspectorAgent.h"
#include "InspectorClient.h"
#include "InspectorConsoleAgent.h"
#include "InspectorDOMStorageAgent.h"
#include "InspectorDatabaseAgent.h"
#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "Pasteboard.h"

#if ENABLE(DATABASE)
#include "Database.h"
#endif

#if ENABLE(DOM_STORAGE)
#include "Storage.h"
#endif

#include "markup.h"

#include <wtf/RefPtr.h>
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

PassRefPtr<InjectedScriptHost> InjectedScriptHost::create()
{
    return adoptRef(new InjectedScriptHost());
}

InjectedScriptHost::InjectedScriptHost()
    : m_inspectorAgent(0)
    , m_consoleAgent(0)
#if ENABLE(DATABASE)
    , m_databaseAgent(0)
#endif
#if ENABLE(DOM_STORAGE)
    , m_domStorageAgent(0)
#endif
    , m_frontend(0)
    , m_lastWorkerId(1 << 31) // Distinguish ids of fake workers from real ones, to minimize the chances they overlap.
{
}

InjectedScriptHost::~InjectedScriptHost()
{
}

void InjectedScriptHost::disconnect()
{
    m_inspectorAgent = 0;
    m_consoleAgent = 0;
#if ENABLE(DATABASE)
    m_databaseAgent = 0;
#endif
#if ENABLE(DOM_STORAGE)
    m_domStorageAgent = 0;
#endif
    m_frontend = 0;
}

void InjectedScriptHost::addInspectedNode(Node* node)
{
    m_inspectedNodes.prepend(node);
    while (m_inspectedNodes.size() > 5)
        m_inspectedNodes.removeLast();
}

void InjectedScriptHost::clearInspectedNodes()
{
    m_inspectedNodes.clear();
}

void InjectedScriptHost::inspectImpl(PassRefPtr<InspectorValue> object, PassRefPtr<InspectorValue> hints)
{
    if (m_frontend)
        m_frontend->inspector()->inspect(object->asObject(), hints->asObject());
}

void InjectedScriptHost::clearConsoleMessages()
{
    if (m_consoleAgent) {
        ErrorString error;
        m_consoleAgent->clearConsoleMessages(&error);
    }
}

void InjectedScriptHost::copyText(const String& text)
{
    Pasteboard::generalPasteboard()->writePlainText(text);
}

Node* InjectedScriptHost::inspectedNode(unsigned int num)
{
    if (num < m_inspectedNodes.size())
        return m_inspectedNodes[num].get();
    return 0;
}

#if ENABLE(DATABASE)
int InjectedScriptHost::databaseIdImpl(Database* database)
{
    if (m_databaseAgent)
        return m_databaseAgent->databaseId(database);
    return 0;
}
#endif

#if ENABLE(DOM_STORAGE)
int InjectedScriptHost::storageIdImpl(Storage* storage)
{
    if (m_domStorageAgent)
        return m_domStorageAgent->storageId(storage);
    return 0;
}
#endif

#if ENABLE(WORKERS)
long InjectedScriptHost::nextWorkerId()
{
    return ++m_lastWorkerId;
}

void InjectedScriptHost::didCreateWorker(long id, const String& url, bool isSharedWorker)
{
    if (m_inspectorAgent)
        m_inspectorAgent->didCreateWorker(static_cast<int>(id), url, isSharedWorker);
}

void InjectedScriptHost::didDestroyWorker(long id)
{
    if (m_inspectorAgent)
        m_inspectorAgent->didDestroyWorker(static_cast<int>(id));
}
#endif // ENABLE(WORKERS)

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
