/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "config.h"
#include "SQLTransactionCoordinator.h"

#if ENABLE(DATABASE)

#include "Database.h"
#include "SQLTransaction.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>

namespace WebCore {

static String getDatabaseIdentifier(SQLTransaction* transaction)
{
    Database* database = transaction->database();
    ASSERT(database);
    return database->stringIdentifier();
}

void SQLTransactionCoordinator::processPendingTransactions(CoordinationInfo& info)
{
    if (info.activeWriteTransaction || info.pendingTransactions.isEmpty())
        return;

    RefPtr<SQLTransaction> firstPendingTransaction = info.pendingTransactions.first();
    if (firstPendingTransaction->isReadOnly()) {
        do {
            firstPendingTransaction = info.pendingTransactions.takeFirst();
            info.activeReadTransactions.add(firstPendingTransaction);
            firstPendingTransaction->lockAcquired();
        } while (!info.pendingTransactions.isEmpty() && info.pendingTransactions.first()->isReadOnly());
    } else if (info.activeReadTransactions.isEmpty()) {
        info.pendingTransactions.removeFirst();
        info.activeWriteTransaction = firstPendingTransaction;
        firstPendingTransaction->lockAcquired();
    }
}

void SQLTransactionCoordinator::acquireLock(SQLTransaction* transaction)
{
    String dbIdentifier = getDatabaseIdentifier(transaction);

    CoordinationInfoMap::iterator coordinationInfoIterator = m_coordinationInfoMap.find(dbIdentifier);
    if (coordinationInfoIterator == m_coordinationInfoMap.end()) {
        // No pending transactions for this DB
        coordinationInfoIterator = m_coordinationInfoMap.add(dbIdentifier, CoordinationInfo()).first;
    }

    CoordinationInfo& info = coordinationInfoIterator->second;
    info.pendingTransactions.append(transaction);
    processPendingTransactions(info);
}

void SQLTransactionCoordinator::releaseLock(SQLTransaction* transaction)
{
    if (m_coordinationInfoMap.isEmpty())
        return;

    String dbIdentifier = getDatabaseIdentifier(transaction);

    CoordinationInfoMap::iterator coordinationInfoIterator = m_coordinationInfoMap.find(dbIdentifier);
    ASSERT(coordinationInfoIterator != m_coordinationInfoMap.end());
    CoordinationInfo& info = coordinationInfoIterator->second;

    if (transaction->isReadOnly()) {
        ASSERT(info.activeReadTransactions.contains(transaction));
        info.activeReadTransactions.remove(transaction);
    } else {
        ASSERT(info.activeWriteTransaction == transaction);
        info.activeWriteTransaction = 0;
    }

    processPendingTransactions(info);
}

void SQLTransactionCoordinator::shutdown()
{
    // Notify all transactions in progress that the database thread is shutting down
    for (CoordinationInfoMap::iterator coordinationInfoIterator = m_coordinationInfoMap.begin();
         coordinationInfoIterator != m_coordinationInfoMap.end(); ++coordinationInfoIterator) {
        CoordinationInfo& info = coordinationInfoIterator->second;
        if (info.activeWriteTransaction)
            info.activeWriteTransaction->notifyDatabaseThreadIsShuttingDown();
        for (HashSet<RefPtr<SQLTransaction> >::iterator activeReadTransactionsIterator =
                     info.activeReadTransactions.begin();
             activeReadTransactionsIterator != info.activeReadTransactions.end();
             ++activeReadTransactionsIterator) {
            (*activeReadTransactionsIterator)->notifyDatabaseThreadIsShuttingDown();
        }
    }

    // Clean up all pending transactions for all databases
    m_coordinationInfoMap.clear();
}

} // namespace WebCore

#endif // ENABLE(DATABASE)
