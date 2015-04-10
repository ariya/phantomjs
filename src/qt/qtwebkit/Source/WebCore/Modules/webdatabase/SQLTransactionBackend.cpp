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
#include "SQLTransactionBackend.h"

#if ENABLE(SQL_DATABASE)

#include "AbstractSQLTransaction.h"
#include "Database.h" // FIXME: Should only be used in the frontend.
#include "DatabaseAuthorizer.h"
#include "DatabaseBackend.h"
#include "DatabaseBackendContext.h"
#include "DatabaseThread.h"
#include "DatabaseTracker.h"
#include "ExceptionCode.h"
#include "Logging.h"
#include "OriginLock.h"
#include "SQLError.h"
#include "SQLStatementBackend.h"
#include "SQLTransactionClient.h"
#include "SQLTransactionCoordinator.h"
#include "SQLValue.h"
#include "SQLiteTransaction.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>


// How does a SQLTransaction work?
// ==============================
// The SQLTransaction is a state machine that executes a series of states / steps.
//
// The work of the transaction states are defined in section of 4.3.2 of the
// webdatabase spec: http://dev.w3.org/html5/webdatabase/#processing-model
//
// the State Transition Graph at a glance:
// ======================================
//
//     Backend                          .   Frontend
//     (works with SQLiteDatabase)      .   (works with Script)
//     ===========================      .   ===================
//                                      .
//     1. Idle                          .
//         v                            .
//     2. AcquireLock                   .
//         v                            .
//     3. OpenTransactionAndPreflight ------------------------------------------.
//         |                            .                                       |
//         `-------------------------------> 8. DeliverTransactionCallback --.  |
//                                      .        |                           v  v
//         ,-------------------------------------'   9. DeliverTransactionErrorCallback +
//         |                            .                                    ^  ^  ^    |
//         v                            .                                    |  |  |    |
//     4. RunStatements -----------------------------------------------------'  |  |    |
//         |        ^  ^ |  ^ |         .                                       |  |    |
//         |--------'  | |  | `------------> 10. DeliverStatementCallback +-----'  |    |
//         |           | |  `---------------------------------------------'        |    |
//         |           | `-----------------> 11. DeliverQuotaIncreaseCallback +    |    |
//         |            `-----------------------------------------------------'    |    |
//         v                            .                                          |    |
//     5. PostflightAndCommit --+--------------------------------------------------'    |
//                              |----------> 12. DeliverSuccessCallback +               |
//         ,--------------------'       .                               |               |
//         v                            .                               |               |
//     6. CleanupAndTerminate <-----------------------------------------'               |
//         v           ^                .                                               |
//     0. End          |                .                                               |
//                     |                .                                               |
//                7: CleanupAfterTransactionErrorCallback <----------------------------'
//                                      .
//
// the States and State Transitions:
// ================================
//     0. SQLTransactionState::End
//         - the end state.
//
//     1. SQLTransactionState::Idle
//         - placeholder state while waiting on frontend/backend, etc. See comment on
//           "State transitions between SQLTransaction and SQLTransactionBackend"
//           below.
//
//     2. SQLTransactionState::AcquireLock (runs in backend)
//         - this is the start state.
//         - acquire the "lock".
//         - on "lock" acquisition, goto SQLTransactionState::OpenTransactionAndPreflight.
//
//     3. SQLTransactionState::openTransactionAndPreflight (runs in backend)
//         - Sets up an SQLiteTransaction.
//         - begin the SQLiteTransaction.
//         - call the SQLTransactionWrapper preflight if available.
//         - schedule script callback.
//         - on error, goto SQLTransactionState::DeliverTransactionErrorCallback.
//         - goto SQLTransactionState::DeliverTransactionCallback.
//
//     4. SQLTransactionState::DeliverTransactionCallback (runs in frontend)
//         - invoke the script function callback() if available.
//         - on error, goto SQLTransactionState::DeliverTransactionErrorCallback.
//         - goto SQLTransactionState::RunStatements.
//
//     5. SQLTransactionState::DeliverTransactionErrorCallback (runs in frontend)
//         - invoke the script function errorCallback if available.
//         - goto SQLTransactionState::CleanupAfterTransactionErrorCallback.
//
//     6. SQLTransactionState::RunStatements (runs in backend)
//         - while there are statements {
//             - run a statement.
//             - if statementCallback is available, goto SQLTransactionState::DeliverStatementCallback.
//             - on error,
//               goto SQLTransactionState::DeliverQuotaIncreaseCallback, or
//               goto SQLTransactionState::DeliverStatementCallback, or
//               goto SQLTransactionState::deliverTransactionErrorCallback.
//           }
//         - goto SQLTransactionState::PostflightAndCommit.
//
//     7. SQLTransactionState::DeliverStatementCallback (runs in frontend)
//         - invoke script statement callback (assume available).
//         - on error, goto SQLTransactionState::DeliverTransactionErrorCallback.
//         - goto SQLTransactionState::RunStatements.
//
//     8. SQLTransactionState::DeliverQuotaIncreaseCallback (runs in frontend)
//         - give client a chance to increase the quota.
//         - goto SQLTransactionState::RunStatements.
//
//     9. SQLTransactionState::PostflightAndCommit (runs in backend)
//         - call the SQLTransactionWrapper postflight if available.
//         - commit the SQLiteTansaction.
//         - on error, goto SQLTransactionState::DeliverTransactionErrorCallback.
//         - if successCallback is available, goto SQLTransactionState::DeliverSuccessCallback.
//           else goto SQLTransactionState::CleanupAndTerminate.
//
//     10. SQLTransactionState::DeliverSuccessCallback (runs in frontend)
//         - invoke the script function successCallback() if available.
//         - goto SQLTransactionState::CleanupAndTerminate.
//
//     11. SQLTransactionState::CleanupAndTerminate (runs in backend)
//         - stop and clear the SQLiteTransaction.
//         - release the "lock".
//         - goto SQLTransactionState::End.
//
//     12. SQLTransactionState::CleanupAfterTransactionErrorCallback (runs in backend)
//         - rollback the SQLiteTransaction.
//         - goto SQLTransactionState::CleanupAndTerminate.
//
// State transitions between SQLTransaction and SQLTransactionBackend
// ==================================================================
// As shown above, there are state transitions that crosses the boundary between
// the frontend and backend. For example,
//
//     OpenTransactionAndPreflight (state 3 in the backend)
//     transitions to DeliverTransactionCallback (state 8 in the frontend),
//     which in turn transitions to RunStatements (state 4 in the backend).
//
// This cross boundary transition is done by posting transition requests to the
// other side and letting the other side's state machine execute the state
// transition in the appropriate thread (i.e. the script thread for the frontend,
// and the database thread for the backend).
//
// Logically, the state transitions work as shown in the graph above. But
// physically, the transition mechanism uses the Idle state (both in the frontend
// and backend) as a waiting state for further activity. For example, taking a
// closer look at the 3 state transition example above, what actually happens
// is as follows:
//
//     Step 1:
//     ======
//     In the frontend thread:
//     - waiting quietly is Idle. Not doing any work.
//
//     In the backend:
//     - is in OpenTransactionAndPreflight, and doing its work.
//     - when done, it transits to the backend DeliverTransactionCallback.
//     - the backend DeliverTransactionCallback sends a request to the frontend
//       to transit to DeliverTransactionCallback, and then itself transits to
//       Idle.
//
//     Step 2:
//     ======
//     In the frontend thread:
//     - transits to DeliverTransactionCallback and does its work.
//     - when done, it transits to the frontend RunStatements.
//     - the frontend RunStatements sends a request to the backend to transit
//       to RunStatements, and then itself transits to Idle.
//
//     In the backend:
//     - waiting quietly in Idle.
//
//     Step 3:
//     ======
//     In the frontend thread:
//     - waiting quietly is Idle. Not doing any work.
//
//     In the backend:
//     - transits to RunStatements, and does its work.
//        ...
//
// So, when the frontend or backend are not active, they will park themselves in
// their Idle states. This means their m_nextState is set to Idle, but they never
// actually run the corresponding state function. Note: for both the frontend and
// backend, the state function for Idle is unreachableState().
//
// The states that send a request to their peer across the front/back boundary
// are implemented with just 2 functions: SQLTransaction::sendToBackendState()
// and SQLTransactionBackend::sendToFrontendState(). These state functions do
// nothing but sends a request to the other side to transit to the current
// state (indicated by m_nextState), and then transits itself to the Idle state
// to wait for further action.


