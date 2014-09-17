/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef DatabaseSync_h
#define DatabaseSync_h

#if ENABLE(DATABASE)
#include "AbstractDatabase.h"
#include "ExceptionCode.h"
#include "PlatformString.h"
#include <wtf/Forward.h>
#ifndef NDEBUG
#include "SecurityOrigin.h"
#endif

namespace WebCore {

class DatabaseCallback;
class SQLTransactionSync;
class SQLTransactionSyncCallback;
class ScriptExecutionContext;
class SecurityOrigin;

// Instances of this class should be created and used only on the worker's context thread.
class DatabaseSync : public AbstractDatabase {
public:
    virtual ~DatabaseSync();

    static PassRefPtr<DatabaseSync> openDatabaseSync(ScriptExecutionContext*, const String& name, const String& expectedVersion,
                                                     const String& displayName, unsigned long estimatedSize, PassRefPtr<DatabaseCallback>, ExceptionCode&);
    void changeVersion(const String& oldVersion, const String& newVersion, PassRefPtr<SQLTransactionSyncCallback>, ExceptionCode&);
    void transaction(PassRefPtr<SQLTransactionSyncCallback>, ExceptionCode&);
    void readTransaction(PassRefPtr<SQLTransactionSyncCallback>, ExceptionCode&);

    virtual void markAsDeletedAndClose();
    virtual void closeImmediately();

private:
    DatabaseSync(ScriptExecutionContext*, const String& name, const String& expectedVersion,
                 const String& displayName, unsigned long estimatedSize);
    void runTransaction(PassRefPtr<SQLTransactionSyncCallback>, bool readOnly, ExceptionCode&);
};

} // namespace WebCore

#endif // ENABLE(DATABASE)

#endif // DatabaseSync_h
