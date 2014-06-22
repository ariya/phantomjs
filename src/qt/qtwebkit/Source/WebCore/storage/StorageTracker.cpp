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

#include "DatabaseThread.h"
#include "FileSystem.h"
#include "StorageThread.h"
#include "Logging.h"
#include "PageGroup.h"
#include "SQLiteFileSystem.h"
#include "SQLiteStatement.h"
#include "SecurityOrigin.h"
#include "StorageTrackerClient.h"
#include "TextEncoding.h"
#include <wtf/Functional.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebCore {

static StorageTracker* storageTracker = 0;

// If there is no document referencing a storage database, close the underlying database
// after it has been idle for m_StorageDatabaseIdleInterval seconds.
static const double DefaultStorageDatabaseIdleInterval = 300;
    
void StorageTracker::initializeTracker(const String& storagePath, StorageTrackerClient* client)
{
    ASSERT(isMainThread());
    ASSERT(!storageTracker || !storageTracker->m_client);
    
    if (!storageTracker)
        storageTracker = new StorageTracker(storagePath);
    
    storageTracker->m_client = client;
    storageTracker->m_needsInitialization = true;
}

void StorageTracker::internalInitialize()
{
    m_needsInitialization = false;

    ASSERT(isMainThread());

    // Make sure text encoding maps have been built on the main thread, as the StorageTracker thread might try to do it there instead.
    // FIXME (<rdar://problem/9127819>): Is there a more explicit way of doing this besides accessing the UTF8Encoding?
    UTF8Encoding();
    
    storageTracker->setIsActive(true);
    storageTracker->m_thread->start();  
    storageTracker->importOriginIdentifiers();
}

StorageTracker& StorageTracker::tracker()
{
    if (!storageTracker)
        storageTracker = new StorageTracker("");
    if (storageTracker->m_needsInitialization)
        storageTracker->internalInitialize();
    
    return *storageTracker;
}

StorageTracker::StorageTracker(const String& storagePath)
    : m_storageDirectoryPath(storagePath.isolatedCopy())
    , m_client(0)
    , m_thread(StorageThread::create())
    , m_isActive(false)
    , m_needsInitialization(false)
    , m_StorageDatabaseIdleInterval(DefaultStorageDatabaseIdleInterval)
{
}

void StorageTracker::setDatabaseDirectoryPath(const String& path)
{
    MutexLocker locker(m_databaseMutex);

    if (m_database.isOpen())
        m_database.close();

    m_storageDirectoryPath = path.isolatedCopy();

    {
        MutexLocker locker(m_originSetMutex);
        m_originSet.clear();
    }

    if (!m_isActive)
        return;

    importOriginIdentifiers();
}

String StorageTracker::databaseDirectoryPath() const
{
    return m_storageDirectoryPath.isolatedCopy();
}

String StorageTracker::trackerDatabasePath()
{
    ASSERT(!m_databaseMutex.tryLock());
    return SQLiteFileSystem::appendDatabaseFileNameToPath(m_storageDirectoryPath, "StorageTracker.db");
}

void StorageTracker::openTrackerDatabase(bool createIfDoesNotExist)
{
    ASSERT(m_isActive);
    ASSERT(!isMainThread());
    ASSERT(!m_databaseMutex.tryLock());

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

    m_thread->dispatch(bind(&StorageTracker::syncImportOriginIdentifiers, this));
}

void StorageTracker::finishedImportingOriginIdentifiers()
{
    MutexLocker locker(m_databaseMutex);
    if (m_client)
        m_client->didFinishLoadingOrigins();
}

void StorageTracker::syncImportOriginIdentifiers()
{
    ASSERT(m_isActive);
    
    ASSERT(!isMainThread());

    {
        MutexLocker locker(m_databaseMutex);

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
                MutexLocker lockOrigins(m_originSetMutex);
                while ((result = statement.step()) == SQLResultRow)
                    m_originSet.add(statement.getColumnText(0).isolatedCopy());
            }
            
            if (result != SQLResultDone) {
                LOG_ERROR("Failed to read in all origins from the database.");
                return;
            }
        }
    }
    
    syncFileSystemAndTrackerDatabase();
    
    {
        MutexLocker locker(m_clientMutex);

        if (m_client) {
            MutexLocker locker(m_originSetMutex);
            OriginSet::const_iterator end = m_originSet.end();
            for (OriginSet::const_iterator it = m_originSet.begin(); it != end; ++it)
                m_client->dispatchDidModifyOrigin(*it);
        }
    }

    callOnMainThread(bind(&StorageTracker::finishedImportingOriginIdentifiers, this));
}
    