// The Life-Cycle of a SQLTransaction i.e. Who's keeping the SQLTransaction alive? 
// ==============================================================================
// The RefPtr chain goes something like this:
//
//     At birth (in DatabaseBackend::runTransaction()):
//     ====================================================
//     DatabaseBackend                    // Deque<RefPtr<SQLTransactionBackend> > m_transactionQueue points to ...
//     --> SQLTransactionBackend          // RefPtr<SQLTransaction> m_frontend points to ...
//         --> SQLTransaction             // RefPtr<SQLTransactionBackend> m_backend points to ...
//             --> SQLTransactionBackend  // which is a circular reference.
//
//     Note: there's a circular reference between the SQLTransaction front-end and
//     back-end. This circular reference is established in the constructor of the
//     SQLTransactionBackend. The circular reference will be broken by calling
//     doCleanup() to nullify m_frontend. This is done at the end of the transaction's
//     clean up state (i.e. when the transaction should no longer be in use thereafter),
//     or if the database was interrupted. See comments on "What happens if a transaction
//     is interrupted?" below for details.
//
//     After scheduling the transaction with the DatabaseThread (DatabaseBackend::scheduleTransaction()):
//     ======================================================================================================
//     DatabaseThread                         // MessageQueue<DatabaseTask> m_queue points to ...
//     --> DatabaseTransactionTask            // RefPtr<SQLTransactionBackend> m_transaction points to ...
//         --> SQLTransactionBackend          // RefPtr<SQLTransaction> m_frontend points to ...
//             --> SQLTransaction             // RefPtr<SQLTransactionBackend> m_backend points to ...
//                 --> SQLTransactionBackend  // which is a circular reference.
//
//     When executing the transaction (in DatabaseThread::databaseThread()):
//     ====================================================================
//     OwnPtr<DatabaseTask> task;             // points to ...
//     --> DatabaseTransactionTask            // RefPtr<SQLTransactionBackend> m_transaction points to ...
//         --> SQLTransactionBackend          // RefPtr<SQLTransaction> m_frontend;
//             --> SQLTransaction             // RefPtr<SQLTransactionBackend> m_backend points to ...
//                 --> SQLTransactionBackend  // which is a circular reference.
//
//     At the end of cleanupAndTerminate():
//     ===================================
//     At the end of the cleanup state, the SQLTransactionBackend::m_frontend is nullified.
//     If by then, a JSObject wrapper is referring to the SQLTransaction, then the reference
//     chain looks like this:
//
//     JSObjectWrapper
//     --> SQLTransaction             // in RefPtr<SQLTransactionBackend> m_backend points to ...
//         --> SQLTransactionBackend  // which no longer points back to its SQLTransaction.
//
//     When the GC collects the corresponding JSObject, the above chain will be cleaned up
//     and deleted.
//
//     If there is no JSObject wrapper referring to the SQLTransaction when the cleanup
//     states nullify SQLTransactionBackend::m_frontend, the SQLTransaction will deleted then.
//     However, there will still be a DatabaseTask pointing to the SQLTransactionBackend (see
//     the "When executing the transaction" chain above). This will keep the
//     SQLTransactionBackend alive until DatabaseThread::databaseThread() releases its
//     task OwnPtr.
//
//     What happens if a transaction is interrupted?
//     ============================================
//     If the transaction is interrupted half way, it won't get to run to state
//     CleanupAndTerminate, and hence, would not have called SQLTransactionBackend's
//     doCleanup(). doCleanup() is where we nullify SQLTransactionBackend::m_frontend
//     to break the reference cycle between the frontend and backend. Hence, we need
//     to cleanup the transaction by other means.
//
//     Note: calling SQLTransactionBackend::notifyDatabaseThreadIsShuttingDown()
//     is effectively the same as calling SQLTransactionBackend::doClean().
//
//     In terms of who needs to call doCleanup(), there are 5 phases in the
//     SQLTransactionBackend life-cycle. These are the phases and how the clean
//     up is done:
//
//     Phase 1. After Birth, before scheduling
//
//     - To clean up, DatabaseThread::databaseThread() will call
//       DatabaseBackend::close() during its shutdown.
//     - DatabaseBackend::close() will iterate
//       DatabaseBackend::m_transactionQueue and call
//       notifyDatabaseThreadIsShuttingDown() on each transaction there.
//        
//     Phase 2. After scheduling, before state AcquireLock
//
//     - If the interruption occures before the DatabaseTransactionTask is
//       scheduled in DatabaseThread::m_queue but hasn't gotten to execute
//       (i.e. DatabaseTransactionTask::performTask() has not been called),
//       then the DatabaseTransactionTask may get destructed before it ever
//       gets to execute.
//     - To clean up, the destructor will check if the task's m_wasExecuted is
//       set. If not, it will call notifyDatabaseThreadIsShuttingDown() on
//       the task's transaction.
//
//     Phase 3. After state AcquireLock, before "lockAcquired"
//
//     - In this phase, the transaction would have been added to the
//       SQLTransactionCoordinator's CoordinationInfo's pendingTransactions.
//     - To clean up, during shutdown, DatabaseThread::databaseThread() calls
//       SQLTransactionCoordinator::shutdown(), which calls
//       notifyDatabaseThreadIsShuttingDown().
//
//     Phase 4: After "lockAcquired", before state CleanupAndTerminate
//
//     - In this phase, the transaction would have been added either to the
//       SQLTransactionCoordinator's CoordinationInfo's activeWriteTransaction
//       or activeReadTransactions.
//     - To clean up, during shutdown, DatabaseThread::databaseThread() calls
//       SQLTransactionCoordinator::shutdown(), which calls
//       notifyDatabaseThreadIsShuttingDown().
//
//     Phase 5: After state CleanupAndTerminate
//
//     - This is how a transaction ends normally.
//     - state CleanupAndTerminate calls doCleanup().


