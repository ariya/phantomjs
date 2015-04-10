/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "SQLTransactionBackendSync.h"

#if ENABLE(SQL_DATABASE)

#include "DatabaseAuthorizer.h"
#include "DatabaseBackendContext.h"
#include "DatabaseSync.h"
#include "SQLException.h"
#include "SQLResultSet.h"
#include "SQLStatementSync.h"
#include "SQLTransactionClient.h"
#include "SQLTransactionSync.h"
#include "SQLTransactionSyncCallback.h"
#include "SQLValue.h"
#include "SQLiteTransaction.h"
#include "ScriptExecutionContext.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

SQLTransactionBackendSync::SQLTransactionBackendSync(DatabaseSync* db, PassRefPtr<SQLTransactionSyncCallback> callback, bool readOnly)
    : m_database(db)
    , m_callback(callback)
    , m_readOnly(readOnly)
    , m_hasVersionMismatch(false)
    , m_modifiedDatabase(false)
    , m_transactionClient(adoptPtr(new SQLTransactionClient()))
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
}

SQLTransactionBackendSync::~SQLTransactionBackendSync()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (m_sqliteTransaction && m_sqliteTransaction->inProgress())
        rollback();
}

PassRefPtr<SQLResultSet> SQLTransactionBackendSync::executeSQL(const String& sqlStatement, const Vector<SQLValue>& arguments, ExceptionCode& ec)
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());

    m_database->setLastErrorMessage("");

    if (!m_database->opened()) {
        m_database->setLastErrorMessage("cannot executeSQL because the database is not open");
        ec = SQLException::UNKNOWN_ERR;
        return 0;
    }

    if (m_hasVersionMismatch) {
        m_database->setLastErrorMessage("cannot executeSQL because there is a version mismatch");
        ec = SQLException::VERSION_ERR;
        return 0;
    }

    if (sqlStatement.isEmpty())
        return 0;

    int permissions = DatabaseAuthorizer::ReadWriteMask;
    if (!m_database->databaseContext()->allowDatabaseAccess())
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
                } else {
                    m_database->setLastErrorMessage("there was not enough remaining storage space");
                    return 0;
                }
            }
        }
    }

    if (m_database->lastActionChangedDatabase())
        m_modifiedDatabase = true;

    return resultSet.release();
}

ExceptionCode SQLTransactionBackendSync::begin()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (!m_database->opened()) {
        m_database->setLastErrorMessage("cannot begin transaction because the database is not open");
        return SQLException::UNKNOWN_ERR;
    }

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
        m_database->setLastErrorMessage("unable to begin transaction",
            m_database->sqliteDatabase().lastError(), m_database->sqliteDatabase().lastErrorMsg());
        m_sqliteTransaction.clear();
        return SQLException::DATABASE_ERR;
    }

    // Note: We intentionally retrieve the actual version even with an empty expected version.
    // In multi-process browsers, we take this opportinutiy to update the cached value for
    // the actual version. In single-process browsers, this is just a map lookup.
    String actualVersion;
    if (!m_database->getActualVersionForTransaction(actualVersion)) {
        m_database->setLastErrorMessage("unable to read version",
            m_database->sqliteDatabase().lastError(), m_database->sqliteDatabase().lastErrorMsg());
        rollback();
        return SQLException::DATABASE_ERR;
    }
    m_hasVersionMismatch = !m_database->expectedVersion().isEmpty() && (m_database->expectedVersion() != actualVersion);
    return 0;
}

ExceptionCode SQLTransactionBackendSync::execute()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (!m_database->opened() || (m_callback && !m_callback->handleEvent(SQLTransactionSync::from(this)))) {
        if (m_database->lastErrorMessage().isEmpty())
            m_database->setLastErrorMessage("failed to execute transaction callback");
        m_callback = 0;
        return SQLException::UNKNOWN_ERR;
    }

    m_callback = 0;
    return 0;
}

ExceptionCode SQLTransactionBackendSync::commit()
{
    ASSERT(m_database->scriptExecutionContext()->isContextThread());
    if (!m_database->opened()) {
        m_database->setLastErrorMessage("unable to commit transaction because the database is not open.");
        return SQLException::UNKNOWN_ERR;
    }

    ASSERT(m_sqliteTransaction);

    m_database->disableAuthorizer();
    m_sqliteTransaction->commit();
    m_database->enableAuthorizer();

    // If the commit failed, the transaction will still be marked as "in progress"
    if (m_sqliteTransaction->inProgress()) {
        m_database->setLastErrorMessage("unable to commit transaction",
            m_database->sqliteDatabase().lastError(), m_database->sqliteDatabase().lastErrorMsg());
        return SQLException::DATABASE_ERR;
    }

    m_sqliteTransaction.clear();

    // Vacuum the database if anything was deleted.
    if (m_database->hadDeletes())
        m_database->incrementalVacuumIfNeeded();

    // The commit was successful. If the transaction modified this database, notify the delegates.
    if (m_modifiedDatabase)
        m_transactionClient->didCommitWriteTransaction(database());
    return 0;
}

void SQLTransactionBackendSync::rollback()
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

#endif // ENABLE(SQL_DATABASE)
