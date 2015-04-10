/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#if ENABLE(GESTURE_EVENTS)

#include "Arguments.h"
#include "WebCoreArgumentCoders.h"

using namespace WebCore;

namespace WebKit {    

WebGestureEvent::WebGestureEvent(Type type, const IntPoint& position, const IntPoint& globalPosition, Modifiers modifiers, double timestamp)
    : WebEvent(type, modifiers, timestamp)
    , m_position(position)
    , m_globalPosition(globalPosition)
{
    ASSERT(isGestureEventType(type));
}

WebGestureEvent::WebGestureEvent(Type type, const IntPoint& position, const IntPoint& globalPosition, Modifiers modifiers, double timestamp, const IntSize& area, const FloatPoint& delta)
    : WebEvent(type, modifiers, timestamp)
    , m_position(position)
    , m_globalPosition(globalPosition)
    , m_area(area)
    , m_delta(delta)
{
    ASSERT(isGestureEventType(type));
}

void WebGestureEvent::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    WebEvent::encode(encoder);

    encoder << m_position;
    encoder << m_globalPosition;
    encoder << m_area;
    encoder << m_delta;
}

bool WebGestureEvent::decode(CoreIPC::ArgumentDecoder& decoder, WebGestureEvent& t)
{
    if (!WebEvent::decode(decoder, t))
        return false;
    if (!decoder.decode(t.m_position))
        return false;
    if (!decoder.decode(t.m_globalPosition))
        return false;
    if (!decoder.decode(t.m_area))
        return false;
    if (!decoder.decode(t.m_delta))
        return false;
    return true;
}

bool WebGestureEvent::isGestureEventType(Type type)
{
    return type == GestureScrollBegin || type == GestureScrollEnd || type == GestureSingleTap;
}

} // namespace WebKit

#endif // ENABLE(GESTURE_EVENTS)
