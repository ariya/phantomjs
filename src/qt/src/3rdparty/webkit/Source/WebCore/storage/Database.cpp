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
#include "Database.h"

#if ENABLE(DATABASE)
#include "ChangeVersionWrapper.h"
#include "DatabaseCallback.h"
#include "DatabaseTask.h"
#include "DatabaseThread.h"
#include "DatabaseTracker.h"
#include "Document.h"
#include "InspectorDatabaseInstrumentation.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "Page.h"
#include "SQLTransactionCallback.h"
#include "SQLTransactionClient.h"
#include "SQLTransactionCoordinator.h"
#include "SQLTransactionErrorCallback.h"
#include "SQLiteStatement.h"
#include "ScriptController.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "VoidCallback.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

#if USE(JSC)
#include "JSDOMWindow.h"
#endif

namespace WebCore {

class DatabaseCreationCallbackTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<DatabaseCreationCallbackTask> create(PassRefPtr<Database> database, PassRefPtr<DatabaseCallback> creationCallback)
    {
        return adoptPtr(new DatabaseCreationCallbackTask(database, creationCallback));
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        m_creationCallback->handleEvent(m_database.get());
    }

private:
    DatabaseCreationCallbackTask(PassRefPtr<Database> database, PassRefPtr<DatabaseCallback> callback)
        : m_database(database)
        , m_creationCallback(callback)
    {
    }

    RefPtr<Database> m_database;
    RefPtr<DatabaseCallback> m_creationCallback;
};

PassRefPtr<Database> Database::openDatabase(ScriptExecutionContext* context, const String& name,
                                            const String& expectedVersion, const String& displayName,
                                            unsigned long estimatedSize, PassRefPtr<DatabaseCallback> creationCallback,
                                            ExceptionCode& e)
{
    if (!DatabaseTracker::tracker().canEstablishDatabase(context, name, displayName, estimatedSize)) {
        LOG(StorageAPI, "Database %s for origin %s not allowed to be established", name.ascii().data(), context->securityOrigin()->toString().ascii().data());
        return 0;
    }

    RefPtr<Database> database = adoptRef(new Database(context, name, expectedVersion, displayName, estimatedSize));

    if (!database->openAndVerifyVersion(!creationCallback, e)) {
        LOG(StorageAPI, "Failed to open and verify version (expected %s) of database %s", expectedVersion.ascii().data(), database->databaseDebugName().ascii().data());
        DatabaseTracker::tracker().removeOpenDatabase(database.get());
        return 0;
    }

    DatabaseTracker::tracker().setDatabaseDetails(context->securityOrigin(), name, displayName, estimatedSize);

    context->setHasOpenDatabases();

    InspectorInstrumentation::didOpenDatabase(context, database, context->securityOrigin()->host(), name, expectedVersion);

    // If it's a new database and a creation callback was provided, reset the expected
    // version to "" and schedule the creation callback. Because of some subtle String
    // implementation issues, we have to reset m_expectedVersion here instead of doing
    // it inside performOpenAndVerify() which is run on the DB thread.
    if (database->isNew() && creationCallback.get()) {
        database->m_expectedVersion = "";
        LOG(StorageAPI, "Scheduling DatabaseCreationCallbackTask for database %p\n", database.get());
        database->m_scriptExecutionContext->postTask(DatabaseCreationCallbackTask::create(database, creationCallback));
    }

    return database;
}

Database::Database(ScriptExecutionContext* context, const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize)
    : AbstractDatabase(context, name, expectedVersion, displayName, estimatedSize)
    , m_transactionInProgress(false)
    , m_isTransactionQueueEnabled(true)
    , m_deleted(false)
{
    m_databaseThreadSecurityOrigin = m_contextThreadSecurityOrigin->threadsafeCopy();

    ScriptController::initializeThreading();
    ASSERT(m_scriptExecutionContext->databaseThread());
}

class DerefContextTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<DerefContextTask> create(PassRefPtr<ScriptExecutionContext> context)
    {
        return adoptPtr(new DerefContextTask(context));
    }

    virtual void performTask(ScriptExecutionContext* context)
    {
        ASSERT_UNUSED(context, context == m_context);
        m_context.clear();
    }

    virtual bool isCleanupTask() const { return true; }

private:
    DerefContextTask(PassRefPtr<ScriptExecutionContext> context)
        : m_context(context)
    {
    }
    
    RefPtr<ScriptExecutionContext> m_context;
};

Database::~Database()
{
    // The reference to the ScriptExecutionContext needs to be cleared on the JavaScript thread.  If we're on that thread already, we can just let the RefPtr's destruction do the dereffing.
    if (!m_scriptExecutionContext->isContextThread()) {
        // Grab a pointer to the script execution here because we're releasing it when we pass it to
        // DerefContextTask::create.
        ScriptExecutionContext* scriptExecutionContext = m_scriptExecutionContext.get();
        
        scriptExecutionContext->postTask(DerefContextTask::create(m_scriptExecutionContext.release()));
    }
}

