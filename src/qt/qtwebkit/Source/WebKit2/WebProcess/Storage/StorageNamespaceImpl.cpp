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
#include "StorageNamespaceImpl.h"

#include "StorageAreaImpl.h"
#include "StorageAreaMap.h"
#include "WebPage.h"
#include "WebPageGroupProxy.h"
#include "WebProcess.h"
#include <WebCore/GroupSettings.h>
#include <WebCore/PageGroup.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/Settings.h>

using namespace WebCore;

namespace WebKit {

typedef HashMap<uint64_t, StorageNamespaceImpl*> LocalStorageNamespaceMap;

static LocalStorageNamespaceMap& localStorageNamespaceMap()
{
    DEFINE_STATIC_LOCAL(LocalStorageNamespaceMap, localStorageNamespaceMap, ());
    return localStorageNamespaceMap;
}

PassRefPtr<StorageNamespaceImpl> StorageNamespaceImpl::createLocalStorageNamespace(PageGroup* pageGroup)
{
    uint64_t pageGroupID = WebProcess::shared().webPageGroup(pageGroup)->pageGroupID();

    LocalStorageNamespaceMap::AddResult result = localStorageNamespaceMap().add(pageGroupID, 0);
    if (!result.isNewEntry)
        return result.iterator->value;

    unsigned quota = pageGroup->groupSettings()->localStorageQuotaBytes();
    RefPtr<StorageNamespaceImpl> localStorageNamespace = adoptRef(new StorageNamespaceImpl(LocalStorage, pageGroupID, quota));

    result.iterator->value = localStorageNamespace.get();
    return localStorageNamespace.release();
}

PassRefPtr<StorageNamespaceImpl> StorageNamespaceImpl::createSessionStorageNamespace(WebPage* webPage)
{
    return adoptRef(new StorageNamespaceImpl(SessionStorage, webPage->pageID(), webPage->corePage()->settings()->sessionStorageQuota()));
}

StorageNamespaceImpl::StorageNamespaceImpl(WebCore::StorageType storageType, uint64_t storageNamespaceID, unsigned quotaInBytes)
    : m_storageType(storageType)
    , m_storageNamespaceID(storageNamespaceID)
    , m_quotaInBytes(quotaInBytes)
{
}

StorageNamespaceImpl::~StorageNamespaceImpl()
{
    if (m_storageType == LocalStorage) {
        ASSERT(localStorageNamespaceMap().contains(m_storageNamespaceID));
        localStorageNamespaceMap().remove(m_storageNamespaceID);
    }
}

PassRefPtr<StorageArea> StorageNamespaceImpl::storageArea(PassRefPtr<SecurityOrigin> securityOrigin)
{
    HashMap<RefPtr<WebCore::SecurityOrigin>, RefPtr<StorageAreaMap> >::AddResult result = m_storageAreaMaps.add(securityOrigin.get(), 0);
    if (result.isNewEntry)
        result.iterator->value = StorageAreaMap::create(this, securityOrigin);

    return StorageAreaImpl::create(result.iterator->value);
}

PassRefPtr<StorageNamespace> StorageNamespaceImpl::copy(Page* newPage)
{
    ASSERT(m_storageNamespaceID);

    return createSessionStorageNamespace(WebPage::fromCorePage(newPage));
}

void StorageNamespaceImpl::close()
{
    // FIXME: Implement this.
    ASSERT_NOT_REACHED();
}

void StorageNamespaceImpl::clearOriginForDeletion(SecurityOrigin*)
{
    // FIXME: Implement this.
    ASSERT_NOT_REACHED();
}

void StorageNamespaceImpl::clearAllOriginsForDeletion()
{
    // FIXME: Implement this.
    ASSERT_NOT_REACHED();
}

void StorageNamespaceImpl::sync()
{
    // FIXME: Implement this.
    ASSERT_NOT_REACHED();
}

void StorageNamespaceImpl::closeIdleLocalStorageDatabases()
{
    // FIXME: Implement this.
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
