/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef StorageTracker_h
#define StorageTracker_h

#include "SQLiteDatabase.h"
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class StorageTask;
class StorageThread;
class SecurityOrigin;
class StorageTrackerClient;    

class StorageTracker {
    WTF_MAKE_NONCOPYABLE(StorageTracker);
    WTF_MAKE_FAST_ALLOCATED;
public:
    static void initializeTracker(const String& storagePath, StorageTrackerClient*);
    static StorageTracker& tracker();

    void setDatabaseDirectoryPath(const String&);
    String databaseDirectoryPath() const;

    void setOriginDetails(const String& originIdentifier, const String& databaseFile);
    
    void deleteAllOrigins();
    void deleteOrigin(SecurityOrigin*);
    void deleteOriginWithIdentifier(const String& originIdentifier);
    void origins(Vector<RefPtr<SecurityOrigin> >& result);
    long long diskUsageForOrigin(SecurityOrigin*);
    
    void cancelDeletingOrigin(const String& originIdentifier);
    
    bool isActive();

    double storageDatabaseIdleInterval() { return m_StorageDatabaseIdleInterval; }
    void setStorageDatabaseIdleInterval(double interval) { m_StorageDatabaseIdleInterval = interval; }

    void syncFileSystemAndTrackerDatabase();

private:
    explicit StorageTracker(const String& storagePath);

    void internalInitialize();

    String trackerDatabasePath();
    void openTrackerDatabase(bool createIfDoesNotExist);

    void importOriginIdentifiers();
    void finishedImportingOriginIdentifiers();
    
    void deleteTrackerFiles();
    String databasePathForOrigin(const String& originIdentifier);

    bool canDeleteOrigin(const String& originIdentifier);
    void willDeleteOrigin(const String& originIdentifier);
    void willDeleteAllOrigins();

    void originFilePaths(Vector<String>& paths);
    
    void setIsActive(bool);

    // Sync to disk on background thread.
    void syncDeleteAllOrigins();
    void syncDeleteOrigin(const String& originIdentifier);
    void syncSetOriginDetails(const String& originIdentifier, const String& databaseFile);
    void syncImportOriginIdentifiers();

    // Mutex for m_database and m_storageDirectoryPath.
    Mutex m_databaseMutex;
    SQLiteDatabase m_database;
    String m_storageDirectoryPath;

    Mutex m_clientMutex;
    StorageTrackerClient* m_client;

    // Guard for m_originSet and m_originsBeingDeleted.
    Mutex m_originSetMutex;
    typedef HashSet<String> OriginSet;
    OriginSet m_originSet;
    OriginSet m_originsBeingDeleted;

    OwnPtr<StorageThread> m_thread;
    
    bool m_isActive;
    bool m_needsInitialization;
    double m_StorageDatabaseIdleInterval;
};
    
} // namespace WebCore

#endif // StorageTracker_h