String Database::version() const
{
    if (m_deleted)
        return String();
    return AbstractDatabase::version();
}

bool Database::openAndVerifyVersion(bool setVersionInNewDatabase, ExceptionCode& e)
{
    DatabaseTaskSynchronizer synchronizer;
    if (!m_scriptExecutionContext->databaseThread() || m_scriptExecutionContext->databaseThread()->terminationRequested(&synchronizer))
        return false;

    bool success = false;
    OwnPtr<DatabaseOpenTask> task = DatabaseOpenTask::create(this, setVersionInNewDatabase, &synchronizer, e, success);
    m_scriptExecutionContext->databaseThread()->scheduleImmediateTask(task.release());
    synchronizer.waitForTaskCompletion();

    return success;
}

void Database::markAsDeletedAndClose()
{
    if (m_deleted || !m_scriptExecutionContext->databaseThread())
        return;

    LOG(StorageAPI, "Marking %s (%p) as deleted", stringIdentifier().ascii().data(), this);
    m_deleted = true;

    DatabaseTaskSynchronizer synchronizer;
    if (m_scriptExecutionContext->databaseThread()->terminationRequested(&synchronizer)) {
        LOG(StorageAPI, "Database handle %p is on a terminated DatabaseThread, cannot be marked for normal closure\n", this);
        return;
    }

    OwnPtr<DatabaseCloseTask> task = DatabaseCloseTask::create(this, &synchronizer);
    m_scriptExecutionContext->databaseThread()->scheduleImmediateTask(task.release());
    synchronizer.waitForTaskCompletion();
}

void Database::close()
{
    ASSERT(m_scriptExecutionContext->databaseThread());
    ASSERT(currentThread() == m_scriptExecutionContext->databaseThread()->getThreadID());

    {
        MutexLocker locker(m_transactionInProgressMutex);
        m_isTransactionQueueEnabled = false;
        m_transactionInProgress = false;
    }

    closeDatabase();

    // Must ref() before calling databaseThread()->recordDatabaseClosed().
    RefPtr<Database> protect = this;
    m_scriptExecutionContext->databaseThread()->recordDatabaseClosed(this);
    m_scriptExecutionContext->databaseThread()->unscheduleDatabaseTasks(this);
    DatabaseTracker::tracker().removeOpenDatabase(this);
}

void Database::closeImmediately()
{
    DatabaseThread* databaseThread = scriptExecutionContext()->databaseThread();
    if (databaseThread && !databaseThread->terminationRequested() && opened())
        databaseThread->scheduleImmediateTask(DatabaseCloseTask::create(this, 0));
}

unsigned long long Database::maximumSize() const
{
    return DatabaseTracker::tracker().getMaxSizeForDatabase(this);
}

bool Database::performOpenAndVerify(bool setVersionInNewDatabase, ExceptionCode& e)
{
    if (AbstractDatabase::performOpenAndVerify(setVersionInNewDatabase, e)) {
        if (m_scriptExecutionContext->databaseThread())
            m_scriptExecutionContext->databaseThread()->recordDatabaseOpen(this);

        return true;
    }

    return false;
}

void Database::changeVersion(const String& oldVersion, const String& newVersion,
                             PassRefPtr<SQLTransactionCallback> callback, PassRefPtr<SQLTransactionErrorCallback> errorCallback,
                             PassRefPtr<VoidCallback> successCallback)
{
    RefPtr<SQLTransaction> transaction =
        SQLTransaction::create(this, callback, errorCallback, successCallback, ChangeVersionWrapper::create(oldVersion, newVersion));
    MutexLocker locker(m_transactionInProgressMutex);
    m_transactionQueue.append(transaction.release());
    if (!m_transactionInProgress)
        scheduleTransaction();
}

void Database::transaction(PassRefPtr<SQLTransactionCallback> callback, PassRefPtr<SQLTransactionErrorCallback> errorCallback, PassRefPtr<VoidCallback> successCallback)
{
    runTransaction(callback, errorCallback, successCallback, false);
}

void Database::readTransaction(PassRefPtr<SQLTransactionCallback> callback, PassRefPtr<SQLTransactionErrorCallback> errorCallback, PassRefPtr<VoidCallback> successCallback)
{
    runTransaction(callback, errorCallback, successCallback, true);
}

