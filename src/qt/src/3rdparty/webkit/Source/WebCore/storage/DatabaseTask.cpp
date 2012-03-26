/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#if ENABLE(DATABASE)

#include "Database.h"
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
    if (!m_taskCompleted)
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

DatabaseTask::DatabaseTask(Database* database, DatabaseTaskSynchronizer* synchronizer)
    : m_database(database)
    , m_synchronizer(synchronizer)
#ifndef NDEBUG
    , m_complete(false)
#endif
{
}

DatabaseTask::~DatabaseTask()
{
    ASSERT(m_complete || !m_synchronizer);
}

void DatabaseTask::performTask()
{
    // Database tasks are meant to be used only once, so make sure this one hasn't been performed before.
    ASSERT(!m_complete);

    LOG(StorageAPI, "Performing %s %p\n", debugTaskName(), this);

    m_database->resetAuthorizer();
    doPerformTask();

    if (m_synchronizer)
        m_synchronizer->taskCompleted();

#ifndef NDEBUG
    m_complete = true;
#endif
}

// *** DatabaseOpenTask ***
// Opens the database file and verifies the version matches the expected version.

Database::DatabaseOpenTask::DatabaseOpenTask(Database* database, bool setVersionInNewDatabase, DatabaseTaskSynchronizer* synchronizer, ExceptionCode& code, bool& success)
    : DatabaseTask(database, synchronizer)
    , m_setVersionInNewDatabase(setVersionInNewDatabase)
    , m_code(code)
    , m_success(success)
{
    ASSERT(synchronizer); // A task with output parameters is supposed to be synchronous.
}

void Database::DatabaseOpenTask::doPerformTask()
{
    m_success = database()->performOpenAndVerify(m_setVersionInNewDatabase, m_code);
}

#ifndef NDEBUG
const char* Database::DatabaseOpenTask::debugTaskName() const
{
    return "DatabaseOpenTask";
}
#endif

// *** DatabaseCloseTask ***
// Closes the database.

Database::DatabaseCloseTask::DatabaseCloseTask(Database* database, DatabaseTaskSynchronizer* synchronizer)
    : DatabaseTask(database, synchronizer)
{
}

void Database::DatabaseCloseTask::doPerformTask()
{
    database()->close();
}

#ifndef NDEBUG
const char* Database::DatabaseCloseTask::debugTaskName() const
{
    return "DatabaseCloseTask";
}
#endif

// *** DatabaseTransactionTask ***
// Starts a transaction that will report its results via a callback.

Database::DatabaseTransactionTask::DatabaseTransactionTask(PassRefPtr<SQLTransaction> transaction)
    : DatabaseTask(transaction->database(), 0)
    , m_transaction(transaction)
{
}

void Database::DatabaseTransactionTask::doPerformTask()
{
    if (m_transaction->performNextStep())
        m_transaction->database()->inProgressTransactionCompleted();
}

#ifndef NDEBUG
const char* Database::DatabaseTransactionTask::debugTaskName() const
{
    return "DatabaseTransactionTask";
}
#endif

// *** DatabaseTableNamesTask ***
// Retrieves a list of all tables in the database - for WebInspector support.

Database::DatabaseTableNamesTask::DatabaseTableNamesTask(Database* database, DatabaseTaskSynchronizer* synchronizer, Vector<String>& names)
    : DatabaseTask(database, synchronizer)
    , m_tableNames(names)
{
    ASSERT(synchronizer); // A task with output parameters is supposed to be synchronous.
}

void Database::DatabaseTableNamesTask::doPerformTask()
{
    m_tableNames = database()->performGetTableNames();
}

#ifndef NDEBUG
const char* Database::DatabaseTableNamesTask::debugTaskName() const
{
    return "DatabaseTableNamesTask";
}
#endif

} // namespace WebCore

#endif
