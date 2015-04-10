/*
 * Copyright (C) 2009 Julien Chaffraix <jchaffraix@pleyo.com>
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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
#define ENABLE_COOKIE_DEBUG 0

#include "config.h"
#include "CookieDatabaseBackingStore.h"

#include "CookieManager.h"
#include "Logging.h"
#include "ParsedCookie.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

#if ENABLE_COOKIE_DEBUG
#include <BlackBerryPlatformLog.h>
#define CookieLog(format, ...) BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelInfo, format, ## __VA_ARGS__)
#else
#define CookieLog(format, ...)
#endif

#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformNavigatorHandler.h>

using BlackBerry::Platform::MessageClient;
using BlackBerry::Platform::TypedReplyBuffer;
using BlackBerry::Platform::createMethodCallMessage;

static const double s_databaseTimerInterval = 2;

namespace WebCore {

CookieDatabaseBackingStore::CookieDatabaseBackingStore()
    : MessageClient(MessageClient::ReplyFeature | MessageClient::SyncFeature)
    , m_tableName("cookies") // This is chosen to match Mozilla's table name.
    , m_dbTimer(this, &CookieDatabaseBackingStore::sendChangesToDatabaseTimerFired)
    , m_insertStatement(0)
    , m_updateStatement(0)
    , m_deleteStatement(0)
{
    m_dbTimerClient = new BlackBerry::Platform::GenericTimerClient(this);
    m_dbTimer.setClient(m_dbTimerClient);

    createThread("cookie_database", pthread_attr_default);
}

CookieDatabaseBackingStore::~CookieDatabaseBackingStore()
{
    deleteGuardedObject(m_dbTimerClient);
    m_dbTimerClient = 0;
    // FIXME: This object will never be deleted due to the set up of CookieManager (it's a singleton)
    CookieLog("CookieBackingStore - Destructing");
#ifndef NDEBUG
    {
        MutexLocker lock(m_mutex);
        ASSERT(m_changedCookies.isEmpty());
    }
#endif
}

void CookieDatabaseBackingStore::onThreadFinished()
{
    CookieLog("CookieManager - flushing cookies to backingStore...");
    // This is called from shutdown, so we need to be sure the OS doesn't kill us before the db write finishes.
    // Once should be enough since this extends terimination by 2 seconds.
    BlackBerry::Platform::NavigatorHandler::sendExtendTerminate();
    sendChangesToDatabaseSynchronously();
    CookieLog("CookieManager - finished flushing cookies to backingStore.");

    MessageClient::onThreadFinished();
}

void CookieDatabaseBackingStore::open(const String& cookieJar)
{
    dispatchMessage(createMethodCallMessage(&CookieDatabaseBackingStore::invokeOpen, this, cookieJar));
}

void CookieDatabaseBackingStore::invokeOpen(const String& cookieJar)
{
    ASSERT(isCurrentThread());
    if (m_db.isOpen())
        close();

    if (!m_db.open(cookieJar)) {
        LOG_ERROR("Could not open the cookie database. No cookie will be stored!");
        LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
        return;
    }

    m_db.executeCommand("PRAGMA locking_mode=EXCLUSIVE;");
    m_db.executeCommand("PRAGMA journal_mode=WAL;");

    const String primaryKeyFields("PRIMARY KEY (protocol, host, path, name)");
    const String databaseFields("name TEXT, value TEXT, host TEXT, path TEXT, expiry DOUBLE, lastAccessed DOUBLE, isSecure INTEGER, isHttpOnly INTEGER, creationTime DOUBLE, protocol TEXT");

    StringBuilder createTableQuery;
    createTableQuery.append("CREATE TABLE IF NOT EXISTS ");
    createTableQuery.append(m_tableName);
    // This table schema is compliant with Mozilla's.
    createTableQuery.append(" (" + databaseFields + ", " + primaryKeyFields+");");

    m_db.setBusyTimeout(1000);

    if (!m_db.executeCommand(createTableQuery.toString())) {
        LOG_ERROR("Could not create the table to store the cookies into. No cookie will be stored!");
        LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
        close();
        return;
    }

    StringBuilder insertQuery;
    insertQuery.append("INSERT OR REPLACE INTO ");
    insertQuery.append(m_tableName);
    insertQuery.append(" (name, value, host, path, expiry, lastAccessed, isSecure, isHttpOnly, creationTime, protocol) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10);");

    m_insertStatement = new SQLiteStatement(m_db, insertQuery.toString());
    if (m_insertStatement->prepare()) {
        LOG_ERROR("Cannot save cookies");
        LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
    }

    StringBuilder updateQuery;
    updateQuery.append("UPDATE ");
    updateQuery.append(m_tableName);
    // The where statement is chosen to match CookieMap key.
    updateQuery.append(" SET name = ?1, value = ?2, host = ?3, path = ?4, expiry = ?5, lastAccessed = ?6, isSecure = ?7, isHttpOnly = ?8, creationTime = ?9, protocol = ?10 where name = ?1 and host = ?3 and path = ?4;");
    m_updateStatement = new SQLiteStatement(m_db, updateQuery.toString());

    if (m_updateStatement->prepare()) {
        LOG_ERROR("Cannot update cookies");
        LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
    }

    StringBuilder deleteQuery;
    deleteQuery.append("DELETE FROM ");
    deleteQuery.append(m_tableName);
    // The where statement is chosen to match CookieMap key.
    deleteQuery.append(" WHERE name=?1 and host=?2 and path=?3 and protocol=?4;");
    m_deleteStatement = new SQLiteStatement(m_db, deleteQuery.toString());

    if (m_deleteStatement->prepare()) {
        LOG_ERROR("Cannot delete cookies");
        LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
    }

    BlackBerry::Platform::webKitThreadMessageClient()->dispatchMessage(createMethodCallMessage(&CookieManager::getBackingStoreCookies, &cookieManager()));
}

void CookieDatabaseBackingStore::close()
{
    ASSERT(isCurrentThread());
    CookieLog("CookieBackingStore - Closing");

    size_t changedCookiesSize;
    {
        MutexLocker lock(m_mutex);
        if (m_dbTimer.started())
            m_dbTimer.stop();
        changedCookiesSize = m_changedCookies.size();
    }

    if (changedCookiesSize > 0)
        invokeSendChangesToDatabase();

    delete m_insertStatement;
    m_insertStatement = 0;
    delete m_updateStatement;
    m_updateStatement = 0;
    delete m_deleteStatement;
    m_deleteStatement = 0;

    if (m_db.isOpen())
        m_db.close();
}

void CookieDatabaseBackingStore::insert(const PassRefPtr<ParsedCookie> cookie)
{
    CookieLog("CookieBackingStore - adding inserting cookie %s to queue.", cookie->toString().utf8().data());
    addToChangeQueue(cookie, Insert);
}

void CookieDatabaseBackingStore::update(const PassRefPtr<ParsedCookie> cookie)
{
    CookieLog("CookieBackingStore - adding updating cookie %s to queue.", cookie->toString().utf8().data());
    addToChangeQueue(cookie, Update);
}

void CookieDatabaseBackingStore::remove(const PassRefPtr<ParsedCookie> cookie)
{
    CookieLog("CookieBackingStore - adding deleting cookie %s to queue.", cookie->toString().utf8().data());
    addToChangeQueue(cookie, Delete);
}

void CookieDatabaseBackingStore::removeAll()
{
    dispatchMessage(createMethodCallMessage(&CookieDatabaseBackingStore::invokeRemoveAll, this));
}

void CookieDatabaseBackingStore::invokeRemoveAll()
{
    ASSERT(isCurrentThread());
    if (!m_db.isOpen())
        return;

    CookieLog("CookieBackingStore - remove All cookies from backingstore");

    {
        MutexLocker lock(m_mutex);
        m_changedCookies.clear();
    }

    StringBuilder deleteQuery;
    deleteQuery.append("DELETE FROM ");
    deleteQuery.append(m_tableName);
    deleteQuery.append(";");

    SQLiteStatement deleteStatement(m_db, deleteQuery.toString());
    if (deleteStatement.prepare()) {
        LOG_ERROR("Could not prepare DELETE * statement");
        LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
        return;
    }

    if (!deleteStatement.executeCommand()) {
        LOG_ERROR("Cannot delete cookie from database");
        LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
        return;
    }
}

void CookieDatabaseBackingStore::getCookiesFromDatabase(Vector<RefPtr<ParsedCookie> >& stackOfCookies, unsigned limit)
{
    // It is not a huge performance hit to wait on the reply here because this is only done once during setup and when turning off private mode.
    TypedReplyBuffer< Vector<RefPtr<ParsedCookie> >* > replyBuffer(0);
    dispatchMessage(createMethodCallMessageWithReturn(&CookieDatabaseBackingStore::invokeGetCookiesWithLimit, &replyBuffer, this, limit));
    Vector<RefPtr<ParsedCookie> >* cookies = replyBuffer.pointer();
    if (cookies)
        stackOfCookies.swap(*cookies);
    delete cookies;
}

Vector<RefPtr<ParsedCookie> >* CookieDatabaseBackingStore::invokeGetCookiesWithLimit(unsigned limit)
{
    ASSERT(isCurrentThread());

    // Check that the table exists to avoid doing an unnecessary request.
    if (!m_db.isOpen())
        return 0;

    StringBuilder selectQuery;
    selectQuery.append("SELECT name, value, host, path, expiry, lastAccessed, isSecure, isHttpOnly, creationTime, protocol FROM ");
    selectQuery.append(m_tableName);
    if (limit > 0) {
        selectQuery.append(" ORDER BY lastAccessed ASC");
        selectQuery.append(" LIMIT " + String::number(limit));
    }
    selectQuery.append(";");

    CookieLog("CookieBackingStore - invokeGetAllCookies with select query %s", selectQuery.toString().utf8().data());

    SQLiteStatement selectStatement(m_db, selectQuery.toString());

    if (selectStatement.prepare()) {
        LOG_ERROR("Cannot retrieved cookies from the database");
        LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
        return 0;
    }

    Vector<RefPtr<ParsedCookie> >* cookies = new Vector<RefPtr<ParsedCookie> >;
    while (selectStatement.step() == SQLResultRow) {
        // There is a row to fetch

        String name = selectStatement.getColumnText(0);
        String value = selectStatement.getColumnText(1);
        String domain = selectStatement.getColumnText(2);
        String path = selectStatement.getColumnText(3);
        double expiry = selectStatement.getColumnDouble(4);
        double lastAccessed = selectStatement.getColumnDouble(5);
        bool isSecure = selectStatement.getColumnInt(6);
        bool isHttpOnly = selectStatement.getColumnInt(7);
        double creationTime = selectStatement.getColumnDouble(8);
        String protocol = selectStatement.getColumnText(9);

        cookies->append(ParsedCookie::create(name, value, domain, protocol, path, expiry, lastAccessed, creationTime, isSecure, isHttpOnly));
    }

    return cookies;
}

void CookieDatabaseBackingStore::openAndLoadDatabaseSynchronously(const String& cookieJar)
{
    CookieLog("CookieBackingStore - loading database into CookieManager immediately");

    if (m_db.isOpen()) {
        if (isCurrentThread())
            BlackBerry::Platform::webKitThreadMessageClient()->dispatchSyncMessage(createMethodCallMessage(&CookieManager::getBackingStoreCookies, &cookieManager()));
        else
            cookieManager().getBackingStoreCookies();
    } else {
        if (isCurrentThread())
            invokeOpen(cookieJar);
        else
            dispatchSyncMessage(createMethodCallMessage(&CookieDatabaseBackingStore::invokeOpen, this, cookieJar));
    }
}

void CookieDatabaseBackingStore::sendChangesToDatabaseSynchronously()
{
    CookieLog("CookieBackingStore - sending to database immediately");
    {
        MutexLocker lock(m_mutex);
        if (m_dbTimer.started())
            m_dbTimer.stop();
    }
    if (isCurrentThread())
        invokeSendChangesToDatabase();
    else
        dispatchSyncMessage(createMethodCallMessage(&CookieDatabaseBackingStore::invokeSendChangesToDatabase, this));
}

void CookieDatabaseBackingStore::sendChangesToDatabase(int nextInterval)
{
    MutexLocker lock(m_mutex);
    if (!m_dbTimer.started()) {
        CookieLog("CookieBackingStore - Starting one shot send to database");
        m_dbTimer.start(nextInterval);
    } else {
#ifndef NDEBUG
        CookieLog("CookieBackingStore - Timer already running, skipping this request");
#endif
    }
}

void CookieDatabaseBackingStore::sendChangesToDatabaseTimerFired()
{
    dispatchMessage(createMethodCallMessage(&CookieDatabaseBackingStore::invokeSendChangesToDatabase, this));
}

void CookieDatabaseBackingStore::invokeSendChangesToDatabase()
{
    ASSERT(isCurrentThread());

    if (!m_db.isOpen()) {
        LOG_ERROR("Timer Fired, but database is closed.");
        return;
    }

    Vector<CookieAction> changedCookies;
    {
        MutexLocker lock(m_mutex);
        changedCookies.swap(m_changedCookies);
        ASSERT(m_changedCookies.isEmpty());
    }

    if (changedCookies.isEmpty()) {
        CookieLog("CookieBackingStore - Timer fired, but no cookies in changelist");
        return;
    }
    CookieLog("CookieBackingStore - Timer fired, sending changes to database. We have %d changes", changedCookies.size());
    SQLiteTransaction transaction(m_db, false);
    transaction.begin();

    // Iterate through every element in the change list to make calls
    // If error occurs, ignore it and continue to the next statement
    size_t sizeOfChange = changedCookies.size();
    for (size_t i = 0; i < sizeOfChange; i++) {
        SQLiteStatement* m_statement;
        const RefPtr<ParsedCookie> cookie = changedCookies[i].first;
        UpdateParameter action = changedCookies[i].second;

        if (action == Delete) {
            m_statement = m_deleteStatement;
            CookieLog("CookieBackingStore - deleting cookie %s.", cookie.toString().utf8().data());

            // Binds all the values
            if (m_statement->bindText(1, cookie->name()) || m_statement->bindText(2, cookie->domain())
                || m_statement->bindText(3, cookie->path()) || m_statement->bindText(4, cookie->protocol())) {
                LOG_ERROR("Cannot bind cookie data to delete");
                LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
                ASSERT_NOT_REACHED();
                continue;
            }
        } else {
            if (action == Update) {
                CookieLog("CookieBackingStore - updating cookie %s.", cookie->toString().utf8().data());
                m_statement = m_updateStatement;
            } else {
                CookieLog("CookieBackingStore - inserting cookie %s.", cookie->toString().utf8().data());
                m_statement = m_insertStatement;
            }

            // Binds all the values
            if (m_statement->bindText(1, cookie->name()) || m_statement->bindText(2, cookie->value())
                || m_statement->bindText(3, cookie->domain()) || m_statement->bindText(4, cookie->path())
                || m_statement->bindDouble(5, cookie->expiry()) || m_statement->bindDouble(6, cookie->lastAccessed())
                || m_statement->bindInt64(7, cookie->isSecure()) || m_statement->bindInt64(8, cookie->isHttpOnly())
                || m_statement->bindDouble(9, cookie->creationTime()) || m_statement->bindText(10, cookie->protocol())) {
                LOG_ERROR("Cannot bind cookie data to save");
                LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
                ASSERT_NOT_REACHED();
                continue;
            }
        }

        int rc = m_statement->step();
        m_statement->reset();
        if (rc != SQLResultOk && rc != SQLResultDone) {
            LOG_ERROR("Cannot make call to the database");
            LOG_ERROR("SQLite Error Message: %s", m_db.lastErrorMsg());
            ASSERT_NOT_REACHED();
            continue;
        }
    }
    transaction.commit();
    CookieLog("CookieBackingStore - transaction complete");
}

void CookieDatabaseBackingStore::addToChangeQueue(const PassRefPtr<ParsedCookie> changedCookie, UpdateParameter actionParam)
{
    ASSERT(!changedCookie->isSession());
    CookieAction action(changedCookie, actionParam);
    {
        MutexLocker lock(m_mutex);
        m_changedCookies.append(action);
        CookieLog("CookieBackingStore - m_changedcookies has %d.", m_changedCookies.size());
    }
    sendChangesToDatabase(s_databaseTimerInterval);
}

} // namespace WebCore
