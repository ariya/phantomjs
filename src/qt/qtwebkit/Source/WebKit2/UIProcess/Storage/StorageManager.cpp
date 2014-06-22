/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "StorageManager.h"

#include "LocalStorageDatabase.h"
#include "LocalStorageDatabaseTracker.h"
#include "SecurityOriginData.h"
#include "StorageAreaMapMessages.h"
#include "StorageManagerMessages.h"
#include "WebProcessProxy.h"
#include "WorkQueue.h"
#include <WebCore/SecurityOriginHash.h>
#include <WebCore/StorageMap.h>
#include <WebCore/TextEncoding.h>

using namespace WebCore;

namespace WebKit {

class StorageManager::StorageArea : public ThreadSafeRefCounted<StorageManager::StorageArea> {
public:
    static PassRefPtr<StorageArea> create(LocalStorageNamespace*, PassRefPtr<SecurityOrigin>, unsigned quotaInBytes);
    ~StorageArea();

    SecurityOrigin* securityOrigin() const { return m_securityOrigin.get(); }

    void addListener(CoreIPC::Connection*, uint64_t storageMapID);
    void removeListener(CoreIPC::Connection*, uint64_t storageMapID);

    PassRefPtr<StorageArea> clone() const;

    void setItem(CoreIPC::Connection* sourceConnection, uint64_t sourceStorageAreaID, const String& key, const String& value, const String& urlString, bool& quotaException);
    void removeItem(CoreIPC::Connection* sourceConnection, uint64_t sourceStorageAreaID, const String& key, const String& urlString);
    void clear(CoreIPC::Connection* sourceConnection, uint64_t sourceStorageAreaID, const String& urlString);

    const HashMap<String, String>& items();
    void clear();

private:
    explicit StorageArea(LocalStorageNamespace*, PassRefPtr<SecurityOrigin>, unsigned quotaInBytes);

    void openDatabaseAndImportItemsIfNeeded();

    void dispatchEvents(CoreIPC::Connection* sourceConnection, uint64_t sourceStorageAreaID, const String& key, const String& oldValue, const String& newValue, const String& urlString) const;

    // Will be null if the storage area belongs to a session storage namespace.
    LocalStorageNamespace* m_localStorageNamespace;
    RefPtr<LocalStorageDatabase> m_localStorageDatabase;
    bool m_didImportItemsFromDatabase;

    RefPtr<SecurityOrigin> m_securityOrigin;
    unsigned m_quotaInBytes;

    RefPtr<StorageMap> m_storageMap;
    HashSet<std::pair<RefPtr<CoreIPC::Connection>, uint64_t> > m_eventListeners;
};

class StorageManager::LocalStorageNamespace : public ThreadSafeRefCounted<LocalStorageNamespace> {
public:
    static PassRefPtr<LocalStorageNamespace> create(StorageManager*, uint64_t storageManagerID);
    ~LocalStorageNamespace();

    StorageManager* storageManager() const { return m_storageManager; }

    PassRefPtr<StorageArea> getOrCreateStorageArea(PassRefPtr<SecurityOrigin>);
    void didDestroyStorageArea(StorageArea*);

    void clearStorageAreasMatchingOrigin(SecurityOrigin*);
    void clearAllStorageAreas();

private:
    explicit LocalStorageNamespace(StorageManager*, uint64_t storageManagerID);

    StorageManager* m_storageManager;
    uint64_t m_storageNamespaceID;
    unsigned m_quotaInBytes;

