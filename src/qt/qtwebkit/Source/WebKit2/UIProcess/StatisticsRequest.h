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

#ifndef StatisticsRequest_h
#define StatisticsRequest_h

#include "GenericCallback.h"
#include "StatisticsData.h"

#include <wtf/HashSet.h>

namespace WebKit {

struct StatisticsData;

typedef GenericCallback<WKDictionaryRef> DictionaryCallback;

enum StatisticsRequestType {
    StatisticsRequestTypeWebContent = 0x00000001,
    StatisticsRequestTypeNetworking = 0x00000002
};

class StatisticsRequest : public RefCounted<StatisticsRequest> {
public:
    static PassRefPtr<StatisticsRequest> create(PassRefPtr<DictionaryCallback> callback)
    {
        return adoptRef(new StatisticsRequest(callback));
    }

    ~StatisticsRequest();

    uint64_t addOutstandingRequest();

    void completedRequest(uint64_t requestID, const StatisticsData&);

private:
    StatisticsRequest(PassRefPtr<DictionaryCallback>);

    HashSet<uint64_t> m_outstandingRequests;
    RefPtr<DictionaryCallback> m_callback;

    RefPtr<MutableDictionary> m_responseDictionary;
};

} // namespace WebKit

#endif // StatisticsRequest_h
