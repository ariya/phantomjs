/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010, 2013 Apple Inc. All rights reserved.
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

#ifndef WheelEvent_h
#define WheelEvent_h

#include "EventDispatchMediator.h"
#include "FloatPoint.h"
#include "MouseEvent.h"

namespace WebCore {

class PlatformWheelEvent;

struct WheelEventInit : public MouseEventInit {
    WheelEventInit();

    int wheelDeltaX;
    int wheelDeltaY;
    unsigned deltaMode;
};

class WheelEvent : public MouseEvent {
public:
    enum { TickMultiplier = 120 };

    enum DeltaMode {
        DOM_DELTA_PIXEL = 0,
        DOM_DELTA_LINE,
        DOM_DELTA_PAGE
    };

    static PassRefPtr<WheelEvent> create()
    {
        return adoptRef(new WheelEvent);
    }

    static PassRefPtr<WheelEvent> create(const AtomicString& type, const WheelEventInit& initializer)
    {
        return adoptRef(new WheelEvent(type, initializer));
    }

    static PassRefPtr<WheelEvent> create(const FloatPoint& wheelTicks,
        const FloatPoint& rawDelta, unsigned deltaMode, PassRefPtr<AbstractView> view,
        const IntPoint& screenLocation, const IntPoint& pageLocation,
        bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool directionInvertedFromDevice, double timestamp)
    {
        return adoptRef(new WheelEvent(wheelTicks, rawDelta, deltaMode, view,
        screenLocation, pageLocation, ctrlKey, altKey, shiftKey, metaKey, directionInvertedFromDevice, timestamp));
    }

    void initWheelEvent(int rawDeltaX, int rawDeltaY, PassRefPtr<AbstractView>,
        int screenX, int screenY, int pageX, int pageY,
        bool ctrlKey, bool altKey, bool shiftKey, bool metaKey);

    void initWebKitWheelEvent(int rawDeltaX, int rawDeltaY, PassRefPtr<AbstractView>,
        int screenX, int screenY, int pageX, int pageY,
        bool ctrlKey, bool altKey, bool shiftKey, bool metaKey);

    int wheelDelta() const { return m_wheelDelta.y() ? m_wheelDelta.y() : m_wheelDelta.x(); }
    int wheelDeltaX() const { return m_wheelDelta.x(); }
    int wheelDeltaY() const { return m_wheelDelta.y(); }
    int rawDeltaX() const { return m_rawDelta.x(); }
    int rawDeltaY() const { return m_rawDelta.y(); }
    unsigned deltaMode() const { return m_deltaMode; }

    bool webkitDirectionInvertedFromDevice() const { return m_directionInvertedFromDevice; }
    // Needed for Objective-C legacy support
    bool isHorizontal() const { return m_wheelDelta.x(); }

    virtual const AtomicString& interfaceName() const;
    virtual bool isMouseEvent() const;

private:
    WheelEvent();
    WheelEvent(const AtomicString&, const WheelEventInit&);
    WheelEvent(const FloatPoint& wheelTicks, const FloatPoint& rawDelta,
        unsigned, PassRefPtr<AbstractView>, const IntPoint& screenLocation, const IntPoint& pageLocation,
        bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool directionInvertedFromDevice, double timestamp);

    IntPoint m_wheelDelta;
    IntPoint m_rawDelta;
    unsigned m_deltaMode;
    bool m_directionInvertedFromDevice;
};

class WheelEventDispatchMediator : public EventDispatchMediator {
public:
    static PassRefPtr<WheelEventDispatchMediator> create(const PlatformWheelEvent&, PassRefPtr<AbstractView>);
private:
    WheelEventDispatchMediator(const PlatformWheelEvent&, PassRefPtr<AbstractView>);
    WheelEvent* event() const;
    virtual bool dispatchEvent(EventDispatcher*) const OVERRIDE;
};

} // namespace WebCore

#endif // WheelEvent_h