// There's no way of knowing exactly how much more space will be required when a statement hits the quota limit.
// For now, we'll arbitrarily choose currentQuota + 1mb.
// In the future we decide to track if a size increase wasn't enough, and ask for larger-and-larger increases until its enough.
static const int DefaultQuotaSizeIncrease = 1048576;

namespace WebCore {

PassRefPtr<SQLTransactionBackend> SQLTransactionBackend::create(DatabaseBackend* db,
    PassRefPtr<AbstractSQLTransaction> frontend, PassRefPtr<SQLTransactionWrapper> wrapper, bool readOnly)
{
    return adoptRef(new SQLTransactionBackend(db, frontend, wrapper, readOnly));
}

SQLTransactionBackend::SQLTransactionBackend(DatabaseBackend* db,
    PassRefPtr<AbstractSQLTransaction> frontend, PassRefPtr<SQLTransactionWrapper> wrapper, bool readOnly)
    : m_frontend(frontend)
    , m_database(db)
    , m_wrapper(wrapper)
    , m_hasCallback(m_frontend->hasCallback())
    , m_hasSuccessCallback(m_frontend->hasSuccessCallback())
    , m_hasErrorCallback(m_frontend->hasErrorCallback())
    , m_shouldRetryCurrentStatement(false)
    , m_modifiedDatabase(false)
    , m_lockAcquired(false)
    , m_readOnly(readOnly)
    , m_hasVersionMismatch(false)
{
    ASSERT(m_database);
    m_frontend->setBackend(this);
    m_requestedState = SQLTransactionState::AcquireLock;
}

SQLTransactionBackend::~SQLTransactionBackend()
{
    ASSERT(!m_sqliteTransaction);
}

void SQLTransactionBackend::doCleanup()
{
    if (!m_frontend)
        return;
    m_frontend = 0; // Break the reference cycle. See comment about the life-cycle above.

    ASSERT(currentThread() == database()->databaseContext()->databaseThread()->getThreadID());

    releaseOriginLockIfNeeded();

    MutexLocker locker(m_statementMutex);
    m_statementQueue.clear();

    if (m_sqliteTransaction) {
        // In the event we got here because of an interruption or error (i.e. if
        // the transaction is in progress), we should roll it back here. Clearing
        // m_sqliteTransaction invokes SQLiteTransaction's destructor which does
        // just that. We might as well do this unconditionally and free up its
        // resources because we're already terminating.
        m_sqliteTransaction.clear();
    }

    // Release the lock on this database
    if (m_lockAcquired)
        m_database->transactionCoordinator()->releaseLock(this);

    // Do some aggresive clean up here except for m_database.
    //
    // We can't clear m_database here because the frontend may asynchronously
    // invoke SQLTransactionBackend::requestTransitToState(), and that function
    // uses m_database to schedule a state transition. This may occur because
    // the frontend (being in another thread) may already be on the way to
    // requesting our next state before it detects an interruption.
    //
    // There is no harm in letting it finish making the request. It'll set
    // m_requestedState, but we won't execute a transition to that state because
    // we've already shut down the transaction.
    //
    // We also can't clear m_currentStatementBackend and m_transactionError.
    // m_currentStatementBackend may be accessed asynchronously by the
    // frontend's deliverStatementCallback() state. Similarly,
    // m_transactionError may be accessed by deliverTransactionErrorCallback().
    // This occurs if requests for transition to those states have already been
    // registered with the frontend just prior to a clean up request arriving.
    //
    // So instead, let our destructor handle their clean up since this
    // SQLTransactionBackend is guaranteed to not destruct until the frontend
    // is also destructing.

    m_wrapper = 0;
}

AbstractSQLStatement* SQLTransactionBackend::currentStatement()
{
    return m_currentStatementBackend->frontend();
}

PassRefPtr<SQLError> SQLTransactionBackend::transactionError()
{
    return m_transactionError;
}

void SQLTransactionBackend::setShouldRetryCurrentStatement(bool shouldRetry)
{
    ASSERT(!m_shouldRetryCurrentStatement);
    m_shouldRetryCurrentStatement = shouldRetry;
}

SQLTransactionBackend::StateFunction SQLTransactionBackend::stateFunctionFor(SQLTransactionState state)
{
    static const StateFunction stateFunctions[] = {
        &SQLTransactionBackend::unreachableState,            // 0. end
        &SQLTransactionBackend::unreachableState,            // 1. idle
        &SQLTransactionBackend::acquireLock,                 // 2.
        &SQLTransactionBackend::openTransactionAndPreflight, // 3.
        &SQLTransactionBackend::runStatements,               // 4.
        &SQLTransactionBackend::postflightAndCommit,         // 5.
        &SQLTransactionBackend::cleanupAndTerminate,         // 6.
        &SQLTransactionBackend::cleanupAfterTransactionErrorCallback, // 7.
        &SQLTransactionBackend::sendToFrontendState,         // 8. deliverTransactionCallback
        &SQLTransactionBackend::sendToFrontendState,         // 9. deliverTransactionErrorCallback
        &SQLTransactionBackend::sendToFrontendState,         // 10. deliverStatementCallback
        &SQLTransactionBackend::sendToFrontendState,         // 11. deliverQuotaIncreaseCallback
        &SQLTransactionBackend::sendToFrontendState          // 12. deliverSuccessCallback
    };

    ASSERT(WTF_ARRAY_LENGTH(stateFunctions) == static_cast<int>(SQLTransactionState::NumberOfStates));
    ASSERT(state < SQLTransactionState::NumberOfStates);

    return stateFunctions[static_cast<int>(state)];
}

void SQLTransactionBackend::enqueueStatementBackend(PassRefPtr<SQLStatementBackend> statementBackend)
{
    MutexLocker locker(m_statementMutex);
    m_statementQueue.append(statementBackend);
}

void SQLTransactionBackend::computeNextStateAndCleanupIfNeeded()
{
    // Only honor the requested state transition if we're not supposed to be
    // cleaning up and shutting down:
    if (m_database->opened() && !m_database->isInterrupted()) {
        setStateToRequestedState();
        ASSERT(m_nextState == SQLTransactionState::AcquireLock
            || m_nextState == SQLTransactionState::OpenTransactionAndPreflight
            || m_nextState == SQLTransactionState::RunStatements
            || m_nextState == SQLTransactionState::PostflightAndCommit
            || m_nextState == SQLTransactionState::CleanupAndTerminate
            || m_nextState == SQLTransactionState::CleanupAfterTransactionErrorCallback);

        LOG(StorageAPI, "State %s\n", nameForSQLTransactionState(m_nextState));
        return;
    }

    // If we get here, then we should be shutting down. Do clean up if needed:
    if (m_nextState == SQLTransactionState::End)
        return;
    m_nextState = SQLTransactionState::End;

    // If the database was stopped, don't do anything and cancel queued work
    LOG(StorageAPI, "Database was stopped or interrupted - cancelling work for this transaction");

    // The current SQLite transaction should be stopped, as well
    if (m_sqliteTransaction) {
        m_sqliteTransaction->stop();
        m_sqliteTransaction.clear();
    }

    // Terminate the frontend state machine. This also gets the frontend to
    // call computeNextStateAndCleanupIfNeeded() and clear its wrappers
    // if needed.
    m_frontend->requestTransitToState(SQLTransactionState::End);

    // Redirect to the end state to abort, clean up, and end the transaction.
    doCleanup();
}

void SQLTransactionBackend::performNextStep()
{
    computeNextStateAndCleanupIfNeeded();
    runStateMachine();
}

void SQLTransactionBackend::executeSQL(PassOwnPtr<AbstractSQLStatement> statement,
    const String& sqlStatement, const Vector<SQLValue>& arguments, int permissions)
{
    RefPtr<SQLStatementBackend> statementBackend;
    statementBackend = SQLStatementBackend::create(statement, sqlStatement, arguments, permissions);

    if (Database::from(m_database.get())->deleted())
        statementBackend->setDatabaseDeletedError();

    enqueueStatementBackend(statementBackend);
}

void SQLTransactionBackend::notifyDatabaseThreadIsShuttingDown()
{
    ASSERT(currentThread() == database()->databaseContext()->databaseThread()->getThreadID());

    // If the transaction is in progress, we should roll it back here, since this
    // is our last opportunity to do something related to this transaction on the
    // DB thread. Amongst other work, doCleanup() will clear m_sqliteTransaction
    // which invokes SQLiteTransaction's destructor, which will do the roll back
    // if necessary.
    doCleanup();
}

SQLTransactionState SQLTransactionBackend::acquireLock()
{
    m_database->transactionCoordinator()->acquireLock(this);
    return SQLTransactionState::Idle;
}

void SQLTransactionBackend::lockAcquired()
{
    m_lockAcquired = true;
    requestTransitToState(SQLTransactionState::OpenTransactionAndPreflight);
}

SQLTransactionState SQLTransactionBackend::openTransactionAndPreflight()
{
    ASSERT(!m_database->sqliteDatabase().transactionInProgress());
    ASSERT(m_lockAcquired);

    LOG(StorageAPI, "Opening and preflighting transaction %p", this);

    // If the database was deleted, jump to the error callback
    if (Database::from(m_database.get())->deleted()) {
        m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "unable to open a transaction, because the user deleted the database");
        return nextStateForTransactionError();
    }

