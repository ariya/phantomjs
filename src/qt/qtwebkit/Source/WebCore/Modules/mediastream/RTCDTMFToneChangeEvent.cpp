/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RTCDTMFToneChangeEvent.h"

#if ENABLE(MEDIA_STREAM)

#include "EventNames.h"

namespace WebCore {

PassRefPtr<RTCDTMFToneChangeEvent> RTCDTMFToneChangeEvent::create()
{
    return adoptRef(new RTCDTMFToneChangeEvent);
}

PassRefPtr<RTCDTMFToneChangeEvent> RTCDTMFToneChangeEvent::create(const String& tone)
{
    return adoptRef(new RTCDTMFToneChangeEvent(tone));
}

PassRefPtr<RTCDTMFToneChangeEvent> RTCDTMFToneChangeEvent::create(const AtomicString& type, const RTCDTMFToneChangeEventInit& initializer)
{
    ASSERT(type == eventNames().tonechangeEvent);
    return adoptRef(new RTCDTMFToneChangeEvent(initializer));
}

RTCDTMFToneChangeEvent::RTCDTMFToneChangeEvent()
{
}

RTCDTMFToneChangeEvent::RTCDTMFToneChangeEvent(const String& tone)
    : Event(eventNames().tonechangeEvent, false, false)
    , m_tone(tone)
{
}

RTCDTMFToneChangeEvent::RTCDTMFToneChangeEvent(const RTCDTMFToneChangeEventInit& initializer)
    : Event(eventNames().tonechangeEvent, initializer)
    , m_tone(initializer.tone)
{
}

RTCDTMFToneChangeEvent::~RTCDTMFToneChangeEvent()
{
}

const String& RTCDTMFToneChangeEvent::tone() const
{
    return m_tone;
}

const AtomicString& RTCDTMFToneChangeEvent::interfaceName() const
{
    return eventNames().interfaceForRTCDTMFToneChangeEvent;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

