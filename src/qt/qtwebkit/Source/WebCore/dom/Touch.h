/*
 * Copyright 2008, The Android Open Source Project
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Touch_h
#define Touch_h

#if ENABLE(TOUCH_EVENTS)

#include "EventTarget.h"
#include "LayoutPoint.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Frame;

class Touch : public RefCounted<Touch> {
public:
    static PassRefPtr<Touch> create(Frame* frame, EventTarget* target,
            unsigned identifier, int screenX, int screenY, int pageX, int pageY,
            int radiusX, int radiusY, float rotationAngle, float force)
    {
        return adoptRef(new Touch(frame, target, identifier, screenX, 
                screenY, pageX, pageY, radiusX, radiusY, rotationAngle, force));
    }

    EventTarget* target() const { return m_target.get(); }
    unsigned identifier() const { return m_identifier; }
    int clientX() const { return m_clientX; }
    int clientY() const { return m_clientY; }
    int screenX() const { return m_screenX; }
    int screenY() const { return m_screenY; }
    int pageX() const { return m_pageX; }
    int pageY() const { return m_pageY; }
    int webkitRadiusX() const { return m_radiusX; }
    int webkitRadiusY() const { return m_radiusY; }
    float webkitRotationAngle() const { return m_rotationAngle; }
    float webkitForce() const { return m_force; }
    const LayoutPoint& absoluteLocation() const { return m_absoluteLocation; }
    PassRefPtr<Touch> cloneWithNewTarget(EventTarget*) const;

private:
    Touch(Frame* frame, EventTarget* target, unsigned identifier,
            int screenX, int screenY, int pageX, int pageY,
            int radiusX, int radiusY, float rotationAngle, float force);

    Touch(EventTarget*, unsigned identifier, int clientX, int clientY,
        int screenX, int screenY, int pageX, int pageY,
        int radiusX, int radiusY, float rotationAngle, float force, LayoutPoint absoluteLocation);

    RefPtr<EventTarget> m_target;
    unsigned m_identifier;
    int m_clientX;
    int m_clientY;
    int m_screenX;
    int m_screenY;
    int m_pageX;
    int m_pageY;
    int m_radiusX;
    int m_radiusY;
    float m_rotationAngle;
    float m_force;
    LayoutPoint m_absoluteLocation;
};

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)

#endif /* Touch_h */