    // Set the maximum usage for this transaction if this transactions is not read-only
    if (!m_readOnly) {
        acquireOriginLock();
        m_database->sqliteDatabase().setMaximumSize(m_database->maximumSize());
    }

    ASSERT(!m_sqliteTransaction);
    m_sqliteTransaction = adoptPtr(new SQLiteTransaction(m_database->sqliteDatabase(), m_readOnly));

    m_database->resetDeletes();
    m_database->disableAuthorizer();
    m_sqliteTransaction->begin();
    m_database->enableAuthorizer();

    // Spec 4.3.2.1+2: Open a transaction to the database, jumping to the error callback if that fails
    if (!m_sqliteTransaction->inProgress()) {
        ASSERT(!m_database->sqliteDatabase().transactionInProgress());
        m_transactionError = SQLError::create(SQLError::DATABASE_ERR, "unable to begin transaction",
            m_database->sqliteDatabase().lastError(), m_database->sqliteDatabase().lastErrorMsg());
        m_sqliteTransaction.clear();
        return nextStateForTransactionError();
    }

    // Note: We intentionally retrieve the actual version even with an empty expected version.
    // In multi-process browsers, we take this opportinutiy to update the cached value for
    // the actual version. In single-process browsers, this is just a map lookup.
    String actualVersion;
    if (!m_database->getActualVersionForTransaction(actualVersion)) {
        m_transactionError = SQLError::create(SQLError::DATABASE_ERR, "unable to read version",
            m_database->sqliteDatabase().lastError(), m_database->sqliteDatabase().lastErrorMsg());
        m_database->disableAuthorizer();
        m_sqliteTransaction.clear();
        m_database->enableAuthorizer();
        return nextStateForTransactionError();
    }
    m_hasVersionMismatch = !m_database->expectedVersion().isEmpty() && (m_database->expectedVersion() != actualVersion);

