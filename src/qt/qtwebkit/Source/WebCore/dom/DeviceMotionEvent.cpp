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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DeviceMotionEvent.h"

#include "DeviceMotionData.h"
#include "EventNames.h"

namespace WebCore {

DeviceMotionEvent::~DeviceMotionEvent()
{
}

DeviceMotionEvent::DeviceMotionEvent()
    : m_deviceMotionData(DeviceMotionData::create())
{
}

DeviceMotionEvent::DeviceMotionEvent(const AtomicString& eventType, DeviceMotionData* deviceMotionData)
    : Event(eventType, false, false) // Can't bubble, not cancelable
    , m_deviceMotionData(deviceMotionData)
{
}

void DeviceMotionEvent::initDeviceMotionEvent(const AtomicString& type, bool bubbles, bool cancelable, DeviceMotionData* deviceMotionData)
{
    if (dispatched())
        return;

    initEvent(type, bubbles, cancelable);
    m_deviceMotionData = deviceMotionData;
}

const AtomicString& DeviceMotionEvent::interfaceName() const
{
#if ENABLE(DEVICE_ORIENTATION)
    return eventNames().interfaceForDeviceMotionEvent;
#else
    // FIXME: ENABLE(DEVICE_ORIENTATION) seems to be in a strange state where
    // it is half-guarded by #ifdefs. DeviceMotionEvent.idl is guarded
    // but DeviceMotionEvent.cpp itself is required by ungarded code.
    return eventNames().interfaceForEvent;
#endif
}

} // namespace WebCore
