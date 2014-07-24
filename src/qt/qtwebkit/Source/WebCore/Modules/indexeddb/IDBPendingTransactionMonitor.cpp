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

#include "config.h"
#include "IDBPendingTransactionMonitor.h"
#include "IDBTransaction.h"
#include <wtf/ThreadSpecific.h>

using WTF::ThreadSpecific;

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

typedef Vector<RefPtr<IDBTransaction> > TransactionList;
static ThreadSpecific<TransactionList>& transactions()
{
    // FIXME: Move the Vector to ScriptExecutionContext to avoid dealing with
    // thread-local storage.
    AtomicallyInitializedStatic(ThreadSpecific<TransactionList>*, transactions = new ThreadSpecific<TransactionList>);
    return *transactions;
}

void IDBPendingTransactionMonitor::addNewTransaction(PassRefPtr<IDBTransaction> transaction)
{
    transactions()->append(transaction);
}

void IDBPendingTransactionMonitor::deactivateNewTransactions()
{
    ThreadSpecific<TransactionList>& list = transactions();
    for (size_t i = 0; i < list->size(); ++i) {
        RefPtr<IDBTransaction> transaction = list->at(i);
        transaction->setActive(false);
    }
    // FIXME: Exercise this call to clear() in a layout test.
    list->clear();
}

};
#endif // ENABLE(INDEXED_DATABASE)
