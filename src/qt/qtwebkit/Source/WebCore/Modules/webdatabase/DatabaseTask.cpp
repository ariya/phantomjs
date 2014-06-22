/*
 * Copyright (C) 2007, 2008, 2013 Apple Inc. All rights reserved.
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
#include "DatabaseTask.h"

#if ENABLE(SQL_DATABASE)

#include "Database.h"
#include "DatabaseBackend.h"
#include "Logging.h"

namespace WebCore {

DatabaseTaskSynchronizer::DatabaseTaskSynchronizer()
    : m_taskCompleted(false)
#ifndef NDEBUG
    , m_hasCheckedForTermination(false)
#endif
{
}

void DatabaseTaskSynchronizer::waitForTaskCompletion()
{
    m_synchronousMutex.lock();
    while (!m_taskCompleted)
        m_synchronousCondition.wait(m_synchronousMutex);
    m_synchronousMutex.unlock();
}

void DatabaseTaskSynchronizer::taskCompleted()
{
    m_synchronousMutex.lock();
    m_taskCompleted = true;
    m_synchronousCondition.signal();
    m_synchronousMutex.unlock();
}

DatabaseTask::DatabaseTask(DatabaseBackend* database, DatabaseTaskSynchronizer* synchronizer)
    : m_database(database)
    , m_synchronizer(synchronizer)
#if !LOG_DISABLED
    , m_complete(false)
#endif
{
}

DatabaseTask::~DatabaseTask()
{
#if !LOG_DISABLED
    ASSERT(m_complete || !m_synchronizer);
#endif
}

void DatabaseTask::performTask()
{
    // Database tasks are meant to be used only once, so make sure this one hasn't been performed before.
#if !LOG_DISABLED
    ASSERT(!m_complete);
#endif

    LOG(StorageAPI, "Performing %s %p\n", debugTaskName(), this);

    m_database->resetAuthorizer();
    doPerformTask();

    if (m_synchronizer)
        m_synchronizer->taskCompleted();

#if !LOG_DISABLED
    m_complete = true;
#endif
}

// *** DatabaseOpenTask ***
// Opens the database file and verifies the version matches the expected version.

DatabaseBackend::DatabaseOpenTask::DatabaseOpenTask(DatabaseBackend* database, bool setVersionInNewDatabase, DatabaseTaskSynchronizer* synchronizer, DatabaseError& error, String& errorMessage, bool& success)
    : DatabaseTask(database, synchronizer)
    , m_setVersionInNewDatabase(setVersionInNewDatabase)
    , m_error(error)
    , m_errorMessage(errorMessage)
    , m_success(success)
{
    ASSERT(synchronizer); // A task with output parameters is supposed to be synchronous.
}

void DatabaseBackend::DatabaseOpenTask::doPerformTask()
{
    String errorMessage;
    m_success = database()->performOpenAndVerify(m_setVersionInNewDatabase, m_error, errorMessage);
    if (!m_success)
        m_errorMessage = errorMessage.isolatedCopy();
}

#if !LOG_DISABLED
const char* DatabaseBackend::DatabaseOpenTask::debugTaskName() const
{
    return "DatabaseOpenTask";
}
#endif

// *** DatabaseCloseTask ***
// Closes the database.

DatabaseBackend::DatabaseCloseTask::DatabaseCloseTask(DatabaseBackend* database, DatabaseTaskSynchronizer* synchronizer)
    : DatabaseTask(database, synchronizer)
{
}

void DatabaseBackend::DatabaseCloseTask::doPerformTask()
{
    Database::from(database())->close();
}

#if !LOG_DISABLED
const char* DatabaseBackend::DatabaseCloseTask::debugTaskName() const
{
    return "DatabaseCloseTask";
}
#endif

// *** DatabaseTransactionTask ***
// Starts a transaction that will report its results via a callback.

DatabaseBackend::DatabaseTransactionTask::DatabaseTransactionTask(PassRefPtr<SQLTransactionBackend> transaction)
    : DatabaseTask(Database::from(transaction->database()), 0)
    , m_transaction(transaction)
    , m_didPerformTask(false)
{
}

DatabaseBackend::DatabaseTransactionTask::~DatabaseTransactionTask()
{
    // If the task is being destructed without the transaction ever being run,
    // then we must either have an error or an interruption. Give the
    // transaction a chance to clean up since it may not have been able to
    // run to its clean up state.

    // Transaction phase 2 cleanup. See comment on "What happens if a
    // transaction is interrupted?" at the top of SQLTransactionBackend.cpp.

    if (!m_didPerformTask)
        m_transaction->notifyDatabaseThreadIsShuttingDown();
}

void DatabaseBackend::DatabaseTransactionTask::doPerformTask()
{
    m_transaction->performNextStep();
    m_didPerformTask = true;
}

#if !LOG_DISABLED
const char* DatabaseBackend::DatabaseTransactionTask::debugTaskName() const
{
    return "DatabaseTransactionTask";
}
#endif

// *** DatabaseTableNamesTask ***
// Retrieves a list of all tables in the database - for WebInspector support.

DatabaseBackend::DatabaseTableNamesTask::DatabaseTableNamesTask(DatabaseBackend* database, DatabaseTaskSynchronizer* synchronizer, Vector<String>& names)
    : DatabaseTask(database, synchronizer)
    , m_tableNames(names)
{
    ASSERT(synchronizer); // A task with output parameters is supposed to be synchronous.
}

void DatabaseBackend::DatabaseTableNamesTask::doPerformTask()
{
    m_tableNames = Database::from(database())->performGetTableNames();
}

#if !LOG_DISABLED
const char* DatabaseBackend::DatabaseTableNamesTask::debugTaskName() const
{
    return "DatabaseTableNamesTask";
}
#endif

} // namespace WebCore

#endif