    // We don't hold an explicit reference to the StorageAreas; they are kept alive by the m_storageAreasByConnection map in StorageManager.
    HashMap<RefPtr<SecurityOrigin>, StorageArea*> m_storageAreaMap;
};

PassRefPtr<StorageManager::StorageArea> StorageManager::StorageArea::create(LocalStorageNamespace* localStorageNamespace, PassRefPtr<SecurityOrigin> securityOrigin, unsigned quotaInBytes)
{
    return adoptRef(new StorageArea(localStorageNamespace, securityOrigin, quotaInBytes));
}

StorageManager::StorageArea::StorageArea(LocalStorageNamespace* localStorageNamespace, PassRefPtr<SecurityOrigin> securityOrigin, unsigned quotaInBytes)
    : m_localStorageNamespace(localStorageNamespace)
    , m_didImportItemsFromDatabase(false)
    , m_securityOrigin(securityOrigin)
    , m_quotaInBytes(quotaInBytes)
    , m_storageMap(StorageMap::create(m_quotaInBytes))
{
}

StorageManager::StorageArea::~StorageArea()
{
    ASSERT(m_eventListeners.isEmpty());

    if (m_localStorageDatabase)
        m_localStorageDatabase->close();

    if (m_localStorageNamespace)
        m_localStorageNamespace->didDestroyStorageArea(this);
}

void StorageManager::StorageArea::addListener(CoreIPC::Connection* connection, uint64_t storageMapID)
{
    ASSERT(!m_eventListeners.contains(std::make_pair(connection, storageMapID)));
    m_eventListeners.add(std::make_pair(connection, storageMapID));
}

void StorageManager::StorageArea::removeListener(CoreIPC::Connection* connection, uint64_t storageMapID)
{
    ASSERT(m_eventListeners.contains(std::make_pair(connection, storageMapID)));
    m_eventListeners.remove(std::make_pair(connection, storageMapID));
}

PassRefPtr<StorageManager::StorageArea> StorageManager::StorageArea::clone() const
{
    ASSERT(!m_localStorageNamespace);

    RefPtr<StorageArea> storageArea = StorageArea::create(0, m_securityOrigin, m_quotaInBytes);
    storageArea->m_storageMap = m_storageMap;

    return storageArea.release();
}

void StorageManager::StorageArea::setItem(CoreIPC::Connection* sourceConnection, uint64_t sourceStorageAreaID, const String& key, const String& value, const String& urlString, bool& quotaException)
{
    openDatabaseAndImportItemsIfNeeded();

    String oldValue;

    RefPtr<StorageMap> newStorageMap = m_storageMap->setItem(key, value, oldValue, quotaException);
    if (newStorageMap)
        m_storageMap = newStorageMap.release();

    if (quotaException)
        return;

    if (m_localStorageDatabase)
        m_localStorageDatabase->setItem(key, value);

    dispatchEvents(sourceConnection, sourceStorageAreaID, key, oldValue, value, urlString);
}

void StorageManager::StorageArea::removeItem(CoreIPC::Connection* sourceConnection, uint64_t sourceStorageAreaID, const String& key, const String& urlString)
{
    openDatabaseAndImportItemsIfNeeded();

    String oldValue;
    RefPtr<StorageMap> newStorageMap = m_storageMap->removeItem(key, oldValue);
    if (newStorageMap)
        m_storageMap = newStorageMap.release();

    if (oldValue.isNull())
        return;

    if (m_localStorageDatabase)
        m_localStorageDatabase->removeItem(key);

    dispatchEvents(sourceConnection, sourceStorageAreaID, key, oldValue, String(), urlString);
}

void StorageManager::StorageArea::clear(CoreIPC::Connection* sourceConnection, uint64_t sourceStorageAreaID, const String& urlString)
{
    openDatabaseAndImportItemsIfNeeded();

    if (!m_storageMap->length())
        return;

    m_storageMap = StorageMap::create(m_quotaInBytes);

    if (m_localStorageDatabase)
        m_localStorageDatabase->clear();

    dispatchEvents(sourceConnection, sourceStorageAreaID, String(), String(), String(), urlString);
}

const HashMap<String, String>& StorageManager::StorageArea::items()
{
    openDatabaseAndImportItemsIfNeeded();

    return m_storageMap->items();
}

void StorageManager::StorageArea::clear()
{
    m_storageMap = StorageMap::create(m_quotaInBytes);

    if (m_localStorageDatabase) {
        m_localStorageDatabase->close();
        m_localStorageDatabase = nullptr;
    }

    for (HashSet<std::pair<RefPtr<CoreIPC::Connection>, uint64_t> >::iterator it = m_eventListeners.begin(), end = m_eventListeners.end(); it != end; ++it)
        it->first->send(Messages::StorageAreaMap::ClearCache(), it->second);
}

void StorageManager::StorageArea::openDatabaseAndImportItemsIfNeeded()
{
    if (!m_localStorageNamespace)
        return;

    // We open the database here even if we've already imported our items to ensure that the database is open if we need to write to it.
    if (!m_localStorageDatabase)
        m_localStorageDatabase = LocalStorageDatabase::create(m_localStorageNamespace->storageManager()->m_queue, m_localStorageNamespace->storageManager()->m_localStorageDatabaseTracker, m_securityOrigin.get());

    if (m_didImportItemsFromDatabase)
        return;

    m_localStorageDatabase->importItems(*m_storageMap);
    m_didImportItemsFromDatabase = true;
}

void StorageManager::StorageArea::dispatchEvents(CoreIPC::Connection* sourceConnection, uint64_t sourceStorageAreaID, const String& key, const String& oldValue, const String& newValue, const String& urlString) const
{
    for (HashSet<std::pair<RefPtr<CoreIPC::Connection>, uint64_t> >::const_iterator it = m_eventListeners.begin(), end = m_eventListeners.end(); it != end; ++it) {
        uint64_t storageAreaID = it->first == sourceConnection ? sourceStorageAreaID : 0;

        it->first->send(Messages::StorageAreaMap::DispatchStorageEvent(storageAreaID, key, oldValue, newValue, urlString), it->second);
    }
}

PassRefPtr<StorageManager::LocalStorageNamespace> StorageManager::LocalStorageNamespace::create(StorageManager* storageManager, uint64_t storageNamespaceID)
{
    return adoptRef(new LocalStorageNamespace(storageManager, storageNamespaceID));
}

// FIXME: The quota value is copied from GroupSettings.cpp.
// We should investigate a way to share it with WebCore.
StorageManager::LocalStorageNamespace::LocalStorageNamespace(StorageManager* storageManager, uint64_t storageNamespaceID)
    : m_storageManager(storageManager)
    , m_storageNamespaceID(storageNamespaceID)
    , m_quotaInBytes(5 * 1024 * 1024)
{
}

StorageManager::LocalStorageNamespace::~LocalStorageNamespace()
{
    ASSERT(m_storageAreaMap.isEmpty());
}

PassRefPtr<StorageManager::StorageArea> StorageManager::LocalStorageNamespace::getOrCreateStorageArea(PassRefPtr<SecurityOrigin> securityOrigin)
{
    HashMap<RefPtr<SecurityOrigin>, StorageArea*>::AddResult result = m_storageAreaMap.add(securityOrigin, 0);
    if (!result.isNewEntry)
        return result.iterator->value;

    RefPtr<StorageArea> storageArea = StorageArea::create(this, result.iterator->key, m_quotaInBytes);
    result.iterator->value = storageArea.get();

    return storageArea.release();
}

void StorageManager::LocalStorageNamespace::didDestroyStorageArea(StorageArea* storageArea)
{
    ASSERT(m_storageAreaMap.contains(storageArea->securityOrigin()));

    m_storageAreaMap.remove(storageArea->securityOrigin());
    if (!m_storageAreaMap.isEmpty())
        return;

    ASSERT(m_storageManager->m_localStorageNamespaces.contains(m_storageNamespaceID));
    m_storageManager->m_localStorageNamespaces.remove(m_storageNamespaceID);
}

void StorageManager::LocalStorageNamespace::clearStorageAreasMatchingOrigin(SecurityOrigin* securityOrigin)
{
    for (HashMap<RefPtr<SecurityOrigin>, StorageArea*>::iterator it = m_storageAreaMap.begin(), end = m_storageAreaMap.end(); it != end; ++it) {
        if (it->key->equal(securityOrigin))
            it->value->clear();
    }
}

void StorageManager::LocalStorageNamespace::clearAllStorageAreas()
{
    for (HashMap<RefPtr<SecurityOrigin>, StorageArea*>::iterator it = m_storageAreaMap.begin(), end = m_storageAreaMap.end(); it != end; ++it)
        it->value->clear();
}

class StorageManager::SessionStorageNamespace : public ThreadSafeRefCounted<SessionStorageNamespace> {
public:
    static PassRefPtr<SessionStorageNamespace> create(CoreIPC::Connection* allowedConnection, unsigned quotaInBytes);
    ~SessionStorageNamespace();

