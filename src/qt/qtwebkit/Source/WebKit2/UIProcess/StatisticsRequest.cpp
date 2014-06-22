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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StatisticsRequest.h"

#include "ImmutableArray.h"
#include "MutableDictionary.h"
#include <wtf/Threading.h>

namespace WebKit {

StatisticsRequest::StatisticsRequest(PassRefPtr<DictionaryCallback> callback)
    : m_callback(callback)
{
}

StatisticsRequest::~StatisticsRequest()
{
    if (m_callback)
        m_callback->invalidate();
}

uint64_t StatisticsRequest::addOutstandingRequest()
{
    static int64_t uniqueRequestID;

#if HAVE(ATOMICS_64BIT)
    uint64_t requestID = atomicIncrement(&uniqueRequestID);
#else
    static Mutex uniqueRequestMutex;
    uniqueRequestMutex.lock();
    uint64_t requestID = ++uniqueRequestID;
    uniqueRequestMutex.unlock();
#endif

    m_outstandingRequests.add(requestID);
    return requestID;
}

static void addToDictionaryFromHashMap(MutableDictionary* dictionary, const HashMap<String, uint64_t>& map)
{
    HashMap<String, uint64_t>::const_iterator end = map.end();
    for (HashMap<String, uint64_t>::const_iterator it = map.begin(); it != end; ++it)
        dictionary->set(it->key, RefPtr<WebUInt64>(WebUInt64::create(it->value)).get());
}

static PassRefPtr<MutableDictionary> createDictionaryFromHashMap(const HashMap<String, uint64_t>& map)
{
    RefPtr<MutableDictionary> result = MutableDictionary::create();
    addToDictionaryFromHashMap(result.get(), map);
    return result;
}

void StatisticsRequest::completedRequest(uint64_t requestID, const StatisticsData& data)
{
    ASSERT(m_outstandingRequests.contains(requestID));
    m_outstandingRequests.remove(requestID);

    if (!m_responseDictionary)
        m_responseDictionary = MutableDictionary::create();
    
    // FIXME (Multi-WebProcess) <rdar://problem/13200059>: This code overwrites any previous response data received.
    // When getting responses from multiple WebProcesses we need to combine items instead of clobbering them.

    addToDictionaryFromHashMap(m_responseDictionary.get(), data.statisticsNumbers);

    if (!data.javaScriptProtectedObjectTypeCounts.isEmpty())
        m_responseDictionary->set("JavaScriptProtectedObjectTypeCounts", createDictionaryFromHashMap(data.javaScriptProtectedObjectTypeCounts).get());
    if (!data.javaScriptObjectTypeCounts.isEmpty())
        m_responseDictionary->set("JavaScriptObjectTypeCounts", createDictionaryFromHashMap(data.javaScriptObjectTypeCounts).get());
    
    size_t cacheStatisticsCount = data.webCoreCacheStatistics.size();
    if (cacheStatisticsCount) {
        Vector<RefPtr<APIObject> > cacheStatisticsVector(cacheStatisticsCount);
        for (size_t i = 0; i < cacheStatisticsCount; ++i)
            cacheStatisticsVector[i] = createDictionaryFromHashMap(data.webCoreCacheStatistics[i]);
        m_responseDictionary->set("WebCoreCacheStatistics", ImmutableArray::adopt(cacheStatisticsVector).get());
    }

    if (m_outstandingRequests.isEmpty()) {
        m_callback->performCallbackWithReturnValue(m_responseDictionary.get());
        m_callback = 0;
    }
}

} // namespace WebKit
