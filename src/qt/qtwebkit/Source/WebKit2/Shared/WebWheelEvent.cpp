/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebEvent.h"

#include "Arguments.h"
#include "WebCoreArgumentCoders.h"

using namespace WebCore;

namespace WebKit {    

WebWheelEvent::WebWheelEvent(Type type, const IntPoint& position, const IntPoint& globalPosition, const FloatSize& delta, const FloatSize& wheelTicks, Granularity granularity, Modifiers modifiers, double timestamp)
    : WebEvent(type, modifiers, timestamp)
    , m_position(position)
    , m_globalPosition(globalPosition)
    , m_delta(delta)
    , m_wheelTicks(wheelTicks)
    , m_granularity(granularity)
    , m_directionInvertedFromDevice(false)
#if PLATFORM(MAC)
    , m_phase(PhaseNone)
    , m_hasPreciseScrollingDeltas(false)
    , m_scrollCount(0)
#endif
{
    ASSERT(isWheelEventType(type));
}

#if PLATFORM(MAC)
WebWheelEvent::WebWheelEvent(Type type, const IntPoint& position, const IntPoint& globalPosition, const FloatSize& delta, const FloatSize& wheelTicks, Granularity granularity, bool directionInvertedFromDevice, Phase phase, Phase momentumPhase, bool hasPreciseScrollingDeltas, uint32_t scrollCount, const WebCore::FloatSize& unacceleratedScrollingDelta, Modifiers modifiers, double timestamp)
    : WebEvent(type, modifiers, timestamp)
    , m_position(position)
    , m_globalPosition(globalPosition)
    , m_delta(delta)
    , m_wheelTicks(wheelTicks)
    , m_granularity(granularity)
    , m_directionInvertedFromDevice(directionInvertedFromDevice)
    , m_phase(phase)
    , m_momentumPhase(momentumPhase)
    , m_hasPreciseScrollingDeltas(hasPreciseScrollingDeltas)
    , m_scrollCount(scrollCount)
    , m_unacceleratedScrollingDelta(unacceleratedScrollingDelta)
{
    ASSERT(isWheelEventType(type));
}
#endif

void WebWheelEvent::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    WebEvent::encode(encoder);

    encoder << m_position;
    encoder << m_globalPosition;
    encoder << m_delta;
    encoder << m_wheelTicks;
    encoder << m_granularity;
    encoder << m_directionInvertedFromDevice;
#if PLATFORM(MAC)
    encoder << m_phase;
    encoder << m_momentumPhase;
    encoder << m_hasPreciseScrollingDeltas;
    encoder << m_scrollCount;
    encoder << m_unacceleratedScrollingDelta;
#endif
}

bool WebWheelEvent::decode(CoreIPC::ArgumentDecoder& decoder, WebWheelEvent& t)
{
    if (!WebEvent::decode(decoder, t))
        return false;
    if (!decoder.decode(t.m_position))
        return false;
    if (!decoder.decode(t.m_globalPosition))
        return false;
    if (!decoder.decode(t.m_delta))
        return false;
    if (!decoder.decode(t.m_wheelTicks))
        return false;
    if (!decoder.decode(t.m_granularity))
        return false;
    if (!decoder.decode(t.m_directionInvertedFromDevice))
        return false;
#if PLATFORM(MAC)
    if (!decoder.decode(t.m_phase))
        return false;
    if (!decoder.decode(t.m_momentumPhase))
        return false;
    if (!decoder.decode(t.m_hasPreciseScrollingDeltas))
        return false;
    if (!decoder.decode(t.m_scrollCount))
        return false;
    if (!decoder.decode(t.m_unacceleratedScrollingDelta))
        return false;
#endif
    return true;
}

bool WebWheelEvent::isWheelEventType(Type type)
{
    return type == Wheel;
}

} // namespace WebKit
