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

#ifndef IDBPendingTransactionMonitor_h
#define IDBPendingTransactionMonitor_h

#if ENABLE(INDEXED_DATABASE)

#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebCore {

class IDBTransactionBackendInterface;

// This class keeps track of the transactions created during the current
// Javascript execution context. A transaction is 'pending' if no asynchronous
// operation is currently queued for it (e.g. an IDBObjectStore::put() or similar).
// All pending transactions are aborted as soon as execution returns from
// the script engine.
//
// FIXME: move the vector of transactions to TLS. Keeping it static
// will not work once we add support for workers. Another possible
// solution is to keep the vector in the ScriptExecutionContext.
class IDBPendingTransactionMonitor {
    WTF_MAKE_NONCOPYABLE(IDBPendingTransactionMonitor);
public:
    static void addPendingTransaction(IDBTransactionBackendInterface*);
    static void removePendingTransaction(IDBTransactionBackendInterface*);
    static void abortPendingTransactions();

private:
    IDBPendingTransactionMonitor();

    static Vector<IDBTransactionBackendInterface*>* m_transactions;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBPendingTransactionMonitor_h
