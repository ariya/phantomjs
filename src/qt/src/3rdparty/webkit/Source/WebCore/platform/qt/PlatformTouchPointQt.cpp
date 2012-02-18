/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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
 *
 */

#include "config.h"
#include "PlatformTouchPoint.h"

#if ENABLE(TOUCH_EVENTS)

namespace WebCore {

PlatformTouchPoint::PlatformTouchPoint(const QTouchEvent::TouchPoint& point)
{
    // The QTouchEvent::TouchPoint API states that ids will be >= 0.
    m_id = static_cast<unsigned>(point.id());
    switch (point.state()) {
    case Qt::TouchPointReleased: m_state = TouchReleased; break;
    case Qt::TouchPointMoved: m_state = TouchMoved; break;
    case Qt::TouchPointPressed: m_state = TouchPressed; break;
    case Qt::TouchPointStationary: m_state = TouchStationary; break;
    }
    m_screenPos = point.screenPos().toPoint();
    m_pos = point.pos().toPoint();
}

}

#endif
