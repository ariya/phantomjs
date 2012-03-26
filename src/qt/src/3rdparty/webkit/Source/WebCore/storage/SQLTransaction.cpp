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
#include "SQLTransaction.h"

#if ENABLE(DATABASE)

#include "Database.h"
#include "DatabaseAuthorizer.h"
#include "DatabaseThread.h"
#include "Logging.h"
#include "PlatformString.h"
#include "ScriptExecutionContext.h"
#include "SQLError.h"
#include "SQLiteTransaction.h"
#include "SQLStatement.h"
#include "SQLStatementCallback.h"
#include "SQLStatementErrorCallback.h"
#include "SQLTransactionCallback.h"
#include "SQLTransactionClient.h"
#include "SQLTransactionCoordinator.h"
#include "SQLTransactionErrorCallback.h"
#include "SQLValue.h"
#include "VoidCallback.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

// There's no way of knowing exactly how much more space will be required when a statement hits the quota limit.
// For now, we'll arbitrarily choose currentQuota + 1mb.
// In the future we decide to track if a size increase wasn't enough, and ask for larger-and-larger increases until its enough.
static const int DefaultQuotaSizeIncrease = 1048576;

namespace WebCore {

PassRefPtr<SQLTransaction> SQLTransaction::create(Database* db, PassRefPtr<SQLTransactionCallback> callback, PassRefPtr<SQLTransactionErrorCallback> errorCallback,
                                                  PassRefPtr<VoidCallback> successCallback, PassRefPtr<SQLTransactionWrapper> wrapper, bool readOnly)
{
    return adoptRef(new SQLTransaction(db, callback, errorCallback, successCallback, wrapper, readOnly));
}

SQLTransaction::SQLTransaction(Database* db, PassRefPtr<SQLTransactionCallback> callback, PassRefPtr<SQLTransactionErrorCallback> errorCallback,
                               PassRefPtr<VoidCallback> successCallback, PassRefPtr<SQLTransactionWrapper> wrapper, bool readOnly)
    : m_nextStep(&SQLTransaction::acquireLock)
    , m_executeSqlAllowed(false)
    , m_database(db)
    , m_wrapper(wrapper)
    , m_callbackWrapper(callback, db->scriptExecutionContext())
    , m_successCallbackWrapper(successCallback, db->scriptExecutionContext())
    , m_errorCallbackWrapper(errorCallback, db->scriptExecutionContext())
    , m_shouldRetryCurrentStatement(false)
    , m_modifiedDatabase(false)
    , m_lockAcquired(false)
    , m_readOnly(readOnly)
{
    ASSERT(m_database);
}

SQLTransaction::~SQLTransaction()
{
    ASSERT(!m_sqliteTransaction);
}

void SQLTransaction::executeSQL(const String& sqlStatement, const Vector<SQLValue>& arguments, PassRefPtr<SQLStatementCallback> callback, PassRefPtr<SQLStatementErrorCallback> callbackError, ExceptionCode& e)
{
    if (!m_executeSqlAllowed || !m_database->opened()) {
        e = INVALID_STATE_ERR;
        return;
    }

    int permissions = DatabaseAuthorizer::ReadWriteMask;
    if (!m_database->scriptExecutionContext()->allowDatabaseAccess())
        permissions |= DatabaseAuthorizer::NoAccessMask;
    else if (m_readOnly)
        permissions |= DatabaseAuthorizer::ReadOnlyMask;

    RefPtr<SQLStatement> statement = SQLStatement::create(m_database.get(), sqlStatement, arguments, callback, callbackError, permissions);

    if (m_database->deleted())
        statement->setDatabaseDeletedError();

    if (!m_database->versionMatchesExpected())
        statement->setVersionMismatchedError();

    enqueueStatement(statement);
}

void SQLTransaction::enqueueStatement(PassRefPtr<SQLStatement> statement)
{
    MutexLocker locker(m_statementMutex);
    m_statementQueue.append(statement);
}

#ifndef NDEBUG
const char* SQLTransaction::debugStepName(SQLTransaction::TransactionStepMethod step)
{
    if (step == &SQLTransaction::acquireLock)
        return "acquireLock";
    else if (step == &SQLTransaction::openTransactionAndPreflight)
        return "openTransactionAndPreflight";
    else if (step == &SQLTransaction::runStatements)
        return "runStatements";
    else if (step == &SQLTransaction::postflightAndCommit)
        return "postflightAndCommit";
    else if (step == &SQLTransaction::cleanupAfterTransactionErrorCallback)
        return "cleanupAfterTransactionErrorCallback";
    else if (step == &SQLTransaction::deliverTransactionCallback)
        return "deliverTransactionCallback";
    else if (step == &SQLTransaction::deliverTransactionErrorCallback)
        return "deliverTransactionErrorCallback";
    else if (step == &SQLTransaction::deliverStatementCallback)
        return "deliverStatementCallback";
    else if (step == &SQLTransaction::deliverQuotaIncreaseCallback)
        return "deliverQuotaIncreaseCallback";
    else if (step == &SQLTransaction::deliverSuccessCallback)
        return "deliverSuccessCallback";
    else if (step == &SQLTransaction::cleanupAfterSuccessCallback)
        return "cleanupAfterSuccessCallback";
    else
        return "UNKNOWN";
}
#endif

void SQLTransaction::checkAndHandleClosedOrInterruptedDatabase()
{
    if (m_database->opened() && !m_database->isInterrupted())
        return;

    // If the database was stopped, don't do anything and cancel queued work
    LOG(StorageAPI, "Database was stopped or interrupted - cancelling work for this transaction");
    MutexLocker locker(m_statementMutex);
    m_statementQueue.clear();
    m_nextStep = 0;

    // Release the unneeded callbacks, to break reference cycles.
    m_callbackWrapper.clear();
    m_successCallbackWrapper.clear();
    m_errorCallbackWrapper.clear();

    // The next steps should be executed only if we're on the DB thread.
    if (currentThread() != database()->scriptExecutionContext()->databaseThread()->getThreadID())
        return;

    // The current SQLite transaction should be stopped, as well
    if (m_sqliteTransaction) {
        m_sqliteTransaction->stop();
        m_sqliteTransaction.clear();
    }

    if (m_lockAcquired)
        m_database->transactionCoordinator()->releaseLock(this);
}


bool SQLTransaction::performNextStep()
{
    LOG(StorageAPI, "Step %s\n", debugStepName(m_nextStep));

    ASSERT(m_nextStep == &SQLTransaction::acquireLock ||
           m_nextStep == &SQLTransaction::openTransactionAndPreflight ||
           m_nextStep == &SQLTransaction::runStatements ||
           m_nextStep == &SQLTransaction::postflightAndCommit ||
           m_nextStep == &SQLTransaction::cleanupAfterSuccessCallback ||
           m_nextStep == &SQLTransaction::cleanupAfterTransactionErrorCallback);

    checkAndHandleClosedOrInterruptedDatabase();

    if (m_nextStep)
        (this->*m_nextStep)();

    // If there is no nextStep after performing the above step, the transaction is complete
    return !m_nextStep;
}

void SQLTransaction::performPendingCallback()
{
    LOG(StorageAPI, "Callback %s\n", debugStepName(m_nextStep));

    ASSERT(m_nextStep == &SQLTransaction::deliverTransactionCallback ||
           m_nextStep == &SQLTransaction::deliverTransactionErrorCallback ||
           m_nextStep == &SQLTransaction::deliverStatementCallback ||
           m_nextStep == &SQLTransaction::deliverQuotaIncreaseCallback ||
           m_nextStep == &SQLTransaction::deliverSuccessCallback);

    checkAndHandleClosedOrInterruptedDatabase();

    if (m_nextStep)
        (this->*m_nextStep)();
}

void SQLTransaction::notifyDatabaseThreadIsShuttingDown()
{
    ASSERT(currentThread() == database()->scriptExecutionContext()->databaseThread()->getThreadID());

    // If the transaction is in progress, we should roll it back here, since this is our last
    // oportunity to do something related to this transaction on the DB thread.
    // Clearing m_sqliteTransaction invokes SQLiteTransaction's destructor which does just that.
    m_sqliteTransaction.clear();
}

void SQLTransaction::acquireLock()
{
    m_database->transactionCoordinator()->acquireLock(this);
}

void SQLTransaction::lockAcquired()
{
    m_lockAcquired = true;
    m_nextStep = &SQLTransaction::openTransactionAndPreflight;
    LOG(StorageAPI, "Scheduling openTransactionAndPreflight immediately for transaction %p\n", this);
    m_database->scheduleTransactionStep(this, true);
}

void SQLTransaction::openTransactionAndPreflight()
{
    ASSERT(!m_database->sqliteDatabase().transactionInProgress());
    ASSERT(m_lockAcquired);

    LOG(StorageAPI, "Opening and preflighting transaction %p", this);

    // If the database was deleted, jump to the error callback
    if (m_database->deleted()) {
        m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "unable to open a transaction, because the user deleted the database");
        handleTransactionError(false);
        return;
    }

