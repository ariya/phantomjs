/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DatabaseBackend.h"

#if ENABLE(SQL_DATABASE)

#include "ChangeVersionData.h"
#include "ChangeVersionWrapper.h"
#include "DatabaseBackendContext.h"
#include "DatabaseTask.h"
#include "DatabaseThread.h"
#include "DatabaseTracker.h"
#include "Logging.h"
#include "SQLTransaction.h"
#include "SQLTransactionBackend.h"
#include "SQLTransactionClient.h"
#include "SQLTransactionCoordinator.h"

namespace WebCore {

DatabaseBackend::DatabaseBackend(PassRefPtr<DatabaseBackendContext> databaseContext, const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize)
    : DatabaseBackendBase(databaseContext, name, expectedVersion, displayName, estimatedSize, DatabaseType::Async)
    , m_transactionInProgress(false)
    , m_isTransactionQueueEnabled(true)
{
}

bool DatabaseBackend::openAndVerifyVersion(bool setVersionInNewDatabase, DatabaseError& error, String& errorMessage)
{
    DatabaseTaskSynchronizer synchronizer;
    if (!databaseContext()->databaseThread() || databaseContext()->databaseThread()->terminationRequested(&synchronizer))
        return false;

    bool success = false;
    OwnPtr<DatabaseOpenTask> task = DatabaseOpenTask::create(this, setVersionInNewDatabase, &synchronizer, error, errorMessage, success);
    databaseContext()->databaseThread()->scheduleImmediateTask(task.release());
    synchronizer.waitForTaskCompletion();

    return success;
}

bool DatabaseBackend::performOpenAndVerify(bool setVersionInNewDatabase, DatabaseError& error, String& errorMessage)
{
    if (DatabaseBackendBase::performOpenAndVerify(setVersionInNewDatabase, error, errorMessage)) {
        if (databaseContext()->databaseThread())
            databaseContext()->databaseThread()->recordDatabaseOpen(this);

        return true;
    }

    return false;
}

void DatabaseBackend::close()
{
    ASSERT(databaseContext()->databaseThread());
    ASSERT(currentThread() == databaseContext()->databaseThread()->getThreadID());

    {
        MutexLocker locker(m_transactionInProgressMutex);

        // Clean up transactions that have not been scheduled yet:
        // Transaction phase 1 cleanup. See comment on "What happens if a
        // transaction is interrupted?" at the top of SQLTransactionBackend.cpp.
        RefPtr<SQLTransactionBackend> transaction;
        while (!m_transactionQueue.isEmpty()) {
            transaction = m_transactionQueue.takeFirst();
            transaction->notifyDatabaseThreadIsShuttingDown();
        }

        m_isTransactionQueueEnabled = false;
        m_transactionInProgress = false;
    }

    closeDatabase();

    // DatabaseThread keeps databases alive by referencing them in its
    // m_openDatabaseSet. DatabaseThread::recordDatabaseClose() will remove
    // this database from that set (which effectively deref's it). We hold on
    // to it with a local pointer here for a liitle longer, so that we can
    // unschedule any DatabaseTasks that refer to it before the database gets
    // deleted.
    RefPtr<DatabaseBackend> protect = this;
    databaseContext()->databaseThread()->recordDatabaseClosed(this);
    databaseContext()->databaseThread()->unscheduleDatabaseTasks(this);
}

PassRefPtr<SQLTransactionBackend> DatabaseBackend::runTransaction(PassRefPtr<SQLTransaction> transaction,
    bool readOnly, const ChangeVersionData* data)
{
    MutexLocker locker(m_transactionInProgressMutex);
    if (!m_isTransactionQueueEnabled)
        return 0;

    RefPtr<SQLTransactionWrapper> wrapper;
    if (data)
        wrapper = ChangeVersionWrapper::create(data->oldVersion(), data->newVersion());

    RefPtr<SQLTransactionBackend> transactionBackend = SQLTransactionBackend::create(this, transaction, wrapper, readOnly);
    m_transactionQueue.append(transactionBackend);
    if (!m_transactionInProgress)
        scheduleTransaction();

    return transactionBackend;
}

void DatabaseBackend::inProgressTransactionCompleted()
{
    MutexLocker locker(m_transactionInProgressMutex);
    m_transactionInProgress = false;
    scheduleTransaction();
}

void DatabaseBackend::scheduleTransaction()
{
    ASSERT(!m_transactionInProgressMutex.tryLock()); // Locked by caller.
    RefPtr<SQLTransactionBackend> transaction;

    if (m_isTransactionQueueEnabled && !m_transactionQueue.isEmpty())
        transaction = m_transactionQueue.takeFirst();

    if (transaction && databaseContext()->databaseThread()) {
        OwnPtr<DatabaseTransactionTask> task = DatabaseTransactionTask::create(transaction);
        LOG(StorageAPI, "Scheduling DatabaseTransactionTask %p for transaction %p\n", task.get(), task->transaction());
        m_transactionInProgress = true;
        databaseContext()->databaseThread()->scheduleTask(task.release());
    } else
        m_transactionInProgress = false;
}

void DatabaseBackend::scheduleTransactionStep(SQLTransactionBackend* transaction)
{
    if (!databaseContext()->databaseThread())
        return;

    OwnPtr<DatabaseTransactionTask> task = DatabaseTransactionTask::create(transaction);
    LOG(StorageAPI, "Scheduling DatabaseTransactionTask %p for the transaction step\n", task.get());
    databaseContext()->databaseThread()->scheduleTask(task.release());
}

SQLTransactionClient* DatabaseBackend::transactionClient() const
{
    return databaseContext()->databaseThread()->transactionClient();
}

SQLTransactionCoordinator* DatabaseBackend::transactionCoordinator() const
{
    return databaseContext()->databaseThread()->transactionCoordinator();
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
