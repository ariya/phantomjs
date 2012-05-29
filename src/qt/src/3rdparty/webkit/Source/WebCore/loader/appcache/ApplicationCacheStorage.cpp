/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ApplicationCacheStorage.h"

#if ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "ApplicationCache.h"
#include "ApplicationCacheGroup.h"
#include "ApplicationCacheHost.h"
#include "ApplicationCacheResource.h"
#include "FileSystem.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include "SecurityOrigin.h"
#include "UUID.h"
#include <wtf/text/CString.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringExtras.h>

using namespace std;

namespace WebCore {

static const char flatFileSubdirectory[] = "ApplicationCache";

template <class T>
class StorageIDJournal {
public:  
    ~StorageIDJournal()
    {
        size_t size = m_records.size();
        for (size_t i = 0; i < size; ++i)
            m_records[i].restore();
    }

    void add(T* resource, unsigned storageID)
    {
        m_records.append(Record(resource, storageID));
    }

    void commit()
    {
        m_records.clear();
    }

private:
    class Record {
    public:
        Record() : m_resource(0), m_storageID(0) { }
        Record(T* resource, unsigned storageID) : m_resource(resource), m_storageID(storageID) { }

        void restore()
        {
            m_resource->setStorageID(m_storageID);
        }

    private:
        T* m_resource;
        unsigned m_storageID;
    };

