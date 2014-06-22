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

#ifndef StorageAreaImpl_h
#define StorageAreaImpl_h

#include "MessageReceiver.h"
#include <WebCore/StorageArea.h>
#include <wtf/HashCountedSet.h>
#include <wtf/HashMap.h>

namespace WebKit {

class StorageAreaMap;

class StorageAreaImpl FINAL : public WebCore::StorageArea {
public:
    static PassRefPtr<StorageAreaImpl> create(PassRefPtr<StorageAreaMap>);
    virtual ~StorageAreaImpl();

    uint64_t storageAreaID() const { return m_storageAreaID; }

private:
    StorageAreaImpl(PassRefPtr<StorageAreaMap>);

    // WebCore::StorageArea.
    virtual unsigned length() OVERRIDE;
    virtual String key(unsigned index) OVERRIDE;
    virtual String item(const String& key) OVERRIDE;
    virtual void setItem(WebCore::Frame* sourceFrame, const String& key, const String& value, bool& quotaException) OVERRIDE;
    virtual void removeItem(WebCore::Frame* sourceFrame, const String& key) OVERRIDE;
    virtual void clear(WebCore::Frame* sourceFrame) OVERRIDE;
    virtual bool contains(const String& key) OVERRIDE;
    virtual bool canAccessStorage(WebCore::Frame*) OVERRIDE;
    virtual WebCore::StorageType storageType() const OVERRIDE;
    virtual size_t memoryBytesUsedByCache() OVERRIDE;
    virtual void incrementAccessCount() OVERRIDE;
    virtual void decrementAccessCount() OVERRIDE;
    virtual void closeDatabaseIfIdle() OVERRIDE;

    uint64_t m_storageAreaID;
    RefPtr<StorageAreaMap> m_storageAreaMap;
};

} // namespace WebKit

#endif // StorageAreaImpl_h
