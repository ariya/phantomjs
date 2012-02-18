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
#include "IDBTransaction.h"

#if ENABLE(INDEXED_DATABASE)

#include "Document.h"
#include "EventException.h"
#include "EventQueue.h"
#include "IDBDatabase.h"
#include "IDBDatabaseException.h"
#include "IDBEventDispatcher.h"
#include "IDBIndex.h"
#include "IDBObjectStore.h"
#include "IDBObjectStoreBackendInterface.h"
#include "IDBPendingTransactionMonitor.h"

namespace WebCore {

PassRefPtr<IDBTransaction> IDBTransaction::create(ScriptExecutionContext* context, PassRefPtr<IDBTransactionBackendInterface> backend, IDBDatabase* db)
{ 
    return adoptRef(new IDBTransaction(context, backend, db));
}

IDBTransaction::IDBTransaction(ScriptExecutionContext* context, PassRefPtr<IDBTransactionBackendInterface> backend, IDBDatabase* db)
    : ActiveDOMObject(context, this)
    , m_backend(backend)
    , m_database(db)
    , m_mode(m_backend->mode())
    , m_finished(false)
{
    ASSERT(m_backend);
    IDBPendingTransactionMonitor::addPendingTransaction(m_backend.get());
}

IDBTransaction::~IDBTransaction()
{
}

IDBTransactionBackendInterface* IDBTransaction::backend() const
{
    return m_backend.get();
}

bool IDBTransaction::finished() const
{
    return m_finished;
}

unsigned short IDBTransaction::mode() const
{
    return m_mode;
}

IDBDatabase* IDBTransaction::db() const
{
    return m_database.get();
}

PassRefPtr<IDBObjectStore> IDBTransaction::objectStore(const String& name, ExceptionCode& ec)
{
    if (m_finished) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return 0;
    }
    RefPtr<IDBObjectStoreBackendInterface> objectStoreBackend = m_backend->objectStore(name, ec);
    if (!objectStoreBackend) {
        ASSERT(ec);
        return 0;
    }
    RefPtr<IDBObjectStore> objectStore = IDBObjectStore::create(objectStoreBackend, this);
    return objectStore.release();
}

void IDBTransaction::abort()
{
    RefPtr<IDBTransaction> selfRef = this;
    if (m_backend)
        m_backend->abort();
}

void IDBTransaction::registerRequest(IDBRequest* request)
{
    m_childRequests.add(request);
}

void IDBTransaction::unregisterRequest(IDBRequest* request)
{
    // If we aborted the request, it will already have been removed.
    m_childRequests.remove(request);
}

void IDBTransaction::onAbort()
{
    while (!m_childRequests.isEmpty()) {
        IDBRequest* request = *m_childRequests.begin();
        m_childRequests.remove(request);
        request->abort();
    }

    enqueueEvent(Event::create(eventNames().abortEvent, true, false));
}

void IDBTransaction::onComplete()
{
    enqueueEvent(Event::create(eventNames().completeEvent, false, false));
}

bool IDBTransaction::hasPendingActivity() const
{
    // FIXME: In an ideal world, we should return true as long as anyone has a or can
    //        get a handle to us or any child request object and any of those have
    //        event listeners. This is  in order to handle user generated events properly.
    return !m_finished || ActiveDOMObject::hasPendingActivity();
}

ScriptExecutionContext* IDBTransaction::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

bool IDBTransaction::dispatchEvent(PassRefPtr<Event> event)
{
    ASSERT(!m_finished);
    ASSERT(scriptExecutionContext());
    ASSERT(event->target() == this);
    ASSERT(!m_finished);
    m_finished = true;

    Vector<RefPtr<EventTarget> > targets;
    targets.append(this);
    targets.append(db());

    // FIXME: When we allow custom event dispatching, this will probably need to change.
    ASSERT(event->type() == eventNames().completeEvent || event->type() == eventNames().abortEvent);
    return IDBEventDispatcher::dispatch(event.get(), targets);
}

bool IDBTransaction::canSuspend() const
{
    // FIXME: Technically we can suspend before the first request is schedule
    //        and after the complete/abort event is enqueued.
    return m_finished;
}

void IDBTransaction::contextDestroyed()
{
    ActiveDOMObject::contextDestroyed();

    // Must happen in contextDestroyed since it can result in ActiveDOMObjects being destructed
    // (and contextDestroyed is the only one resilient against this).
    RefPtr<IDBTransaction> selfRef = this;
    if (m_backend)
        m_backend->abort();

    m_finished = true;
}

void IDBTransaction::enqueueEvent(PassRefPtr<Event> event)
{
    ASSERT(!m_finished);
    if (!scriptExecutionContext())
        return;

    ASSERT(scriptExecutionContext()->isDocument());
    EventQueue* eventQueue = static_cast<Document*>(scriptExecutionContext())->eventQueue();
    event->setTarget(this);
    eventQueue->enqueueEvent(event);
}

EventTargetData* IDBTransaction::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* IDBTransaction::ensureEventTargetData()
{
    return &m_eventTargetData;
}

}

#endif // ENABLE(INDEXED_DATABASE)
