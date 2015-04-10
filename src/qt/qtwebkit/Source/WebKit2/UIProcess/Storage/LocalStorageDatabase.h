/*
 * Copyright (C) 2008, 2009, 2010, 2013 Apple Inc. All rights reserved.
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

#ifndef LocalStorageDatabase_h
#define LocalStorageDatabase_h

#include <WebCore/SQLiteDatabase.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>

class WorkQueue;

namespace WebCore {
class SecurityOrigin;
class StorageMap;
}

namespace WebKit {

class LocalStorageDatabaseTracker;

class LocalStorageDatabase : public ThreadSafeRefCounted<LocalStorageDatabase> {
public:
    static PassRefPtr<LocalStorageDatabase> create(PassRefPtr<WorkQueue>, PassRefPtr<LocalStorageDatabaseTracker>, PassRefPtr<WebCore::SecurityOrigin>);
    ~LocalStorageDatabase();

    // Will block until the import is complete.
    void importItems(WebCore::StorageMap&);

    void setItem(const String& key, const String& value);
    void removeItem(const String& key);
    void clear();

    // Will block until all pending changes have been written to disk.
    void close();

private:
    LocalStorageDatabase(PassRefPtr<WorkQueue>, PassRefPtr<LocalStorageDatabaseTracker>, PassRefPtr<WebCore::SecurityOrigin>);

    enum DatabaseOpeningStrategy {
        CreateIfNonExistent,
        SkipIfNonExistent
    };
    bool tryToOpenDatabase(DatabaseOpeningStrategy);
    void openDatabase(DatabaseOpeningStrategy);

    bool migrateItemTableIfNeeded();

    void itemDidChange(const String& key, const String& value);

    void scheduleDatabaseUpdate();
    void updateDatabase();
    void updateDatabaseWithChangedItems(const HashMap<String, String>&);

    bool databaseIsEmpty();

    RefPtr<WorkQueue> m_queue;
    RefPtr<LocalStorageDatabaseTracker> m_tracker;
    RefPtr<WebCore::SecurityOrigin> m_securityOrigin;

    String m_databasePath;
    WebCore::SQLiteDatabase m_database;
    bool m_failedToOpenDatabase;
    bool m_didImportItems;
    bool m_isClosed;

    bool m_didScheduleDatabaseUpdate;
    bool m_shouldClearItems;
    HashMap<String, String> m_changedItems;
};


} // namespace WebKit

#endif // LocalStorageDatabase_h
