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
#include "IDBDatabase.h"

#include "Document.h"
#include "EventQueue.h"
#include "IDBAny.h"
#include "IDBDatabaseCallbacksImpl.h"
#include "IDBDatabaseError.h"
#include "IDBDatabaseException.h"
#include "IDBEventDispatcher.h"
#include "IDBFactoryBackendInterface.h"
#include "IDBIndex.h"
#include "IDBObjectStore.h"
#include "IDBVersionChangeEvent.h"
#include "IDBVersionChangeRequest.h"
#include "IDBTransaction.h"
#include "ScriptExecutionContext.h"
#include <limits>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

PassRefPtr<IDBDatabase> IDBDatabase::create(ScriptExecutionContext* context, PassRefPtr<IDBDatabaseBackendInterface> database)
{
    return adoptRef(new IDBDatabase(context, database));
}

IDBDatabase::IDBDatabase(ScriptExecutionContext* context, PassRefPtr<IDBDatabaseBackendInterface> backend)
    : ActiveDOMObject(context, this)
    , m_backend(backend)
    , m_noNewTransactions(false)
    , m_stopped(false)
{
    // We pass a reference of this object before it can be adopted.
    relaxAdoptionRequirement();
    m_databaseCallbacks = IDBDatabaseCallbacksImpl::create(this);
}

IDBDatabase::~IDBDatabase()
{
    m_databaseCallbacks->unregisterDatabase(this);
}

void IDBDatabase::setSetVersionTransaction(IDBTransaction* transaction)
{
    m_setVersionTransaction = transaction;
}

PassRefPtr<IDBObjectStore> IDBDatabase::createObjectStore(const String& name, const OptionsObject& options, ExceptionCode& ec)
{
    if (!m_setVersionTransaction) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return 0;
    }

    String keyPath;
    options.getKeyString("keyPath", keyPath);
    bool autoIncrement = false;
    options.getKeyBool("autoIncrement", autoIncrement);
    // FIXME: Look up evictable and pass that on as well.

    RefPtr<IDBObjectStoreBackendInterface> objectStore = m_backend->createObjectStore(name, keyPath, autoIncrement, m_setVersionTransaction->backend(), ec);
    if (!objectStore) {
        ASSERT(ec);
        return 0;
    }
    return IDBObjectStore::create(objectStore.release(), m_setVersionTransaction.get());
}

void IDBDatabase::deleteObjectStore(const String& name, ExceptionCode& ec)
{
    if (!m_setVersionTransaction) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return;
    }

    m_backend->deleteObjectStore(name, m_setVersionTransaction->backend(), ec);
}

PassRefPtr<IDBVersionChangeRequest> IDBDatabase::setVersion(ScriptExecutionContext* context, const String& version, ExceptionCode& ec)
{
    RefPtr<IDBVersionChangeRequest> request = IDBVersionChangeRequest::create(context, IDBAny::create(this), version);
    m_backend->setVersion(version, request, m_databaseCallbacks, ec);
    return request;
}

PassRefPtr<IDBTransaction> IDBDatabase::transaction(ScriptExecutionContext* context, PassRefPtr<DOMStringList> prpStoreNames, unsigned short mode, ExceptionCode& ec)
{
    RefPtr<DOMStringList> storeNames = prpStoreNames;
    if (!storeNames)
        storeNames = DOMStringList::create();

    if (mode != IDBTransaction::READ_WRITE && mode != IDBTransaction::READ_ONLY) {
        // FIXME: May need to change when specced: http://www.w3.org/Bugs/Public/show_bug.cgi?id=11406
        ec = IDBDatabaseException::CONSTRAINT_ERR;
        return 0;
    }
    if (m_noNewTransactions) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return 0;
    }

    // We need to create a new transaction synchronously. Locks are acquired asynchronously. Operations
    // can be queued against the transaction at any point. They will start executing as soon as the
    // appropriate locks have been acquired.
    // Also note that each backend object corresponds to exactly one IDBTransaction object.
    RefPtr<IDBTransactionBackendInterface> transactionBackend = m_backend->transaction(storeNames.get(), mode, ec);
    if (!transactionBackend) {
        ASSERT(ec);
        return 0;
    }
    RefPtr<IDBTransaction> transaction = IDBTransaction::create(context, transactionBackend, this);
    transactionBackend->setCallbacks(transaction.get());
    return transaction.release();
}

void IDBDatabase::close()
{
    if (m_noNewTransactions)
        return;

    ASSERT(scriptExecutionContext()->isDocument());
    EventQueue* eventQueue = static_cast<Document*>(scriptExecutionContext())->eventQueue();
    // Remove any pending versionchange events scheduled to fire on this
    // connection. They would have been scheduled by the backend when another
    // connection called setVersion, but the frontend connection is being
    // closed before they could fire.
    for (size_t i = 0; i < m_enqueuedEvents.size(); ++i) {
        bool removed = eventQueue->cancelEvent(m_enqueuedEvents[i].get());
        ASSERT_UNUSED(removed, removed);
    }

    m_noNewTransactions = true;
    m_backend->close(m_databaseCallbacks);
}

void IDBDatabase::onVersionChange(const String& version)
{
    enqueueEvent(IDBVersionChangeEvent::create(version, eventNames().versionchangeEvent));
}

bool IDBDatabase::hasPendingActivity() const
{
    // FIXME: Try to find some way not to just leak this object until page navigation.
    // FIXME: In an ideal world, we should return true as long as anyone has or can
    //        get a handle to us or any derivative transaction/request object and any
    //        of those have event listeners. This is in order to handle user generated
    //        events properly.
    return !m_stopped || ActiveDOMObject::hasPendingActivity();
}

void IDBDatabase::open()
{
    m_backend->open(m_databaseCallbacks);
}

void IDBDatabase::enqueueEvent(PassRefPtr<Event> event)
{
    ASSERT(scriptExecutionContext()->isDocument());
    EventQueue* eventQueue = static_cast<Document*>(scriptExecutionContext())->eventQueue();
    event->setTarget(this);
    eventQueue->enqueueEvent(event.get());
    m_enqueuedEvents.append(event);
}

bool IDBDatabase::dispatchEvent(PassRefPtr<Event> event)
{
    ASSERT(event->type() == eventNames().versionchangeEvent);
    for (size_t i = 0; i < m_enqueuedEvents.size(); ++i) {
        if (m_enqueuedEvents[i].get() == event.get())
            m_enqueuedEvents.remove(i);
    }
    return EventTarget::dispatchEvent(event.get());
}

void IDBDatabase::stop()
{
    // Stop fires at a deterministic time, so we need to call close in it.
    close();

    m_stopped = true;
}

ScriptExecutionContext* IDBDatabase::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

EventTargetData* IDBDatabase::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* IDBDatabase::ensureEventTargetData()
{
    return &m_eventTargetData;
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