    // Spec 4.3.2.3: Perform preflight steps, jumping to the error callback if they fail
    if (m_wrapper && !m_wrapper->performPreflight(this)) {
        m_database->disableAuthorizer();
        m_sqliteTransaction.clear();
        m_database->enableAuthorizer();
        m_transactionError = m_wrapper->sqlError();
        if (!m_transactionError)
            m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "unknown error occurred during transaction preflight");
        return nextStateForTransactionError();
    }

    // Spec 4.3.2.4: Invoke the transaction callback with the new SQLTransaction object
    if (m_hasCallback)
        return SQLTransactionState::DeliverTransactionCallback;

    // If we have no callback to make, skip pass to the state after:
    return SQLTransactionState::RunStatements;
}

SQLTransactionState SQLTransactionBackend::runStatements()
{
    ASSERT(m_lockAcquired);
    SQLTransactionState nextState;

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
            // that means it ended in an error. Handle it now
            if (m_currentStatementBackend && m_currentStatementBackend->lastExecutionFailedDueToQuota()) {
                return nextStateForCurrentStatementError();
            }

            // Otherwise, advance to the next statement
            getNextStatement();
        }
        nextState = runCurrentStatementAndGetNextState();
    } while (nextState == SQLTransactionState::RunStatements);

    return nextState;
}