    // Set the maximum usage for this transaction if this transactions is not read-only
    if (!m_readOnly)
        m_database->sqliteDatabase().setMaximumSize(m_database->maximumSize());

    ASSERT(!m_sqliteTransaction);
    m_sqliteTransaction = adoptPtr(new SQLiteTransaction(m_database->sqliteDatabase(), m_readOnly));

    m_database->resetDeletes();
    m_database->disableAuthorizer();
    m_sqliteTransaction->begin();
    m_database->enableAuthorizer();

    // Transaction Steps 1+2 - Open a transaction to the database, jumping to the error callback if that fails
    if (!m_sqliteTransaction->inProgress()) {
        ASSERT(!m_database->sqliteDatabase().transactionInProgress());
        m_sqliteTransaction.clear();
        m_transactionError = SQLError::create(SQLError::DATABASE_ERR, "unable to open a transaction to the database");
        handleTransactionError(false);
        return;
    }

    // Transaction Steps 3 - Peform preflight steps, jumping to the error callback if they fail
    if (m_wrapper && !m_wrapper->performPreflight(this)) {
        m_sqliteTransaction.clear();
        m_transactionError = m_wrapper->sqlError();
        if (!m_transactionError)
            m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "unknown error occured setting up transaction");

        handleTransactionError(false);
        return;
    }

    // Transaction Step 4 - Invoke the transaction callback with the new SQLTransaction object
    m_nextStep = &SQLTransaction::deliverTransactionCallback;
    LOG(StorageAPI, "Scheduling deliverTransactionCallback for transaction %p\n", this);
    m_database->scheduleTransactionCallback(this);
}

