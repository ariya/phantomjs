/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SQLTransactionBackendSync_h
#define SQLTransactionBackendSync_h

#if ENABLE(SQL_DATABASE)

#include "DatabaseBasicTypes.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DatabaseSync;
class SQLResultSet;
class SQLTransactionClient;
class SQLTransactionSyncCallback;
class SQLValue;
class SQLiteTransaction;

// Instances of this class should be created and used only on the worker's context thread.
class SQLTransactionBackendSync : public RefCounted<SQLTransactionBackendSync> {
public:
    ~SQLTransactionBackendSync();

    PassRefPtr<SQLResultSet> executeSQL(const String& sqlStatement, const Vector<SQLValue>& arguments, ExceptionCode&);

    DatabaseSync* database() { return m_database.get(); }
    bool isReadOnly() const { return m_readOnly; }

    ExceptionCode begin();
    ExceptionCode execute();
    ExceptionCode commit();
    void rollback();

private:
    SQLTransactionBackendSync(DatabaseSync*, PassRefPtr<SQLTransactionSyncCallback>, bool readOnly);

    RefPtr<DatabaseSync> m_database;
    RefPtr<SQLTransactionSyncCallback> m_callback;
    bool m_readOnly;
    bool m_hasVersionMismatch;

    bool m_modifiedDatabase;
    OwnPtr<SQLTransactionClient> m_transactionClient;
    OwnPtr<SQLiteTransaction> m_sqliteTransaction;

    friend class SQLTransactionSync; // FIXME: Remove this once the front-end has been properly isolated.
};

} // namespace WebCore

#endif

#endif // SQLTransactionBackendSync_h
