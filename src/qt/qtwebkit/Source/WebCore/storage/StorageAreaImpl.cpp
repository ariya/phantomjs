/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "StorageAreaImpl.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "Page.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "StorageAreaSync.h"
#include "StorageEventDispatcher.h"
#include "StorageMap.h"
#include "StorageSyncManager.h"
#include "StorageTracker.h"
#include <wtf/MainThread.h>

namespace WebCore {

StorageAreaImpl::~StorageAreaImpl()
{
    ASSERT(isMainThread());
}

inline StorageAreaImpl::StorageAreaImpl(StorageType storageType, PassRefPtr<SecurityOrigin> origin, PassRefPtr<StorageSyncManager> syncManager, unsigned quota)
    : m_storageType(storageType)
    , m_securityOrigin(origin)
    , m_storageMap(StorageMap::create(quota))
    , m_storageSyncManager(syncManager)
#ifndef NDEBUG
    , m_isShutdown(false)
#endif
    , m_accessCount(0)
    , m_closeDatabaseTimer(this, &StorageAreaImpl::closeDatabaseTimerFired)
{
    ASSERT(isMainThread());
    ASSERT(m_securityOrigin);
    ASSERT(m_storageMap);
    
    // Accessing the shared global StorageTracker when a StorageArea is created 
    // ensures that the tracker is properly initialized before anyone actually needs to use it.
    StorageTracker::tracker();
}

PassRefPtr<StorageAreaImpl> StorageAreaImpl::create(StorageType storageType, PassRefPtr<SecurityOrigin> origin, PassRefPtr<StorageSyncManager> syncManager, unsigned quota)
{
    RefPtr<StorageAreaImpl> area = adoptRef(new StorageAreaImpl(storageType, origin, syncManager, quota));

    // FIXME: If there's no backing storage for LocalStorage, the default WebKit behavior should be that of private browsing,
    // not silently ignoring it. https://bugs.webkit.org/show_bug.cgi?id=25894
    if (area->m_storageSyncManager) {
        area->m_storageAreaSync = StorageAreaSync::create(area->m_storageSyncManager, area.get(), area->m_securityOrigin->databaseIdentifier());
        ASSERT(area->m_storageAreaSync);
    }

    return area.release();
}

PassRefPtr<StorageAreaImpl> StorageAreaImpl::copy()
{
    ASSERT(!m_isShutdown);
    return adoptRef(new StorageAreaImpl(this));
}

StorageAreaImpl::StorageAreaImpl(StorageAreaImpl* area)
    : m_storageType(area->m_storageType)
    , m_securityOrigin(area->m_securityOrigin)
    , m_storageMap(area->m_storageMap)
    , m_storageSyncManager(area->m_storageSyncManager)
#ifndef NDEBUG
    , m_isShutdown(area->m_isShutdown)
#endif
    , m_accessCount(0)
    , m_closeDatabaseTimer(this, &StorageAreaImpl::closeDatabaseTimerFired)
{
    ASSERT(isMainThread());
    ASSERT(m_securityOrigin);
    ASSERT(m_storageMap);
    ASSERT(!m_isShutdown);
}

bool StorageAreaImpl::canAccessStorage(Frame* frame)
{
    return frame && frame->page();
}

StorageType StorageAreaImpl::storageType() const
{
    return m_storageType;
}

unsigned StorageAreaImpl::length()
{
    ASSERT(!m_isShutdown);
    blockUntilImportComplete();

    return m_storageMap->length();
}

String StorageAreaImpl::key(unsigned index)
{
    ASSERT(!m_isShutdown);
    blockUntilImportComplete();

    return m_storageMap->key(index);
}

String StorageAreaImpl::item(const String& key)
{
    ASSERT(!m_isShutdown);
    blockUntilImportComplete();

    return m_storageMap->getItem(key);
}

void StorageAreaImpl::setItem(Frame* sourceFrame, const String& key, const String& value, bool& quotaException)
{
    ASSERT(!m_isShutdown);
    ASSERT(!value.isNull());
    blockUntilImportComplete();

    String oldValue;
    RefPtr<StorageMap> newMap = m_storageMap->setItem(key, value, oldValue, quotaException);
    if (newMap)
        m_storageMap = newMap.release();

    if (quotaException)
        return;

    if (oldValue == value)
        return;

    if (m_storageAreaSync)
        m_storageAreaSync->scheduleItemForSync(key, value);

    dispatchStorageEvent(key, oldValue, value, sourceFrame);
}

void StorageAreaImpl::removeItem(Frame* sourceFrame, const String& key)
{
    ASSERT(!m_isShutdown);
    blockUntilImportComplete();

    String oldValue;
    RefPtr<StorageMap> newMap = m_storageMap->removeItem(key, oldValue);
    if (newMap)
        m_storageMap = newMap.release();

    if (oldValue.isNull())
        return;

    if (m_storageAreaSync)
        m_storageAreaSync->scheduleItemForSync(key, String());

    dispatchStorageEvent(key, oldValue, String(), sourceFrame);
}

void StorageAreaImpl::clear(Frame* sourceFrame)
{
    ASSERT(!m_isShutdown);
    blockUntilImportComplete();

    if (!m_storageMap->length())
        return;

    unsigned quota = m_storageMap->quota();
    m_storageMap = StorageMap::create(quota);

    if (m_storageAreaSync)
        m_storageAreaSync->scheduleClear();

    dispatchStorageEvent(String(), String(), String(), sourceFrame);
}

bool StorageAreaImpl::contains(const String& key)
{
    ASSERT(!m_isShutdown);
    blockUntilImportComplete();

    return m_storageMap->contains(key);
}

void StorageAreaImpl::importItems(const HashMap<String, String>& items)
{
    ASSERT(!m_isShutdown);

    m_storageMap->importItems(items);
}

void StorageAreaImpl::close()
{
    if (m_storageAreaSync)
        m_storageAreaSync->scheduleFinalSync();

#ifndef NDEBUG
    m_isShutdown = true;
#endif
}

void StorageAreaImpl::clearForOriginDeletion()
{
    ASSERT(!m_isShutdown);
    blockUntilImportComplete();
    
    if (m_storageMap->length()) {
        unsigned quota = m_storageMap->quota();
        m_storageMap = StorageMap::create(quota);
    }

    if (m_storageAreaSync) {
        m_storageAreaSync->scheduleClear();
        m_storageAreaSync->scheduleCloseDatabase();
    }
}
    
void StorageAreaImpl::sync()
{
    ASSERT(!m_isShutdown);
    blockUntilImportComplete();
    
    if (m_storageAreaSync)
        m_storageAreaSync->scheduleSync();
}

void StorageAreaImpl::blockUntilImportComplete() const
{
    if (m_storageAreaSync)
        m_storageAreaSync->blockUntilImportComplete();
}

size_t StorageAreaImpl::memoryBytesUsedByCache()
{
    return 0;
}

void StorageAreaImpl::incrementAccessCount()
{
    m_accessCount++;

    if (m_closeDatabaseTimer.isActive())
        m_closeDatabaseTimer.stop();
}

void StorageAreaImpl::decrementAccessCount()
{
    ASSERT(m_accessCount);
    --m_accessCount;

    if (!m_accessCount) {
        if (m_closeDatabaseTimer.isActive())
            m_closeDatabaseTimer.stop();
        m_closeDatabaseTimer.startOneShot(StorageTracker::tracker().storageDatabaseIdleInterval());
    }
}

void StorageAreaImpl::closeDatabaseTimerFired(Timer<StorageAreaImpl> *)
{
    blockUntilImportComplete();
    if (m_storageAreaSync)
        m_storageAreaSync->scheduleCloseDatabase();
}

void StorageAreaImpl::closeDatabaseIfIdle()
{
    if (m_closeDatabaseTimer.isActive()) {
        ASSERT(!m_accessCount);
        m_closeDatabaseTimer.stop();

        closeDatabaseTimerFired(&m_closeDatabaseTimer);
    }
}

void StorageAreaImpl::dispatchStorageEvent(const String& key, const String& oldValue, const String& newValue, Frame* sourceFrame)
{
    if (m_storageType == LocalStorage)
        StorageEventDispatcher::dispatchLocalStorageEvents(key, oldValue, newValue, m_securityOrigin.get(), sourceFrame);
    else
        StorageEventDispatcher::dispatchSessionStorageEvents(key, oldValue, newValue, m_securityOrigin.get(), sourceFrame);
}

} // namespace WebCore