void StorageTracker::syncFileSystemAndTrackerDatabase()
{
    ASSERT(!isMainThread());
    ASSERT(m_isActive);

    Vector<String> paths;
    {
        MutexLocker locker(m_databaseMutex);
        paths = listDirectory(m_storageDirectoryPath, "*.localstorage");
    }

    // Use a copy of m_originSet to find expired entries and to schedule their
    // deletions from disk and from m_originSet.
    OriginSet originSetCopy;
    {
        MutexLocker locker(m_originSetMutex);
        for (OriginSet::const_iterator it = m_originSet.begin(), end = m_originSet.end(); it != end; ++it)
            originSetCopy.add((*it).isolatedCopy());
    }
    
    // Add missing StorageTracker records.
    OriginSet foundOrigins;
    String fileExtension = ASCIILiteral(".localstorage");

    for (Vector<String>::const_iterator it = paths.begin(), end = paths.end(); it != end; ++it) {
        const String& path = *it;

        if (path.length() > fileExtension.length() && path.endsWith(fileExtension, true)) {
            String file = pathGetFileName(path);
            String originIdentifier = file.substring(0, file.length() - fileExtension.length());
            if (!originSetCopy.contains(originIdentifier))
                syncSetOriginDetails(originIdentifier, path);

            foundOrigins.add(originIdentifier);
        }
    }

    // Delete stale StorageTracker records.
    for (OriginSet::const_iterator it = originSetCopy.begin(), end = originSetCopy.end(); it != end; ++it) {
        const String& originIdentifier = *it;
        if (foundOrigins.contains(originIdentifier))
            continue;

        callOnMainThread(bind(&StorageTracker::deleteOriginWithIdentifier, this, originIdentifier.isolatedCopy()));
    }
}

void StorageTracker::setOriginDetails(const String& originIdentifier, const String& databaseFile)
{
    if (!m_isActive)
        return;

    {
        MutexLocker locker(m_originSetMutex);

        if (m_originSet.contains(originIdentifier))
            return;

        m_originSet.add(originIdentifier);
    }

    Function<void ()> function = bind(&StorageTracker::syncSetOriginDetails, this, originIdentifier.isolatedCopy(), databaseFile.isolatedCopy());

    if (isMainThread()) {
        ASSERT(m_thread);
        m_thread->dispatch(function);
    } else {
        // FIXME: This weird ping-ponging was done to fix a deadlock. We should figure out a cleaner way to avoid it instead.
        callOnMainThread(bind(&StorageThread::dispatch, m_thread.get(), function));
    }
}

void StorageTracker::syncSetOriginDetails(const String& originIdentifier, const String& databaseFile)
{
    ASSERT(!isMainThread());

    MutexLocker locker(m_databaseMutex);

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
        MutexLocker locker(m_originSetMutex);
        if (!m_originSet.contains(originIdentifier))
            m_originSet.add(originIdentifier);
    }

    {
        MutexLocker locker(m_clientMutex);
        if (m_client)
            m_client->dispatchDidModifyOrigin(originIdentifier);
    }
}

void StorageTracker::origins(Vector<RefPtr<SecurityOrigin> >& result)
{
    ASSERT(m_isActive);
    
    if (!m_isActive)
        return;

    MutexLocker locker(m_originSetMutex);

    for (OriginSet::const_iterator it = m_originSet.begin(), end = m_originSet.end(); it != end; ++it)
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
        MutexLocker locker(m_originSetMutex);
        willDeleteAllOrigins();
        m_originSet.clear();
    }

    PageGroup::clearLocalStorageForAllOrigins();

    m_thread->dispatch(bind(&StorageTracker::syncDeleteAllOrigins, this));
}
    
