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

#ifndef DatabaseBackend_h
#define DatabaseBackend_h

#if ENABLE(SQL_DATABASE)

#include "DatabaseBackendBase.h"
#include <wtf/Deque.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ChangeVersionData;
class Database;
class DatabaseServer;
class SQLTransaction;
class SQLTransactionBackend;
class SQLTransactionClient;
class SQLTransactionCoordinator;

// FIXME: This implementation of DatabaseBackend is only a place holder
// for the split out of the Database backend to be done later. This
// place holder is needed to allow other code that need to reference the
// DatabaseBackend to do so before the proper backend split is
// available. This should be replaced with the actual implementation later.

class DatabaseBackend : public DatabaseBackendBase {
public:
    DatabaseBackend(PassRefPtr<DatabaseBackendContext>, const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize);

    virtual bool openAndVerifyVersion(bool setVersionInNewDatabase, DatabaseError&, String& errorMessage);
    void close();

    PassRefPtr<SQLTransactionBackend> runTransaction(PassRefPtr<SQLTransaction>, bool readOnly, const ChangeVersionData*);
    void scheduleTransactionStep(SQLTransactionBackend*);
    void inProgressTransactionCompleted();

    SQLTransactionClient* transactionClient() const;
    SQLTransactionCoordinator* transactionCoordinator() const;

private:
    class DatabaseOpenTask;
    class DatabaseCloseTask;
    class DatabaseTransactionTask;
    class DatabaseTableNamesTask;

    virtual bool performOpenAndVerify(bool setVersionInNewDatabase, DatabaseError&, String& errorMessage);

    void scheduleTransaction();

    Deque<RefPtr<SQLTransactionBackend> > m_transactionQueue;
    Mutex m_transactionInProgressMutex;
    bool m_transactionInProgress;
    bool m_isTransactionQueueEnabled;

    friend class Database;
};

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // DatabaseBackend_h