void SQLTransaction::deliverTransactionCallback()
{
    bool shouldDeliverErrorCallback = false;

    RefPtr<SQLTransactionCallback> callback = m_callbackWrapper.unwrap();
    if (callback) {
        m_executeSqlAllowed = true;
        shouldDeliverErrorCallback = !callback->handleEvent(this);
        m_executeSqlAllowed = false;
    }

    // Transaction Step 5 - If the transaction callback was null or raised an exception, jump to the error callback
    if (shouldDeliverErrorCallback) {
        m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "the SQLTransactionCallback was null or threw an exception");
        deliverTransactionErrorCallback();
    } else
        scheduleToRunStatements();
}

void SQLTransaction::scheduleToRunStatements()
{
    m_nextStep = &SQLTransaction::runStatements;
    LOG(StorageAPI, "Scheduling runStatements for transaction %p\n", this);
    m_database->scheduleTransactionStep(this);
}

void SQLTransaction::runStatements()
{
    ASSERT(m_lockAcquired);

    // If there is a series of statements queued up that are all successful and have no associated
    // SQLStatementCallback objects, then we can burn through the queue
    do {
        if (m_shouldRetryCurrentStatement && !m_sqliteTransaction->wasRolledBackBySqlite()) {
            m_shouldRetryCurrentStatement = false;
            // FIXME - Another place that needs fixing up after <rdar://problem/5628468> is addressed.
            // See ::openTransactionAndPreflight() for discussion

            // Reset the maximum size here, as it was increased to allow us to retry this statement.
            // m_shouldRetryCurrentStatement is set to true only when a statement exceeds
            // the quota, which can happen only in a read-write transaction. Therefore, there
            // is no need to check here if the transaction is read-write.
            m_database->sqliteDatabase().setMaximumSize(m_database->maximumSize());
        } else {
            // If the current statement has already been run, failed due to quota constraints, and we're not retrying it,
            // that means it ended in an error.  Handle it now
            if (m_currentStatement && m_currentStatement->lastExecutionFailedDueToQuota()) {
                handleCurrentStatementError();
                break;
            }

            // Otherwise, advance to the next statement
            getNextStatement();
        }
    } while (runCurrentStatement());

    // If runCurrentStatement() returned false, that means either there was no current statement to run,
    // or the current statement requires a callback to complete.  In the later case, it also scheduled
    // the callback or performed any other additional work so we can return
    if (!m_currentStatement)
        postflightAndCommit();
}

