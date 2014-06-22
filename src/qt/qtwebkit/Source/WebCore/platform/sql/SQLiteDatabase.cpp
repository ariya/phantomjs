/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "SQLiteDatabase.h"

#include "DatabaseAuthorizer.h"
#include "Logging.h"
#include "SQLiteFileSystem.h"
#include "SQLiteStatement.h"
#include <sqlite3.h>
#include <wtf/Threading.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

const int SQLResultDone = SQLITE_DONE;
const int SQLResultError = SQLITE_ERROR;
const int SQLResultOk = SQLITE_OK;
const int SQLResultRow = SQLITE_ROW;
const int SQLResultSchema = SQLITE_SCHEMA;
const int SQLResultFull = SQLITE_FULL;
const int SQLResultInterrupt = SQLITE_INTERRUPT;
const int SQLResultConstraint = SQLITE_CONSTRAINT;

static const char notOpenErrorMessage[] = "database is not open";

SQLiteDatabase::SQLiteDatabase()
    : m_db(0)
    , m_pageSize(-1)
    , m_transactionInProgress(false)
    , m_sharable(false)
    , m_openingThread(0)
    , m_interrupted(false)
    , m_openError(SQLITE_ERROR)
    , m_openErrorMessage()
    , m_lastChangesCount(0)
{
}

SQLiteDatabase::~SQLiteDatabase()
{
    close();
}

bool SQLiteDatabase::open(const String& filename, bool forWebSQLDatabase)
{
    close();

    m_openError = SQLiteFileSystem::openDatabase(filename, &m_db, forWebSQLDatabase);
    if (m_openError != SQLITE_OK) {
        m_openErrorMessage = m_db ? sqlite3_errmsg(m_db) : "sqlite_open returned null";
        LOG_ERROR("SQLite database failed to load from %s\nCause - %s", filename.ascii().data(),
            m_openErrorMessage.data());
        sqlite3_close(m_db);
        m_db = 0;
        return false;
    }

    m_openError = sqlite3_extended_result_codes(m_db, 1);
    if (m_openError != SQLITE_OK) {
        m_openErrorMessage = sqlite3_errmsg(m_db);
        LOG_ERROR("SQLite database error when enabling extended errors - %s", m_openErrorMessage.data());
        sqlite3_close(m_db);
        m_db = 0;
        return false;
    }

    if (isOpen())
        m_openingThread = currentThread();
    else
        m_openErrorMessage = "sqlite_open returned null";

    if (!SQLiteStatement(*this, ASCIILiteral("PRAGMA temp_store = MEMORY;")).executeCommand())
        LOG_ERROR("SQLite database could not set temp_store to memory");

    return isOpen();
}

void SQLiteDatabase::close()
{
    if (m_db) {
        // FIXME: This is being called on the main thread during JS GC. <rdar://problem/5739818>
        // ASSERT(currentThread() == m_openingThread);
        sqlite3* db = m_db;
        {
            MutexLocker locker(m_databaseClosingMutex);
            m_db = 0;
        }
        sqlite3_close(db);
    }

    m_openingThread = 0;
    m_openError = SQLITE_ERROR;
    m_openErrorMessage = CString();
}

void SQLiteDatabase::interrupt()
{
    m_interrupted = true;
    while (!m_lockingMutex.tryLock()) {
        MutexLocker locker(m_databaseClosingMutex);
        if (!m_db)
            return;
        sqlite3_interrupt(m_db);
        yield();
    }

    m_lockingMutex.unlock();
}

bool SQLiteDatabase::isInterrupted()
{
    ASSERT(!m_lockingMutex.tryLock());
    return m_interrupted;
}

void SQLiteDatabase::setFullsync(bool fsync) 
{
    if (fsync) 
        executeCommand(ASCIILiteral("PRAGMA fullfsync = 1;"));
    else
        executeCommand(ASCIILiteral("PRAGMA fullfsync = 0;"));
}