    Vector<Record> m_records;
};

static unsigned urlHostHash(const KURL& url)
{
    unsigned hostStart = url.hostStart();
    unsigned hostEnd = url.hostEnd();
    
    return AlreadyHashed::avoidDeletedValue(StringHasher::computeHash(url.string().characters() + hostStart, hostEnd - hostStart));
}

ApplicationCacheGroup* ApplicationCacheStorage::loadCacheGroup(const KURL& manifestURL)
{
    openDatabase(false);
    if (!m_database.isOpen())
        return 0;

    SQLiteStatement statement(m_database, "SELECT id, manifestURL, newestCache FROM CacheGroups WHERE newestCache IS NOT NULL AND manifestURL=?");
    if (statement.prepare() != SQLResultOk)
        return 0;
    
    statement.bindText(1, manifestURL);
   
    int result = statement.step();
    if (result == SQLResultDone)
        return 0;
    
    if (result != SQLResultRow) {
        LOG_ERROR("Could not load cache group, error \"%s\"", m_database.lastErrorMsg());
        return 0;
    }
    
    unsigned newestCacheStorageID = static_cast<unsigned>(statement.getColumnInt64(2));

    RefPtr<ApplicationCache> cache = loadCache(newestCacheStorageID);
    if (!cache)
        return 0;
        
    ApplicationCacheGroup* group = new ApplicationCacheGroup(manifestURL);
      
    group->setStorageID(static_cast<unsigned>(statement.getColumnInt64(0)));
    group->setNewestCache(cache.release());

    return group;
}    

ApplicationCacheGroup* ApplicationCacheStorage::findOrCreateCacheGroup(const KURL& manifestURL)
{
    ASSERT(!manifestURL.hasFragmentIdentifier());

    std::pair<CacheGroupMap::iterator, bool> result = m_cachesInMemory.add(manifestURL, 0);
    
    if (!result.second) {
        ASSERT(result.first->second);
        return result.first->second;
    }

    // Look up the group in the database
    ApplicationCacheGroup* group = loadCacheGroup(manifestURL);
    
    // If the group was not found we need to create it
    if (!group) {
        group = new ApplicationCacheGroup(manifestURL);
        m_cacheHostSet.add(urlHostHash(manifestURL));
    }
    
    result.first->second = group;
    
    return group;
}

ApplicationCacheGroup* ApplicationCacheStorage::findInMemoryCacheGroup(const KURL& manifestURL) const
{
    return m_cachesInMemory.get(manifestURL);
}

void ApplicationCacheStorage::loadManifestHostHashes()
{
    static bool hasLoadedHashes = false;
    
    if (hasLoadedHashes)
        return;
    
    // We set this flag to true before the database has been opened
    // to avoid trying to open the database over and over if it doesn't exist.
    hasLoadedHashes = true;
    
    openDatabase(false);
    if (!m_database.isOpen())
        return;

    // Fetch the host hashes.
    SQLiteStatement statement(m_database, "SELECT manifestHostHash FROM CacheGroups");    
    if (statement.prepare() != SQLResultOk)
        return;
    
    while (statement.step() == SQLResultRow)
        m_cacheHostSet.add(static_cast<unsigned>(statement.getColumnInt64(0)));
}    

ApplicationCacheGroup* ApplicationCacheStorage::cacheGroupForURL(const KURL& url)
{
    ASSERT(!url.hasFragmentIdentifier());
    
    loadManifestHostHashes();
    
    // Hash the host name and see if there's a manifest with the same host.
    if (!m_cacheHostSet.contains(urlHostHash(url)))
        return 0;

    // Check if a cache already exists in memory.
    CacheGroupMap::const_iterator end = m_cachesInMemory.end();
    for (CacheGroupMap::const_iterator it = m_cachesInMemory.begin(); it != end; ++it) {
        ApplicationCacheGroup* group = it->second;

        ASSERT(!group->isObsolete());

        if (!protocolHostAndPortAreEqual(url, group->manifestURL()))
            continue;
        
        if (ApplicationCache* cache = group->newestCache()) {
            ApplicationCacheResource* resource = cache->resourceForURL(url);
            if (!resource)
                continue;
            if (resource->type() & ApplicationCacheResource::Foreign)
                continue;
            return group;
        }
    }
    
    if (!m_database.isOpen())
        return 0;
        
    // Check the database. Look for all cache groups with a newest cache.
    SQLiteStatement statement(m_database, "SELECT id, manifestURL, newestCache FROM CacheGroups WHERE newestCache IS NOT NULL");
    if (statement.prepare() != SQLResultOk)
        return 0;
    
    int result;
    while ((result = statement.step()) == SQLResultRow) {
        KURL manifestURL = KURL(ParsedURLString, statement.getColumnText(1));

        if (m_cachesInMemory.contains(manifestURL))
            continue;

        if (!protocolHostAndPortAreEqual(url, manifestURL))
            continue;

        // We found a cache group that matches. Now check if the newest cache has a resource with
        // a matching URL.
        unsigned newestCacheID = static_cast<unsigned>(statement.getColumnInt64(2));
        RefPtr<ApplicationCache> cache = loadCache(newestCacheID);
        if (!cache)
            continue;

        ApplicationCacheResource* resource = cache->resourceForURL(url);
        if (!resource)
            continue;
        if (resource->type() & ApplicationCacheResource::Foreign)
            continue;

        ApplicationCacheGroup* group = new ApplicationCacheGroup(manifestURL);
        
        group->setStorageID(static_cast<unsigned>(statement.getColumnInt64(0)));
        group->setNewestCache(cache.release());
        
        m_cachesInMemory.set(group->manifestURL(), group);
        
        return group;
    }

    if (result != SQLResultDone)
        LOG_ERROR("Could not load cache group, error \"%s\"", m_database.lastErrorMsg());
    
    return 0;
}

ApplicationCacheGroup* ApplicationCacheStorage::fallbackCacheGroupForURL(const KURL& url)
{
    ASSERT(!url.hasFragmentIdentifier());

    // Check if an appropriate cache already exists in memory.
    CacheGroupMap::const_iterator end = m_cachesInMemory.end();
    for (CacheGroupMap::const_iterator it = m_cachesInMemory.begin(); it != end; ++it) {
        ApplicationCacheGroup* group = it->second;
        
        ASSERT(!group->isObsolete());

        if (ApplicationCache* cache = group->newestCache()) {
            KURL fallbackURL;
            if (cache->isURLInOnlineWhitelist(url))
                continue;
            if (!cache->urlMatchesFallbackNamespace(url, &fallbackURL))
                continue;
            if (cache->resourceForURL(fallbackURL)->type() & ApplicationCacheResource::Foreign)
                continue;
            return group;
        }
    }
    
    if (!m_database.isOpen())
        return 0;
        
    // Check the database. Look for all cache groups with a newest cache.
    SQLiteStatement statement(m_database, "SELECT id, manifestURL, newestCache FROM CacheGroups WHERE newestCache IS NOT NULL");
    if (statement.prepare() != SQLResultOk)
        return 0;
    
    int result;
    while ((result = statement.step()) == SQLResultRow) {
        KURL manifestURL = KURL(ParsedURLString, statement.getColumnText(1));

        if (m_cachesInMemory.contains(manifestURL))
            continue;

        // Fallback namespaces always have the same origin as manifest URL, so we can avoid loading caches that cannot match.
        if (!protocolHostAndPortAreEqual(url, manifestURL))
            continue;

        // We found a cache group that matches. Now check if the newest cache has a resource with
        // a matching fallback namespace.
        unsigned newestCacheID = static_cast<unsigned>(statement.getColumnInt64(2));
        RefPtr<ApplicationCache> cache = loadCache(newestCacheID);

        KURL fallbackURL;
        if (cache->isURLInOnlineWhitelist(url))
            continue;
        if (!cache->urlMatchesFallbackNamespace(url, &fallbackURL))
            continue;
        if (cache->resourceForURL(fallbackURL)->type() & ApplicationCacheResource::Foreign)
            continue;

        ApplicationCacheGroup* group = new ApplicationCacheGroup(manifestURL);
        
        group->setStorageID(static_cast<unsigned>(statement.getColumnInt64(0)));
        group->setNewestCache(cache.release());
        
        m_cachesInMemory.set(group->manifestURL(), group);
        
        return group;
    }

    if (result != SQLResultDone)
        LOG_ERROR("Could not load cache group, error \"%s\"", m_database.lastErrorMsg());
    
    return 0;
}

void ApplicationCacheStorage::cacheGroupDestroyed(ApplicationCacheGroup* group)
{
    if (group->isObsolete()) {
        ASSERT(!group->storageID());
        ASSERT(m_cachesInMemory.get(group->manifestURL()) != group);
        return;
    }

    ASSERT(m_cachesInMemory.get(group->manifestURL()) == group);

    m_cachesInMemory.remove(group->manifestURL());
    
    // If the cache group is half-created, we don't want it in the saved set (as it is not stored in database).
    if (!group->storageID())
        m_cacheHostSet.remove(urlHostHash(group->manifestURL()));
}

void ApplicationCacheStorage::cacheGroupMadeObsolete(ApplicationCacheGroup* group)
{
    ASSERT(m_cachesInMemory.get(group->manifestURL()) == group);
    ASSERT(m_cacheHostSet.contains(urlHostHash(group->manifestURL())));

    if (ApplicationCache* newestCache = group->newestCache())
        remove(newestCache);

    m_cachesInMemory.remove(group->manifestURL());
    m_cacheHostSet.remove(urlHostHash(group->manifestURL()));
}

void ApplicationCacheStorage::setCacheDirectory(const String& cacheDirectory)
{
    ASSERT(m_cacheDirectory.isNull());
    ASSERT(!cacheDirectory.isNull());
    
    m_cacheDirectory = cacheDirectory;
}

const String& ApplicationCacheStorage::cacheDirectory() const
{
    return m_cacheDirectory;
}

void ApplicationCacheStorage::setMaximumSize(int64_t size)
{
    m_maximumSize = size;
}

int64_t ApplicationCacheStorage::maximumSize() const
{
    return m_maximumSize;
}

bool ApplicationCacheStorage::isMaximumSizeReached() const
{
    return m_isMaximumSizeReached;
}

int64_t ApplicationCacheStorage::spaceNeeded(int64_t cacheToSave)
{
    int64_t spaceNeeded = 0;
    long long fileSize = 0;
    if (!getFileSize(m_cacheFile, fileSize))
        return 0;

    int64_t currentSize = fileSize + flatFileAreaSize();

    // Determine the amount of free space we have available.
    int64_t totalAvailableSize = 0;
    if (m_maximumSize < currentSize) {
        // The max size is smaller than the actual size of the app cache file.
        // This can happen if the client previously imposed a larger max size
        // value and the app cache file has already grown beyond the current
        // max size value.
        // The amount of free space is just the amount of free space inside
        // the database file. Note that this is always 0 if SQLite is compiled
        // with AUTO_VACUUM = 1.
        totalAvailableSize = m_database.freeSpaceSize();
    } else {
        // The max size is the same or larger than the current size.
        // The amount of free space available is the amount of free space
        // inside the database file plus the amount we can grow until we hit
        // the max size.
        totalAvailableSize = (m_maximumSize - currentSize) + m_database.freeSpaceSize();
    }

    // The space needed to be freed in order to accommodate the failed cache is
    // the size of the failed cache minus any already available free space.
    spaceNeeded = cacheToSave - totalAvailableSize;
    // The space needed value must be positive (or else the total already
    // available free space would be larger than the size of the failed cache and
    // saving of the cache should have never failed).
    ASSERT(spaceNeeded);
    return spaceNeeded;
}

void ApplicationCacheStorage::setDefaultOriginQuota(int64_t quota)
{
    m_defaultOriginQuota = quota;
}

bool ApplicationCacheStorage::quotaForOrigin(const SecurityOrigin* origin, int64_t& quota)
{
    // If an Origin record doesn't exist, then the COUNT will be 0 and quota will be 0.
    // Using the count to determine if a record existed or not is a safe way to determine
    // if a quota of 0 is real, from the record, or from null.
    SQLiteStatement statement(m_database, "SELECT COUNT(quota), quota FROM Origins WHERE origin=?");
    if (statement.prepare() != SQLResultOk)
        return false;

    statement.bindText(1, origin->databaseIdentifier());
    int result = statement.step();

    // Return the quota, or if it was null the default.
    if (result == SQLResultRow) {
        bool wasNoRecord = statement.getColumnInt64(0) == 0;
        quota = wasNoRecord ? m_defaultOriginQuota : statement.getColumnInt64(1);
        return true;
    }

    LOG_ERROR("Could not get the quota of an origin, error \"%s\"", m_database.lastErrorMsg());
    return false;
}

bool ApplicationCacheStorage::usageForOrigin(const SecurityOrigin* origin, int64_t& usage)
{
    // If an Origins record doesn't exist, then the SUM will be null,
    // which will become 0, as expected, when converting to a number.
    SQLiteStatement statement(m_database, "SELECT SUM(Caches.size)"
                                          "  FROM CacheGroups"
                                          " INNER JOIN Origins ON CacheGroups.origin = Origins.origin"
                                          " INNER JOIN Caches ON CacheGroups.id = Caches.cacheGroup"
                                          " WHERE Origins.origin=?");
    if (statement.prepare() != SQLResultOk)
        return false;

    statement.bindText(1, origin->databaseIdentifier());
    int result = statement.step();

    if (result == SQLResultRow) {
        usage = statement.getColumnInt64(0);
        return true;
    }

    LOG_ERROR("Could not get the quota of an origin, error \"%s\"", m_database.lastErrorMsg());
    return false;
}

bool ApplicationCacheStorage::remainingSizeForOriginExcludingCache(const SecurityOrigin* origin, ApplicationCache* cache, int64_t& remainingSize)
{
    openDatabase(false);
    if (!m_database.isOpen())
        return false;

    // Remaining size = total origin quota - size of all caches with origin excluding the provided cache.
    // Keep track of the number of caches so we can tell if the result was a calculation or not.
    const char* query;
    int64_t excludingCacheIdentifier = cache ? cache->storageID() : 0;
    if (excludingCacheIdentifier != 0) {
        query = "SELECT COUNT(Caches.size), Origins.quota - SUM(Caches.size)"
                "  FROM CacheGroups"
                " INNER JOIN Origins ON CacheGroups.origin = Origins.origin"
                " INNER JOIN Caches ON CacheGroups.id = Caches.cacheGroup"
                " WHERE Origins.origin=?"
                "   AND Caches.id!=?";
    } else {
        query = "SELECT COUNT(Caches.size), Origins.quota - SUM(Caches.size)"
                "  FROM CacheGroups"
                " INNER JOIN Origins ON CacheGroups.origin = Origins.origin"
                " INNER JOIN Caches ON CacheGroups.id = Caches.cacheGroup"
                " WHERE Origins.origin=?";
    }

    SQLiteStatement statement(m_database, query);
    if (statement.prepare() != SQLResultOk)
        return false;

    statement.bindText(1, origin->databaseIdentifier());
    if (excludingCacheIdentifier != 0)
        statement.bindInt64(2, excludingCacheIdentifier);
    int result = statement.step();

    // If the count was 0 that then we have to query the origin table directly
    // for its quota. Otherwise we can use the calculated value.
    if (result == SQLResultRow) {
        int64_t numberOfCaches = statement.getColumnInt64(0);
        if (numberOfCaches == 0)
            quotaForOrigin(origin, remainingSize);
        else
            remainingSize = statement.getColumnInt64(1);
        return true;
    }

    LOG_ERROR("Could not get the remaining size of an origin's quota, error \"%s\"", m_database.lastErrorMsg());
    return false;
}

bool ApplicationCacheStorage::storeUpdatedQuotaForOrigin(const SecurityOrigin* origin, int64_t quota)
{
    openDatabase(true);
    if (!m_database.isOpen())
        return false;

    if (!ensureOriginRecord(origin))
        return false;

    SQLiteStatement updateStatement(m_database, "UPDATE Origins SET quota=? WHERE origin=?");
    if (updateStatement.prepare() != SQLResultOk)
        return false;

    updateStatement.bindInt64(1, quota);
    updateStatement.bindText(2, origin->databaseIdentifier());

    return executeStatement(updateStatement);
}

bool ApplicationCacheStorage::executeSQLCommand(const String& sql)
{
    ASSERT(m_database.isOpen());
    
    bool result = m_database.executeCommand(sql);
    if (!result)
        LOG_ERROR("Application Cache Storage: failed to execute statement \"%s\" error \"%s\"", 
                  sql.utf8().data(), m_database.lastErrorMsg());

    return result;
}

// Update the schemaVersion when the schema of any the Application Cache
// SQLite tables changes. This allows the database to be rebuilt when
// a new, incompatible change has been introduced to the database schema.
static const int schemaVersion = 7;
    
void ApplicationCacheStorage::verifySchemaVersion()
{
    int version = SQLiteStatement(m_database, "PRAGMA user_version").getColumnInt(0);
    if (version == schemaVersion)
        return;

    deleteTables();

    // Update user version.
    SQLiteTransaction setDatabaseVersion(m_database);
    setDatabaseVersion.begin();

    char userVersionSQL[32];
    int unusedNumBytes = snprintf(userVersionSQL, sizeof(userVersionSQL), "PRAGMA user_version=%d", schemaVersion);
    ASSERT_UNUSED(unusedNumBytes, static_cast<int>(sizeof(userVersionSQL)) >= unusedNumBytes);

    SQLiteStatement statement(m_database, userVersionSQL);
    if (statement.prepare() != SQLResultOk)
        return;
    
    executeStatement(statement);
    setDatabaseVersion.commit();
}
    
void ApplicationCacheStorage::openDatabase(bool createIfDoesNotExist)
{
    if (m_database.isOpen())
        return;

    // The cache directory should never be null, but if it for some weird reason is we bail out.
    if (m_cacheDirectory.isNull())
        return;

    m_cacheFile = pathByAppendingComponent(m_cacheDirectory, "ApplicationCache.db");
    if (!createIfDoesNotExist && !fileExists(m_cacheFile))
        return;

    makeAllDirectories(m_cacheDirectory);
    m_database.open(m_cacheFile);
    
    if (!m_database.isOpen())
        return;
    
    verifySchemaVersion();
    
    // Create tables
    executeSQLCommand("CREATE TABLE IF NOT EXISTS CacheGroups (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "manifestHostHash INTEGER NOT NULL ON CONFLICT FAIL, manifestURL TEXT UNIQUE ON CONFLICT FAIL, newestCache INTEGER, origin TEXT)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS Caches (id INTEGER PRIMARY KEY AUTOINCREMENT, cacheGroup INTEGER, size INTEGER)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS CacheWhitelistURLs (url TEXT NOT NULL ON CONFLICT FAIL, cache INTEGER NOT NULL ON CONFLICT FAIL)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS CacheAllowsAllNetworkRequests (wildcard INTEGER NOT NULL ON CONFLICT FAIL, cache INTEGER NOT NULL ON CONFLICT FAIL)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS FallbackURLs (namespace TEXT NOT NULL ON CONFLICT FAIL, fallbackURL TEXT NOT NULL ON CONFLICT FAIL, "
                      "cache INTEGER NOT NULL ON CONFLICT FAIL)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS CacheEntries (cache INTEGER NOT NULL ON CONFLICT FAIL, type INTEGER, resource INTEGER NOT NULL)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS CacheResources (id INTEGER PRIMARY KEY AUTOINCREMENT, url TEXT NOT NULL ON CONFLICT FAIL, "
                      "statusCode INTEGER NOT NULL, responseURL TEXT NOT NULL, mimeType TEXT, textEncodingName TEXT, headers TEXT, data INTEGER NOT NULL ON CONFLICT FAIL)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS CacheResourceData (id INTEGER PRIMARY KEY AUTOINCREMENT, data BLOB, path TEXT)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS DeletedCacheResources (id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT)");
    executeSQLCommand("CREATE TABLE IF NOT EXISTS Origins (origin TEXT UNIQUE ON CONFLICT IGNORE, quota INTEGER NOT NULL ON CONFLICT FAIL)");

    // When a cache is deleted, all its entries and its whitelist should be deleted.
    executeSQLCommand("CREATE TRIGGER IF NOT EXISTS CacheDeleted AFTER DELETE ON Caches"
                      " FOR EACH ROW BEGIN"
                      "  DELETE FROM CacheEntries WHERE cache = OLD.id;"
                      "  DELETE FROM CacheWhitelistURLs WHERE cache = OLD.id;"
                      "  DELETE FROM CacheAllowsAllNetworkRequests WHERE cache = OLD.id;"
                      "  DELETE FROM FallbackURLs WHERE cache = OLD.id;"
                      " END");

    // When a cache entry is deleted, its resource should also be deleted.
    executeSQLCommand("CREATE TRIGGER IF NOT EXISTS CacheEntryDeleted AFTER DELETE ON CacheEntries"
                      " FOR EACH ROW BEGIN"
                      "  DELETE FROM CacheResources WHERE id = OLD.resource;"
                      " END");

    // When a cache resource is deleted, its data blob should also be deleted.
    executeSQLCommand("CREATE TRIGGER IF NOT EXISTS CacheResourceDeleted AFTER DELETE ON CacheResources"
                      " FOR EACH ROW BEGIN"
                      "  DELETE FROM CacheResourceData WHERE id = OLD.data;"
                      " END");
    
    // When a cache resource is deleted, if it contains a non-empty path, that path should
    // be added to the DeletedCacheResources table so the flat file at that path can
    // be deleted at a later time.
    executeSQLCommand("CREATE TRIGGER IF NOT EXISTS CacheResourceDataDeleted AFTER DELETE ON CacheResourceData"
                      " FOR EACH ROW"
                      " WHEN OLD.path NOT NULL BEGIN"
                      "  INSERT INTO DeletedCacheResources (path) values (OLD.path);"
                      " END");
}

bool ApplicationCacheStorage::executeStatement(SQLiteStatement& statement)
{
    bool result = statement.executeCommand();
    if (!result)
        LOG_ERROR("Application Cache Storage: failed to execute statement \"%s\" error \"%s\"", 
                  statement.query().utf8().data(), m_database.lastErrorMsg());
    
    return result;
}    

bool ApplicationCacheStorage::store(ApplicationCacheGroup* group, GroupStorageIDJournal* journal)
{
    ASSERT(group->storageID() == 0);
    ASSERT(journal);

    SQLiteStatement statement(m_database, "INSERT INTO CacheGroups (manifestHostHash, manifestURL, origin) VALUES (?, ?, ?)");
    if (statement.prepare() != SQLResultOk)
        return false;

    statement.bindInt64(1, urlHostHash(group->manifestURL()));
    statement.bindText(2, group->manifestURL());
    statement.bindText(3, group->origin()->databaseIdentifier());

    if (!executeStatement(statement))
        return false;

    unsigned groupStorageID = static_cast<unsigned>(m_database.lastInsertRowID());

    if (!ensureOriginRecord(group->origin()))
        return false;

    group->setStorageID(groupStorageID);
    journal->add(group, 0);
    return true;
}    

bool ApplicationCacheStorage::store(ApplicationCache* cache, ResourceStorageIDJournal* storageIDJournal)
{
    ASSERT(cache->storageID() == 0);
    ASSERT(cache->group()->storageID() != 0);
    ASSERT(storageIDJournal);
    
    SQLiteStatement statement(m_database, "INSERT INTO Caches (cacheGroup, size) VALUES (?, ?)");
    if (statement.prepare() != SQLResultOk)
        return false;

    statement.bindInt64(1, cache->group()->storageID());
    statement.bindInt64(2, cache->estimatedSizeInStorage());

    if (!executeStatement(statement))
        return false;
    
    unsigned cacheStorageID = static_cast<unsigned>(m_database.lastInsertRowID());

    // Store all resources
    {
        ApplicationCache::ResourceMap::const_iterator end = cache->end();
        for (ApplicationCache::ResourceMap::const_iterator it = cache->begin(); it != end; ++it) {
            unsigned oldStorageID = it->second->storageID();
            if (!store(it->second.get(), cacheStorageID))
                return false;

            // Storing the resource succeeded. Log its old storageID in case
            // it needs to be restored later.
            storageIDJournal->add(it->second.get(), oldStorageID);
        }
    }
    
    // Store the online whitelist
    const Vector<KURL>& onlineWhitelist = cache->onlineWhitelist();
    {
        size_t whitelistSize = onlineWhitelist.size();
        for (size_t i = 0; i < whitelistSize; ++i) {
            SQLiteStatement statement(m_database, "INSERT INTO CacheWhitelistURLs (url, cache) VALUES (?, ?)");
            statement.prepare();

            statement.bindText(1, onlineWhitelist[i]);
            statement.bindInt64(2, cacheStorageID);

            if (!executeStatement(statement))
                return false;
        }
    }

    // Store online whitelist wildcard flag.
    {
        SQLiteStatement statement(m_database, "INSERT INTO CacheAllowsAllNetworkRequests (wildcard, cache) VALUES (?, ?)");
        statement.prepare();

        statement.bindInt64(1, cache->allowsAllNetworkRequests());
        statement.bindInt64(2, cacheStorageID);

        if (!executeStatement(statement))
            return false;
    }
    
    // Store fallback URLs.
    const FallbackURLVector& fallbackURLs = cache->fallbackURLs();
    {
        size_t fallbackCount = fallbackURLs.size();
        for (size_t i = 0; i < fallbackCount; ++i) {
            SQLiteStatement statement(m_database, "INSERT INTO FallbackURLs (namespace, fallbackURL, cache) VALUES (?, ?, ?)");
            statement.prepare();

            statement.bindText(1, fallbackURLs[i].first);
            statement.bindText(2, fallbackURLs[i].second);
            statement.bindInt64(3, cacheStorageID);

            if (!executeStatement(statement))
                return false;
        }
    }

    cache->setStorageID(cacheStorageID);
    return true;
}

bool ApplicationCacheStorage::store(ApplicationCacheResource* resource, unsigned cacheStorageID)
{
    ASSERT(cacheStorageID);
    ASSERT(!resource->storageID());
    
    openDatabase(true);

    // openDatabase(true) could still fail, for example when cacheStorage is full or no longer available.
    if (!m_database.isOpen())
        return false;

    // First, insert the data
    SQLiteStatement dataStatement(m_database, "INSERT INTO CacheResourceData (data, path) VALUES (?, ?)");
    if (dataStatement.prepare() != SQLResultOk)
        return false;
    

    String fullPath;
    if (!resource->path().isEmpty())
        dataStatement.bindText(2, pathGetFileName(resource->path()));
    else if (shouldStoreResourceAsFlatFile(resource)) {
        // First, check to see if creating the flat file would violate the maximum total quota. We don't need
        // to check the per-origin quota here, as it was already checked in storeNewestCache().
        if (m_database.totalSize() + flatFileAreaSize() + resource->data()->size() > m_maximumSize) {
            m_isMaximumSizeReached = true;
            return false;
        }
        
        String flatFileDirectory = pathByAppendingComponent(m_cacheDirectory, flatFileSubdirectory);
        makeAllDirectories(flatFileDirectory);

        String extension;
        
        String fileName = resource->response().suggestedFilename();
        size_t dotIndex = fileName.reverseFind('.');
        if (dotIndex != notFound && dotIndex < (fileName.length() - 1))
            extension = fileName.substring(dotIndex);

        String path;
        if (!writeDataToUniqueFileInDirectory(resource->data(), flatFileDirectory, path, extension))
            return false;
        
        fullPath = pathByAppendingComponent(flatFileDirectory, path);
        resource->setPath(fullPath);
        dataStatement.bindText(2, path);
    } else {
        if (resource->data()->size())
            dataStatement.bindBlob(1, resource->data()->data(), resource->data()->size());
    }
    
    if (!dataStatement.executeCommand()) {
        // Clean up the file which we may have written to:
        if (!fullPath.isEmpty())
            deleteFile(fullPath);

        return false;
    }

    unsigned dataId = static_cast<unsigned>(m_database.lastInsertRowID());

    // Then, insert the resource
    
    // Serialize the headers
    Vector<UChar> stringBuilder;
    
    HTTPHeaderMap::const_iterator end = resource->response().httpHeaderFields().end();
    for (HTTPHeaderMap::const_iterator it = resource->response().httpHeaderFields().begin(); it!= end; ++it) {
        stringBuilder.append(it->first.characters(), it->first.length());
        stringBuilder.append((UChar)':');
        stringBuilder.append(it->second.characters(), it->second.length());
        stringBuilder.append((UChar)'\n');
    }
    
    String headers = String::adopt(stringBuilder);
    
    SQLiteStatement resourceStatement(m_database, "INSERT INTO CacheResources (url, statusCode, responseURL, headers, data, mimeType, textEncodingName) VALUES (?, ?, ?, ?, ?, ?, ?)");
    if (resourceStatement.prepare() != SQLResultOk)
        return false;
    
    // The same ApplicationCacheResource are used in ApplicationCacheResource::size()
    // to calculate the approximate size of an ApplicationCacheResource object. If
    // you change the code below, please also change ApplicationCacheResource::size().
    resourceStatement.bindText(1, resource->url());
    resourceStatement.bindInt64(2, resource->response().httpStatusCode());
    resourceStatement.bindText(3, resource->response().url());
    resourceStatement.bindText(4, headers);
    resourceStatement.bindInt64(5, dataId);
    resourceStatement.bindText(6, resource->response().mimeType());
    resourceStatement.bindText(7, resource->response().textEncodingName());

    if (!executeStatement(resourceStatement))
        return false;

    unsigned resourceId = static_cast<unsigned>(m_database.lastInsertRowID());
    
    // Finally, insert the cache entry
    SQLiteStatement entryStatement(m_database, "INSERT INTO CacheEntries (cache, type, resource) VALUES (?, ?, ?)");
    if (entryStatement.prepare() != SQLResultOk)
        return false;
    
    entryStatement.bindInt64(1, cacheStorageID);
    entryStatement.bindInt64(2, resource->type());
    entryStatement.bindInt64(3, resourceId);
    
    if (!executeStatement(entryStatement))
        return false;
    
    // Did we successfully write the resource data to a file? If so,
    // release the resource's data and free up a potentially large amount
    // of memory:
    if (!fullPath.isEmpty())
        resource->data()->clear();

    resource->setStorageID(resourceId);
    return true;
}

bool ApplicationCacheStorage::storeUpdatedType(ApplicationCacheResource* resource, ApplicationCache* cache)
{
    ASSERT_UNUSED(cache, cache->storageID());
    ASSERT(resource->storageID());

    // First, insert the data
    SQLiteStatement entryStatement(m_database, "UPDATE CacheEntries SET type=? WHERE resource=?");
    if (entryStatement.prepare() != SQLResultOk)
        return false;

    entryStatement.bindInt64(1, resource->type());
    entryStatement.bindInt64(2, resource->storageID());

    return executeStatement(entryStatement);
}

bool ApplicationCacheStorage::store(ApplicationCacheResource* resource, ApplicationCache* cache)
{
    ASSERT(cache->storageID());
    
    openDatabase(true);

    if (!m_database.isOpen())
        return false;
 
    m_isMaximumSizeReached = false;
    m_database.setMaximumSize(m_maximumSize - flatFileAreaSize());

    SQLiteTransaction storeResourceTransaction(m_database);
    storeResourceTransaction.begin();
    
    if (!store(resource, cache->storageID())) {
        checkForMaxSizeReached();
        return false;
    }

    // A resource was added to the cache. Update the total data size for the cache.
    SQLiteStatement sizeUpdateStatement(m_database, "UPDATE Caches SET size=size+? WHERE id=?");
    if (sizeUpdateStatement.prepare() != SQLResultOk)
        return false;

    sizeUpdateStatement.bindInt64(1, resource->estimatedSizeInStorage());
    sizeUpdateStatement.bindInt64(2, cache->storageID());

    if (!executeStatement(sizeUpdateStatement))
        return false;
    
    storeResourceTransaction.commit();
    return true;
}

bool ApplicationCacheStorage::ensureOriginRecord(const SecurityOrigin* origin)
{
    SQLiteStatement insertOriginStatement(m_database, "INSERT INTO Origins (origin, quota) VALUES (?, ?)");
    if (insertOriginStatement.prepare() != SQLResultOk)
        return false;

    insertOriginStatement.bindText(1, origin->databaseIdentifier());
    insertOriginStatement.bindInt64(2, m_defaultOriginQuota);
    if (!executeStatement(insertOriginStatement))
        return false;

    return true;
}

bool ApplicationCacheStorage::storeNewestCache(ApplicationCacheGroup* group, ApplicationCache* oldCache, FailureReason& failureReason)
{
    openDatabase(true);

    if (!m_database.isOpen())
        return false;

    m_isMaximumSizeReached = false;
    m_database.setMaximumSize(m_maximumSize - flatFileAreaSize());

    SQLiteTransaction storeCacheTransaction(m_database);
    
    storeCacheTransaction.begin();

    // Check if this would reach the per-origin quota.
    int64_t remainingSpaceInOrigin;
    if (remainingSizeForOriginExcludingCache(group->origin(), oldCache, remainingSpaceInOrigin)) {
        if (remainingSpaceInOrigin < group->newestCache()->estimatedSizeInStorage()) {
            failureReason = OriginQuotaReached;
            return false;
        }
    }

    GroupStorageIDJournal groupStorageIDJournal;
    if (!group->storageID()) {
        // Store the group
        if (!store(group, &groupStorageIDJournal)) {
            checkForMaxSizeReached();
            failureReason = isMaximumSizeReached() ? TotalQuotaReached : DiskOrOperationFailure;
            return false;
        }
    }
    
    ASSERT(group->newestCache());
    ASSERT(!group->isObsolete());
    ASSERT(!group->newestCache()->storageID());
    
    // Log the storageID changes to the in-memory resource objects. The journal
    // object will roll them back automatically in case a database operation
    // fails and this method returns early.
    ResourceStorageIDJournal resourceStorageIDJournal;

    // Store the newest cache
    if (!store(group->newestCache(), &resourceStorageIDJournal)) {
        checkForMaxSizeReached();
        failureReason = isMaximumSizeReached() ? TotalQuotaReached : DiskOrOperationFailure;
        return false;
    }
    
    // Update the newest cache in the group.
    
    SQLiteStatement statement(m_database, "UPDATE CacheGroups SET newestCache=? WHERE id=?");
    if (statement.prepare() != SQLResultOk) {
        failureReason = DiskOrOperationFailure;
        return false;
    }
    
    statement.bindInt64(1, group->newestCache()->storageID());
    statement.bindInt64(2, group->storageID());
    
    if (!executeStatement(statement)) {
        failureReason = DiskOrOperationFailure;
        return false;
    }
    
    groupStorageIDJournal.commit();
    resourceStorageIDJournal.commit();
    storeCacheTransaction.commit();
    return true;
}

bool ApplicationCacheStorage::storeNewestCache(ApplicationCacheGroup* group)
{
    // Ignore the reason for failing, just attempt the store.
    FailureReason ignoredFailureReason;
    return storeNewestCache(group, 0, ignoredFailureReason);
}

static inline void parseHeader(const UChar* header, size_t headerLength, ResourceResponse& response)
{
    size_t pos = find(header, headerLength, ':');
    ASSERT(pos != notFound);
    
    AtomicString headerName = AtomicString(header, pos);
    String headerValue = String(header + pos + 1, headerLength - pos - 1);
    
    response.setHTTPHeaderField(headerName, headerValue);
}

static inline void parseHeaders(const String& headers, ResourceResponse& response)
{
    unsigned startPos = 0;
    size_t endPos;
    while ((endPos = headers.find('\n', startPos)) != notFound) {
        ASSERT(startPos != endPos);

        parseHeader(headers.characters() + startPos, endPos - startPos, response);
        
        startPos = endPos + 1;
    }
    
    if (startPos != headers.length())
        parseHeader(headers.characters(), headers.length(), response);
}
    
PassRefPtr<ApplicationCache> ApplicationCacheStorage::loadCache(unsigned storageID)
{
    SQLiteStatement cacheStatement(m_database, 
                                   "SELECT url, type, mimeType, textEncodingName, headers, CacheResourceData.data, CacheResourceData.path FROM CacheEntries INNER JOIN CacheResources ON CacheEntries.resource=CacheResources.id "
                                   "INNER JOIN CacheResourceData ON CacheResourceData.id=CacheResources.data WHERE CacheEntries.cache=?");
    if (cacheStatement.prepare() != SQLResultOk) {
        LOG_ERROR("Could not prepare cache statement, error \"%s\"", m_database.lastErrorMsg());
        return 0;
    }
    
    cacheStatement.bindInt64(1, storageID);

    RefPtr<ApplicationCache> cache = ApplicationCache::create();

    String flatFileDirectory = pathByAppendingComponent(m_cacheDirectory, flatFileSubdirectory);

    int result;
    while ((result = cacheStatement.step()) == SQLResultRow) {
        KURL url(ParsedURLString, cacheStatement.getColumnText(0));
        
        unsigned type = static_cast<unsigned>(cacheStatement.getColumnInt64(1));

        Vector<char> blob;
        cacheStatement.getColumnBlobAsVector(5, blob);
        
        RefPtr<SharedBuffer> data = SharedBuffer::adoptVector(blob);
        
        String path = cacheStatement.getColumnText(6);
        long long size = 0;
        if (path.isEmpty())
            size = data->size();
        else {
            path = pathByAppendingComponent(flatFileDirectory, path);
            getFileSize(path, size);
        }
        
        String mimeType = cacheStatement.getColumnText(2);
        String textEncodingName = cacheStatement.getColumnText(3);
        
        ResourceResponse response(url, mimeType, size, textEncodingName, "");

        String headers = cacheStatement.getColumnText(4);
        parseHeaders(headers, response);
        
        RefPtr<ApplicationCacheResource> resource = ApplicationCacheResource::create(url, response, type, data.release(), path);

        if (type & ApplicationCacheResource::Manifest)
            cache->setManifestResource(resource.release());
        else
            cache->addResource(resource.release());
    }

    if (result != SQLResultDone)
        LOG_ERROR("Could not load cache resources, error \"%s\"", m_database.lastErrorMsg());
    
    // Load the online whitelist
    SQLiteStatement whitelistStatement(m_database, "SELECT url FROM CacheWhitelistURLs WHERE cache=?");
    if (whitelistStatement.prepare() != SQLResultOk)
        return 0;
    whitelistStatement.bindInt64(1, storageID);
    
    Vector<KURL> whitelist;
    while ((result = whitelistStatement.step()) == SQLResultRow) 
        whitelist.append(KURL(ParsedURLString, whitelistStatement.getColumnText(0)));

    if (result != SQLResultDone)
        LOG_ERROR("Could not load cache online whitelist, error \"%s\"", m_database.lastErrorMsg());

    cache->setOnlineWhitelist(whitelist);

    // Load online whitelist wildcard flag.
    SQLiteStatement whitelistWildcardStatement(m_database, "SELECT wildcard FROM CacheAllowsAllNetworkRequests WHERE cache=?");
    if (whitelistWildcardStatement.prepare() != SQLResultOk)
        return 0;
    whitelistWildcardStatement.bindInt64(1, storageID);
    
    result = whitelistWildcardStatement.step();
    if (result != SQLResultRow)
        LOG_ERROR("Could not load cache online whitelist wildcard flag, error \"%s\"", m_database.lastErrorMsg());

    cache->setAllowsAllNetworkRequests(whitelistWildcardStatement.getColumnInt64(0));

    if (whitelistWildcardStatement.step() != SQLResultDone)
        LOG_ERROR("Too many rows for online whitelist wildcard flag");

    // Load fallback URLs.
    SQLiteStatement fallbackStatement(m_database, "SELECT namespace, fallbackURL FROM FallbackURLs WHERE cache=?");
    if (fallbackStatement.prepare() != SQLResultOk)
        return 0;
    fallbackStatement.bindInt64(1, storageID);
    
    FallbackURLVector fallbackURLs;
    while ((result = fallbackStatement.step()) == SQLResultRow) 
        fallbackURLs.append(make_pair(KURL(ParsedURLString, fallbackStatement.getColumnText(0)), KURL(ParsedURLString, fallbackStatement.getColumnText(1))));

    if (result != SQLResultDone)
        LOG_ERROR("Could not load fallback URLs, error \"%s\"", m_database.lastErrorMsg());

    cache->setFallbackURLs(fallbackURLs);
    
    cache->setStorageID(storageID);

    return cache.release();
}    
    
void ApplicationCacheStorage::remove(ApplicationCache* cache)
{
    if (!cache->storageID())
        return;
    
    openDatabase(false);
    if (!m_database.isOpen())
        return;

    ASSERT(cache->group());
    ASSERT(cache->group()->storageID());

    // All associated data will be deleted by database triggers.
    SQLiteStatement statement(m_database, "DELETE FROM Caches WHERE id=?");
    if (statement.prepare() != SQLResultOk)
        return;
    
    statement.bindInt64(1, cache->storageID());
    executeStatement(statement);

    cache->clearStorageID();

    if (cache->group()->newestCache() == cache) {
        // Currently, there are no triggers on the cache group, which is why the cache had to be removed separately above.
        SQLiteStatement groupStatement(m_database, "DELETE FROM CacheGroups WHERE id=?");
        if (groupStatement.prepare() != SQLResultOk)
            return;
        
        groupStatement.bindInt64(1, cache->group()->storageID());
        executeStatement(groupStatement);

        cache->group()->clearStorageID();
    }
    
    checkForDeletedResources();
}    

void ApplicationCacheStorage::empty()
{
    openDatabase(false);
    
    if (!m_database.isOpen())
        return;
    
    // Clear cache groups, caches, cache resources, and origins.
    executeSQLCommand("DELETE FROM CacheGroups");
    executeSQLCommand("DELETE FROM Caches");
    executeSQLCommand("DELETE FROM Origins");
    
    // Clear the storage IDs for the caches in memory.
    // The caches will still work, but cached resources will not be saved to disk 
    // until a cache update process has been initiated.
    CacheGroupMap::const_iterator end = m_cachesInMemory.end();
    for (CacheGroupMap::const_iterator it = m_cachesInMemory.begin(); it != end; ++it)
        it->second->clearStorageID();
    
    checkForDeletedResources();
}
    
void ApplicationCacheStorage::deleteTables()
{
    empty();
    m_database.clearAllTables();
}
    
bool ApplicationCacheStorage::shouldStoreResourceAsFlatFile(ApplicationCacheResource* resource)
{
    return resource->response().mimeType().startsWith("audio/", false) 
        || resource->response().mimeType().startsWith("video/", false);
}
    
bool ApplicationCacheStorage::writeDataToUniqueFileInDirectory(SharedBuffer* data, const String& directory, String& path, const String& fileExtension)
{
    String fullPath;
    
    do {
        path = encodeForFileName(createCanonicalUUIDString()) + fileExtension;
        // Guard against the above function being called on a platform which does not implement
        // createCanonicalUUIDString().
        ASSERT(!path.isEmpty());
        if (path.isEmpty())
            return false;
        
        fullPath = pathByAppendingComponent(directory, path);
    } while (directoryName(fullPath) != directory || fileExists(fullPath));
    
    PlatformFileHandle handle = openFile(fullPath, OpenForWrite);
    if (!handle)
        return false;
    
    int64_t writtenBytes = writeToFile(handle, data->data(), data->size());
    closeFile(handle);
    
    if (writtenBytes != static_cast<int64_t>(data->size())) {
        deleteFile(fullPath);
        return false;
    }
    
    return true;
}

bool ApplicationCacheStorage::storeCopyOfCache(const String& cacheDirectory, ApplicationCacheHost* cacheHost)
{
    ApplicationCache* cache = cacheHost->applicationCache();
    if (!cache)
        return true;

    // Create a new cache.
    RefPtr<ApplicationCache> cacheCopy = ApplicationCache::create();

    cacheCopy->setOnlineWhitelist(cache->onlineWhitelist());
    cacheCopy->setFallbackURLs(cache->fallbackURLs());

    // Traverse the cache and add copies of all resources.
    ApplicationCache::ResourceMap::const_iterator end = cache->end();
    for (ApplicationCache::ResourceMap::const_iterator it = cache->begin(); it != end; ++it) {
        ApplicationCacheResource* resource = it->second.get();
        
        RefPtr<ApplicationCacheResource> resourceCopy = ApplicationCacheResource::create(resource->url(), resource->response(), resource->type(), resource->data(), resource->path());
        
        cacheCopy->addResource(resourceCopy.release());
    }
    
    // Now create a new cache group.
    OwnPtr<ApplicationCacheGroup> groupCopy(adoptPtr(new ApplicationCacheGroup(cache->group()->manifestURL(), true)));
    
    groupCopy->setNewestCache(cacheCopy);
    
    ApplicationCacheStorage copyStorage;
    copyStorage.setCacheDirectory(cacheDirectory);
    
    // Empty the cache in case something was there before.
    copyStorage.empty();
    
    return copyStorage.storeNewestCache(groupCopy.get());
}

bool ApplicationCacheStorage::manifestURLs(Vector<KURL>* urls)
{
    ASSERT(urls);
    openDatabase(false);
    if (!m_database.isOpen())
        return false;

    SQLiteStatement selectURLs(m_database, "SELECT manifestURL FROM CacheGroups");

    if (selectURLs.prepare() != SQLResultOk)
        return false;

    while (selectURLs.step() == SQLResultRow)
        urls->append(KURL(ParsedURLString, selectURLs.getColumnText(0)));

    return true;
}

bool ApplicationCacheStorage::cacheGroupSize(const String& manifestURL, int64_t* size)
{
    ASSERT(size);
    openDatabase(false);
    if (!m_database.isOpen())
        return false;

    SQLiteStatement statement(m_database, "SELECT sum(Caches.size) FROM Caches INNER JOIN CacheGroups ON Caches.cacheGroup=CacheGroups.id WHERE CacheGroups.manifestURL=?");
    if (statement.prepare() != SQLResultOk)
        return false;

    statement.bindText(1, manifestURL);

    int result = statement.step();
    if (result == SQLResultDone)
        return false;

    if (result != SQLResultRow) {
        LOG_ERROR("Could not get the size of the cache group, error \"%s\"", m_database.lastErrorMsg());
        return false;
    }

    *size = statement.getColumnInt64(0);
    return true;
}

bool ApplicationCacheStorage::deleteCacheGroup(const String& manifestURL)
{
    SQLiteTransaction deleteTransaction(m_database);
    // Check to see if the group is in memory.
    ApplicationCacheGroup* group = m_cachesInMemory.get(manifestURL);
    if (group)
        cacheGroupMadeObsolete(group);
    else {
        // The cache group is not in memory, so remove it from the disk.
        openDatabase(false);
        if (!m_database.isOpen())
            return false;

        SQLiteStatement idStatement(m_database, "SELECT id FROM CacheGroups WHERE manifestURL=?");
        if (idStatement.prepare() != SQLResultOk)
            return false;

        idStatement.bindText(1, manifestURL);

        int result = idStatement.step();
        if (result == SQLResultDone)
            return false;

        if (result != SQLResultRow) {
            LOG_ERROR("Could not load cache group id, error \"%s\"", m_database.lastErrorMsg());
            return false;
        }

        int64_t groupId = idStatement.getColumnInt64(0);

        SQLiteStatement cacheStatement(m_database, "DELETE FROM Caches WHERE cacheGroup=?");
        if (cacheStatement.prepare() != SQLResultOk)
            return false;

        SQLiteStatement groupStatement(m_database, "DELETE FROM CacheGroups WHERE id=?");
        if (groupStatement.prepare() != SQLResultOk)
            return false;

        cacheStatement.bindInt64(1, groupId);
        executeStatement(cacheStatement);
        groupStatement.bindInt64(1, groupId);
        executeStatement(groupStatement);
    }

    deleteTransaction.commit();
    
    checkForDeletedResources();
    
    return true;
}

void ApplicationCacheStorage::vacuumDatabaseFile()
{
    openDatabase(false);
    if (!m_database.isOpen())
        return;

    m_database.runVacuumCommand();
}

void ApplicationCacheStorage::checkForMaxSizeReached()
{
    if (m_database.lastError() == SQLResultFull)
        m_isMaximumSizeReached = true;
}
    
void ApplicationCacheStorage::checkForDeletedResources()
{
    openDatabase(false);
    if (!m_database.isOpen())
        return;

    // Select only the paths in DeletedCacheResources that do not also appear in CacheResourceData:
    SQLiteStatement selectPaths(m_database, "SELECT DeletedCacheResources.path "
        "FROM DeletedCacheResources "
        "LEFT JOIN CacheResourceData "
        "ON DeletedCacheResources.path = CacheResourceData.path "
        "WHERE (SELECT DeletedCacheResources.path == CacheResourceData.path) IS NULL");
    
    if (selectPaths.prepare() != SQLResultOk)
        return;
    
    if (selectPaths.step() != SQLResultRow)
        return;
    
    do {
        String path = selectPaths.getColumnText(0);
        if (path.isEmpty())
            continue;
        
        String flatFileDirectory = pathByAppendingComponent(m_cacheDirectory, flatFileSubdirectory);
        String fullPath = pathByAppendingComponent(flatFileDirectory, path);
        
        // Don't exit the flatFileDirectory! This should only happen if the "path" entry contains a directory 
        // component, but protect against it regardless.
        if (directoryName(fullPath) != flatFileDirectory)
            continue;
        
        deleteFile(fullPath);
    } while (selectPaths.step() == SQLResultRow);
    
    executeSQLCommand("DELETE FROM DeletedCacheResources");
}
    
long long ApplicationCacheStorage::flatFileAreaSize()
{
    openDatabase(false);
    if (!m_database.isOpen())
        return 0;
    
    SQLiteStatement selectPaths(m_database, "SELECT path FROM CacheResourceData WHERE path NOT NULL");

    if (selectPaths.prepare() != SQLResultOk) {
        LOG_ERROR("Could not load flat file cache resource data, error \"%s\"", m_database.lastErrorMsg());
        return 0;
    }

    long long totalSize = 0;
    String flatFileDirectory = pathByAppendingComponent(m_cacheDirectory, flatFileSubdirectory);
    while (selectPaths.step() == SQLResultRow) {
        String path = selectPaths.getColumnText(0);
        String fullPath = pathByAppendingComponent(flatFileDirectory, path);
        long long pathSize = 0;
        if (!getFileSize(fullPath, pathSize))
            continue;
        totalSize += pathSize;
    }
    
    return totalSize;
}

void ApplicationCacheStorage::getOriginsWithCache(HashSet<RefPtr<SecurityOrigin>, SecurityOriginHash>& origins)
{
    Vector<KURL> urls;
    if (!manifestURLs(&urls)) {
        LOG_ERROR("Failed to retrieve ApplicationCache manifest URLs");
        return;
    }

    // Multiple manifest URLs might share the same SecurityOrigin, so we might be creating extra, wasted origins here.
    // The current schema doesn't allow for a more efficient way of building this list.
    size_t count = urls.size();
    for (size_t i = 0; i < count; ++i) {
        RefPtr<SecurityOrigin> origin = SecurityOrigin::create(urls[i]);
        origins.add(origin);
    }
}

void ApplicationCacheStorage::deleteAllEntries()
{
    empty();
    vacuumDatabaseFile();
}

ApplicationCacheStorage::ApplicationCacheStorage() 
    : m_maximumSize(ApplicationCacheStorage::noQuota())
    , m_isMaximumSizeReached(false)
    , m_defaultOriginQuota(ApplicationCacheStorage::noQuota())
{
}

ApplicationCacheStorage& cacheStorage()
{
    DEFINE_STATIC_LOCAL(ApplicationCacheStorage, storage, ());
    
    return storage;
}

} // namespace WebCore

#endif // ENABLE(OFFLINE_WEB_APPLICATIONS)
