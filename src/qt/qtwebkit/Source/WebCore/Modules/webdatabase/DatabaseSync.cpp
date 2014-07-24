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
#include "DatabaseSync.h"

#if ENABLE(SQL_DATABASE)

#include "DatabaseBackendContext.h"
#include "DatabaseBackendSync.h"
#include "DatabaseCallback.h"
#include "DatabaseContext.h"
#include "DatabaseManager.h"
#include "DatabaseTracker.h"
#include "Logging.h"
#include "SQLException.h"
#include "SQLTransactionSync.h"
#include "SQLTransactionSyncCallback.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {

PassRefPtr<DatabaseSync> DatabaseSync::create(ScriptExecutionContext*, PassRefPtr<DatabaseBackendBase> backend)
{
    return static_cast<DatabaseSync*>(backend.get());
}

DatabaseSync::DatabaseSync(PassRefPtr<DatabaseBackendContext> databaseContext,
    const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize)
    : DatabaseBase(databaseContext->scriptExecutionContext())
    , DatabaseBackendSync(databaseContext, name, expectedVersion, displayName, estimatedSize)
{
    setFrontend(this);
}

DatabaseSync::~DatabaseSync()
{
    ASSERT(m_scriptExecutionContext->isContextThread());
}

PassRefPtr<DatabaseBackendSync> DatabaseSync::backend()
{
    return this;
}

void DatabaseSync::changeVersion(const String& oldVersion, const String& newVersion, PassRefPtr<SQLTransactionSyncCallback> changeVersionCallback, ExceptionCode& ec)
{
    ASSERT(m_scriptExecutionContext->isContextThread());

    if (sqliteDatabase().transactionInProgress()) {
        setLastErrorMessage("unable to changeVersion from within a transaction");
        ec = SQLException::DATABASE_ERR;
        return;
    }

    RefPtr<SQLTransactionSync> transaction = SQLTransactionSync::create(this, changeVersionCallback, false);
    if ((ec = transaction->begin())) {
        ASSERT(!lastErrorMessage().isEmpty());
        return;
    }

    String actualVersion;
    if (!getVersionFromDatabase(actualVersion)) {
        setLastErrorMessage("unable to read the current version", sqliteDatabase().lastError(), sqliteDatabase().lastErrorMsg());
        ec = SQLException::UNKNOWN_ERR;
        return;
    }

    if (actualVersion != oldVersion) {
        setLastErrorMessage("current version of the database and `oldVersion` argument do not match");
        ec = SQLException::VERSION_ERR;
        return;
    }

    if ((ec = transaction->execute())) {
        ASSERT(!lastErrorMessage().isEmpty());
        return;
    }

    if (!setVersionInDatabase(newVersion)) {
        setLastErrorMessage("unable to set the new version", sqliteDatabase().lastError(), sqliteDatabase().lastErrorMsg());
        ec = SQLException::UNKNOWN_ERR;
        return;
    }

    if ((ec = transaction->commit())) {
        ASSERT(!lastErrorMessage().isEmpty());
        setCachedVersion(oldVersion);
        return;
    }

    setExpectedVersion(newVersion);
    setLastErrorMessage("");
}

void DatabaseSync::transaction(PassRefPtr<SQLTransactionSyncCallback> callback, ExceptionCode& ec)
{
    runTransaction(callback, false, ec);
}

void DatabaseSync::readTransaction(PassRefPtr<SQLTransactionSyncCallback> callback, ExceptionCode& ec)
{
    runTransaction(callback, true, ec);
}

void DatabaseSync::runTransaction(PassRefPtr<SQLTransactionSyncCallback> callback, bool readOnly, ExceptionCode& ec)
{
    ASSERT(m_scriptExecutionContext->isContextThread());

    if (sqliteDatabase().transactionInProgress()) {
        setLastErrorMessage("unable to start a transaction from within a transaction");
        ec = SQLException::DATABASE_ERR;
        return;
    }

    RefPtr<SQLTransactionSync> transaction = SQLTransactionSync::create(this, callback, readOnly);
    if ((ec = transaction->begin()) || (ec = transaction->execute()) || (ec = transaction->commit())) {
        ASSERT(!lastErrorMessage().isEmpty());
        transaction->rollback();
    }

    setLastErrorMessage("");
}

void DatabaseSync::markAsDeletedAndClose()
{
    // FIXME: need to do something similar to closeImmediately(), but in a sync way
}

class CloseSyncDatabaseOnContextThreadTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<CloseSyncDatabaseOnContextThreadTask> create(PassRefPtr<DatabaseSync> database)
    {
        return adoptPtr(new CloseSyncDatabaseOnContextThreadTask(database));
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        m_database->closeImmediately();
    }

private:
    CloseSyncDatabaseOnContextThreadTask(PassRefPtr<DatabaseSync> database)
        : m_database(database)
    {
    }

    RefPtr<DatabaseSync> m_database;
};

void DatabaseSync::closeImmediately()
{
    ASSERT(m_scriptExecutionContext->isContextThread());

    if (!opened())
        return;

    logErrorMessage("forcibly closing database");
    closeDatabase();
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
