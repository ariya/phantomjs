/*
 * Copyright (C) 2007, 2008, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Database_h
#define Database_h

#if ENABLE(SQL_DATABASE)

#include "DatabaseBackend.h"
#include "DatabaseBase.h"
#include "DatabaseBasicTypes.h"
#include "DatabaseError.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class ChangeVersionData;
class DatabaseCallback;
class DatabaseContext;
class SecurityOrigin;
class SQLTransaction;
class SQLTransactionBackend;
class SQLTransactionCallback;
class SQLTransactionErrorCallback;
class VoidCallback;

class Database : public DatabaseBase, public DatabaseBackend {
public:
    virtual ~Database();

    // Direct support for the DOM API
    virtual String version() const;
    void changeVersion(const String& oldVersion, const String& newVersion, PassRefPtr<SQLTransactionCallback>,
                       PassRefPtr<SQLTransactionErrorCallback>, PassRefPtr<VoidCallback> successCallback);
    void transaction(PassRefPtr<SQLTransactionCallback>, PassRefPtr<SQLTransactionErrorCallback>, PassRefPtr<VoidCallback> successCallback);
    void readTransaction(PassRefPtr<SQLTransactionCallback>, PassRefPtr<SQLTransactionErrorCallback>, PassRefPtr<VoidCallback> successCallback);

    // Internal engine support
    static Database* from(DatabaseBackend*);
    DatabaseContext* databaseContext() const { return m_databaseContext.get(); }

    Vector<String> tableNames();

    virtual SecurityOrigin* securityOrigin() const;

    virtual void markAsDeletedAndClose();
    bool deleted() const { return m_deleted; }

    virtual void closeImmediately();

    void scheduleTransactionCallback(SQLTransaction*);

private:
    Database(PassRefPtr<DatabaseBackendContext>, const String& name,
        const String& expectedVersion, const String& displayName, unsigned long estimatedSize);
    PassRefPtr<DatabaseBackend> backend();
    static PassRefPtr<Database> create(ScriptExecutionContext*, PassRefPtr<DatabaseBackendBase>);

    void runTransaction(PassRefPtr<SQLTransactionCallback>, PassRefPtr<SQLTransactionErrorCallback>,
        PassRefPtr<VoidCallback> successCallback, bool readOnly, const ChangeVersionData* = 0);

    Vector<String> performGetTableNames();

    RefPtr<SecurityOrigin> m_databaseThreadSecurityOrigin;
    RefPtr<DatabaseContext> m_databaseContext;

    bool m_deleted;

    friend class DatabaseManager;
    friend class DatabaseServer; // FIXME: remove this when the backend has been split out.
    friend class DatabaseBackend; // FIXME: remove this when the backend has been split out.
    friend class SQLStatement;
    friend class SQLTransaction;
};

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // Database_h
