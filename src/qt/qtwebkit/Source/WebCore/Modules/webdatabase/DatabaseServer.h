/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef DatabaseServer_h
#define DatabaseServer_h

#if ENABLE(SQL_DATABASE)

#include "AbstractDatabaseServer.h"

namespace WebCore {

class DatabaseServer: public AbstractDatabaseServer {
public:
    DatabaseServer() { };
    virtual ~DatabaseServer() { }

    virtual void initialize(const String& databasePath);

    virtual void setClient(DatabaseManagerClient*);
    virtual String databaseDirectoryPath() const;
    virtual void setDatabaseDirectoryPath(const String&);

    virtual String fullPathForDatabase(SecurityOrigin*, const String& name, bool createIfDoesNotExist);

    virtual PassRefPtr<DatabaseBackendBase> openDatabase(RefPtr<DatabaseBackendContext>&, DatabaseType,
        const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize,
        bool setVersionInNewDatabase, DatabaseError&, String& errorMessage, OpenAttempt);

    virtual bool hasEntryForOrigin(SecurityOrigin*);
    virtual void origins(Vector<RefPtr<SecurityOrigin> >& result);
    virtual bool databaseNamesForOrigin(SecurityOrigin*, Vector<String>& result);
    virtual DatabaseDetails detailsForNameAndOrigin(const String&, SecurityOrigin*);

    virtual unsigned long long usageForOrigin(SecurityOrigin*);
    virtual unsigned long long quotaForOrigin(SecurityOrigin*);

    virtual void setQuota(SecurityOrigin*, unsigned long long);

    virtual void deleteAllDatabases();
    virtual bool deleteOrigin(SecurityOrigin*);
    virtual bool deleteDatabase(SecurityOrigin*, const String& name);

    virtual void interruptAllDatabasesForContext(const DatabaseBackendContext*);

protected:
    virtual PassRefPtr<DatabaseBackendBase> createDatabase(RefPtr<DatabaseBackendContext>&, DatabaseType,
        const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize,
        bool setVersionInNewDatabase, DatabaseError&, String& errorMessage);
};

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // DatabaseServer_h
