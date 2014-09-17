/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "StorageTracker.h"

#if ENABLE(DOM_STORAGE)

#include "DatabaseThread.h"
#include "FileSystem.h"
#include "LocalStorageTask.h"
#include "LocalStorageThread.h"
#include "Logging.h"
#include "PageGroup.h"
#include "SQLiteFileSystem.h"
#include "SQLiteStatement.h"
#include "SecurityOrigin.h"
#include "StorageTrackerClient.h"
#include "TextEncoding.h"
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebCore {

static StorageTracker* storageTracker = 0;

void StorageTracker::initializeTracker(const String& storagePath)
{
    ASSERT(isMainThread());
    ASSERT(!storageTracker);
    
    if (!storageTracker)
        storageTracker = new StorageTracker(storagePath);
    
    // Make sure text encoding maps have been built on the main thread, as the StorageTracker thread might try to do it there instead.
    // FIXME (<rdar://problem/9127819>): Is there a more explicit way of doing this besides accessing the UTF8Encoding?
    UTF8Encoding();
    
    SQLiteFileSystem::registerSQLiteVFS();
    storageTracker->setIsActive(true);
    storageTracker->m_thread->start();  
    storageTracker->importOriginIdentifiers();
}

StorageTracker& StorageTracker::tracker()
{
    if (!storageTracker)
        storageTracker = new StorageTracker("");
    
    return *storageTracker;
}

StorageTracker::StorageTracker(const String& storagePath)
    : m_client(0)
    , m_thread(LocalStorageThread::create())
    , m_isActive(false)
{
    setStorageDirectoryPath(storagePath);
}

void StorageTracker::setStorageDirectoryPath(const String& path)
{
    MutexLocker lockDatabase(m_databaseGuard);
    ASSERT(!m_database.isOpen());
    
    m_storageDirectoryPath = path.threadsafeCopy();
}

String StorageTracker::trackerDatabasePath()
{
    ASSERT(!m_databaseGuard.tryLock());
    return SQLiteFileSystem::appendDatabaseFileNameToPath(m_storageDirectoryPath, "StorageTracker.db");
}

void StorageTracker::openTrackerDatabase(bool createIfDoesNotExist)
{
    ASSERT(m_isActive);
    ASSERT(!isMainThread());
    ASSERT(!m_databaseGuard.tryLock());

    if (m_database.isOpen())
        return;
    
    String databasePath = trackerDatabasePath();
    
    if (!SQLiteFileSystem::ensureDatabaseFileExists(databasePath, createIfDoesNotExist)) {
        if (createIfDoesNotExist)
            LOG_ERROR("Failed to create database file '%s'", databasePath.ascii().data());
        return;
    }
    
    if (!m_database.open(databasePath)) {
        LOG_ERROR("Failed to open databasePath %s.", databasePath.ascii().data());
        return;
    }
    
    m_database.disableThreadingChecks();
    
    if (!m_database.tableExists("Origins")) {
        if (!m_database.executeCommand("CREATE TABLE Origins (origin TEXT UNIQUE ON CONFLICT REPLACE, path TEXT);"))
            LOG_ERROR("Failed to create Origins table.");
    }
}

void StorageTracker::importOriginIdentifiers()
{   
    if (!m_isActive)
        return;
    
    ASSERT(isMainThread());
    ASSERT(m_thread);

    m_thread->scheduleTask(LocalStorageTask::createOriginIdentifiersImport());
}

void StorageTracker::syncImportOriginIdentifiers()
{
    ASSERT(m_isActive);
    
    ASSERT(!isMainThread());

    {
        MutexLocker lockDatabase(m_databaseGuard);

        // Don't force creation of StorageTracker's db just because a tracker
        // was initialized. It will be created if local storage dbs are found
        // by syncFileSystemAndTrackerDatabse() or the next time a local storage
        // db is created by StorageAreaSync.
        openTrackerDatabase(false);

        if (m_database.isOpen()) {
            SQLiteStatement statement(m_database, "SELECT origin FROM Origins");
            if (statement.prepare() != SQLResultOk) {
                LOG_ERROR("Failed to prepare statement.");
                return;
            }
            
            int result;
            
            {
                MutexLocker lockOrigins(m_originSetGuard);
                while ((result = statement.step()) == SQLResultRow)
                    m_originSet.add(statement.getColumnText(0).threadsafeCopy());
            }
            
            if (result != SQLResultDone) {
                LOG_ERROR("Failed to read in all origins from the database.");
                return;
            }
        }
    }
    
    syncFileSystemAndTrackerDatabase();
    
    {
        MutexLocker lockClient(m_clientGuard);
        if (m_client) {
            MutexLocker lockOrigins(m_originSetGuard);
            OriginSet::const_iterator end = m_originSet.end();
            for (OriginSet::const_iterator it = m_originSet.begin(); it != end; ++it)
                m_client->dispatchDidModifyOrigin(*it);
        }
    }
}
    
void StorageTracker::syncFileSystemAndTrackerDatabase()
{
    ASSERT(!isMainThread());
    ASSERT(m_isActive);

    m_databaseGuard.lock();
    DEFINE_STATIC_LOCAL(const String, fileMatchPattern, ("*.localstorage"));
    DEFINE_STATIC_LOCAL(const String, fileExt, (".localstorage"));
    DEFINE_STATIC_LOCAL(const unsigned, fileExtLength, (fileExt.length()));
    m_databaseGuard.unlock();

    Vector<String> paths;
    {
        MutexLocker lock(m_databaseGuard);
        paths = listDirectory(m_storageDirectoryPath, fileMatchPattern);
    }

    // Use a copy of m_originSet to find expired entries and to schedule their
    // deletions from disk and from m_originSet.
    OriginSet originSetCopy;
    {
        MutexLocker lock(m_originSetGuard);
        OriginSet::const_iterator end = m_originSet.end();
        for (OriginSet::const_iterator it = m_originSet.begin(); it != end; ++it)
            originSetCopy.add((*it).threadsafeCopy());
    }
    
    // Add missing StorageTracker records.
    OriginSet foundOrigins;
    Vector<String>::const_iterator end = paths.end();
    for (Vector<String>::const_iterator it = paths.begin(); it != end; ++it) {
        String path = *it;
        if (path.endsWith(fileExt, true) && path.length() > fileExtLength) {
            String file = pathGetFileName(path);
            String originIdentifier = file.substring(0, file.length() - fileExtLength);
            if (!originSetCopy.contains(originIdentifier))
                syncSetOriginDetails(originIdentifier, path);

            foundOrigins.add(originIdentifier);
        }
    }

    // Delete stale StorageTracker records.
    OriginSet::const_iterator setEnd = originSetCopy.end();
    for (OriginSet::const_iterator it = originSetCopy.begin(); it != setEnd; ++it) {
        if (!foundOrigins.contains(*it)) {
            RefPtr<StringImpl> originIdentifier = (*it).threadsafeCopy().impl();
            callOnMainThread(deleteOriginOnMainThread, originIdentifier.release().leakRef());
        }
    }
}

void StorageTracker::setOriginDetails(const String& originIdentifier, const String& databaseFile)
{
    if (!m_isActive)
        return;

    {
        MutexLocker lockOrigins(m_originSetGuard);

        if (m_originSet.contains(originIdentifier))
            return;

        m_originSet.add(originIdentifier);
    }

    OwnPtr<LocalStorageTask> task = LocalStorageTask::createSetOriginDetails(originIdentifier.threadsafeCopy(), databaseFile);

    if (isMainThread()) {
        ASSERT(m_thread);
        m_thread->scheduleTask(task.release());
    } else 
        callOnMainThread(scheduleTask, reinterpret_cast<void*>(task.leakPtr()));
}

void StorageTracker::scheduleTask(void* taskIn)
{
    ASSERT(isMainThread());
    ASSERT(StorageTracker::tracker().m_thread);
    
    OwnPtr<LocalStorageTask> task = adoptPtr(reinterpret_cast<LocalStorageTask*>(taskIn));

    StorageTracker::tracker().m_thread->scheduleTask(task.release());
}

void StorageTracker::syncSetOriginDetails(const String& originIdentifier, const String& databaseFile)
{
    ASSERT(!isMainThread());

    MutexLocker lockDatabase(m_databaseGuard);

    openTrackerDatabase(true);
    
    if (!m_database.isOpen())
        return;

    SQLiteStatement statement(m_database, "INSERT INTO Origins VALUES (?, ?)");
    if (statement.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to establish origin '%s' in the tracker", originIdentifier.ascii().data());
        return;
    } 
    
    statement.bindText(1, originIdentifier);
    statement.bindText(2, databaseFile);
    
    if (statement.step() != SQLResultDone)
        LOG_ERROR("Unable to establish origin '%s' in the tracker", originIdentifier.ascii().data());

    {
        MutexLocker lockOrigins(m_originSetGuard);
        if (!m_originSet.contains(originIdentifier))
            m_originSet.add(originIdentifier);
    }

    {
        MutexLocker lockClient(m_clientGuard);
        if (m_client)
            m_client->dispatchDidModifyOrigin(originIdentifier);
    }
}

void StorageTracker::origins(Vector<RefPtr<SecurityOrigin> >& result)
{
    ASSERT(m_isActive);
    
    if (!m_isActive)
        return;

    MutexLocker lockOrigins(m_originSetGuard);

    OriginSet::const_iterator end = m_originSet.end();
    for (OriginSet::const_iterator it = m_originSet.begin(); it != end; ++it)
        result.append(SecurityOrigin::createFromDatabaseIdentifier(*it));
}

void StorageTracker::deleteAllOrigins()
{
    ASSERT(m_isActive);
    ASSERT(isMainThread());
    ASSERT(m_thread);
    
    if (!m_isActive)
        return;

    {
        MutexLocker lockOrigins(m_originSetGuard);
        willDeleteAllOrigins();
        m_originSet.clear();
    }

    PageGroup::clearLocalStorageForAllOrigins();
    
    m_thread->scheduleTask(LocalStorageTask::createDeleteAllOrigins());
}
    
void StorageTracker::syncDeleteAllOrigins()
{
    ASSERT(!isMainThread());
    
    MutexLocker lockDatabase(m_databaseGuard);
    
    openTrackerDatabase(false);
    if (!m_database.isOpen())
        return;
    
    SQLiteStatement statement(m_database, "SELECT origin, path FROM Origins");
    if (statement.prepare() != SQLResultOk) {
        LOG_ERROR("Failed to prepare statement.");
        return;
    }
    
    int result;
    while ((result = statement.step()) == SQLResultRow) {
        if (!canDeleteOrigin(statement.getColumnText(0)))
            continue;

        SQLiteFileSystem::deleteDatabaseFile(statement.getColumnText(1));

        {
            MutexLocker lockClient(m_clientGuard);
            if (m_client)
                m_client->dispatchDidModifyOrigin(statement.getColumnText(0));
        }
    }
    
    if (result != SQLResultDone)
        LOG_ERROR("Failed to read in all origins from the database.");
    
    if (m_database.isOpen())
        m_database.close();
    
    if (!SQLiteFileSystem::deleteDatabaseFile(trackerDatabasePath())) {
        // In the case where it is not possible to delete the database file (e.g some other program
        // like a virus scanner is accessing it), make sure to remove all entries.
        openTrackerDatabase(false);
        if (!m_database.isOpen())
            return;
        SQLiteStatement deleteStatement(m_database, "DELETE FROM Origins");
        if (deleteStatement.prepare() != SQLResultOk) {
            LOG_ERROR("Unable to prepare deletion of all origins");
            return;
        }
        if (!deleteStatement.executeCommand()) {
            LOG_ERROR("Unable to execute deletion of all origins");
            return;
        }
    }
    SQLiteFileSystem::deleteEmptyDatabaseDirectory(m_storageDirectoryPath);
}

void StorageTracker::deleteOriginOnMainThread(void* originIdentifier)
{
    ASSERT(isMainThread());

    String identifier = adoptRef(reinterpret_cast<StringImpl*>(originIdentifier));
    tracker().deleteOrigin(identifier);
}

void StorageTracker::deleteOrigin(const String& originIdentifier)
{
    deleteOrigin(SecurityOrigin::createFromDatabaseIdentifier(originIdentifier).get());
}

void StorageTracker::deleteOrigin(SecurityOrigin* origin)
{    
    ASSERT(m_isActive);
    ASSERT(isMainThread());
    ASSERT(m_thread);
    
    if (!m_isActive)
        return;

    // Before deleting database, we need to clear in-memory local storage data
    // in StorageArea, and to close the StorageArea db. It's possible for an
    // item to be added immediately after closing the db and cause StorageAreaSync
    // to reopen the db before the db is deleted by a StorageTracker thread.
    // In this case, reopening the db in StorageAreaSync will cancel a pending
    // StorageTracker db deletion.
    PageGroup::clearLocalStorageForOrigin(origin);

    String originId = origin->databaseIdentifier();
    
    {
        MutexLocker lockOrigins(m_originSetGuard);
        willDeleteOrigin(originId);
        m_originSet.remove(originId);
    }
    
    m_thread->scheduleTask(LocalStorageTask::createDeleteOrigin(originId));
}

void StorageTracker::syncDeleteOrigin(const String& originIdentifier)
{
    ASSERT(!isMainThread());

    MutexLocker lockDatabase(m_databaseGuard);
    
    if (!canDeleteOrigin(originIdentifier)) {
        LOG_ERROR("Attempted to delete origin '%s' while it was being created\n", originIdentifier.ascii().data());
        return;
    }
    
    openTrackerDatabase(false);
    if (!m_database.isOpen())
        return;
    
    // Get origin's db file path, delete entry in tracker's db, then delete db file.
    SQLiteStatement pathStatement(m_database, "SELECT path FROM Origins WHERE origin=?");
    if (pathStatement.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare selection of path for origin '%s'", originIdentifier.ascii().data());
        return;
    }
    pathStatement.bindText(1, originIdentifier);
    int result = pathStatement.step();
    if (result != SQLResultRow) {
        LOG_ERROR("Unable to find origin '%s' in Origins table", originIdentifier.ascii().data());
        return;
    }
    
    String path = pathStatement.getColumnText(0);

    ASSERT(!path.isEmpty());
    
    SQLiteStatement deleteStatement(m_database, "DELETE FROM Origins where origin=?");
    if (deleteStatement.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare deletion of origin '%s'", originIdentifier.ascii().data());
        return;
    }
    deleteStatement.bindText(1, originIdentifier);
    if (!deleteStatement.executeCommand()) {
        LOG_ERROR("Unable to execute deletion of origin '%s'", originIdentifier.ascii().data());
        return;
    }

    SQLiteFileSystem::deleteDatabaseFile(path);
    
    bool shouldDeleteTrackerFiles = false;
    {
        MutexLocker originLock(m_originSetGuard);
        m_originSet.remove(originIdentifier);
        shouldDeleteTrackerFiles = m_originSet.isEmpty();
    }

    if (shouldDeleteTrackerFiles) {
        m_database.close();
        SQLiteFileSystem::deleteDatabaseFile(trackerDatabasePath());
        SQLiteFileSystem::deleteEmptyDatabaseDirectory(m_storageDirectoryPath);
    }

    {
        MutexLocker lockClient(m_clientGuard);
        if (m_client)
            m_client->dispatchDidModifyOrigin(originIdentifier);
    }
}
    
void StorageTracker::willDeleteAllOrigins()
{
    ASSERT(!m_originSetGuard.tryLock());

    OriginSet::const_iterator end = m_originSet.end();
    for (OriginSet::const_iterator it = m_originSet.begin(); it != end; ++it)
        m_originsBeingDeleted.add((*it).threadsafeCopy());
}

void StorageTracker::willDeleteOrigin(const String& originIdentifier)
{
    ASSERT(isMainThread());
    ASSERT(!m_originSetGuard.tryLock());

    m_originsBeingDeleted.add(originIdentifier);
}
    
bool StorageTracker::canDeleteOrigin(const String& originIdentifier)
{
    ASSERT(!m_databaseGuard.tryLock());
    MutexLocker lockOrigins(m_originSetGuard);
    return m_originsBeingDeleted.contains(originIdentifier);
}

void StorageTracker::cancelDeletingOrigin(const String& originIdentifier)
{
    if (!m_isActive)
        return;

    MutexLocker lockDatabase(m_databaseGuard);
    MutexLocker lockOrigins(m_originSetGuard);
    if (!m_originsBeingDeleted.isEmpty())
        m_originsBeingDeleted.remove(originIdentifier);
}

void StorageTracker::setClient(StorageTrackerClient* client)
{
    MutexLocker lockClient(m_clientGuard);
    m_client = client;
}

void StorageTracker::syncLocalStorage()
{
    PageGroup::syncLocalStorage();
}
    
bool StorageTracker::isActive()
{
    return m_isActive;
}

void StorageTracker::setIsActive(bool flag)
{
    m_isActive = flag;
}

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)