    bool isEmpty() const { return m_storageAreaMap.isEmpty(); }

    CoreIPC::Connection* allowedConnection() const { return m_allowedConnection.get(); }
    void setAllowedConnection(CoreIPC::Connection*);

    PassRefPtr<StorageArea> getOrCreateStorageArea(PassRefPtr<SecurityOrigin>);

    void cloneTo(SessionStorageNamespace& newSessionStorageNamespace);

private:
    SessionStorageNamespace(CoreIPC::Connection* allowedConnection, unsigned quotaInBytes);

    RefPtr<CoreIPC::Connection> m_allowedConnection;
    unsigned m_quotaInBytes;

    HashMap<RefPtr<SecurityOrigin>, RefPtr<StorageArea> > m_storageAreaMap;
};

PassRefPtr<StorageManager::SessionStorageNamespace> StorageManager::SessionStorageNamespace::create(CoreIPC::Connection* allowedConnection, unsigned quotaInBytes)
{
    return adoptRef(new SessionStorageNamespace(allowedConnection, quotaInBytes));
}

StorageManager::SessionStorageNamespace::SessionStorageNamespace(CoreIPC::Connection* allowedConnection, unsigned quotaInBytes)
    : m_allowedConnection(allowedConnection)
    , m_quotaInBytes(quotaInBytes)
{
}

StorageManager::SessionStorageNamespace::~SessionStorageNamespace()
{
}

void StorageManager::SessionStorageNamespace::setAllowedConnection(CoreIPC::Connection* allowedConnection)
{
    ASSERT(!allowedConnection || !m_allowedConnection);

    m_allowedConnection = allowedConnection;
}

PassRefPtr<StorageManager::StorageArea> StorageManager::SessionStorageNamespace::getOrCreateStorageArea(PassRefPtr<SecurityOrigin> securityOrigin)
{
    HashMap<RefPtr<SecurityOrigin>, RefPtr<StorageArea> >::AddResult result = m_storageAreaMap.add(securityOrigin, 0);
    if (result.isNewEntry)
        result.iterator->value = StorageArea::create(0, result.iterator->key, m_quotaInBytes);

    return result.iterator->value;
}

void StorageManager::SessionStorageNamespace::cloneTo(SessionStorageNamespace& newSessionStorageNamespace)
{
    ASSERT_UNUSED(newSessionStorageNamespace, newSessionStorageNamespace.isEmpty());

    for (HashMap<RefPtr<SecurityOrigin>, RefPtr<StorageArea> >::const_iterator it = m_storageAreaMap.begin(), end = m_storageAreaMap.end(); it != end; ++it)
        newSessionStorageNamespace.m_storageAreaMap.add(it->key, it->value->clone());
}

PassRefPtr<StorageManager> StorageManager::create()
{
    return adoptRef(new StorageManager);
}

StorageManager::StorageManager()
    : m_queue(WorkQueue::create("com.apple.WebKit.StorageManager"))
    , m_localStorageDatabaseTracker(LocalStorageDatabaseTracker::create(m_queue))
{
    // Make sure the encoding is initialized before we start dispatching things to the queue.
    UTF8Encoding();
}

StorageManager::~StorageManager()
{
}

void StorageManager::setLocalStorageDirectory(const String& localStorageDirectory)
{
    m_localStorageDatabaseTracker->setLocalStorageDirectory(localStorageDirectory);
}

void StorageManager::createSessionStorageNamespace(uint64_t storageNamespaceID, CoreIPC::Connection* allowedConnection, unsigned quotaInBytes)
{
    m_queue->dispatch(bind(&StorageManager::createSessionStorageNamespaceInternal, this, storageNamespaceID, RefPtr<CoreIPC::Connection>(allowedConnection), quotaInBytes));
}

void StorageManager::destroySessionStorageNamespace(uint64_t storageNamespaceID)
{
    m_queue->dispatch(bind(&StorageManager::destroySessionStorageNamespaceInternal, this, storageNamespaceID));
}

void StorageManager::setAllowedSessionStorageNamespaceConnection(uint64_t storageNamespaceID, CoreIPC::Connection* allowedConnection)
{
    m_queue->dispatch(bind(&StorageManager::setAllowedSessionStorageNamespaceConnectionInternal, this, storageNamespaceID, RefPtr<CoreIPC::Connection>(allowedConnection)));
}

void StorageManager::cloneSessionStorageNamespace(uint64_t storageNamespaceID, uint64_t newStorageNamespaceID)
{
    m_queue->dispatch(bind(&StorageManager::cloneSessionStorageNamespaceInternal, this, storageNamespaceID, newStorageNamespaceID));
}

void StorageManager::processWillOpenConnection(WebProcessProxy* webProcessProxy)
{
    webProcessProxy->connection()->addWorkQueueMessageReceiver(Messages::StorageManager::messageReceiverName(), m_queue.get(), this);
}

void StorageManager::processWillCloseConnection(WebProcessProxy* webProcessProxy)
{
    webProcessProxy->connection()->removeWorkQueueMessageReceiver(Messages::StorageManager::messageReceiverName());

    m_queue->dispatch(bind(&StorageManager::invalidateConnectionInternal, this, RefPtr<CoreIPC::Connection>(webProcessProxy->connection())));
}

void StorageManager::getOrigins(FunctionDispatcher* callbackDispatcher, void* context, void (*callback)(const Vector<RefPtr<WebCore::SecurityOrigin> >& securityOrigins, void* context))
{
    m_queue->dispatch(bind(&StorageManager::getOriginsInternal, this, RefPtr<FunctionDispatcher>(callbackDispatcher), context, callback));
}

void StorageManager::deleteEntriesForOrigin(SecurityOrigin* securityOrigin)
{
    m_queue->dispatch(bind(&StorageManager::deleteEntriesForOriginInternal, this, RefPtr<SecurityOrigin>(securityOrigin)));
}

void StorageManager::deleteAllEntries()
{
    m_queue->dispatch(bind(&StorageManager::deleteAllEntriesInternal, this));
}

void StorageManager::createLocalStorageMap(CoreIPC::Connection* connection, uint64_t storageMapID, uint64_t storageNamespaceID, const SecurityOriginData& securityOriginData)
{
    std::pair<RefPtr<CoreIPC::Connection>, uint64_t> connectionAndStorageMapIDPair(connection, storageMapID);

    // FIXME: This should be a message check.
    ASSERT((HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> >::isValidKey(connectionAndStorageMapIDPair)));

    HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> >::AddResult result = m_storageAreasByConnection.add(connectionAndStorageMapIDPair, 0);

    // FIXME: These should be a message checks.
    ASSERT(result.isNewEntry);
    ASSERT((HashMap<uint64_t, RefPtr<LocalStorageNamespace> >::isValidKey(storageNamespaceID)));

    LocalStorageNamespace* localStorageNamespace = getOrCreateLocalStorageNamespace(storageNamespaceID);

    // FIXME: This should be a message check.
    ASSERT(localStorageNamespace);

    RefPtr<StorageArea> storageArea = localStorageNamespace->getOrCreateStorageArea(securityOriginData.securityOrigin());
    storageArea->addListener(connection, storageMapID);

    result.iterator->value = storageArea.release();
}

void StorageManager::createSessionStorageMap(CoreIPC::Connection* connection, uint64_t storageMapID, uint64_t storageNamespaceID, const SecurityOriginData& securityOriginData)
{
    // FIXME: This should be a message check.
    ASSERT((HashMap<uint64_t, RefPtr<SessionStorageNamespace> >::isValidKey(storageNamespaceID)));
    SessionStorageNamespace* sessionStorageNamespace = m_sessionStorageNamespaces.get(storageNamespaceID);
    if (!sessionStorageNamespace) {
        // We're getting an incoming message from the web process that's for session storage for a web page
        // that has already been closed, just ignore it.
        return;
    }

    std::pair<RefPtr<CoreIPC::Connection>, uint64_t> connectionAndStorageMapIDPair(connection, storageMapID);

    // FIXME: This should be a message check.
    ASSERT((HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> >::isValidKey(connectionAndStorageMapIDPair)));

    HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> >::AddResult result = m_storageAreasByConnection.add(connectionAndStorageMapIDPair, 0);

    // FIXME: This should be a message check.
    ASSERT(result.isNewEntry);

    // FIXME: This should be a message check.
    ASSERT(connection == sessionStorageNamespace->allowedConnection());

    RefPtr<StorageArea> storageArea = sessionStorageNamespace->getOrCreateStorageArea(securityOriginData.securityOrigin());
    storageArea->addListener(connection, storageMapID);

    result.iterator->value = storageArea.release();
}

void StorageManager::destroyStorageMap(CoreIPC::Connection* connection, uint64_t storageMapID)
{
    std::pair<RefPtr<CoreIPC::Connection>, uint64_t> connectionAndStorageMapIDPair(connection, storageMapID);

    // FIXME: This should be a message check.
    ASSERT((HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> >::isValidKey(connectionAndStorageMapIDPair)));

    HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> >::iterator it = m_storageAreasByConnection.find(connectionAndStorageMapIDPair);
    if (it == m_storageAreasByConnection.end()) {
        // The connection has been removed because the last page was closed.
        return;
    }

    it->value->removeListener(connection, storageMapID);
    m_storageAreasByConnection.remove(connectionAndStorageMapIDPair);
}

void StorageManager::getValues(CoreIPC::Connection* connection, uint64_t storageMapID, uint64_t storageMapSeed, HashMap<String, String>& values)
{
    StorageArea* storageArea = findStorageArea(connection, storageMapID);
    if (!storageArea) {
        // This is a session storage area for a page that has already been closed. Ignore it.
        return;
    }

    values = storageArea->items();
    connection->send(Messages::StorageAreaMap::DidGetValues(storageMapSeed), storageMapID);
}

void StorageManager::setItem(CoreIPC::Connection* connection, uint64_t storageMapID, uint64_t sourceStorageAreaID, uint64_t storageMapSeed, const String& key, const String& value, const String& urlString)
{
    StorageArea* storageArea = findStorageArea(connection, storageMapID);
    if (!storageArea) {
        // This is a session storage area for a page that has already been closed. Ignore it.
        return;
    }

    bool quotaError;
    storageArea->setItem(connection, sourceStorageAreaID, key, value, urlString, quotaError);
    connection->send(Messages::StorageAreaMap::DidSetItem(storageMapSeed, key, quotaError), storageMapID);
}

void StorageManager::removeItem(CoreIPC::Connection* connection, uint64_t storageMapID, uint64_t sourceStorageAreaID, uint64_t storageMapSeed, const String& key, const String& urlString)
{
    StorageArea* storageArea = findStorageArea(connection, storageMapID);
    if (!storageArea) {
        // This is a session storage area for a page that has already been closed. Ignore it.
        return;
    }

    storageArea->removeItem(connection, sourceStorageAreaID, key, urlString);
    connection->send(Messages::StorageAreaMap::DidRemoveItem(storageMapSeed, key), storageMapID);
}

void StorageManager::clear(CoreIPC::Connection* connection, uint64_t storageMapID, uint64_t sourceStorageAreaID, uint64_t storageMapSeed, const String& urlString)
{
    StorageArea* storageArea = findStorageArea(connection, storageMapID);
    if (!storageArea) {
        // This is a session storage area for a page that has already been closed. Ignore it.
        return;
    }

    storageArea->clear(connection, sourceStorageAreaID, urlString);
    connection->send(Messages::StorageAreaMap::DidClear(storageMapSeed), storageMapID);
}

void StorageManager::createSessionStorageNamespaceInternal(uint64_t storageNamespaceID, CoreIPC::Connection* allowedConnection, unsigned quotaInBytes)
{
    ASSERT(!m_sessionStorageNamespaces.contains(storageNamespaceID));

    m_sessionStorageNamespaces.set(storageNamespaceID, SessionStorageNamespace::create(allowedConnection, quotaInBytes));
}

void StorageManager::destroySessionStorageNamespaceInternal(uint64_t storageNamespaceID)
{
    ASSERT(m_sessionStorageNamespaces.contains(storageNamespaceID));
    m_sessionStorageNamespaces.remove(storageNamespaceID);
}

void StorageManager::setAllowedSessionStorageNamespaceConnectionInternal(uint64_t storageNamespaceID, CoreIPC::Connection* allowedConnection)
{
    ASSERT(m_sessionStorageNamespaces.contains(storageNamespaceID));

    m_sessionStorageNamespaces.get(storageNamespaceID)->setAllowedConnection(allowedConnection);
}

void StorageManager::cloneSessionStorageNamespaceInternal(uint64_t storageNamespaceID, uint64_t newStorageNamespaceID)
{
    SessionStorageNamespace* sessionStorageNamespace = m_sessionStorageNamespaces.get(storageNamespaceID);
    ASSERT(sessionStorageNamespace);

    SessionStorageNamespace* newSessionStorageNamespace = m_sessionStorageNamespaces.get(newStorageNamespaceID);
    ASSERT(newSessionStorageNamespace);

    sessionStorageNamespace->cloneTo(*newSessionStorageNamespace);
}

void StorageManager::invalidateConnectionInternal(CoreIPC::Connection* connection)
{
    Vector<std::pair<RefPtr<CoreIPC::Connection>, uint64_t> > connectionAndStorageMapIDPairsToRemove;
    HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> > storageAreasByConnection = m_storageAreasByConnection;
    for (HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> >::const_iterator it = storageAreasByConnection.begin(), end = storageAreasByConnection.end(); it != end; ++it) {
        if (it->key.first != connection)
            continue;

        it->value->removeListener(it->key.first.get(), it->key.second);
        connectionAndStorageMapIDPairsToRemove.append(it->key);
    }

    for (size_t i = 0; i < connectionAndStorageMapIDPairsToRemove.size(); ++i)
        m_storageAreasByConnection.remove(connectionAndStorageMapIDPairsToRemove[i]);
}

StorageManager::StorageArea* StorageManager::findStorageArea(CoreIPC::Connection* connection, uint64_t storageMapID) const
{
    std::pair<CoreIPC::Connection*, uint64_t> connectionAndStorageMapIDPair(connection, storageMapID);
    if (!HashMap<std::pair<RefPtr<CoreIPC::Connection>, uint64_t>, RefPtr<StorageArea> >::isValidKey(connectionAndStorageMapIDPair))
        return 0;

    return m_storageAreasByConnection.get(connectionAndStorageMapIDPair);
}

StorageManager::LocalStorageNamespace* StorageManager::getOrCreateLocalStorageNamespace(uint64_t storageNamespaceID)
{
    if (!HashMap<uint64_t, RefPtr<LocalStorageNamespace> >::isValidKey(storageNamespaceID))
        return 0;

    HashMap<uint64_t, RefPtr<LocalStorageNamespace> >::AddResult result = m_localStorageNamespaces.add(storageNamespaceID, 0);
    if (result.isNewEntry)
        result.iterator->value = LocalStorageNamespace::create(this, storageNamespaceID);

    return result.iterator->value.get();
}

static void callCallbackFunction(void* context, void (*callbackFunction)(const Vector<RefPtr<WebCore::SecurityOrigin> >& securityOrigins, void* context), Vector<RefPtr<WebCore::SecurityOrigin> >* securityOriginsPtr)
{
    OwnPtr<Vector<RefPtr<WebCore::SecurityOrigin> > > securityOrigins = adoptPtr(securityOriginsPtr);
    callbackFunction(*securityOrigins, context);
}

void StorageManager::getOriginsInternal(FunctionDispatcher* dispatcher, void* context, void (*callbackFunction)(const Vector<RefPtr<WebCore::SecurityOrigin> >& securityOrigins, void* context))
{
    OwnPtr<Vector<RefPtr<WebCore::SecurityOrigin> > > securityOrigins = adoptPtr(new Vector<RefPtr<WebCore::SecurityOrigin> >(m_localStorageDatabaseTracker->origins()));
    dispatcher->dispatch(bind(callCallbackFunction, context, callbackFunction, securityOrigins.leakPtr()));
}

void StorageManager::deleteEntriesForOriginInternal(SecurityOrigin* securityOrigin)
{
    for (HashMap<uint64_t, RefPtr<LocalStorageNamespace> >::iterator it = m_localStorageNamespaces.begin(), end = m_localStorageNamespaces.end(); it != end; ++it)
        it->value->clearStorageAreasMatchingOrigin(securityOrigin);

    m_localStorageDatabaseTracker->deleteDatabaseWithOrigin(securityOrigin);
}

void StorageManager::deleteAllEntriesInternal()
{
    for (HashMap<uint64_t, RefPtr<LocalStorageNamespace> >::iterator it = m_localStorageNamespaces.begin(), end = m_localStorageNamespaces.end(); it != end; ++it)
        it->value->clearAllStorageAreas();

    m_localStorageDatabaseTracker->deleteAllDatabases();
}


} // namespace WebKit
