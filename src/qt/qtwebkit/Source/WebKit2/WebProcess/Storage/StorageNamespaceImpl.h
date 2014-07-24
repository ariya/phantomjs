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

#ifndef StorageNamespaceImpl_h
#define StorageNamespaceImpl_h

#include <WebCore/SecurityOriginHash.h>
#include <WebCore/StorageArea.h>
#include <WebCore/StorageNamespace.h>
#include <wtf/HashMap.h>

namespace WebKit {

class StorageAreaMap;
class WebPage;

class StorageNamespaceImpl : public WebCore::StorageNamespace {
public:
    static PassRefPtr<StorageNamespaceImpl> createLocalStorageNamespace(WebCore::PageGroup*);
    static PassRefPtr<StorageNamespaceImpl> createSessionStorageNamespace(WebPage*);
    virtual ~StorageNamespaceImpl();

    WebCore::StorageType storageType() const { return m_storageType; }
    uint64_t storageNamespaceID() const { return m_storageNamespaceID; }
    unsigned quotaInBytes() const { return m_quotaInBytes; }

private:
    explicit StorageNamespaceImpl(WebCore::StorageType, uint64_t storageNamespaceID, unsigned quotaInBytes);

    virtual PassRefPtr<WebCore::StorageArea> storageArea(PassRefPtr<WebCore::SecurityOrigin>) OVERRIDE;
    virtual PassRefPtr<WebCore::StorageNamespace> copy(WebCore::Page*) OVERRIDE;
    virtual void close() OVERRIDE;
    virtual void clearOriginForDeletion(WebCore::SecurityOrigin*) OVERRIDE;
    virtual void clearAllOriginsForDeletion() OVERRIDE;
    virtual void sync() OVERRIDE;
    virtual void closeIdleLocalStorageDatabases() OVERRIDE;

    WebCore::StorageType m_storageType;
    uint64_t m_storageNamespaceID;
    unsigned m_quotaInBytes;

    HashMap<RefPtr<WebCore::SecurityOrigin>, RefPtr<StorageAreaMap> > m_storageAreaMaps;
};

} // namespace WebKit

#endif // StorageNamespaceImpl_h
