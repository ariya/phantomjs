/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "OriginQuotaManager.h"

#if ENABLE(DATABASE)

#include "AbstractDatabase.h"
#include "OriginUsageRecord.h"

namespace WebCore {

OriginQuotaManager::OriginQuotaManager()
#ifndef NDEBUG
    : m_usageRecordGuardLocked(false)
#endif
{
}

bool OriginQuotaManager::tryLock()
{
    bool locked = m_usageRecordGuard.tryLock();
#ifndef NDEBUG
    if (locked)
        m_usageRecordGuardLocked = true;
    else
        ASSERT(m_usageRecordGuardLocked);
#endif
    return locked;
}

void OriginQuotaManager::lock()
{
    m_usageRecordGuard.lock();
#ifndef NDEBUG
    m_usageRecordGuardLocked = true;
#endif
}

void OriginQuotaManager::unlock()
{
#ifndef NDEBUG
    m_usageRecordGuardLocked = false;
#endif
    m_usageRecordGuard.unlock();
}

void OriginQuotaManager::trackOrigin(PassRefPtr<SecurityOrigin> origin)
{
    ASSERT(m_usageRecordGuardLocked);
    ASSERT(!m_usageMap.contains(origin.get()));

    m_usageMap.set(origin->threadsafeCopy(), new OriginUsageRecord);
}

bool OriginQuotaManager::tracksOrigin(SecurityOrigin* origin) const
{
    ASSERT(m_usageRecordGuardLocked);
    return m_usageMap.contains(origin);
}

void OriginQuotaManager::addDatabase(SecurityOrigin* origin, const String& databaseIdentifier, const String& fullPath)
{
    ASSERT(m_usageRecordGuardLocked);

    OriginUsageRecord* usageRecord = m_usageMap.get(origin);
    ASSERT(usageRecord);

    usageRecord->addDatabase(databaseIdentifier.threadsafeCopy(), fullPath.threadsafeCopy());
}

void OriginQuotaManager::removeDatabase(SecurityOrigin* origin, const String& databaseIdentifier)
{
    ASSERT(m_usageRecordGuardLocked);

    if (OriginUsageRecord* usageRecord = m_usageMap.get(origin))
        usageRecord->removeDatabase(databaseIdentifier);
}

void OriginQuotaManager::removeOrigin(SecurityOrigin* origin)
{
    ASSERT(m_usageRecordGuardLocked);

    if (OriginUsageRecord* usageRecord = m_usageMap.get(origin)) {
        m_usageMap.remove(origin);
        delete usageRecord;
    }
}

void OriginQuotaManager::markDatabase(AbstractDatabase* database)
{
    ASSERT(database);
    ASSERT(m_usageRecordGuardLocked);
    OriginUsageRecord* usageRecord = m_usageMap.get(database->securityOrigin());
    ASSERT(usageRecord);

    usageRecord->markDatabase(database->stringIdentifier());
}

unsigned long long OriginQuotaManager::diskUsage(SecurityOrigin* origin) const
{
    ASSERT(m_usageRecordGuardLocked);

    OriginUsageRecord* usageRecord = m_usageMap.get(origin);
    ASSERT(usageRecord);

    return usageRecord->diskUsage();
}

}

#endif // ENABLE(DATABASE)
