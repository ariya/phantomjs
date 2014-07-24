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
#include "RTCDataChannelEvent.h"

#if ENABLE(MEDIA_STREAM)

#include "EventNames.h"
#include "RTCDataChannel.h"

namespace WebCore {

PassRefPtr<RTCDataChannelEvent> RTCDataChannelEvent::create()
{
    return adoptRef(new RTCDataChannelEvent);
}

PassRefPtr<RTCDataChannelEvent> RTCDataChannelEvent::create(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<RTCDataChannel> channel)
{
    return adoptRef(new RTCDataChannelEvent(type, canBubble, cancelable, channel));
}


RTCDataChannelEvent::RTCDataChannelEvent()
{
}

RTCDataChannelEvent::RTCDataChannelEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<RTCDataChannel> channel)
    : Event(type, canBubble, cancelable)
    , m_channel(channel)
{
}

RTCDataChannelEvent::~RTCDataChannelEvent()
{
}

RTCDataChannel* RTCDataChannelEvent::channel() const
{
    return m_channel.get();
}

const AtomicString& RTCDataChannelEvent::interfaceName() const
{
    return eventNames().interfaceForRTCDataChannelEvent;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

