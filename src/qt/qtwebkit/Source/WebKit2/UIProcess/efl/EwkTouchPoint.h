/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef EwkTouchPoint_h
#define EwkTouchPoint_h

#if ENABLE(TOUCH_EVENTS)

#include "APIObject.h"
#include "WKArray.h"
#include "WKEventEfl.h"
#include "WKRetainPtr.h"
#include <wtf/PassRefPtr.h>

namespace WebKit {

class EwkTouchPoint : public APIObject {
public:
    static const APIObject::Type APIType = TypeTouchPoint;

    static PassRefPtr<EwkTouchPoint> create(uint32_t id, WKTouchPointState state, const WKPoint& screenPosition, const WKPoint& position, const WKSize& radius, float rotationAngle = 0, float forceFactor = 1)
    {
        return adoptRef(new EwkTouchPoint(id, state, screenPosition, position, radius, rotationAngle, forceFactor));
    }

    uint32_t id() const { return m_id; }
    WKTouchPointState state() const { return m_state; }
    WKPoint screenPosition() const { return m_screenPosition; }
    WKPoint position() const { return m_position; }
    WKSize radius() const { return m_radius; }
    float rotationAngle() const { return m_rotationAngle; }
    float forceFactor() const { return m_forceFactor; }

private:
    EwkTouchPoint(uint32_t id, WKTouchPointState, const WKPoint&, const WKPoint&, const WKSize&, float rotationAngle, float forceFactor);

    virtual APIObject::Type type() const { return APIType; }

    uint32_t m_id;
    WKTouchPointState m_state;
    WKPoint m_screenPosition;
    WKPoint m_position;
    WKSize m_radius;
    float m_rotationAngle;
    float m_forceFactor;
};

} // namespace WebKit

#endif // ENABLE(TOUCH_EVENTS)

#endif /* EwkTouchPoint_h */
