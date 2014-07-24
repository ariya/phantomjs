/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef PlatformTouchEvent_h
#define PlatformTouchEvent_h

#include "PlatformEvent.h"
#include "PlatformTouchPoint.h"
#include <wtf/Vector.h>

#if ENABLE(TOUCH_EVENTS)

#if PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QTouchEvent;
QT_END_NAMESPACE
#endif

#if PLATFORM(BLACKBERRY)
namespace BlackBerry {
namespace Platform {
class TouchEvent;
};
};
#endif

namespace WebCore {


class PlatformTouchEvent : public PlatformEvent {
public:
    PlatformTouchEvent()
        : PlatformEvent(PlatformEvent::TouchStart)
#if PLATFORM(BLACKBERRY)
        , m_rotation(0)
        , m_scale(1)
        , m_doubleTap(false)
        , m_touchHold(false)
#endif
    {
    }

#if PLATFORM(BLACKBERRY)
    explicit PlatformTouchEvent(BlackBerry::Platform::TouchEvent*);
#endif

    const Vector<PlatformTouchPoint>& touchPoints() const { return m_touchPoints; }

#if PLATFORM(BLACKBERRY)
    float rotation() const { return m_rotation; }
    float scale() const { return m_scale; }
    bool doubleTap() const { return m_doubleTap; }
    bool touchHold() const { return m_touchHold; }
#endif

protected:
    Vector<PlatformTouchPoint> m_touchPoints;
#if PLATFORM(BLACKBERRY)
    float m_rotation;
    float m_scale;
    bool m_doubleTap;
    bool m_touchHold;
#endif
};

}

#endif // ENABLE(TOUCH_EVENTS)

#endif // PlatformTouchEvent_h
