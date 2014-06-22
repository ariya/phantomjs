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
#include "IDBCursorBackendImpl.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseCallbacks.h"
#include "IDBDatabaseException.h"
#include "IDBTracing.h"
#include "IDBTransactionCoordinator.h"

namespace WebCore {

PassRefPtr<IDBTransactionBackendImpl> IDBTransactionBackendImpl::create(int64_t id, PassRefPtr<IDBDatabaseCallbacks> callbacks, const Vector<int64_t>& objectStoreIds, IndexedDB::TransactionMode mode, IDBDatabaseBackendImpl* database)
{
    HashSet<int64_t> objectStoreHashSet;
    for (size_t i = 0; i < objectStoreIds.size(); ++i)
        objectStoreHashSet.add(objectStoreIds[i]);

    return adoptRef(new IDBTransactionBackendImpl(id, callbacks, objectStoreHashSet, mode, database));
}

IDBTransactionBackendImpl::IDBTransactionBackendImpl(int64_t id, PassRefPtr<IDBDatabaseCallbacks> callbacks, const HashSet<int64_t>& objectStoreIds, IndexedDB::TransactionMode mode, IDBDatabaseBackendImpl* database)
    : m_id(id)
    , m_objectStoreIds(objectStoreIds)
    , m_mode(mode)
    , m_state(Unused)
    , m_commitPending(false)
    , m_callbacks(callbacks)
    , m_database(database)
    , m_transaction(database->backingStore().get())
    , m_taskTimer(this, &IDBTransactionBackendImpl::taskTimerFired)
    , m_pendingPreemptiveEvents(0)
{
    // We pass a reference of this object before it can be adopted.
    relaxAdoptionRequirement();

    m_database->transactionCoordinator()->didCreateTransaction(this);
}

IDBTransactionBackendImpl::~IDBTransactionBackendImpl()
{
    // It shouldn't be possible for this object to get deleted until it's either complete or aborted.
    ASSERT(m_state == Finished);
}

void IDBTransactionBackendImpl::scheduleTask(IDBDatabaseBackendInterface::TaskType type, PassOwnPtr<Operation> task, PassOwnPtr<Operation> abortTask)
{
    if (m_state == Finished)
        return;

    if (type == IDBDatabaseBackendInterface::NormalTask)
        m_taskQueue.append(task);
    else
        m_preemptiveTaskQueue.append(task);

    if (abortTask)
        m_abortTaskQueue.prepend(abortTask);

    if (m_state == Unused)
        start();
    else if (m_state == Running && !m_taskTimer.isActive())
        m_taskTimer.startOneShot(0);
}

void IDBTransactionBackendImpl::abort()
{
    abort(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error (unknown cause)"));
}

void IDBTransactionBackendImpl::abort(PassRefPtr<IDBDatabaseError> error)
{
    IDB_TRACE("IDBTransactionBackendImpl::abort");
    if (m_state == Finished)
        return;

    bool wasRunning = m_state == Running;

    // The last reference to this object may be released while performing the
    // abort steps below. We therefore take a self reference to keep ourselves
    // alive while executing this method.
    RefPtr<IDBTransactionBackendImpl> protect(this);

    m_state = Finished;
    m_taskTimer.stop();

    if (wasRunning)
        m_transaction.rollback();

    // Run the abort tasks, if any.
    while (!m_abortTaskQueue.isEmpty()) {
        OwnPtr<Operation> task(m_abortTaskQueue.takeFirst());
        task->perform(0);
    }

    // Backing store resources (held via cursors) must be released before script callbacks
    // are fired, as the script callbacks may release references and allow the backing store
    // itself to be released, and order is critical.
    closeOpenCursors();
    m_transaction.reset();

    // Transactions must also be marked as completed before the front-end is notified, as
    // the transaction completion unblocks operations like closing connections.
    m_database->transactionCoordinator()->didFinishTransaction(this);
    ASSERT(!m_database->transactionCoordinator()->isActive(this));
    m_database->transactionFinished(this);

    if (m_callbacks)
        m_callbacks->onAbort(m_id, error);

    m_database->transactionFinishedAndAbortFired(this);

    m_database = 0;
}

bool IDBTransactionBackendImpl::isTaskQueueEmpty() const
{
    return m_preemptiveTaskQueue.isEmpty() && m_taskQueue.isEmpty();
}

bool IDBTransactionBackendImpl::hasPendingTasks() const
{
    return m_pendingPreemptiveEvents || !isTaskQueueEmpty();
}

void IDBTransactionBackendImpl::registerOpenCursor(IDBCursorBackendImpl* cursor)
{
    m_openCursors.add(cursor);
}

void IDBTransactionBackendImpl::unregisterOpenCursor(IDBCursorBackendImpl* cursor)
{
    m_openCursors.remove(cursor);
}

void IDBTransactionBackendImpl::run()
{
    // TransactionCoordinator has started this transaction. Schedule a timer
    // to process the first task.
    ASSERT(m_state == StartPending || m_state == Running);
    ASSERT(!m_taskTimer.isActive());

    m_taskTimer.startOneShot(0);
}

void IDBTransactionBackendImpl::start()
{
    ASSERT(m_state == Unused);

    m_state = StartPending;
    m_database->transactionCoordinator()->didStartTransaction(this);
    m_database->transactionStarted(this);
}

void IDBTransactionBackendImpl::commit()
{
    IDB_TRACE("IDBTransactionBackendImpl::commit");

    // In multiprocess ports, front-end may have requested a commit but an abort has already
    // been initiated asynchronously by the back-end.
    if (m_state == Finished)
        return;

    ASSERT(m_state == Unused || m_state == Running);
    m_commitPending = true;

    // Front-end has requested a commit, but there may be tasks like createIndex which
    // are considered synchronous by the front-end but are processed asynchronously.
    if (hasPendingTasks())
        return;

    // The last reference to this object may be released while performing the
    // commit steps below. We therefore take a self reference to keep ourselves
    // alive while executing this method.
    RefPtr<IDBTransactionBackendImpl> protect(this);

    bool unused = m_state == Unused;
    m_state = Finished;

    bool committed = unused || m_transaction.commit();

    // Backing store resources (held via cursors) must be released before script callbacks
    // are fired, as the script callbacks may release references and allow the backing store
    // itself to be released, and order is critical.
    closeOpenCursors();
    m_transaction.reset();

    // Transactions must also be marked as completed before the front-end is notified, as
    // the transaction completion unblocks operations like closing connections.
    if (!unused)
        m_database->transactionCoordinator()->didFinishTransaction(this);
    m_database->transactionFinished(this);

    if (committed) {
        m_callbacks->onComplete(m_id);
        m_database->transactionFinishedAndCompleteFired(this);
    } else {
        m_callbacks->onAbort(m_id, IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error committing transaction."));
        m_database->transactionFinishedAndAbortFired(this);
    }

    m_database = 0;
}

void IDBTransactionBackendImpl::taskTimerFired(Timer<IDBTransactionBackendImpl>*)
{
    IDB_TRACE("IDBTransactionBackendImpl::taskTimerFired");
    ASSERT(!isTaskQueueEmpty());

    if (m_state == StartPending) {
        m_transaction.begin();
        m_state = Running;
    }

    // The last reference to this object may be released while performing the
    // tasks. Take take a self reference to keep this object alive so that
    // the loop termination conditions can be checked.
    RefPtr<IDBTransactionBackendImpl> protect(this);

    TaskQueue* taskQueue = m_pendingPreemptiveEvents ? &m_preemptiveTaskQueue : &m_taskQueue;
    while (!taskQueue->isEmpty() && m_state != Finished) {
        ASSERT(m_state == Running);
        OwnPtr<Operation> task(taskQueue->takeFirst());
        task->perform(this);

        // Event itself may change which queue should be processed next.
        taskQueue = m_pendingPreemptiveEvents ? &m_preemptiveTaskQueue : &m_taskQueue;
    }

    // If there are no pending tasks, we haven't already committed/aborted,
    // and the front-end requested a commit, it is now safe to do so.
    if (!hasPendingTasks() && m_state != Finished && m_commitPending)
        commit();
}

void IDBTransactionBackendImpl::closeOpenCursors()
{
    for (HashSet<IDBCursorBackendImpl*>::iterator i = m_openCursors.begin(); i != m_openCursors.end(); ++i)
        (*i)->close();
    m_openCursors.clear();
}

};

#endif // ENABLE(INDEXED_DATABASE)
