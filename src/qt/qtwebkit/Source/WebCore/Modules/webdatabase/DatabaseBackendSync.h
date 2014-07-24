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

#ifndef DatabaseBackendSync_h
#define DatabaseBackendSync_h

#if ENABLE(SQL_DATABASE)

#include "DatabaseBackendBase.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class DatabaseServer;

// FIXME: This implementation of DatabaseBackendSync is only a place holder
// for the split out of the DatabaseSync backend to be done later. This
// place holder is needed to allow other code that need to reference the
// DatabaseBackendSync to do so before the proper backend split is
// available. This should be replaced with the actual implementation later.

class DatabaseBackendSync : public DatabaseBackendBase {
public:
    virtual ~DatabaseBackendSync();

    virtual bool openAndVerifyVersion(bool setVersionInNewDatabase, DatabaseError&, String& errorMessage);

protected:
    DatabaseBackendSync(PassRefPtr<DatabaseBackendContext>, const String& name, const String& expectedVersion, const String& displayName, unsigned long estimatedSize);

    friend class DatabaseServer;
};

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // DatabaseBackendSync_h
