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
#include "StorageNamespaceImpl.h"

#include "GroupSettings.h"
#include "Page.h"
#include "PageGroup.h"
#include "SecurityOriginHash.h"
#include "Settings.h"
#include "StorageAreaImpl.h"
#include "StorageMap.h"
#include "StorageSyncManager.h"
#include "StorageTracker.h"
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

typedef HashMap<String, StorageNamespace*> LocalStorageNamespaceMap;

static LocalStorageNamespaceMap& localStorageNamespaceMap()
{
    DEFINE_STATIC_LOCAL(LocalStorageNamespaceMap, localStorageNamespaceMap, ());
    return localStorageNamespaceMap;
}

PassRefPtr<StorageNamespace> StorageNamespaceImpl::localStorageNamespace(PageGroup* pageGroup)
{
    // Need a page in this page group to query the settings for the local storage database path.
    // Having these parameters attached to the page settings is unfortunate since these settings are
    // not per-page (and, in fact, we simply grab the settings from some page at random), but
    // at this point we're stuck with it.
    Page* page = *pageGroup->pages().begin();
    const String& path = page->settings()->localStorageDatabasePath();
    unsigned quota = pageGroup->groupSettings()->localStorageQuotaBytes();
    const String lookupPath = path.isNull() ? emptyString() : path;

    LocalStorageNamespaceMap::AddResult result = localStorageNamespaceMap().add(lookupPath, 0);
    if (!result.isNewEntry)
        return result.iterator->value;

    RefPtr<StorageNamespace> storageNamespace = adoptRef(new StorageNamespaceImpl(LocalStorage, lookupPath, quota));

    result.iterator->value = storageNamespace.get();
    return storageNamespace.release();
}

PassRefPtr<StorageNamespace> StorageNamespaceImpl::sessionStorageNamespace(Page* page)
{
    return adoptRef(new StorageNamespaceImpl(SessionStorage, String(), page->settings()->sessionStorageQuota()));
}

PassRefPtr<StorageNamespace> StorageNamespaceImpl::transientLocalStorageNamespace(PageGroup* pageGroup, SecurityOrigin*)
{
    // FIXME: A smarter implementation would create a special namespace type instead of just piggy-backing off
    // SessionStorageNamespace here.
    return StorageNamespaceImpl::sessionStorageNamespace(*pageGroup->pages().begin());
}

StorageNamespaceImpl::StorageNamespaceImpl(StorageType storageType, const String& path, unsigned quota)
    : m_storageType(storageType)
    , m_path(path.isolatedCopy())
    , m_syncManager(0)
    , m_quota(quota)
    , m_isShutdown(false)
{
    if (m_storageType == LocalStorage && !m_path.isEmpty())
        m_syncManager = StorageSyncManager::create(m_path);
}

StorageNamespaceImpl::~StorageNamespaceImpl()
{
    ASSERT(isMainThread());

    if (m_storageType == LocalStorage) {
        ASSERT(localStorageNamespaceMap().get(m_path) == this);
        localStorageNamespaceMap().remove(m_path);
    }

    if (!m_isShutdown)
        close();
}

PassRefPtr<StorageNamespace> StorageNamespaceImpl::copy(Page*)
{
    ASSERT(isMainThread());
    ASSERT(!m_isShutdown);
    ASSERT(m_storageType == SessionStorage);

    RefPtr<StorageNamespaceImpl> newNamespace = adoptRef(new StorageNamespaceImpl(m_storageType, m_path, m_quota));

    StorageAreaMap::iterator end = m_storageAreaMap.end();
    for (StorageAreaMap::iterator i = m_storageAreaMap.begin(); i != end; ++i)
        newNamespace->m_storageAreaMap.set(i->key, i->value->copy());
    return newNamespace.release();
}

PassRefPtr<StorageArea> StorageNamespaceImpl::storageArea(PassRefPtr<SecurityOrigin> prpOrigin)
{
    ASSERT(isMainThread());
    ASSERT(!m_isShutdown);

    RefPtr<SecurityOrigin> origin = prpOrigin;
    RefPtr<StorageAreaImpl> storageArea;
    if ((storageArea = m_storageAreaMap.get(origin)))
        return storageArea.release();

    storageArea = StorageAreaImpl::create(m_storageType, origin, m_syncManager, m_quota);
    m_storageAreaMap.set(origin.release(), storageArea);
    return storageArea.release();
}

void StorageNamespaceImpl::close()
{
    ASSERT(isMainThread());

    if (m_isShutdown)
        return;

    // If we're session storage, we shouldn't need to do any work here.
    if (m_storageType == SessionStorage) {
        ASSERT(!m_syncManager);
        return;
    }

    StorageAreaMap::iterator end = m_storageAreaMap.end();
    for (StorageAreaMap::iterator it = m_storageAreaMap.begin(); it != end; ++it)
        it->value->close();

    if (m_syncManager)
        m_syncManager->close();

    m_isShutdown = true;
}

void StorageNamespaceImpl::clearOriginForDeletion(SecurityOrigin* origin)
{
    ASSERT(isMainThread());

    RefPtr<StorageAreaImpl> storageArea = m_storageAreaMap.get(origin);
    if (storageArea)
        storageArea->clearForOriginDeletion();
}

void StorageNamespaceImpl::clearAllOriginsForDeletion()
{
    ASSERT(isMainThread());

    StorageAreaMap::iterator end = m_storageAreaMap.end();
    for (StorageAreaMap::iterator it = m_storageAreaMap.begin(); it != end; ++it)
        it->value->clearForOriginDeletion();
}
    
void StorageNamespaceImpl::sync()
{
    ASSERT(isMainThread());
    StorageAreaMap::iterator end = m_storageAreaMap.end();
    for (StorageAreaMap::iterator it = m_storageAreaMap.begin(); it != end; ++it)
        it->value->sync();
}

void StorageNamespaceImpl::closeIdleLocalStorageDatabases()
{
    ASSERT(isMainThread());
    StorageAreaMap::iterator end = m_storageAreaMap.end();
    for (StorageAreaMap::iterator it = m_storageAreaMap.begin(); it != end; ++it)
        it->value->closeDatabaseIfIdle();
}

} // namespace WebCore
