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

#include "RTCStatsRequestImpl.h"

#include "RTCStatsCallback.h"
#include "RTCStatsRequest.h"
#include "RTCStatsResponse.h"

namespace WebCore {

PassRefPtr<RTCStatsRequestImpl> RTCStatsRequestImpl::create(ScriptExecutionContext* context, PassRefPtr<RTCStatsCallback> callback, PassRefPtr<MediaStreamTrack> selector)
{
    RefPtr<RTCStatsRequestImpl> request = adoptRef(new RTCStatsRequestImpl(context, callback, selector));
    request->suspendIfNeeded();
    return request.release();
}

RTCStatsRequestImpl::RTCStatsRequestImpl(ScriptExecutionContext* context, PassRefPtr<RTCStatsCallback> callback, PassRefPtr<MediaStreamTrack> selector)
    : ActiveDOMObject(context)
    , m_successCallback(callback)
    , m_stream(selector ? selector->component()->stream() : 0)
    , m_component(selector ? selector->component() : 0)
{
}

RTCStatsRequestImpl::~RTCStatsRequestImpl()
{
}

PassRefPtr<RTCStatsResponseBase> RTCStatsRequestImpl::createResponse()
{
    return RTCStatsResponse::create();
}

bool RTCStatsRequestImpl::hasSelector()
{
    return m_stream;
}

MediaStreamDescriptor* RTCStatsRequestImpl::stream()
{
    return m_stream.get();
}

MediaStreamComponent* RTCStatsRequestImpl::component()
{
    return m_component.get();
}

void RTCStatsRequestImpl::requestSucceeded(PassRefPtr<RTCStatsResponseBase> response)
{
    if (!m_successCallback)
        return;
    m_successCallback->handleEvent(static_cast<RTCStatsResponse*>(response.get()));
    clear();
}

void RTCStatsRequestImpl::stop()
{
    clear();
}

void RTCStatsRequestImpl::clear()
{
    m_successCallback.clear();
}


} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
