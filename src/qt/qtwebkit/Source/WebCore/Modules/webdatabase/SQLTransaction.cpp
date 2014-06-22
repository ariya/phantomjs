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
#include "SQLTransaction.h"

#if ENABLE(SQL_DATABASE)

#include "AbstractSQLTransactionBackend.h"
#include "Database.h"
#include "DatabaseAuthorizer.h"
#include "DatabaseContext.h"
#include "ExceptionCode.h"
#include "Logging.h"
#include "SQLError.h"
#include "SQLStatementCallback.h"
#include "SQLStatementErrorCallback.h"
#include "SQLTransactionCallback.h"
#include "SQLTransactionClient.h" // FIXME: Should be used in the backend only.
#include "SQLTransactionErrorCallback.h"
#include "VoidCallback.h"
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

namespace WebCore {

PassRefPtr<SQLTransaction> SQLTransaction::create(Database* db, PassRefPtr<SQLTransactionCallback> callback,
    PassRefPtr<VoidCallback> successCallback, PassRefPtr<SQLTransactionErrorCallback> errorCallback,
    bool readOnly)
{
    return adoptRef(new SQLTransaction(db, callback, successCallback, errorCallback, readOnly));
}

SQLTransaction::SQLTransaction(Database* db, PassRefPtr<SQLTransactionCallback> callback,
    PassRefPtr<VoidCallback> successCallback, PassRefPtr<SQLTransactionErrorCallback> errorCallback,
    bool readOnly)
    : m_database(db)
    , m_callbackWrapper(callback, db->scriptExecutionContext())
    , m_successCallbackWrapper(successCallback, db->scriptExecutionContext())
    , m_errorCallbackWrapper(errorCallback, db->scriptExecutionContext())
    , m_executeSqlAllowed(false)
    , m_readOnly(readOnly)
{
    ASSERT(m_database);
}

bool SQLTransaction::hasCallback() const
{
    return m_callbackWrapper.hasCallback();
}

bool SQLTransaction::hasSuccessCallback() const
{
    return m_successCallbackWrapper.hasCallback();
}

bool SQLTransaction::hasErrorCallback() const
{
    return m_errorCallbackWrapper.hasCallback();
}

void SQLTransaction::setBackend(AbstractSQLTransactionBackend* backend)
{
    ASSERT(!m_backend);
    m_backend = backend;
}

SQLTransaction::StateFunction SQLTransaction::stateFunctionFor(SQLTransactionState state)
{
    static const StateFunction stateFunctions[] = {
        &SQLTransaction::unreachableState,                // 0. illegal
        &SQLTransaction::unreachableState,                // 1. idle
        &SQLTransaction::unreachableState,                // 2. acquireLock
        &SQLTransaction::unreachableState,                // 3. openTransactionAndPreflight
        &SQLTransaction::sendToBackendState,              // 4. runStatements
        &SQLTransaction::unreachableState,                // 5. postflightAndCommit
        &SQLTransaction::sendToBackendState,              // 6. cleanupAndTerminate
        &SQLTransaction::sendToBackendState,              // 7. cleanupAfterTransactionErrorCallback
        &SQLTransaction::deliverTransactionCallback,      // 8.
        &SQLTransaction::deliverTransactionErrorCallback, // 9.
        &SQLTransaction::deliverStatementCallback,        // 10.
        &SQLTransaction::deliverQuotaIncreaseCallback,    // 11.
        &SQLTransaction::deliverSuccessCallback           // 12.
    };

    ASSERT(WTF_ARRAY_LENGTH(stateFunctions) == static_cast<int>(SQLTransactionState::NumberOfStates));
    ASSERT(state < SQLTransactionState::NumberOfStates);

    return stateFunctions[static_cast<int>(state)];
}

// requestTransitToState() can be called from the backend. Hence, it should
// NOT be modifying SQLTransactionBackend in general. The only safe field to
// modify is m_requestedState which is meant for this purpose.
void SQLTransaction::requestTransitToState(SQLTransactionState nextState)
{
    LOG(StorageAPI, "Scheduling %s for transaction %p\n", nameForSQLTransactionState(nextState), this);
    m_requestedState = nextState;
    m_database->scheduleTransactionCallback(this);
}

SQLTransactionState SQLTransaction::nextStateForTransactionError()
{
    ASSERT(m_transactionError);
    if (m_errorCallbackWrapper.hasCallback())
        return SQLTransactionState::DeliverTransactionErrorCallback;

    // No error callback, so fast-forward to:
    // Transaction Step 11 - Rollback the transaction.
    return SQLTransactionState::CleanupAfterTransactionErrorCallback;
}

SQLTransactionState SQLTransaction::deliverTransactionCallback()
{
    bool shouldDeliverErrorCallback = false;

    // Spec 4.3.2 4: Invoke the transaction callback with the new SQLTransaction object
    RefPtr<SQLTransactionCallback> callback = m_callbackWrapper.unwrap();
    if (callback) {
        m_executeSqlAllowed = true;
        shouldDeliverErrorCallback = !callback->handleEvent(this);
        m_executeSqlAllowed = false;
    }

    // Spec 4.3.2 5: If the transaction callback was null or raised an exception, jump to the error callback
    SQLTransactionState nextState = SQLTransactionState::RunStatements;
    if (shouldDeliverErrorCallback) {
        m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "the SQLTransactionCallback was null or threw an exception");
        nextState = SQLTransactionState::DeliverTransactionErrorCallback;
    }
    return nextState;
}

