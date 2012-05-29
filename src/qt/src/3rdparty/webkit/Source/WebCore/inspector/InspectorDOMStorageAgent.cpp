/*
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

#include "InspectorDOMStorageAgent.h"

#if ENABLE(INSPECTOR) && ENABLE(DOM_STORAGE)

#include "Database.h"
#include "DOMWindow.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "InspectorDOMStorageResource.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "Storage.h"
#include "StorageArea.h"
#include "VoidCallback.h"

#include <wtf/Vector.h>

namespace WebCore {

namespace DOMStorageAgentState {
static const char domStorageAgentEnabled[] = "domStorageAgentEnabled";
};

typedef HashMap<int, RefPtr<InspectorDOMStorageResource> > DOMStorageResourcesMap;

InspectorDOMStorageAgent::InspectorDOMStorageAgent(InstrumentingAgents* instrumentingAgents, InspectorState* state)
    : m_instrumentingAgents(instrumentingAgents)
    , m_inspectorState(state)
    , m_frontend(0)
    , m_enabled(false)
{
    m_instrumentingAgents->setInspectorDOMStorageAgent(this);
}

InspectorDOMStorageAgent::~InspectorDOMStorageAgent()
{
    m_instrumentingAgents->setInspectorDOMStorageAgent(0);
    m_instrumentingAgents = 0;
}

void InspectorDOMStorageAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend;
}

void InspectorDOMStorageAgent::clearFrontend()
{
    DOMStorageResourcesMap::iterator domStorageEnd = m_resources.end();
    for (DOMStorageResourcesMap::iterator it = m_resources.begin(); it != domStorageEnd; ++it)
        it->second->unbind();
    m_frontend = 0;
    disable(0);
}

void InspectorDOMStorageAgent::restore()
{
    m_enabled =  m_inspectorState->getBoolean(DOMStorageAgentState::domStorageAgentEnabled);
}

void InspectorDOMStorageAgent::enable(ErrorString*)
{
    if (m_enabled)
        return;
    m_enabled = true;
    m_inspectorState->setBoolean(DOMStorageAgentState::domStorageAgentEnabled, m_enabled);

    DOMStorageResourcesMap::iterator resourcesEnd = m_resources.end();
    for (DOMStorageResourcesMap::iterator it = m_resources.begin(); it != resourcesEnd; ++it)
        it->second->bind(m_frontend);
}

void InspectorDOMStorageAgent::disable(ErrorString*)
{
    if (!m_enabled)
        return;
    m_enabled = false;
    m_inspectorState->setBoolean(DOMStorageAgentState::domStorageAgentEnabled, m_enabled);
}

void InspectorDOMStorageAgent::getDOMStorageEntries(ErrorString*, int storageId, RefPtr<InspectorArray>* entries)
{
    InspectorDOMStorageResource* storageResource = getDOMStorageResourceForId(storageId);
    if (storageResource) {
        storageResource->startReportingChangesToFrontend();
        Storage* domStorage = storageResource->domStorage();
        for (unsigned i = 0; i < domStorage->length(); ++i) {
            String name(domStorage->key(i));
            String value(domStorage->getItem(name));
            RefPtr<InspectorArray> entry = InspectorArray::create();
            entry->pushString(name);
            entry->pushString(value);
            (*entries)->pushArray(entry);
        }
    }
}

void InspectorDOMStorageAgent::setDOMStorageItem(ErrorString*, int storageId, const String& key, const String& value, bool* success)
{
    InspectorDOMStorageResource* storageResource = getDOMStorageResourceForId(storageId);
    if (storageResource) {
        ExceptionCode exception = 0;
        storageResource->domStorage()->setItem(key, value, exception);
        *success = !exception;
    }
}

void InspectorDOMStorageAgent::removeDOMStorageItem(ErrorString*, int storageId, const String& key, bool* success)
{
    InspectorDOMStorageResource* storageResource = getDOMStorageResourceForId(storageId);
    if (storageResource) {
        storageResource->domStorage()->removeItem(key);
        *success = true;
    }
}

int InspectorDOMStorageAgent::storageId(Storage* storage)
{
    ASSERT(storage);
    Frame* frame = storage->frame();
    ExceptionCode ec = 0;
    bool isLocalStorage = (frame->domWindow()->localStorage(ec) == storage && !ec);
    DOMStorageResourcesMap::iterator domStorageEnd = m_resources.end();
    for (DOMStorageResourcesMap::iterator it = m_resources.begin(); it != domStorageEnd; ++it) {
        if (it->second->isSameHostAndType(frame, isLocalStorage))
            return it->first;
    }
    return 0;
}

InspectorDOMStorageResource* InspectorDOMStorageAgent::getDOMStorageResourceForId(int storageId)
{
    DOMStorageResourcesMap::iterator it = m_resources.find(storageId);
    if (it == m_resources.end())
        return 0;
    return it->second.get();
}

void InspectorDOMStorageAgent::didUseDOMStorage(StorageArea* storageArea, bool isLocalStorage, Frame* frame)
{
    DOMStorageResourcesMap::iterator domStorageEnd = m_resources.end();
    for (DOMStorageResourcesMap::iterator it = m_resources.begin(); it != domStorageEnd; ++it) {
        if (it->second->isSameHostAndType(frame, isLocalStorage))
            return;
    }

    RefPtr<Storage> domStorage = Storage::create(frame, storageArea);
    RefPtr<InspectorDOMStorageResource> resource = InspectorDOMStorageResource::create(domStorage.get(), isLocalStorage, frame);

    m_resources.set(resource->id(), resource);

    // Resources are only bound while visible.
    if (m_frontend)
        resource->bind(m_frontend);
}

void InspectorDOMStorageAgent::clearResources()
{
    m_resources.clear();
}


} // namespace WebCore

#endif // ENABLE(INSPECTOR) && ENABLE(DOM_STORE)
