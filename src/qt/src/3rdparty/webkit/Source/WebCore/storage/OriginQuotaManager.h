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

#ifndef OriginQuotaManager_h
#define OriginQuotaManager_h

#if ENABLE(DATABASE)

#include "SecurityOriginHash.h"
#include <wtf/HashMap.h>
#include <wtf/Threading.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class AbstractDatabase;
class OriginUsageRecord;

class OriginQuotaManager {
    WTF_MAKE_NONCOPYABLE(OriginQuotaManager); WTF_MAKE_FAST_ALLOCATED;
public:
    OriginQuotaManager();

    bool tryLock();
    void lock();
    void unlock();

    void trackOrigin(PassRefPtr<SecurityOrigin>);
    bool tracksOrigin(SecurityOrigin*) const;
    void addDatabase(SecurityOrigin*, const String& databaseIdentifier, const String& fullPath);
    void removeDatabase(SecurityOrigin*, const String& databaseIdentifier);
    void removeOrigin(SecurityOrigin*);

    void markDatabase(AbstractDatabase*); // Mark dirtiness of a specific database.
    unsigned long long diskUsage(SecurityOrigin*) const;

private:
    mutable Mutex m_usageRecordGuard;
#ifndef NDEBUG
    bool m_usageRecordGuardLocked;
#endif

    typedef HashMap<RefPtr<SecurityOrigin>, OriginUsageRecord*, SecurityOriginHash> OriginUsageMap;
    OriginUsageMap m_usageMap;
};

} // namespace WebCore

#endif // ENABLE(DATABASE)

#endif // OriginQuotaManager_h
