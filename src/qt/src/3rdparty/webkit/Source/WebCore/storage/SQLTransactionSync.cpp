/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SQLTransactionSync.h"

#if ENABLE(DATABASE)

#include "DatabaseAuthorizer.h"
#include "DatabaseSync.h"
#include "PlatformString.h"
#include "SQLException.h"
#include "SQLResultSet.h"
#include "SQLStatementSync.h"
#include "SQLTransactionClient.h"
#include "SQLTransactionSyncCallback.h"
#include "SQLValue.h"
#include "SQLiteTransaction.h"
#include "ScriptExecutionContext.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

PassRefPtr<SQLTransactionSync> SQLTransactionSync::create(DatabaseSync* db, PassRefPtr<SQLTransactionSyncCallback> callback, bool readOnly)
{
    return adoptRef(new SQLTransactionSync(db, callback, readOnly));
}

SQLTransactionSync::SQLTransactionSync(DatabaseSync* db, PassRefPtr<SQLTransactionSyncCallback> callback, bool readOnly)
    : m_database(db)
    , m_callback(callback)
    , m_readOnly(readOnly)
    , m_modifiedDatabase(false)
    , m_transactionClient(adoptPtr(new SQLTransactionClient()))
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
}

SQLTransactionSync::~SQLTransactionSync()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (m_sqliteTransaction && m_sqliteTransaction->inProgress())
        rollback();
}

PassRefPtr<SQLResultSet> SQLTransactionSync::executeSQL(const String& sqlStatement, const Vector<SQLValue>& arguments, ExceptionCode& ec)
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (!m_database->opened()) {
        ec = SQLException::UNKNOWN_ERR;
        return 0;
    }

    if (!m_database->versionMatchesExpected()) {
        ec = SQLException::VERSION_ERR;
        return 0;
    }

    if (sqlStatement.isEmpty())
        return 0;

    int permissions = DatabaseAuthorizer::ReadWriteMask;
    if (!m_database->scriptExecutionContext()->allowDatabaseAccess())
      permissions |= DatabaseAuthorizer::NoAccessMask;
    else if (m_readOnly)
      permissions |= DatabaseAuthorizer::ReadOnlyMask;

    SQLStatementSync statement(sqlStatement, arguments, permissions);

    m_database->resetAuthorizer();
    bool retryStatement = true;
    RefPtr<SQLResultSet> resultSet;
    while (retryStatement) {
        retryStatement = false;
        resultSet = statement.execute(m_database.get(), ec);
        if (!resultSet) {
            if (m_sqliteTransaction->wasRolledBackBySqlite())
                return 0;

            if (ec == SQLException::QUOTA_ERR) {
                if (m_transactionClient->didExceedQuota(database())) {
                    ec = 0;
                    retryStatement = true;
                } else
                    return 0;
            }
        }
    }

    if (m_database->lastActionChangedDatabase()) {
        m_modifiedDatabase = true;
        m_transactionClient->didExecuteStatement(database());
    }

    return resultSet.release();
}

ExceptionCode SQLTransactionSync::begin()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (!m_database->opened())
        return SQLException::UNKNOWN_ERR;

    ASSERT(!m_database->sqliteDatabase().transactionInProgress());

    // Set the maximum usage for this transaction if this transactions is not read-only.
    if (!m_readOnly)
        m_database->sqliteDatabase().setMaximumSize(m_database->maximumSize());

    ASSERT(!m_sqliteTransaction);
    m_sqliteTransaction = adoptPtr(new SQLiteTransaction(m_database->sqliteDatabase(), m_readOnly));

    m_database->resetDeletes();
    m_database->disableAuthorizer();
    m_sqliteTransaction->begin();
    m_database->enableAuthorizer();

    // Check if begin() succeeded.
    if (!m_sqliteTransaction->inProgress()) {
        ASSERT(!m_database->sqliteDatabase().transactionInProgress());
        m_sqliteTransaction.clear();
        return SQLException::DATABASE_ERR;
    }

    return 0;
}

ExceptionCode SQLTransactionSync::execute()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (!m_database->opened() || (m_callback && !m_callback->handleEvent(this))) {
        m_callback = 0;
        return SQLException::UNKNOWN_ERR;
    }

    m_callback = 0;
    return 0;
}

ExceptionCode SQLTransactionSync::commit()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (!m_database->opened())
        return SQLException::UNKNOWN_ERR;

    ASSERT(m_sqliteTransaction);

    m_database->disableAuthorizer();
    m_sqliteTransaction->commit();
    m_database->enableAuthorizer();

    // If the commit failed, the transaction will still be marked as "in progress"
    if (m_sqliteTransaction->inProgress())
        return SQLException::DATABASE_ERR;

    m_sqliteTransaction.clear();

    // Vacuum the database if anything was deleted.
    if (m_database->hadDeletes())
        m_database->incrementalVacuumIfNeeded();

    // The commit was successful. If the transaction modified this database, notify the delegates.
    if (m_modifiedDatabase)
        m_transactionClient->didCommitWriteTransaction(database());

    return 0;
}

void SQLTransactionSync::rollback()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    m_database->disableAuthorizer();
    if (m_sqliteTransaction) {
        m_sqliteTransaction->rollback();
        m_sqliteTransaction.clear();
    }
    m_database->enableAuthorizer();

    ASSERT(!m_database->sqliteDatabase().transactionInProgress());
}

} // namespace WebCore

#endif // ENABLE(DATABASE)
