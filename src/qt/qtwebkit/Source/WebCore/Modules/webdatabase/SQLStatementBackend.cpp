/*
 * Copyright (C) 2007, 2013 Apple Inc. All rights reserved.
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
#include "SQLStatementBackend.h"

#if ENABLE(SQL_DATABASE)

#include "AbstractSQLStatement.h"
#include "DatabaseBackend.h"
#include "Logging.h"
#include "SQLError.h"
#include "SQLValue.h"
#include "SQLiteDatabase.h"
#include "SQLiteStatement.h"
#include <wtf/text/CString.h>


// The Life-Cycle of a SQLStatement i.e. Who's keeping the SQLStatement alive? 
// ==========================================================================
// The RefPtr chain goes something like this:
//
//     At birth (in SQLTransactionBackend::executeSQL()):
//     =================================================
//     SQLTransactionBackend           // Deque<RefPtr<SQLStatementBackend> > m_statementQueue points to ...
//     --> SQLStatementBackend         // OwnPtr<SQLStatement> m_frontend points to ...
//         --> SQLStatement
//
//     After grabbing the statement for execution (in SQLTransactionBackend::getNextStatement()):
//     =========================================================================================
//     SQLTransactionBackend           // RefPtr<SQLStatementBackend> m_currentStatementBackend points to ...
//     --> SQLStatementBackend         // OwnPtr<SQLStatement> m_frontend points to ...
//         --> SQLStatement
//
//     Then we execute the statement in SQLTransactionBackend::runCurrentStatementAndGetNextState().
//     And we callback to the script in SQLTransaction::deliverStatementCallback() if
//     necessary.
//     - Inside SQLTransaction::deliverStatementCallback(), we operate on a raw SQLStatement*.
//       This pointer is valid because it is owned by SQLTransactionBackend's
//       SQLTransactionBackend::m_currentStatementBackend.
//
//     After we're done executing the statement (in SQLTransactionBackend::getNextStatement()):
//     =======================================================================================
//     When we're done executing, we'll grab the next statement. But before we
//     do that, getNextStatement() nullify SQLTransactionBackend::m_currentStatementBackend.
//     This will trigger the deletion of the SQLStatementBackend and SQLStatement.
//
//     Note: unlike with SQLTransaction, there is no JS representation of SQLStatement.
//     Hence, there is no GC dependency at play here.

namespace WebCore {

PassRefPtr<SQLStatementBackend> SQLStatementBackend::create(PassOwnPtr<AbstractSQLStatement> frontend,
    const String& statement, const Vector<SQLValue>& arguments, int permissions)
{
    return adoptRef(new SQLStatementBackend(frontend, statement, arguments, permissions));
}

SQLStatementBackend::SQLStatementBackend(PassOwnPtr<AbstractSQLStatement> frontend,
    const String& statement, const Vector<SQLValue>& arguments, int permissions)
    : m_frontend(frontend)
    , m_statement(statement.isolatedCopy())
    , m_arguments(arguments)
    , m_hasCallback(m_frontend->hasCallback())
    , m_hasErrorCallback(m_frontend->hasErrorCallback())
    , m_permissions(permissions)
{
    m_frontend->setBackend(this);
}

AbstractSQLStatement* SQLStatementBackend::frontend()
{
    return m_frontend.get();
}

PassRefPtr<SQLError> SQLStatementBackend::sqlError() const
{
    return m_error;
}

PassRefPtr<SQLResultSet> SQLStatementBackend::sqlResultSet() const
{
    return m_resultSet;
}

bool SQLStatementBackend::execute(DatabaseBackend* db)
{
    ASSERT(!m_resultSet);

    // If we're re-running this statement after a quota violation, we need to clear that error now
    clearFailureDueToQuota();

    // This transaction might have been marked bad while it was being set up on the main thread,
    // so if there is still an error, return false.
    if (m_error)
        return false;

    db->setAuthorizerPermissions(m_permissions);

    SQLiteDatabase* database = &db->sqliteDatabase();

    SQLiteStatement statement(*database, m_statement);
    int result = statement.prepare();

    if (result != SQLResultOk) {
        LOG(StorageAPI, "Unable to verify correctness of statement %s - error %i (%s)", m_statement.ascii().data(), result, database->lastErrorMsg());
        if (result == SQLResultInterrupt)
            m_error = SQLError::create(SQLError::DATABASE_ERR, "could not prepare statement", result, "interrupted");
        else
            m_error = SQLError::create(SQLError::SYNTAX_ERR, "could not prepare statement", result, database->lastErrorMsg());
        return false;
    }

    // FIXME: If the statement uses the ?### syntax supported by sqlite, the bind parameter count is very likely off from the number of question marks.
    // If this is the case, they might be trying to do something fishy or malicious
    if (statement.bindParameterCount() != m_arguments.size()) {
        LOG(StorageAPI, "Bind parameter count doesn't match number of question marks");
        m_error = SQLError::create(db->isInterrupted() ? SQLError::DATABASE_ERR : SQLError::SYNTAX_ERR, "number of '?'s in statement string does not match argument count");
        return false;
    }

    for (unsigned i = 0; i < m_arguments.size(); ++i) {
        result = statement.bindValue(i + 1, m_arguments[i]);
        if (result == SQLResultFull) {
            setFailureDueToQuota();
            return false;
        }

        if (result != SQLResultOk) {
            LOG(StorageAPI, "Failed to bind value index %i to statement for query '%s'", i + 1, m_statement.ascii().data());
            m_error = SQLError::create(SQLError::DATABASE_ERR, "could not bind value", result, database->lastErrorMsg());
            return false;
        }
    }

    RefPtr<SQLResultSet> resultSet = SQLResultSet::create();

    // Step so we can fetch the column names.
    result = statement.step();
    if (result == SQLResultRow) {
        int columnCount = statement.columnCount();
        SQLResultSetRowList* rows = resultSet->rows();

        for (int i = 0; i < columnCount; i++)
            rows->addColumn(statement.getColumnName(i));

        do {
            for (int i = 0; i < columnCount; i++)
                rows->addResult(statement.getColumnValue(i));

            result = statement.step();
        } while (result == SQLResultRow);

        if (result != SQLResultDone) {
            m_error = SQLError::create(SQLError::DATABASE_ERR, "could not iterate results", result, database->lastErrorMsg());
            return false;
        }
    } else if (result == SQLResultDone) {
        // Didn't find anything, or was an insert
        if (db->lastActionWasInsert())
            resultSet->setInsertId(database->lastInsertRowID());
    } else if (result == SQLResultFull) {
        // Return the Quota error - the delegate will be asked for more space and this statement might be re-run
        setFailureDueToQuota();
        return false;
    } else if (result == SQLResultConstraint) {
        m_error = SQLError::create(SQLError::CONSTRAINT_ERR, "could not execute statement due to a constaint failure", result, database->lastErrorMsg());
        return false;
    } else {
        m_error = SQLError::create(SQLError::DATABASE_ERR, "could not execute statement", result, database->lastErrorMsg());
        return false;
    }

    // FIXME: If the spec allows triggers, and we want to be "accurate" in a different way, we'd use
    // sqlite3_total_changes() here instead of sqlite3_changed, because that includes rows modified from within a trigger
    // For now, this seems sufficient
    resultSet->setRowsAffected(database->lastChanges());

    m_resultSet = resultSet;
    return true;
}

void SQLStatementBackend::setDatabaseDeletedError()
{
    ASSERT(!m_error && !m_resultSet);
    m_error = SQLError::create(SQLError::UNKNOWN_ERR, "unable to execute statement, because the user deleted the database");
}

void SQLStatementBackend::setVersionMismatchedError()
{
    ASSERT(!m_error && !m_resultSet);
    m_error = SQLError::create(SQLError::VERSION_ERR, "current version of the database and `oldVersion` argument do not match");
}

void SQLStatementBackend::setFailureDueToQuota()
{
    ASSERT(!m_error && !m_resultSet);
    m_error = SQLError::create(SQLError::QUOTA_ERR, "there was not enough remaining storage space, or the storage quota was reached and the user declined to allow more space");
}

void SQLStatementBackend::clearFailureDueToQuota()
{
    if (lastExecutionFailedDueToQuota())
        m_error = 0;
}

bool SQLStatementBackend::lastExecutionFailedDueToQuota() const
{
    return m_error && m_error->code() == SQLError::QUOTA_ERR;
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
