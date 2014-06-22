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

#ifndef PlatformTouchPoint_h
#define PlatformTouchPoint_h

#include "IntPoint.h"
#include <wtf/Vector.h>

#if ENABLE(TOUCH_EVENTS)

#if PLATFORM(QT)
#include <QTouchEvent>
#endif

#if PLATFORM(BLACKBERRY)
namespace BlackBerry {
namespace Platform {
class TouchPoint;
};
};
#endif

namespace WebCore {

class PlatformTouchEvent;

class PlatformTouchPoint {
public:
    enum State {
        TouchReleased,
        TouchPressed,
        TouchMoved,
        TouchStationary,
        TouchCancelled,
        TouchStateEnd // Placeholder: must remain the last item.
    };

    // This is necessary for us to be able to build synthetic events.
    PlatformTouchPoint()
        : m_id(0)
        , m_radiusY(0)
        , m_radiusX(0)
        , m_rotationAngle(0)
        , m_force(0)
    {
    }

#if PLATFORM(BLACKBERRY)
    PlatformTouchPoint(const BlackBerry::Platform::TouchPoint&);
#endif

    unsigned id() const { return m_id; }
    State state() const { return m_state; }
    IntPoint screenPos() const { return m_screenPos; }
    IntPoint pos() const { return m_pos; }
    int radiusX() const { return m_radiusX; }
    int radiusY() const { return m_radiusY; }
    float rotationAngle() const { return m_rotationAngle; }
    float force() const { return m_force; }

protected:
    unsigned m_id;
    State m_state;
    IntPoint m_screenPos;
    IntPoint m_pos;
    int m_radiusY;
    int m_radiusX;
    float m_rotationAngle;
    float m_force;
};

}

#endif // ENABLE(TOUCH_EVENTS)

#endif // PlatformTouchPoint_h
