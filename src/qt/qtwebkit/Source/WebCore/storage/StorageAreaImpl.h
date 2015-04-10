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

#ifndef StorageAreaImpl_h
#define StorageAreaImpl_h

#include "StorageArea.h"
#include "Timer.h"

#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class SecurityOrigin;
class StorageMap;
class StorageAreaSync;

class StorageAreaImpl : public StorageArea {
public:
    static PassRefPtr<StorageAreaImpl> create(StorageType, PassRefPtr<SecurityOrigin>, PassRefPtr<StorageSyncManager>, unsigned quota);
    virtual ~StorageAreaImpl();

    virtual unsigned length() OVERRIDE;
    virtual String key(unsigned index) OVERRIDE;
    virtual String item(const String& key) OVERRIDE;
    virtual void setItem(Frame* sourceFrame, const String& key, const String& value, bool& quotaException) OVERRIDE;
    virtual void removeItem(Frame* sourceFrame, const String& key) OVERRIDE;
    virtual void clear(Frame* sourceFrame) OVERRIDE;
    virtual bool contains(const String& key) OVERRIDE;

    virtual bool canAccessStorage(Frame* sourceFrame) OVERRIDE;
    virtual StorageType storageType() const OVERRIDE;

    virtual size_t memoryBytesUsedByCache() OVERRIDE;

    virtual void incrementAccessCount();
    virtual void decrementAccessCount();
    virtual void closeDatabaseIfIdle();

    PassRefPtr<StorageAreaImpl> copy();
    void close();

    // Only called from a background thread.
    void importItems(const HashMap<String, String>& items);

    // Used to clear a StorageArea and close db before backing db file is deleted.
    void clearForOriginDeletion();

    void sync();

private:
    StorageAreaImpl(StorageType, PassRefPtr<SecurityOrigin>, PassRefPtr<StorageSyncManager>, unsigned quota);
    explicit StorageAreaImpl(StorageAreaImpl*);

    void blockUntilImportComplete() const;
    void closeDatabaseTimerFired(Timer<StorageAreaImpl>*);

    void dispatchStorageEvent(const String& key, const String& oldValue, const String& newValue, Frame* sourceFrame);

    StorageType m_storageType;
    RefPtr<SecurityOrigin> m_securityOrigin;
    RefPtr<StorageMap> m_storageMap;

    RefPtr<StorageAreaSync> m_storageAreaSync;
    RefPtr<StorageSyncManager> m_storageSyncManager;

#ifndef NDEBUG
    bool m_isShutdown;
#endif
    unsigned m_accessCount;
    Timer<StorageAreaImpl> m_closeDatabaseTimer;
};

} // namespace WebCore

#endif // StorageAreaImpl_h
