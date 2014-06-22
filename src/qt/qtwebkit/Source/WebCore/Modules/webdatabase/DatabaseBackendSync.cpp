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

#include "config.h"
#include "DatabaseBackendSync.h"

#if ENABLE(SQL_DATABASE)

#include "DatabaseBackendContext.h"
#include "DatabaseTracker.h"

namespace WebCore {

DatabaseBackendSync::DatabaseBackendSync(PassRefPtr<DatabaseBackendContext> databaseContext, const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize)
    : DatabaseBackendBase(databaseContext, name, expectedVersion, displayName, estimatedSize, DatabaseType::Sync)
{
}

DatabaseBackendSync::~DatabaseBackendSync()
{
    // SQLite is "multi-thread safe", but each database handle can only be used
    // on a single thread at a time.
    //
    // For DatabaseBackendSync, we open the SQLite database on the script context
    // thread. And hence we should also close it on that same thread. This means
    // that the SQLite database need to be closed here in the destructor.

    ASSERT(m_databaseContext->isContextThread());
    if (opened())
        closeDatabase();
}

bool DatabaseBackendSync::openAndVerifyVersion(bool setVersionInNewDatabase, DatabaseError& error, String& errorMessage)
{
    return performOpenAndVerify(setVersionInNewDatabase, error, errorMessage);
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
