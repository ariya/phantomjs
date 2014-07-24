/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "PlatformTouchPoint.h"

#include <BlackBerryPlatformAssert.h>
#include <BlackBerryPlatformTouchEvent.h>

#if ENABLE(TOUCH_EVENTS)

namespace WebCore {

PlatformTouchPoint::PlatformTouchPoint(const BlackBerry::Platform::TouchPoint& point)
    : m_id(point.id())
    , m_screenPos(point.screenPosition())
    // FIXME: We should be calculating a new viewport position from the current scroll
    // position and the documentContentPosition, in case we scrolled since the platform
    // event was created.
    , m_pos(point.documentViewportPosition())
{
    switch (point.state()) {
    case BlackBerry::Platform::TouchPoint::TouchReleased:
        m_state = TouchReleased;
        break;
    case BlackBerry::Platform::TouchPoint::TouchMoved:
        m_state = TouchMoved;
        break;
    case BlackBerry::Platform::TouchPoint::TouchPressed:
        m_state = TouchPressed;
        break;
    case BlackBerry::Platform::TouchPoint::TouchStationary:
        m_state = TouchStationary;
        break;
    default:
        m_state = TouchStationary; // make sure m_state is initialized
        BLACKBERRY_ASSERT(false);
        break;
    }
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
