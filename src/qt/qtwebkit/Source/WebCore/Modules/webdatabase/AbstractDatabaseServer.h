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

#ifndef AbstractDatabaseServer_h
#define AbstractDatabaseServer_h

#if ENABLE(SQL_DATABASE)

#include "DatabaseBasicTypes.h"
#include "DatabaseDetails.h"
#include "DatabaseError.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DatabaseBackendBase;
class DatabaseBackendContext;
class DatabaseManagerClient;
class SecurityOrigin;

class AbstractDatabaseServer {
public:
    virtual void initialize(const String& databasePath) = 0;

    virtual void setClient(DatabaseManagerClient*) = 0;
    virtual String databaseDirectoryPath() const = 0;
    virtual void setDatabaseDirectoryPath(const String&) = 0;

    virtual String fullPathForDatabase(SecurityOrigin*, const String& name, bool createIfDoesNotExist = true) = 0;

    enum OpenAttempt {
        FirstTryToOpenDatabase,
        RetryOpenDatabase
    };

    virtual PassRefPtr<DatabaseBackendBase> openDatabase(RefPtr<DatabaseBackendContext>&, DatabaseType,
        const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize,
        bool setVersionInNewDatabase, DatabaseError&, String& errorMessage, OpenAttempt = FirstTryToOpenDatabase) = 0;

    virtual bool hasEntryForOrigin(SecurityOrigin*) = 0;
    virtual void origins(Vector<RefPtr<SecurityOrigin> >& result) = 0;
    virtual bool databaseNamesForOrigin(SecurityOrigin*, Vector<String>& result) = 0;
    virtual DatabaseDetails detailsForNameAndOrigin(const String&, SecurityOrigin*) = 0;

    virtual unsigned long long usageForOrigin(SecurityOrigin*) = 0;
    virtual unsigned long long quotaForOrigin(SecurityOrigin*) = 0;

    virtual void setQuota(SecurityOrigin*, unsigned long long) = 0;

    virtual void deleteAllDatabases() = 0;
    virtual bool deleteOrigin(SecurityOrigin*) = 0;
    virtual bool deleteDatabase(SecurityOrigin*, const String& name) = 0;

    virtual void interruptAllDatabasesForContext(const DatabaseBackendContext*) = 0;

protected:
    AbstractDatabaseServer() { }
    virtual ~AbstractDatabaseServer() { }
};

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // AbstractDatabaseServer_h