int64_t SQLiteDatabase::maximumSize()
{
    int64_t maxPageCount = 0;

    {
        MutexLocker locker(m_authorizerLock);
        enableAuthorizer(false);
        SQLiteStatement statement(*this, ASCIILiteral("PRAGMA max_page_count"));
        maxPageCount = statement.getColumnInt64(0);
        enableAuthorizer(true);
    }

    return maxPageCount * pageSize();
}

void SQLiteDatabase::setMaximumSize(int64_t size)
{
    if (size < 0)
        size = 0;
    
    int currentPageSize = pageSize();

    ASSERT(currentPageSize || !m_db);
    int64_t newMaxPageCount = currentPageSize ? size / currentPageSize : 0;
    
    MutexLocker locker(m_authorizerLock);
    enableAuthorizer(false);

    SQLiteStatement statement(*this, "PRAGMA max_page_count = " + String::number(newMaxPageCount));
    statement.prepare();
    if (statement.step() != SQLResultRow)
#if OS(WINDOWS)
        LOG_ERROR("Failed to set maximum size of database to %I64i bytes", static_cast<long long>(size));
#else
        LOG_ERROR("Failed to set maximum size of database to %lli bytes", static_cast<long long>(size));
#endif

    enableAuthorizer(true);

}

int SQLiteDatabase::pageSize()
{
    // Since the page size of a database is locked in at creation and therefore cannot be dynamic, 
    // we can cache the value for future use
    if (m_pageSize == -1) {
        MutexLocker locker(m_authorizerLock);
        enableAuthorizer(false);
        
        SQLiteStatement statement(*this, ASCIILiteral("PRAGMA page_size"));
        m_pageSize = statement.getColumnInt(0);
        
        enableAuthorizer(true);
    }

    return m_pageSize;
}

int64_t SQLiteDatabase::freeSpaceSize()
{
    int64_t freelistCount = 0;

    {
        MutexLocker locker(m_authorizerLock);
        enableAuthorizer(false);
        // Note: freelist_count was added in SQLite 3.4.1.
        SQLiteStatement statement(*this, ASCIILiteral("PRAGMA freelist_count"));
        freelistCount = statement.getColumnInt64(0);
        enableAuthorizer(true);
    }

    return freelistCount * pageSize();
}

int64_t SQLiteDatabase::totalSize()
{
    int64_t pageCount = 0;

    {
        MutexLocker locker(m_authorizerLock);
        enableAuthorizer(false);
        SQLiteStatement statement(*this, ASCIILiteral("PRAGMA page_count"));
        pageCount = statement.getColumnInt64(0);
        enableAuthorizer(true);
    }

    return pageCount * pageSize();
}

void SQLiteDatabase::setSynchronous(SynchronousPragma sync)
{
    executeCommand("PRAGMA synchronous = " + String::number(sync));
}

void SQLiteDatabase::setBusyTimeout(int ms)
{
    if (m_db)
        sqlite3_busy_timeout(m_db, ms);
    else
        LOG(SQLDatabase, "BusyTimeout set on non-open database");
}

void SQLiteDatabase::setBusyHandler(int(*handler)(void*, int))
{
    if (m_db)
        sqlite3_busy_handler(m_db, handler, NULL);
    else
        LOG(SQLDatabase, "Busy handler set on non-open database");
}

bool SQLiteDatabase::executeCommand(const String& sql)
{
    return SQLiteStatement(*this, sql).executeCommand();
}

bool SQLiteDatabase::returnsAtLeastOneResult(const String& sql)
{
    return SQLiteStatement(*this, sql).returnsAtLeastOneResult();
}

bool SQLiteDatabase::tableExists(const String& tablename)
{
    if (!isOpen())
        return false;

    String statement = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = '" + tablename + "';";

    SQLiteStatement sql(*this, statement);
    sql.prepare();
    return sql.step() == SQLITE_ROW;
}

