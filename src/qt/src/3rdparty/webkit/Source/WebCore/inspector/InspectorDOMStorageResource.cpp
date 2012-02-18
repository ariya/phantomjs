/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
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

#include "config.h"

#if ENABLE(DOM_STORAGE) && ENABLE(INSPECTOR)

#include "InspectorDOMStorageResource.h"

#include "DOMWindow.h"
#include "Document.h"
#include "EventNames.h"
#include "Frame.h"
#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "SecurityOrigin.h"
#include "Storage.h"
#include "StorageEvent.h"

using namespace JSC;

namespace WebCore {

int InspectorDOMStorageResource::s_nextUnusedId = 1;

InspectorDOMStorageResource::InspectorDOMStorageResource(Storage* domStorage, bool isLocalStorage, Frame* frame)
    :  EventListener(InspectorDOMStorageResourceType)
    , m_domStorage(domStorage)
    , m_isLocalStorage(isLocalStorage)
    , m_frame(frame)
    , m_frontend(0)
    , m_id(s_nextUnusedId++)
    , m_reportingChangesToFrontend(false)
{
}

bool InspectorDOMStorageResource::isSameHostAndType(Frame* frame, bool isLocalStorage) const
{
    return equalIgnoringCase(m_frame->document()->securityOrigin()->host(), frame->document()->securityOrigin()->host()) && m_isLocalStorage == isLocalStorage;
}

void InspectorDOMStorageResource::bind(InspectorFrontend* frontend)
{
    ASSERT(!m_frontend);
    m_frontend = frontend->domstorage();

    RefPtr<InspectorObject> jsonObject = InspectorObject::create();
    jsonObject->setString("host", m_frame->document()->securityOrigin()->host());
    jsonObject->setBoolean("isLocalStorage", m_isLocalStorage);
    jsonObject->setNumber("id", m_id);
    m_frontend->addDOMStorage(jsonObject);
}

void InspectorDOMStorageResource::unbind()
{
    if (!m_frontend)
        return;  // Already unbound.

    if (m_reportingChangesToFrontend) {
        m_frame->domWindow()->removeEventListener(eventNames().storageEvent, this, true);
        m_reportingChangesToFrontend = false;
    }
    m_frontend = 0;
}

void InspectorDOMStorageResource::startReportingChangesToFrontend()
{
    ASSERT(m_frontend);
    if (!m_reportingChangesToFrontend) {
        m_frame->domWindow()->addEventListener(eventNames().storageEvent, this, true);
        m_reportingChangesToFrontend = true;
    }
}

void InspectorDOMStorageResource::handleEvent(ScriptExecutionContext*, Event* event)
{
    ASSERT(m_frontend);
    if (event->type() != eventNames().storageEvent || !event->isStorageEvent())
        return;

    StorageEvent* storageEvent = static_cast<StorageEvent*>(event);
    Storage* storage = storageEvent->storageArea();
    ExceptionCode ec = 0;
    bool isLocalStorage = (storage->frame()->domWindow()->localStorage(ec) == storage && !ec);
    if (isSameHostAndType(storage->frame(), isLocalStorage))
        m_frontend->updateDOMStorage(m_id);
}

bool InspectorDOMStorageResource::operator==(const EventListener& listener)
{
    return (this == InspectorDOMStorageResource::cast(&listener));
}

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE) && ENABLE(INSPECTOR)

