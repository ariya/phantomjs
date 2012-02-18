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
#include "IDBTransactionCoordinator.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBDatabaseBackendImpl.h"
#include "IDBObjectStoreBackendInterface.h"
#include "IDBTransactionBackendImpl.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

PassRefPtr<IDBTransactionCoordinator> IDBTransactionCoordinator::create()
{
    return adoptRef(new IDBTransactionCoordinator());
}

IDBTransactionCoordinator::IDBTransactionCoordinator() 
{
}

IDBTransactionCoordinator::~IDBTransactionCoordinator()
{
}

void IDBTransactionCoordinator::didCreateTransaction(IDBTransactionBackendImpl* transaction)
{
    ASSERT(!m_transactions.contains(transaction));
    m_transactions.add(transaction, transaction);
}

void IDBTransactionCoordinator::didStartTransaction(IDBTransactionBackendImpl* transaction)
{
    ASSERT(m_transactions.contains(transaction));

    m_startedTransactions.add(transaction);
    processStartedTransactions();
}

void IDBTransactionCoordinator::didFinishTransaction(IDBTransactionBackendImpl* transaction)
{
    ASSERT(m_transactions.contains(transaction));

    if (m_startedTransactions.contains(transaction)) {
        ASSERT(!m_runningTransactions.contains(transaction));
        m_startedTransactions.remove(transaction);
    } else if (m_runningTransactions.contains(transaction))
        m_runningTransactions.remove(transaction);

    m_transactions.remove(transaction);

    processStartedTransactions();
}

#ifndef NDEBUG
// Verifies internal consistiency while returning whether anything is found.
bool IDBTransactionCoordinator::isActive(IDBTransactionBackendImpl* transaction)
{
    bool found = false;
    if (m_startedTransactions.contains(transaction))
        found = true;
    if (m_runningTransactions.contains(transaction)) {
        ASSERT(!found);
        found = true;
    }
    ASSERT(found == m_transactions.contains(transaction));
    return found;
}
#endif

void IDBTransactionCoordinator::processStartedTransactions()
{
    // FIXME: For now, we only allow one transaction to run at a time.
    if (!m_runningTransactions.isEmpty())
        return;

    if (m_startedTransactions.isEmpty())
        return;

    IDBTransactionBackendImpl* transaction = *m_startedTransactions.begin();
    m_startedTransactions.remove(transaction);
    m_runningTransactions.add(transaction);
    transaction->run();
}

};

#endif // ENABLE(INDEXED_DATABASE)