void SQLTransactionBackend::getNextStatement()
{
    m_currentStatementBackend = 0;

    MutexLocker locker(m_statementMutex);
    if (!m_statementQueue.isEmpty())
        m_currentStatementBackend = m_statementQueue.takeFirst();
}

SQLTransactionState SQLTransactionBackend::runCurrentStatementAndGetNextState()
{
    if (!m_currentStatementBackend) {
        // No more statements to run. So move on to the next state.
        return SQLTransactionState::PostflightAndCommit;
    }

    m_database->resetAuthorizer();

    if (m_hasVersionMismatch)
        m_currentStatementBackend->setVersionMismatchedError();

    if (m_currentStatementBackend->execute(m_database.get())) {
        if (m_database->lastActionChangedDatabase()) {
            // Flag this transaction as having changed the database for later delegate notification
            m_modifiedDatabase = true;
        }

        if (m_currentStatementBackend->hasStatementCallback()) {
            return SQLTransactionState::DeliverStatementCallback;
        }

        // If we get here, then the statement doesn't have a callback to invoke.
        // We can move on to the next statement. Hence, stay in this state.
        return SQLTransactionState::RunStatements;
    }

    if (m_currentStatementBackend->lastExecutionFailedDueToQuota()) {
        return SQLTransactionState::DeliverQuotaIncreaseCallback;
    }

    return nextStateForCurrentStatementError();
}

