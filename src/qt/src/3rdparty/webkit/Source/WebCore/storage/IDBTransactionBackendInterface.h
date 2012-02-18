/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef IDBTransactionBackendInterface_h
#define IDBTransactionBackendInterface_h

#include "ExceptionCode.h"
#include "IDBCallbacks.h"
#include "PlatformString.h"
#include "ScriptExecutionContext.h"
#include <wtf/Threading.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class IDBObjectStoreBackendInterface;
class IDBTransactionCallbacks;

// This class is shared by IDBTransaction (async) and IDBTransactionSync (sync).
// This is implemented by IDBTransactionBackendImpl and optionally others (in order to proxy
// calls across process barriers). All calls to these classes should be non-blocking and
// trigger work on a background thread if necessary.
class IDBTransactionBackendInterface : public ThreadSafeRefCounted<IDBTransactionBackendInterface> {
public:
    virtual ~IDBTransactionBackendInterface() { }

    virtual PassRefPtr<IDBObjectStoreBackendInterface> objectStore(const String& name, ExceptionCode&) = 0;
    virtual unsigned short mode() const = 0;
    virtual bool scheduleTask(PassOwnPtr<ScriptExecutionContext::Task> task, PassOwnPtr<ScriptExecutionContext::Task> abortTask = nullptr) = 0;
    virtual void didCompleteTaskEvents() = 0;
    virtual void abort() = 0;
    virtual void setCallbacks(IDBTransactionCallbacks*) = 0;
};

} // namespace WebCore

#endif

#endif // IDBTransactionBackendInterface_h