void SQLTransaction::getNextStatement()
{
    m_currentStatement = 0;

    MutexLocker locker(m_statementMutex);
    if (!m_statementQueue.isEmpty()) {
        m_currentStatement = m_statementQueue.takeFirst();
    }
}

bool SQLTransaction::runCurrentStatement()
{
    if (!m_currentStatement)
        return false;

    m_database->resetAuthorizer();

    if (m_currentStatement->execute(m_database.get())) {
        if (m_database->lastActionChangedDatabase()) {
            // Flag this transaction as having changed the database for later delegate notification
            m_modifiedDatabase = true;
            // Also dirty the size of this database file for calculating quota usage
            m_database->transactionClient()->didExecuteStatement(database());
        }

        if (m_currentStatement->hasStatementCallback()) {
            m_nextStep = &SQLTransaction::deliverStatementCallback;
            LOG(StorageAPI, "Scheduling deliverStatementCallback for transaction %p\n", this);
            m_database->scheduleTransactionCallback(this);
            return false;
        }
        return true;
    }

    if (m_currentStatement->lastExecutionFailedDueToQuota()) {
        m_nextStep = &SQLTransaction::deliverQuotaIncreaseCallback;
        LOG(StorageAPI, "Scheduling deliverQuotaIncreaseCallback for transaction %p\n", this);
        m_database->scheduleTransactionCallback(this);
        return false;
    }

    handleCurrentStatementError();

    return false;
}

void SQLTransaction::handleCurrentStatementError()
{
    // Transaction Steps 6.error - Call the statement's error callback, but if there was no error callback,
    // or the transaction was rolled back, jump to the transaction error callback
    if (m_currentStatement->hasStatementErrorCallback() && !m_sqliteTransaction->wasRolledBackBySqlite()) {
        m_nextStep = &SQLTransaction::deliverStatementCallback;
        LOG(StorageAPI, "Scheduling deliverStatementCallback for transaction %p\n", this);
        m_database->scheduleTransactionCallback(this);
    } else {
        m_transactionError = m_currentStatement->sqlError();
        if (!m_transactionError)
            m_transactionError = SQLError::create(SQLError::DATABASE_ERR, "the statement failed to execute");
        handleTransactionError(false);
    }
}

void SQLTransaction::deliverStatementCallback()
{
    ASSERT(m_currentStatement);

    // Transaction Step 6.6 and 6.3(error) - If the statement callback went wrong, jump to the transaction error callback
    // Otherwise, continue to loop through the statement queue
    m_executeSqlAllowed = true;
    bool result = m_currentStatement->performCallback(this);
    m_executeSqlAllowed = false;

    if (result) {
        m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "the statement callback raised an exception or statement error callback did not return false");
        handleTransactionError(true);
    } else
        scheduleToRunStatements();
}

void SQLTransaction::deliverQuotaIncreaseCallback()
{
    ASSERT(m_currentStatement);
    ASSERT(!m_shouldRetryCurrentStatement);

    m_shouldRetryCurrentStatement = m_database->transactionClient()->didExceedQuota(database());

    m_nextStep = &SQLTransaction::runStatements;
    LOG(StorageAPI, "Scheduling runStatements for transaction %p\n", this);
    m_database->scheduleTransactionStep(this);
}

void SQLTransaction::postflightAndCommit()
{
    ASSERT(m_lockAcquired);

    // Transaction Step 7 - Peform postflight steps, jumping to the error callback if they fail
    if (m_wrapper && !m_wrapper->performPostflight(this)) {
        m_transactionError = m_wrapper->sqlError();
        if (!m_transactionError)
            m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "unknown error occured setting up transaction");
        handleTransactionError(false);
        return;
    }

    // Transacton Step 8+9 - Commit the transaction, jumping to the error callback if that fails
    ASSERT(m_sqliteTransaction);

    m_database->disableAuthorizer();
    m_sqliteTransaction->commit();
    m_database->enableAuthorizer();

    // If the commit failed, the transaction will still be marked as "in progress"
    if (m_sqliteTransaction->inProgress()) {
        m_successCallbackWrapper.clear();
        m_transactionError = SQLError::create(SQLError::DATABASE_ERR, "failed to commit the transaction");
        handleTransactionError(false);
        return;
    }

    // Vacuum the database if anything was deleted.
    if (m_database->hadDeletes())
        m_database->incrementalVacuumIfNeeded();

    // The commit was successful. If the transaction modified this database, notify the delegates.
    if (m_modifiedDatabase)
        m_database->transactionClient()->didCommitWriteTransaction(database());

    // Now release our unneeded callbacks, to break reference cycles.
    m_errorCallbackWrapper.clear();

    // Transaction Step 10 - Deliver success callback, if there is one
    if (m_successCallbackWrapper.hasCallback()) {
        m_nextStep = &SQLTransaction::deliverSuccessCallback;
        LOG(StorageAPI, "Scheduling deliverSuccessCallback for transaction %p\n", this);
        m_database->scheduleTransactionCallback(this);
    } else
        cleanupAfterSuccessCallback();
}

