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

#ifndef DeviceProximityEvent_h
#define DeviceProximityEvent_h

#if ENABLE(PROXIMITY_EVENTS)

#include "Event.h"

namespace WebCore {

struct DeviceProximityEventInit : public EventInit {
    DeviceProximityEventInit()
        : value(std::numeric_limits<double>::infinity())
        , min(-std::numeric_limits<double>::infinity())
        , max(std::numeric_limits<double>::infinity())
    {
        // Default value of bubbles is true by the Proximity Events spec.
        // http://www.w3.org/TR/proximity/#deviceproximityevent-interface
        bubbles = true;
    };

    double value;
    double min;
    double max;
};

class DeviceProximityEvent : public Event {
public:
    ~DeviceProximityEvent() { }

    static PassRefPtr<DeviceProximityEvent> create()
    {
        return adoptRef(new DeviceProximityEvent());
    }

    static PassRefPtr<DeviceProximityEvent> create(const AtomicString& eventType, const double value, const double min, const double max)
    {
        return adoptRef(new DeviceProximityEvent(eventType, value, min, max));
    }

    static PassRefPtr<DeviceProximityEvent> create(const AtomicString& type, const DeviceProximityEventInit& initializer)
    {
        return adoptRef(new DeviceProximityEvent(type, initializer));
    }

    double value() { return m_value; }
    double min() { return m_min; }
    double max() { return m_max; }

    virtual const AtomicString& interfaceName() const { return eventNames().interfaceForDeviceProximityEvent; }

private:
    DeviceProximityEvent();
    DeviceProximityEvent(const AtomicString& eventType, const double value, const double min, const double max);
    DeviceProximityEvent(const AtomicString& eventType, const DeviceProximityEventInit&);

    double m_value;
    double m_min;
    double m_max;
};

} // namespace WebCore

#endif // DeviceProximityEvent_h
#endif // PROXIMITY_EVENTS