SQLTransactionState SQLTransactionBackend::nextStateForCurrentStatementError()
{
    // Spec 4.3.2.6.6: error - Call the statement's error callback, but if there was no error callback,
    // or the transaction was rolled back, jump to the transaction error callback
    if (m_currentStatementBackend->hasStatementErrorCallback() && !m_sqliteTransaction->wasRolledBackBySqlite())
        return SQLTransactionState::DeliverStatementCallback;

    m_transactionError = m_currentStatementBackend->sqlError();
    if (!m_transactionError)
        m_transactionError = SQLError::create(SQLError::DATABASE_ERR, "the statement failed to execute");
    return nextStateForTransactionError();
}

SQLTransactionState SQLTransactionBackend::postflightAndCommit()
{
    ASSERT(m_lockAcquired);

    // Spec 4.3.2.7: Perform postflight steps, jumping to the error callback if they fail.
    if (m_wrapper && !m_wrapper->performPostflight(this)) {
        m_transactionError = m_wrapper->sqlError();
        if (!m_transactionError)
            m_transactionError = SQLError::create(SQLError::UNKNOWN_ERR, "unknown error occurred during transaction postflight");
        return nextStateForTransactionError();
    }

    // Spec 4.3.2.7: Commit the transaction, jumping to the error callback if that fails.
    ASSERT(m_sqliteTransaction);

    m_database->disableAuthorizer();
    m_sqliteTransaction->commit();
    m_database->enableAuthorizer();

    releaseOriginLockIfNeeded();

    // If the commit failed, the transaction will still be marked as "in progress"
    if (m_sqliteTransaction->inProgress()) {
        if (m_wrapper)
            m_wrapper->handleCommitFailedAfterPostflight(this);
        m_transactionError = SQLError::create(SQLError::DATABASE_ERR, "unable to commit transaction",
            m_database->sqliteDatabase().lastError(), m_database->sqliteDatabase().lastErrorMsg());
        return nextStateForTransactionError();
    }

    // Vacuum the database if anything was deleted.
    if (m_database->hadDeletes())
        m_database->incrementalVacuumIfNeeded();

    // The commit was successful. If the transaction modified this database, notify the delegates.
    if (m_modifiedDatabase)
        m_database->transactionClient()->didCommitWriteTransaction(database());

    // Spec 4.3.2.8: Deliver success callback, if there is one.
    return SQLTransactionState::DeliverSuccessCallback;
}