void SQLTransaction::deliverSuccessCallback()
{
    // Transaction Step 10 - Deliver success callback
    RefPtr<VoidCallback> successCallback = m_successCallbackWrapper.unwrap();
    if (successCallback)
        successCallback->handleEvent();

    // Schedule a "post-success callback" step to return control to the database thread in case there
    // are further transactions queued up for this Database
    m_nextStep = &SQLTransaction::cleanupAfterSuccessCallback;
    LOG(StorageAPI, "Scheduling cleanupAfterSuccessCallback for transaction %p\n", this);
    m_database->scheduleTransactionStep(this);
}

void SQLTransaction::cleanupAfterSuccessCallback()
{
    ASSERT(m_lockAcquired);

    // Transaction Step 11 - End transaction steps
    // There is no next step
    LOG(StorageAPI, "Transaction %p is complete\n", this);
    ASSERT(!m_database->sqliteDatabase().transactionInProgress());
    m_sqliteTransaction.clear();
    m_nextStep = 0;

    // Release the lock on this database
    m_database->transactionCoordinator()->releaseLock(this);
}

void SQLTransaction::handleTransactionError(bool inCallback)
{
    if (m_errorCallbackWrapper.hasCallback()) {
        if (inCallback)
            deliverTransactionErrorCallback();
        else {
            m_nextStep = &SQLTransaction::deliverTransactionErrorCallback;
            LOG(StorageAPI, "Scheduling deliverTransactionErrorCallback for transaction %p\n", this);
            m_database->scheduleTransactionCallback(this);
        }
        return;
    }

    // No error callback, so fast-forward to:
    // Transaction Step 12 - Rollback the transaction.
    if (inCallback) {
        m_nextStep = &SQLTransaction::cleanupAfterTransactionErrorCallback;
        LOG(StorageAPI, "Scheduling cleanupAfterTransactionErrorCallback for transaction %p\n", this);
        m_database->scheduleTransactionStep(this);
    } else {
        cleanupAfterTransactionErrorCallback();
    }
}

void SQLTransaction::deliverTransactionErrorCallback()
{
    ASSERT(m_transactionError);

    // Transaction Step 12 - If exists, invoke error callback with the last
    // error to have occurred in this transaction.
    RefPtr<SQLTransactionErrorCallback> errorCallback = m_errorCallbackWrapper.unwrap();
    if (errorCallback)
        errorCallback->handleEvent(m_transactionError.get());

    m_nextStep = &SQLTransaction::cleanupAfterTransactionErrorCallback;
    LOG(StorageAPI, "Scheduling cleanupAfterTransactionErrorCallback for transaction %p\n", this);
    m_database->scheduleTransactionStep(this);
}

void SQLTransaction::cleanupAfterTransactionErrorCallback()
{
    ASSERT(m_lockAcquired);

    m_database->disableAuthorizer();
    if (m_sqliteTransaction) {
        // Transaction Step 12 - Rollback the transaction.
        m_sqliteTransaction->rollback();

        ASSERT(!m_database->sqliteDatabase().transactionInProgress());
        m_sqliteTransaction.clear();
    }
    m_database->enableAuthorizer();

    // Transaction Step 12 - Any still-pending statements in the transaction are discarded.
    {
        MutexLocker locker(m_statementMutex);
        m_statementQueue.clear();
    }

    // Transaction is complete!  There is no next step
    LOG(StorageAPI, "Transaction %p is complete with an error\n", this);
    ASSERT(!m_database->sqliteDatabase().transactionInProgress());
    m_nextStep = 0;

    // Now release the lock on this database
    m_database->transactionCoordinator()->releaseLock(this);
}

} // namespace WebCore

#endif // ENABLE(DATABASE)
