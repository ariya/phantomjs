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

#ifndef IDBTransactionCoordinator_h
#define IDBTransactionCoordinator_h

#if ENABLE(INDEXED_DATABASE)

#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class IDBTransactionBackendImpl;
class IDBDatabaseBackendImpl;

// Transactions are executed in the order the were created.
class IDBTransactionCoordinator {
public:
    static PassOwnPtr<IDBTransactionCoordinator> create();
    virtual ~IDBTransactionCoordinator();

    // Called by transactions as they start and finish.
    void didCreateTransaction(IDBTransactionBackendImpl*);
    void didStartTransaction(IDBTransactionBackendImpl*);
    void didFinishTransaction(IDBTransactionBackendImpl*);

#ifndef NDEBUG
    bool isActive(IDBTransactionBackendImpl*);
#endif

private:
    IDBTransactionCoordinator();

    void processStartedTransactions();
    bool canRunTransaction(IDBTransactionBackendImpl*);

    // This is just an efficient way to keep references to all transactions.
    HashMap<IDBTransactionBackendImpl*, RefPtr<IDBTransactionBackendImpl> > m_transactions;
    // Transactions in different states are grouped below.
    ListHashSet<IDBTransactionBackendImpl*> m_queuedTransactions;
    HashSet<IDBTransactionBackendImpl*> m_startedTransactions;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBTransactionCoordinator_h
