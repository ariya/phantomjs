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

#ifndef StorageAreaMap_h
#define StorageAreaMap_h

#include "MessageReceiver.h"
#include <WebCore/SecurityOrigin.h>
#include <WebCore/StorageArea.h>
#include <wtf/Forward.h>
#include <wtf/HashCountedSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {
class SecurityOrigin;
class StorageMap;
}

namespace WebKit {

class StorageAreaImpl;
class StorageNamespaceImpl;

class StorageAreaMap : public RefCounted<StorageAreaMap>, private CoreIPC::MessageReceiver {
public:
    static PassRefPtr<StorageAreaMap> create(StorageNamespaceImpl*, PassRefPtr<WebCore::SecurityOrigin>);
    ~StorageAreaMap();

    WebCore::StorageType storageType() const { return m_storageType; }

    unsigned length();
    String key(unsigned index);
    String item(const String& key);
    void setItem(WebCore::Frame* sourceFrame, StorageAreaImpl* sourceArea, const String& key, const String& value, bool& quotaException);
    void removeItem(WebCore::Frame* sourceFrame, StorageAreaImpl* sourceArea, const String& key);
    void clear(WebCore::Frame* sourceFrame, StorageAreaImpl* sourceArea);
    bool contains(const String& key);

private:
    StorageAreaMap(StorageNamespaceImpl*, PassRefPtr<WebCore::SecurityOrigin>);

    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    void didGetValues(uint64_t storageMapSeed);
    void didSetItem(uint64_t storageMapSeed, const String& key, bool quotaError);
    void didRemoveItem(uint64_t storageMapSeed, const String& key);
    void didClear(uint64_t storageMapSeed);

    void dispatchStorageEvent(uint64_t sourceStorageAreaID, const String& key, const String& oldValue, const String& newValue, const String& urlString);
    void clearCache();

    void resetValues();
    void loadValuesIfNeeded();

    bool shouldApplyChangeForKey(const String& key) const;
    void applyChange(const String& key, const String& newValue);

    void dispatchSessionStorageEvent(uint64_t sourceStorageAreaID, const String& key, const String& oldValue, const String& newValue, const String& urlString);
    void dispatchLocalStorageEvent(uint64_t sourceStorageAreaID, const String& key, const String& oldValue, const String& newValue, const String& urlString);

    uint64_t m_storageMapID;

    WebCore::StorageType m_storageType;
    uint64_t m_storageNamespaceID;
    unsigned m_quotaInBytes;
    RefPtr<WebCore::SecurityOrigin> m_securityOrigin;

    RefPtr<WebCore::StorageMap> m_storageMap;

    uint64_t m_currentSeed;
    bool m_hasPendingClear;
    bool m_hasPendingGetValues;
    HashCountedSet<String> m_pendingValueChanges;
};

} // namespace WebKit

#endif // StorageAreaMap_h
