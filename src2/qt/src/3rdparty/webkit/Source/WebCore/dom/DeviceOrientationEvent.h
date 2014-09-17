/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DeviceOrientationEvent_h
#define DeviceOrientationEvent_h

#include "Event.h"

namespace WebCore {

class DeviceOrientation;

class DeviceOrientationEvent : public Event {
public:
    ~DeviceOrientationEvent();
    static PassRefPtr<DeviceOrientationEvent> create()
    {
        return adoptRef(new DeviceOrientationEvent);
    }
    static PassRefPtr<DeviceOrientationEvent> create(const AtomicString& eventType, DeviceOrientation* orientation)
    {
        return adoptRef(new DeviceOrientationEvent(eventType, orientation));
    }

    void initDeviceOrientationEvent(const AtomicString& type, bool bubbles, bool cancelable, DeviceOrientation*);

    virtual bool isDeviceOrientationEvent() const { return true; }

    DeviceOrientation* orientation() const { return m_orientation.get(); }

private:
    DeviceOrientationEvent();
    DeviceOrientationEvent(const AtomicString& eventType, DeviceOrientation*);

    RefPtr<DeviceOrientation> m_orientation;
};

} // namespace WebCore

#endif // DeviceOrientationEvent_h