SQLTransactionState SQLTransaction::deliverTransactionErrorCallback()
{
    // Spec 4.3.2.10: If exists, invoke error callback with the last
    // error to have occurred in this transaction.
    RefPtr<SQLTransactionErrorCallback> errorCallback = m_errorCallbackWrapper.unwrap();
    if (errorCallback) {
        // If we get here with an empty m_transactionError, then the backend
        // must be waiting in the idle state waiting for this state to finish.
        // Hence, it's thread safe to fetch the backend transactionError without
        // a lock.
        if (!m_transactionError)
            m_transactionError = m_backend->transactionError();

        ASSERT(m_transactionError);
        errorCallback->handleEvent(m_transactionError.get());

        m_transactionError = 0;
    }

    clearCallbackWrappers();

    // Spec 4.3.2.10: Rollback the transaction.
    return SQLTransactionState::CleanupAfterTransactionErrorCallback;
}

SQLTransactionState SQLTransaction::deliverStatementCallback()
{
    // Spec 4.3.2.6.6 and 4.3.2.6.3: If the statement callback went wrong, jump to the transaction error callback
    // Otherwise, continue to loop through the statement queue
    m_executeSqlAllowed = true;

    AbstractSQLStatement* currentAbstractStatement = m_backend->currentStatement();
    SQLStatement* currentStatement = static_cast<SQLStatement*>(currentAbstractStatement);
    ASSERT(currentStatement);

    bool result = currentStatement->performCallback(this);

    m_executeSqlAllowed = false;

    if (result) {
        m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "the statement callback raised an exception or statement error callback did not return false");
        return nextStateForTransactionError();
    }
    return SQLTransactionState::RunStatements;
}

SQLTransactionState SQLTransaction::deliverQuotaIncreaseCallback()
{
    ASSERT(m_backend->currentStatement());

    bool shouldRetryCurrentStatement = m_database->transactionClient()->didExceedQuota(database());
    m_backend->setShouldRetryCurrentStatement(shouldRetryCurrentStatement);

    return SQLTransactionState::RunStatements;
}

SQLTransactionState SQLTransaction::deliverSuccessCallback()
{
    // Spec 4.3.2.8: Deliver success callback.
    RefPtr<VoidCallback> successCallback = m_successCallbackWrapper.unwrap();
    if (successCallback)
        successCallback->handleEvent();

    clearCallbackWrappers();

    // Schedule a "post-success callback" step to return control to the database thread in case there
    // are further transactions queued up for this Database
    return SQLTransactionState::CleanupAndTerminate;
}

// This state function is used as a stub function to plug unimplemented states
// in the state dispatch table. They are unimplemented because they should
// never be reached in the course of correct execution.
SQLTransactionState SQLTransaction::unreachableState()
{
    ASSERT_NOT_REACHED();
    return SQLTransactionState::End;
}

SQLTransactionState SQLTransaction::sendToBackendState()
{
    ASSERT(m_nextState != SQLTransactionState::Idle);
    m_backend->requestTransitToState(m_nextState);
    return SQLTransactionState::Idle;
}

void SQLTransaction::performPendingCallback()
{
    computeNextStateAndCleanupIfNeeded();
    runStateMachine();
}

void SQLTransaction::executeSQL(const String& sqlStatement, const Vector<SQLValue>& arguments, PassRefPtr<SQLStatementCallback> callback, PassRefPtr<SQLStatementErrorCallback> callbackError, ExceptionCode& e)
{
    if (!m_executeSqlAllowed || !m_database->opened()) {
        e = INVALID_STATE_ERR;
        return;
    }

    int permissions = DatabaseAuthorizer::ReadWriteMask;
    if (!m_database->databaseContext()->allowDatabaseAccess())
        permissions |= DatabaseAuthorizer::NoAccessMask;
    else if (m_readOnly)
        permissions |= DatabaseAuthorizer::ReadOnlyMask;

    OwnPtr<SQLStatement> statement = SQLStatement::create(m_database.get(), callback, callbackError);
    m_backend->executeSQL(statement.release(), sqlStatement, arguments, permissions);
}

bool SQLTransaction::computeNextStateAndCleanupIfNeeded()
{
    // Only honor the requested state transition if we're not supposed to be
    // cleaning up and shutting down:
    if (m_database->opened() && !m_database->isInterrupted()) {
        setStateToRequestedState();
        ASSERT(m_nextState == SQLTransactionState::End
            || m_nextState == SQLTransactionState::DeliverTransactionCallback
            || m_nextState == SQLTransactionState::DeliverTransactionErrorCallback
            || m_nextState == SQLTransactionState::DeliverStatementCallback
            || m_nextState == SQLTransactionState::DeliverQuotaIncreaseCallback
            || m_nextState == SQLTransactionState::DeliverSuccessCallback);

        LOG(StorageAPI, "Callback %s\n", nameForSQLTransactionState(m_nextState));
        return false;
    }

    clearCallbackWrappers();
    m_nextState = SQLTransactionState::CleanupAndTerminate;

    return true;
}

void SQLTransaction::clearCallbackWrappers()
{
    // Release the unneeded callbacks, to break reference cycles.
    m_callbackWrapper.clear();
    m_successCallbackWrapper.clear();
    m_errorCallbackWrapper.clear();
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