void SQLiteDatabase::clearAllTables()
{
    String query = ASCIILiteral("SELECT name FROM sqlite_master WHERE type='table';");
    Vector<String> tables;
    if (!SQLiteStatement(*this, query).returnTextResults(0, tables)) {
        LOG(SQLDatabase, "Unable to retrieve list of tables from database");
        return;
    }
    
    for (Vector<String>::iterator table = tables.begin(); table != tables.end(); ++table ) {
        if (*table == "sqlite_sequence")
            continue;
        if (!executeCommand("DROP TABLE " + *table))
            LOG(SQLDatabase, "Unable to drop table %s", (*table).ascii().data());
    }
}

int SQLiteDatabase::runVacuumCommand()
{
    if (!executeCommand(ASCIILiteral("VACUUM;")))
        LOG(SQLDatabase, "Unable to vacuum database - %s", lastErrorMsg());
    return lastError();
}

int SQLiteDatabase::runIncrementalVacuumCommand()
{
    MutexLocker locker(m_authorizerLock);
    enableAuthorizer(false);

    if (!executeCommand(ASCIILiteral("PRAGMA incremental_vacuum")))
        LOG(SQLDatabase, "Unable to run incremental vacuum - %s", lastErrorMsg());

    enableAuthorizer(true);
    return lastError();
}

int64_t SQLiteDatabase::lastInsertRowID()
{
    if (!m_db)
        return 0;
    return sqlite3_last_insert_rowid(m_db);
}

void SQLiteDatabase::updateLastChangesCount()
{
    if (!m_db)
        return;

    m_lastChangesCount = sqlite3_total_changes(m_db);
}

int SQLiteDatabase::lastChanges()
{
    if (!m_db)
        return 0;

    return sqlite3_total_changes(m_db) - m_lastChangesCount;
}

int SQLiteDatabase::lastError()
{
    return m_db ? sqlite3_errcode(m_db) : m_openError;
}

const char* SQLiteDatabase::lastErrorMsg()
{
    if (m_db)
        return sqlite3_errmsg(m_db);
    return m_openErrorMessage.isNull() ? notOpenErrorMessage : m_openErrorMessage.data();
}

#ifndef NDEBUG
void SQLiteDatabase::disableThreadingChecks()
{
    // This doesn't guarantee that SQList was compiled with -DTHREADSAFE, or that you haven't turned off the mutexes.
#if SQLITE_VERSION_NUMBER >= 3003001
    m_sharable = true;
#else
    ASSERT(0); // Your SQLite doesn't support sharing handles across threads.
#endif
}
#endif