void Database::runTransaction(PassRefPtr<SQLTransactionCallback> callback, PassRefPtr<SQLTransactionErrorCallback> errorCallback,
                              PassRefPtr<VoidCallback> successCallback, bool readOnly)
{
    RefPtr<SQLTransaction> transaction =
        SQLTransaction::create(this, callback, errorCallback, successCallback, 0, readOnly);
    MutexLocker locker(m_transactionInProgressMutex);
    m_transactionQueue.append(transaction.release());
    if (!m_transactionInProgress)
        scheduleTransaction();
}

void Database::inProgressTransactionCompleted()
{
    MutexLocker locker(m_transactionInProgressMutex);
    m_transactionInProgress = false;
    scheduleTransaction();
}

void Database::scheduleTransaction()
{
    ASSERT(!m_transactionInProgressMutex.tryLock()); // Locked by caller.
    RefPtr<SQLTransaction> transaction;

    if (m_isTransactionQueueEnabled && !m_transactionQueue.isEmpty()) {
        transaction = m_transactionQueue.takeFirst();
    }

    if (transaction && m_scriptExecutionContext->databaseThread()) {
        OwnPtr<DatabaseTransactionTask> task = DatabaseTransactionTask::create(transaction);
        LOG(StorageAPI, "Scheduling DatabaseTransactionTask %p for transaction %p\n", task.get(), task->transaction());
        m_transactionInProgress = true;
        m_scriptExecutionContext->databaseThread()->scheduleTask(task.release());
    } else
        m_transactionInProgress = false;
}

void Database::scheduleTransactionStep(SQLTransaction* transaction, bool immediately)
{
    if (!m_scriptExecutionContext->databaseThread())
        return;

    OwnPtr<DatabaseTransactionTask> task = DatabaseTransactionTask::create(transaction);
    LOG(StorageAPI, "Scheduling DatabaseTransactionTask %p for the transaction step\n", task.get());
    if (immediately)
        m_scriptExecutionContext->databaseThread()->scheduleImmediateTask(task.release());
    else
        m_scriptExecutionContext->databaseThread()->scheduleTask(task.release());
}

class DeliverPendingCallbackTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<DeliverPendingCallbackTask> create(PassRefPtr<SQLTransaction> transaction)
    {
        return adoptPtr(new DeliverPendingCallbackTask(transaction));
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        m_transaction->performPendingCallback();
    }

private:
    DeliverPendingCallbackTask(PassRefPtr<SQLTransaction> transaction)
        : m_transaction(transaction)
    {
    }

    RefPtr<SQLTransaction> m_transaction;
};

void Database::scheduleTransactionCallback(SQLTransaction* transaction)
{
    m_scriptExecutionContext->postTask(DeliverPendingCallbackTask::create(transaction));
}

Vector<String> Database::performGetTableNames()
{
    disableAuthorizer();

    SQLiteStatement statement(sqliteDatabase(), "SELECT name FROM sqlite_master WHERE type='table';");
    if (statement.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to retrieve list of tables for database %s", databaseDebugName().ascii().data());
        enableAuthorizer();
        return Vector<String>();
    }

    Vector<String> tableNames;
    int result;
    while ((result = statement.step()) == SQLResultRow) {
        String name = statement.getColumnText(0);
        if (name != databaseInfoTableName())
            tableNames.append(name);
    }

    enableAuthorizer();

    if (result != SQLResultDone) {
        LOG_ERROR("Error getting tables for database %s", databaseDebugName().ascii().data());
        return Vector<String>();
    }

    return tableNames;
}

SQLTransactionClient* Database::transactionClient() const
{
    return m_scriptExecutionContext->databaseThread()->transactionClient();
}

SQLTransactionCoordinator* Database::transactionCoordinator() const
{
    return m_scriptExecutionContext->databaseThread()->transactionCoordinator();
}

Vector<String> Database::tableNames()
{
    // FIXME: Not using threadsafeCopy on these strings looks ok since threads take strict turns
    // in dealing with them. However, if the code changes, this may not be true anymore.
    Vector<String> result;
    DatabaseTaskSynchronizer synchronizer;
    if (!m_scriptExecutionContext->databaseThread() || m_scriptExecutionContext->databaseThread()->terminationRequested(&synchronizer))
        return result;

    OwnPtr<DatabaseTableNamesTask> task = DatabaseTableNamesTask::create(this, &synchronizer, result);
    m_scriptExecutionContext->databaseThread()->scheduleImmediateTask(task.release());
    synchronizer.waitForTaskCompletion();

    return result;
}

SecurityOrigin* Database::securityOrigin() const
{
    if (m_scriptExecutionContext->isContextThread())
        return m_contextThreadSecurityOrigin.get();
    if (currentThread() == m_scriptExecutionContext->databaseThread()->getThreadID())
        return m_databaseThreadSecurityOrigin.get();
    return 0;
}

} // namespace WebCore

#endif // ENABLE(DATABASE)
