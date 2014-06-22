/*
 * Copyright (C) 2008, 2009, 2010, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LocalStorageDatabase.h"

#include "LocalStorageDatabaseTracker.h"
#include "WorkQueue.h"
#include <WebCore/FileSystem.h>
#include <WebCore/SQLiteStatement.h>
#include <WebCore/SQLiteTransaction.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/StorageMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

static const double databaseUpdateIntervalInSeconds = 1.0;

static const int maximumItemsToUpdate = 100;

namespace WebKit {

PassRefPtr<LocalStorageDatabase> LocalStorageDatabase::create(PassRefPtr<WorkQueue> queue, PassRefPtr<LocalStorageDatabaseTracker> tracker, PassRefPtr<SecurityOrigin> securityOrigin)
{
    return adoptRef(new LocalStorageDatabase(queue, tracker, securityOrigin));
}

LocalStorageDatabase::LocalStorageDatabase(PassRefPtr<WorkQueue> queue, PassRefPtr<LocalStorageDatabaseTracker> tracker, PassRefPtr<SecurityOrigin> securityOrigin)
    : m_queue(queue)
    , m_tracker(tracker)
    , m_securityOrigin(securityOrigin)
    , m_databasePath(m_tracker->databasePath(m_securityOrigin.get()))
    , m_failedToOpenDatabase(false)
    , m_didImportItems(false)
    , m_isClosed(false)
    , m_didScheduleDatabaseUpdate(false)
    , m_shouldClearItems(false)
{
}

LocalStorageDatabase::~LocalStorageDatabase()
{
    ASSERT(m_isClosed);
}

void LocalStorageDatabase::openDatabase(DatabaseOpeningStrategy openingStrategy)
{
    ASSERT(!m_database.isOpen());
    ASSERT(!m_failedToOpenDatabase);

    if (!tryToOpenDatabase(openingStrategy)) {
        m_failedToOpenDatabase = true;
        return;
    }

    if (m_database.isOpen())
        m_tracker->didOpenDatabaseWithOrigin(m_securityOrigin.get());
}

bool LocalStorageDatabase::tryToOpenDatabase(DatabaseOpeningStrategy openingStrategy)
{
    if (!fileExists(m_databasePath) && openingStrategy == SkipIfNonExistent)
        return true;

    if (m_databasePath.isEmpty()) {
        LOG_ERROR("Filename for local storage database is empty - cannot open for persistent storage");
        return false;
    }

    if (!m_database.open(m_databasePath)) {
        LOG_ERROR("Failed to open database file %s for local storage", m_databasePath.utf8().data());
        return false;
    }

    // Since a WorkQueue isn't bound to a specific thread, we have to disable threading checks
    // even though we never access the database from different threads simultaneously.
    m_database.disableThreadingChecks();

    if (!migrateItemTableIfNeeded()) {
        // We failed to migrate the item table. In order to avoid trying to migrate the table over and over,
        // just delete it and start from scratch.
        if (!m_database.executeCommand("DROP TABLE ItemTable"))
            LOG_ERROR("Failed to delete table ItemTable for local storage");
    }

    if (!m_database.executeCommand("CREATE TABLE IF NOT EXISTS ItemTable (key TEXT UNIQUE ON CONFLICT REPLACE, value BLOB NOT NULL ON CONFLICT FAIL)")) {
        LOG_ERROR("Failed to create table ItemTable for local storage");
        return false;
    }

    return true;
}

bool LocalStorageDatabase::migrateItemTableIfNeeded()
{
    if (!m_database.tableExists("ItemTable"))
        return true;

    SQLiteStatement query(m_database, "SELECT value FROM ItemTable LIMIT 1");

    // This query isn't ever executed, it's just used to check the column type.
    if (query.isColumnDeclaredAsBlob(0))
        return true;

    // Create a new table with the right type, copy all the data over to it and then replace the new table with the old table.
    static const char* commands[] = {
        "DROP TABLE IF EXISTS ItemTable2",
        "CREATE TABLE ItemTable2 (key TEXT UNIQUE ON CONFLICT REPLACE, value BLOB NOT NULL ON CONFLICT FAIL)",
        "INSERT INTO ItemTable2 SELECT * from ItemTable",
        "DROP TABLE ItemTable",
        "ALTER TABLE ItemTable2 RENAME TO ItemTable",
        0,
    };

    SQLiteTransaction transaction(m_database, false);
    transaction.begin();

    for (size_t i = 0; commands[i]; ++i) {
        if (m_database.executeCommand(commands[i]))
            continue;

        LOG_ERROR("Failed to migrate table ItemTable for local storage when executing: %s", commands[i]);
        transaction.rollback();

        return false;
    }

    transaction.commit();
    return true;
}

void LocalStorageDatabase::importItems(StorageMap& storageMap)
{
    if (m_didImportItems)
        return;

    // FIXME: If it can't import, then the default WebKit behavior should be that of private browsing,
    // not silently ignoring it. https://bugs.webkit.org/show_bug.cgi?id=25894

    // We set this to true even if we don't end up importing any items due to failure because
    // there's really no good way to recover other than not importing anything.
    m_didImportItems = true;

    openDatabase(SkipIfNonExistent);
    if (!m_database.isOpen())
        return;

    SQLiteStatement query(m_database, "SELECT key, value FROM ItemTable");
    if (query.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to select items from ItemTable for local storage");
        return;
    }

    HashMap<String, String> items;

    int result = query.step();
    while (result == SQLResultRow) {
        items.set(query.getColumnText(0), query.getColumnBlobAsString(1));
        result = query.step();
    }

    if (result != SQLResultDone) {
        LOG_ERROR("Error reading items from ItemTable for local storage");
        return;
    }

    storageMap.importItems(items);
}

void LocalStorageDatabase::setItem(const String& key, const String& value)
{
    itemDidChange(key, value);
}

void LocalStorageDatabase::removeItem(const String& key)
{
    itemDidChange(key, String());
}

void LocalStorageDatabase::clear()
{
    m_changedItems.clear();
    m_shouldClearItems = true;

    scheduleDatabaseUpdate();
}

void LocalStorageDatabase::close()
{
    ASSERT(!m_isClosed);
    m_isClosed = true;

    if (m_didScheduleDatabaseUpdate) {
        updateDatabaseWithChangedItems(m_changedItems);
        m_changedItems.clear();
    }

    bool isEmpty = databaseIsEmpty();

    if (m_database.isOpen())
        m_database.close();

    if (isEmpty)
        m_tracker->deleteDatabaseWithOrigin(m_securityOrigin.get());
}

void LocalStorageDatabase::itemDidChange(const String& key, const String& value)
{
    m_changedItems.set(key, value);
    scheduleDatabaseUpdate();
}

void LocalStorageDatabase::scheduleDatabaseUpdate()
{
    if (m_didScheduleDatabaseUpdate)
        return;

    m_didScheduleDatabaseUpdate = true;
    m_queue->dispatchAfterDelay(bind(&LocalStorageDatabase::updateDatabase, this), databaseUpdateIntervalInSeconds);
}

void LocalStorageDatabase::updateDatabase()
{
    if (m_isClosed)
        return;

    ASSERT(m_didScheduleDatabaseUpdate);
    m_didScheduleDatabaseUpdate = false;

    HashMap<String, String> changedItems;
    if (m_changedItems.size() <= maximumItemsToUpdate) {
        // There are few enough changed items that we can just always write all of them.
        m_changedItems.swap(changedItems);
    } else {
        for (int i = 0; i < maximumItemsToUpdate; ++i) {
            HashMap<String, String>::iterator it = m_changedItems.begin();
            changedItems.add(it->key, it->value);

            m_changedItems.remove(it);
        }

        ASSERT(changedItems.size() <= maximumItemsToUpdate);

        // Reschedule the update for the remaining items.
        scheduleDatabaseUpdate();
    }

    updateDatabaseWithChangedItems(changedItems);
}

void LocalStorageDatabase::updateDatabaseWithChangedItems(const HashMap<String, String>& changedItems)
{
    if (!m_database.isOpen())
        openDatabase(CreateIfNonExistent);
    if (!m_database.isOpen())
        return;

    if (m_shouldClearItems) {
        m_shouldClearItems = false;

        SQLiteStatement clearStatement(m_database, "DELETE FROM ItemTable");
        if (clearStatement.prepare() != SQLResultOk) {
            LOG_ERROR("Failed to prepare clear statement - cannot write to local storage database");
            return;
        }

        int result = clearStatement.step();
        if (result != SQLResultDone) {
            LOG_ERROR("Failed to clear all items in the local storage database - %i", result);
            return;
        }
    }

    SQLiteStatement insertStatement(m_database, "INSERT INTO ItemTable VALUES (?, ?)");
    if (insertStatement.prepare() != SQLResultOk) {
        LOG_ERROR("Failed to prepare insert statement - cannot write to local storage database");
        return;
    }

    SQLiteStatement deleteStatement(m_database, "DELETE FROM ItemTable WHERE key=?");
    if (deleteStatement.prepare() != SQLResultOk) {
        LOG_ERROR("Failed to prepare delete statement - cannot write to local storage database");
        return;
    }

    SQLiteTransaction transaction(m_database);
    transaction.begin();

    HashMap<String, String>::const_iterator it = changedItems.begin();
    const HashMap<String, String>::const_iterator end = changedItems.end();
    for (; it != end; ++it) {
        // A null value means that the key/value pair should be deleted.
        SQLiteStatement& statement = it->value.isNull() ? deleteStatement : insertStatement;

        statement.bindText(1, it->key);

        // If we're inserting a key/value pair, bind the value as well.
        if (!it->value.isNull())
            statement.bindBlob(2, it->value);

        int result = statement.step();
        if (result != SQLResultDone) {
            LOG_ERROR("Failed to update item in the local storage database - %i", result);
            break;
        }

        statement.reset();
    }

    transaction.commit();
}

bool LocalStorageDatabase::databaseIsEmpty()
{
    if (!m_database.isOpen())
        return false;

    SQLiteStatement query(m_database, "SELECT COUNT(*) FROM ItemTable");
    if (query.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to count number of rows in ItemTable for local storage");
        return false;
    }

    int result = query.step();
    if (result != SQLResultRow) {
        LOG_ERROR("No results when counting number of rows in ItemTable for local storage");
        return false;
    }

    return !query.getColumnInt(0);
}

} // namespace WebKit