int SQLiteDatabase::authorizerFunction(void* userData, int actionCode, const char* parameter1, const char* parameter2, const char* /*databaseName*/, const char* /*trigger_or_view*/)
{
    DatabaseAuthorizer* auth = static_cast<DatabaseAuthorizer*>(userData);
    ASSERT(auth);

    switch (actionCode) {
        case SQLITE_CREATE_INDEX:
            return auth->createIndex(parameter1, parameter2);
        case SQLITE_CREATE_TABLE:
            return auth->createTable(parameter1);
        case SQLITE_CREATE_TEMP_INDEX:
            return auth->createTempIndex(parameter1, parameter2);
        case SQLITE_CREATE_TEMP_TABLE:
            return auth->createTempTable(parameter1);
        case SQLITE_CREATE_TEMP_TRIGGER:
            return auth->createTempTrigger(parameter1, parameter2);
        case SQLITE_CREATE_TEMP_VIEW:
            return auth->createTempView(parameter1);
        case SQLITE_CREATE_TRIGGER:
            return auth->createTrigger(parameter1, parameter2);
        case SQLITE_CREATE_VIEW:
            return auth->createView(parameter1);
        case SQLITE_DELETE:
            return auth->allowDelete(parameter1);
        case SQLITE_DROP_INDEX:
            return auth->dropIndex(parameter1, parameter2);
        case SQLITE_DROP_TABLE:
            return auth->dropTable(parameter1);
        case SQLITE_DROP_TEMP_INDEX:
            return auth->dropTempIndex(parameter1, parameter2);
        case SQLITE_DROP_TEMP_TABLE:
            return auth->dropTempTable(parameter1);
        case SQLITE_DROP_TEMP_TRIGGER:
            return auth->dropTempTrigger(parameter1, parameter2);
        case SQLITE_DROP_TEMP_VIEW:
            return auth->dropTempView(parameter1);
        case SQLITE_DROP_TRIGGER:
            return auth->dropTrigger(parameter1, parameter2);
        case SQLITE_DROP_VIEW:
            return auth->dropView(parameter1);
        case SQLITE_INSERT:
            return auth->allowInsert(parameter1);
        case SQLITE_PRAGMA:
            return auth->allowPragma(parameter1, parameter2);
        case SQLITE_READ:
            return auth->allowRead(parameter1, parameter2);
        case SQLITE_SELECT:
            return auth->allowSelect();
        case SQLITE_TRANSACTION:
            return auth->allowTransaction();
        case SQLITE_UPDATE:
            return auth->allowUpdate(parameter1, parameter2);
        case SQLITE_ATTACH:
            return auth->allowAttach(parameter1);
        case SQLITE_DETACH:
            return auth->allowDetach(parameter1);
        case SQLITE_ALTER_TABLE:
            return auth->allowAlterTable(parameter1, parameter2);
        case SQLITE_REINDEX:
            return auth->allowReindex(parameter1);
#if SQLITE_VERSION_NUMBER >= 3003013 
        case SQLITE_ANALYZE:
            return auth->allowAnalyze(parameter1);
        case SQLITE_CREATE_VTABLE:
            return auth->createVTable(parameter1, parameter2);
        case SQLITE_DROP_VTABLE:
            return auth->dropVTable(parameter1, parameter2);
        case SQLITE_FUNCTION:
            return auth->allowFunction(parameter2);
#endif
        default:
            ASSERT_NOT_REACHED();
            return SQLAuthDeny;
    }
}

void SQLiteDatabase::setAuthorizer(PassRefPtr<DatabaseAuthorizer> auth)
{
    if (!m_db) {
        LOG_ERROR("Attempt to set an authorizer on a non-open SQL database");
        ASSERT_NOT_REACHED();
        return;
    }

    MutexLocker locker(m_authorizerLock);

    m_authorizer = auth;
    
    enableAuthorizer(true);
}

void SQLiteDatabase::enableAuthorizer(bool enable)
{
    if (m_authorizer && enable)
        sqlite3_set_authorizer(m_db, SQLiteDatabase::authorizerFunction, m_authorizer.get());
    else
        sqlite3_set_authorizer(m_db, NULL, 0);
}

bool SQLiteDatabase::isAutoCommitOn() const
{
    return sqlite3_get_autocommit(m_db);
}

bool SQLiteDatabase::turnOnIncrementalAutoVacuum()
{
    SQLiteStatement statement(*this, ASCIILiteral("PRAGMA auto_vacuum"));
    int autoVacuumMode = statement.getColumnInt(0);
    int error = lastError();

    // Check if we got an error while trying to get the value of the auto_vacuum flag.
    // If we got a SQLITE_BUSY error, then there's probably another transaction in
    // progress on this database. In this case, keep the current value of the
    // auto_vacuum flag and try to set it to INCREMENTAL the next time we open this
    // database. If the error is not SQLITE_BUSY, then we probably ran into a more
    // serious problem and should return false (to log an error message).
    if (error != SQLITE_ROW)
        return false;

    switch (autoVacuumMode) {
    case AutoVacuumIncremental:
        return true;
    case AutoVacuumFull:
        return executeCommand(ASCIILiteral("PRAGMA auto_vacuum = 2"));
    case AutoVacuumNone:
    default:
        if (!executeCommand(ASCIILiteral("PRAGMA auto_vacuum = 2")))
            return false;
        runVacuumCommand();
        error = lastError();
        return (error == SQLITE_OK);
    }
}

} // namespace WebCore
