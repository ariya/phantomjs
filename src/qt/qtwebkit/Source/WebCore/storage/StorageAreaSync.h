/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
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

#ifndef StorageAreaSync_h
#define StorageAreaSync_h

#include "SQLiteDatabase.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class Frame;
class StorageAreaImpl;
class StorageSyncManager;

class StorageAreaSync : public ThreadSafeRefCounted<StorageAreaSync> {
public:
    static PassRefPtr<StorageAreaSync> create(PassRefPtr<StorageSyncManager>, PassRefPtr<StorageAreaImpl>, const String& databaseIdentifier);
    ~StorageAreaSync();

    void scheduleFinalSync();
    void blockUntilImportComplete();

    void scheduleItemForSync(const String& key, const String& value);
    void scheduleClear();
    void scheduleCloseDatabase();

    void scheduleSync();

private:
    StorageAreaSync(PassRefPtr<StorageSyncManager>, PassRefPtr<StorageAreaImpl>, const String& databaseIdentifier);

    void dispatchStorageEvent(const String& key, const String& oldValue, const String& newValue, Frame* sourceFrame);

    Timer<StorageAreaSync> m_syncTimer;
    HashMap<String, String> m_changedItems;
    bool m_itemsCleared;

    bool m_finalSyncScheduled;

    RefPtr<StorageAreaImpl> m_storageArea;
    RefPtr<StorageSyncManager> m_syncManager;

    // The database handle will only ever be opened and used on the background thread.
    SQLiteDatabase m_database;

    // The following members are subject to thread synchronization issues.
public:
    // Called from the background thread
    void performImport();
    void performSync();
    void deleteEmptyDatabase();

private:
    enum OpenDatabaseParamType {
      CreateIfNonExistent,
      SkipIfNonExistent
    };

    void syncTimerFired(Timer<StorageAreaSync>*);
    void openDatabase(OpenDatabaseParamType openingStrategy);
    void sync(bool clearItems, const HashMap<String, String>& items);

    const String m_databaseIdentifier;

    Mutex m_syncLock;
    HashMap<String, String> m_itemsPendingSync;
    bool m_clearItemsWhileSyncing;
    bool m_syncScheduled;
    bool m_syncInProgress;
    bool m_databaseOpenFailed;

    bool m_syncCloseDatabase;

    mutable Mutex m_importLock;
    mutable ThreadCondition m_importCondition;
    mutable bool m_importComplete;
    void markImported();
    void migrateItemTableIfNeeded();
};

} // namespace WebCore

#endif // StorageAreaSync_h