void StorageTracker::syncDeleteAllOrigins()
{
    ASSERT(!isMainThread());
    
    MutexLocker locker(m_databaseMutex);
    
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
            MutexLocker locker(m_clientMutex);
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

void StorageTracker::deleteOriginWithIdentifier(const String& originIdentifier)
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
        MutexLocker locker(m_originSetMutex);
        willDeleteOrigin(originId);
        m_originSet.remove(originId);
    }

    m_thread->dispatch(bind(&StorageTracker::syncDeleteOrigin, this, originId.isolatedCopy()));
}

void StorageTracker::syncDeleteOrigin(const String& originIdentifier)
{
    ASSERT(!isMainThread());

    MutexLocker locker(m_databaseMutex);
    
    if (!canDeleteOrigin(originIdentifier)) {
        LOG_ERROR("Attempted to delete origin '%s' while it was being created\n", originIdentifier.ascii().data());
        return;
    }
    
    openTrackerDatabase(false);
    if (!m_database.isOpen())
        return;

    String path = databasePathForOrigin(originIdentifier);
    if (path.isEmpty()) {
        // It is possible to get a request from the API to delete the storage for an origin that
        // has no such storage.
        return;
    }
    
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
        MutexLocker locker(m_originSetMutex);
        m_originSet.remove(originIdentifier);
        shouldDeleteTrackerFiles = m_originSet.isEmpty();
    }

    if (shouldDeleteTrackerFiles) {
        m_database.close();
        SQLiteFileSystem::deleteDatabaseFile(trackerDatabasePath());
        SQLiteFileSystem::deleteEmptyDatabaseDirectory(m_storageDirectoryPath);
    }

    {
        MutexLocker locker(m_clientMutex);
        if (m_client)
            m_client->dispatchDidModifyOrigin(originIdentifier);
    }
}
    
void StorageTracker::willDeleteAllOrigins()
{
    ASSERT(!m_originSetMutex.tryLock());

    OriginSet::const_iterator end = m_originSet.end();
    for (OriginSet::const_iterator it = m_originSet.begin(); it != end; ++it)
        m_originsBeingDeleted.add((*it).isolatedCopy());
}

void StorageTracker::willDeleteOrigin(const String& originIdentifier)
{
    ASSERT(isMainThread());
    ASSERT(!m_originSetMutex.tryLock());

    m_originsBeingDeleted.add(originIdentifier);
}
    
bool StorageTracker::canDeleteOrigin(const String& originIdentifier)
{
    ASSERT(!m_databaseMutex.tryLock());
    MutexLocker locker(m_originSetMutex);
    return m_originsBeingDeleted.contains(originIdentifier);
}

void StorageTracker::cancelDeletingOrigin(const String& originIdentifier)
{
    if (!m_isActive)
        return;

    MutexLocker locker(m_databaseMutex);
    {
        MutexLocker locker(m_originSetMutex);
        if (!m_originsBeingDeleted.isEmpty())
            m_originsBeingDeleted.remove(originIdentifier);
    }
}

bool StorageTracker::isActive()
{
    return m_isActive;
}

void StorageTracker::setIsActive(bool flag)
{
    m_isActive = flag;
}
    
String StorageTracker::databasePathForOrigin(const String& originIdentifier)
{
    ASSERT(!m_databaseMutex.tryLock());
    ASSERT(m_isActive);
    
    if (!m_database.isOpen())
        return String();
    
    SQLiteStatement pathStatement(m_database, "SELECT path FROM Origins WHERE origin=?");
    if (pathStatement.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare selection of path for origin '%s'", originIdentifier.ascii().data());
        return String();
    }
    pathStatement.bindText(1, originIdentifier);
    int result = pathStatement.step();
    if (result != SQLResultRow)
        return String();

    return pathStatement.getColumnText(0);
}
    
long long StorageTracker::diskUsageForOrigin(SecurityOrigin* origin)
{
    if (!m_isActive)
        return 0;

    MutexLocker locker(m_databaseMutex);

    String path = databasePathForOrigin(origin->databaseIdentifier());
    if (path.isEmpty())
        return 0;

    return SQLiteFileSystem::getDatabaseFileSize(path);
}

} // namespace WebCore
