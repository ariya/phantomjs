/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
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

#include "MouseRelatedEvent.h"

namespace WebCore {

    // extension: mouse wheel event
    class WheelEvent : public MouseRelatedEvent {
    public:
        enum Granularity { Pixel, Line, Page };

        static PassRefPtr<WheelEvent> create()
        {
            return adoptRef(new WheelEvent);
        }
        static PassRefPtr<WheelEvent> create(float wheelTicksX, float wheelTicksY,
            float rawDeltaX, float rawDeltaY, Granularity granularity, PassRefPtr<AbstractView> view,
            int screenX, int screenY, int pageX, int pageY,
            bool ctrlKey, bool altKey, bool shiftKey, bool metaKey)
        {
            return adoptRef(new WheelEvent(wheelTicksX, wheelTicksY, rawDeltaX, rawDeltaY,
                granularity, view, screenX, screenY, pageX, pageY,
                ctrlKey, altKey, shiftKey, metaKey));
        }

        void initWheelEvent(int rawDeltaX, int rawDeltaY, PassRefPtr<AbstractView>,
                            int screenX, int screenY, int pageX, int pageY,
                            bool ctrlKey, bool altKey, bool shiftKey, bool metaKey);

        void initWebKitWheelEvent(int rawDeltaX, int rawDeltaY, PassRefPtr<AbstractView>,
                                  int screenX, int screenY, int pageX, int pageY,
                                  bool ctrlKey, bool altKey, bool shiftKey, bool metaKey);

        int wheelDelta() const { if (m_wheelDeltaY == 0) return m_wheelDeltaX; return m_wheelDeltaY; }
        int wheelDeltaX() const { return m_wheelDeltaX; }
        int wheelDeltaY() const { return m_wheelDeltaY; }
        int rawDeltaX() const { return m_rawDeltaX; }
        int rawDeltaY() const { return m_rawDeltaY; }
        Granularity granularity() const { return m_granularity; }

        // Needed for Objective-C legacy support
        bool isHorizontal() const { return m_wheelDeltaX; }

    private:
        WheelEvent();
        WheelEvent(float wheelTicksX, float wheelTicksY, float rawDeltaX, float rawDeltaY,
                   Granularity granularity, PassRefPtr<AbstractView>,
                   int screenX, int screenY, int pageX, int pageY,
                   bool ctrlKey, bool altKey, bool shiftKey, bool metaKey);

        virtual bool isWheelEvent() const;
        
        int m_wheelDeltaX;
        int m_wheelDeltaY;

        int m_rawDeltaX;
        int m_rawDeltaY;
        Granularity m_granularity;
    };

class WheelEventDispatchMediator : public EventDispatchMediator {
public:
    WheelEventDispatchMediator(const PlatformWheelEvent&, PassRefPtr<AbstractView>);

private:
    WheelEvent* event() const;
    virtual bool dispatchEvent(EventDispatcher*) const;
};

} // namespace WebCore

#endif // WheelEvent_h
