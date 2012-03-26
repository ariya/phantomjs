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
#include "IDBTransactionBackendImpl.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBBackingStore.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseException.h"
#include "IDBTransactionCoordinator.h"

namespace WebCore {

PassRefPtr<IDBTransactionBackendImpl> IDBTransactionBackendImpl::create(DOMStringList* objectStores, unsigned short mode, IDBDatabaseBackendImpl* database)
{
    return adoptRef(new IDBTransactionBackendImpl(objectStores, mode, database));
}

IDBTransactionBackendImpl::IDBTransactionBackendImpl(DOMStringList* objectStores, unsigned short mode, IDBDatabaseBackendImpl* database)
    : m_objectStoreNames(objectStores)
    , m_mode(mode)
    , m_state(Unused)
    , m_database(database)
    , m_transaction(database->backingStore()->createTransaction())
    , m_taskTimer(this, &IDBTransactionBackendImpl::taskTimerFired)
    , m_taskEventTimer(this, &IDBTransactionBackendImpl::taskEventTimerFired)
    , m_pendingEvents(0)
{
    ASSERT(m_objectStoreNames);
    m_database->transactionCoordinator()->didCreateTransaction(this);
}

IDBTransactionBackendImpl::~IDBTransactionBackendImpl()
{
    // It shouldn't be possible for this object to get deleted until it's either complete or aborted.
    ASSERT(m_state == Finished);
}

PassRefPtr<IDBObjectStoreBackendInterface> IDBTransactionBackendImpl::objectStore(const String& name, ExceptionCode& ec)
{
    if (m_state == Finished) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return 0;
    }

    // Does a linear search, but it really shouldn't be that slow in practice.
    if (!m_objectStoreNames->isEmpty() && !m_objectStoreNames->contains(name)) {
        ec = IDBDatabaseException::NOT_FOUND_ERR;
        return 0;
    }

    RefPtr<IDBObjectStoreBackendInterface> objectStore = m_database->objectStore(name);
    // FIXME: This is only necessary right now beacuse a setVersion transaction could modify things
    //        between its creation (where another check occurs) and the .objectStore call.
    //        There's a bug to make this impossible in the spec. When we make it impossible here, we
    //        can remove this check.
    if (!objectStore) {
        ec = IDBDatabaseException::NOT_FOUND_ERR;
        return 0;
    }
    return objectStore.release();
}

bool IDBTransactionBackendImpl::scheduleTask(PassOwnPtr<ScriptExecutionContext::Task> task, PassOwnPtr<ScriptExecutionContext::Task> abortTask)
{
    if (m_state == Finished)
        return false;

    m_taskQueue.append(task);
    if (abortTask)
        m_abortTaskQueue.prepend(abortTask);

    if (m_state == Unused)
        start();

    return true;
}

void IDBTransactionBackendImpl::abort()
{
    if (m_state == Finished)
        return;

    // The last reference to this object may be released while performing the
    // abort steps below. We therefore take a self reference to keep ourselves
    // alive while executing this method.
    RefPtr<IDBTransactionBackendImpl> self(this);

    m_state = Finished;
    m_taskTimer.stop();
    m_taskEventTimer.stop();
    m_transaction->rollback();

    // Run the abort tasks, if any.
    while (!m_abortTaskQueue.isEmpty()) {
        OwnPtr<ScriptExecutionContext::Task> task(m_abortTaskQueue.first().release());
        m_abortTaskQueue.removeFirst();
        task->performTask(0);
    }

    m_callbacks->onAbort();
    m_database->transactionCoordinator()->didFinishTransaction(this);
    ASSERT(!m_database->transactionCoordinator()->isActive(this));
    m_database = 0;
}

void IDBTransactionBackendImpl::didCompleteTaskEvents()
{
    if (m_state == Finished)
        return;

    ASSERT(m_state == Running);
    ASSERT(m_pendingEvents);
    m_pendingEvents--;

    if (!m_taskEventTimer.isActive())
        m_taskEventTimer.startOneShot(0);
}

void IDBTransactionBackendImpl::run()
{
    ASSERT(m_state == StartPending || m_state == Running);
    ASSERT(!m_taskTimer.isActive());

    m_taskTimer.startOneShot(0);
}

void IDBTransactionBackendImpl::start()
{
    ASSERT(m_state == Unused);

    m_state = StartPending;
    m_database->transactionCoordinator()->didStartTransaction(this);
}

void IDBTransactionBackendImpl::commit()
{
    // The last reference to this object may be released while performing the
    // commit steps below. We therefore take a self reference to keep ourselves
    // alive while executing this method.
    RefPtr<IDBTransactionBackendImpl> self(this);
    ASSERT(m_state == Running);

    m_state = Finished;
    m_transaction->commit();
    m_callbacks->onComplete();
    m_database->transactionCoordinator()->didFinishTransaction(this);
    m_database = 0;
}

void IDBTransactionBackendImpl::taskTimerFired(Timer<IDBTransactionBackendImpl>*)
{
    ASSERT(!m_taskQueue.isEmpty());

    if (m_state == StartPending) {
        m_transaction->begin();
        m_state = Running;
    }

    TaskQueue queue;
    queue.swap(m_taskQueue);
    while (!queue.isEmpty() && m_state != Finished) {
        ASSERT(m_state == Running);
        OwnPtr<ScriptExecutionContext::Task> task(queue.first().release());
        queue.removeFirst();
        m_pendingEvents++;
        task->performTask(0);
    }
}

void IDBTransactionBackendImpl::taskEventTimerFired(Timer<IDBTransactionBackendImpl>*)
{
    ASSERT(m_state == Running);

    if (!m_pendingEvents && m_taskQueue.isEmpty()) {
        // The last task event has completed and the task
        // queue is empty. Commit the transaction.
        commit();
        return;
    }

    // We are still waiting for other events to complete. However,
    // the task queue is non-empty and the timer is inactive.
    // We can therfore schedule the timer again.
    if (!m_taskQueue.isEmpty() && !m_taskTimer.isActive())
        m_taskTimer.startOneShot(0);
}

};

#endif // ENABLE(INDEXED_DATABASE)
