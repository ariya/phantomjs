/*
 *  Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "DeviceProximityEvent.h"

#if ENABLE(PROXIMITY_EVENTS)

namespace WebCore {

DeviceProximityEvent::DeviceProximityEvent()
    : m_value(std::numeric_limits<double>::infinity())
    , m_min(-std::numeric_limits<double>::infinity())
    , m_max(std::numeric_limits<double>::infinity())
{
}

DeviceProximityEvent::DeviceProximityEvent(const AtomicString& eventType, const double value, const double min, const double max)
    : Event(eventType, true, false) // Default event is bubbles, not cancelable.
    , m_value(value)
    , m_min(min)
    , m_max(max)
{
}

DeviceProximityEvent::DeviceProximityEvent(const AtomicString& eventType, const DeviceProximityEventInit& initializer)
    : Event(eventType, initializer)
    , m_value(initializer.value)
    , m_min(initializer.min)
    , m_max(initializer.max)
{
}

} // namespace WebCore

#endif // PROXIMITY_EVENTS