SQLTransactionState SQLTransactionBackend::cleanupAndTerminate()
{
    ASSERT(m_lockAcquired);

    // Spec 4.3.2.9: End transaction steps. There is no next step.
    LOG(StorageAPI, "Transaction %p is complete\n", this);
    ASSERT(!m_database->sqliteDatabase().transactionInProgress());

    // Phase 5 cleanup. See comment on the SQLTransaction life-cycle above.
    doCleanup();
    m_database->inProgressTransactionCompleted();
    return SQLTransactionState::End;
}

SQLTransactionState SQLTransactionBackend::nextStateForTransactionError()
{
    ASSERT(m_transactionError);
    if (m_hasErrorCallback)
        return SQLTransactionState::DeliverTransactionErrorCallback;

    // No error callback, so fast-forward to the next state and rollback the
    // transaction.
    return SQLTransactionState::CleanupAfterTransactionErrorCallback;
}

SQLTransactionState SQLTransactionBackend::cleanupAfterTransactionErrorCallback()
{
    ASSERT(m_lockAcquired);

    LOG(StorageAPI, "Transaction %p is complete with an error\n", this);
    m_database->disableAuthorizer();
    if (m_sqliteTransaction) {
        // Spec 4.3.2.10: Rollback the transaction.
        m_sqliteTransaction->rollback();

        ASSERT(!m_database->sqliteDatabase().transactionInProgress());
        m_sqliteTransaction.clear();
    }
    m_database->enableAuthorizer();

    releaseOriginLockIfNeeded();

    ASSERT(!m_database->sqliteDatabase().transactionInProgress());

    return SQLTransactionState::CleanupAndTerminate;
}

// requestTransitToState() can be called from the frontend. Hence, it should
// NOT be modifying SQLTransactionBackend in general. The only safe field to
// modify is m_requestedState which is meant for this purpose.
void SQLTransactionBackend::requestTransitToState(SQLTransactionState nextState)
{
    LOG(StorageAPI, "Scheduling %s for transaction %p\n", nameForSQLTransactionState(nextState), this);
    m_requestedState = nextState;
    ASSERT(m_requestedState != SQLTransactionState::End);
    m_database->scheduleTransactionStep(this);
}

// This state function is used as a stub function to plug unimplemented states
// in the state dispatch table. They are unimplemented because they should
// never be reached in the course of correct execution.
SQLTransactionState SQLTransactionBackend::unreachableState()
{
    ASSERT_NOT_REACHED();
    return SQLTransactionState::End;
}

SQLTransactionState SQLTransactionBackend::sendToFrontendState()
{
    ASSERT(m_nextState != SQLTransactionState::Idle);
    m_frontend->requestTransitToState(m_nextState);
    return SQLTransactionState::Idle;
}

void SQLTransactionBackend::acquireOriginLock()
{
    ASSERT(!m_originLock);
    m_originLock = DatabaseTracker::tracker().originLockFor(m_database->securityOrigin());
    m_originLock->lock();
}

void SQLTransactionBackend::releaseOriginLockIfNeeded()
{
    if (m_originLock) {
        m_originLock->unlock();
        m_originLock.clear();
    }
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
