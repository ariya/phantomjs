/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "RTCStatsResponse.h"

namespace WebCore {

PassRefPtr<RTCStatsResponse> RTCStatsResponse::create()
{
    return adoptRef(new RTCStatsResponse());
}

RTCStatsResponse::RTCStatsResponse()
{
}

PassRefPtr<RTCStatsReport> RTCStatsResponse::namedItem(const AtomicString& name)
{
    if (m_idmap.find(name) != m_idmap.end())
        return m_result[m_idmap.get(name)];
    return 0;
}

size_t RTCStatsResponse::addReport(String id, String type, double timestamp)
{
    m_result.append(RTCStatsReport::create(id, type, timestamp));
    m_idmap.add(id, m_result.size() - 1);
    return m_result.size() - 1;
}

void RTCStatsResponse::addStatistic(size_t report, String name, String value)
{
    ASSERT_WITH_SECURITY_IMPLICATION(report < m_result.size());
    m_result[report]->addStatistic(name, value);
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
