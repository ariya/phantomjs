/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebEventConversion.h"

#include "WebEvent.h"

namespace WebKit {

class WebKit2PlatformMouseEvent : public WebCore::PlatformMouseEvent {
public:
    WebKit2PlatformMouseEvent(const WebMouseEvent& webEvent)
    {
        // PlatformEvent
        switch (webEvent.type()) {
        case WebEvent::MouseDown:
            m_type = WebCore::PlatformEvent::MousePressed;
            break;
        case WebEvent::MouseUp:
            m_type = WebCore::PlatformEvent::MouseReleased;
            break;
        case WebEvent::MouseMove:
            m_type = WebCore::PlatformEvent::MouseMoved;
            break;
        default:
            ASSERT_NOT_REACHED();
        }

        m_modifiers = 0;
        if (webEvent.shiftKey())
            m_modifiers |= ShiftKey;
        if (webEvent.controlKey())
            m_modifiers |= CtrlKey;
        if (webEvent.altKey())
            m_modifiers |= AltKey;
        if (webEvent.metaKey())
            m_modifiers |= MetaKey;

        m_timestamp = webEvent.timestamp();

        // PlatformMouseEvent
        switch (webEvent.button()) {
        case WebMouseEvent::NoButton:
            m_button = WebCore::NoButton;
            break;
        case WebMouseEvent::LeftButton:
            m_button = WebCore::LeftButton;
            break;
        case WebMouseEvent::MiddleButton:
            m_button = WebCore::MiddleButton;
            break;
        case WebMouseEvent::RightButton:
            m_button = WebCore::RightButton;
            break;
        default:
            ASSERT_NOT_REACHED();
        }

        m_position = webEvent.position();
        m_globalPosition = webEvent.globalPosition();
        m_clickCount = webEvent.clickCount();

        m_modifierFlags = 0;
        if (webEvent.shiftKey())
            m_modifierFlags |= WebEvent::ShiftKey;
        if (webEvent.controlKey())
            m_modifierFlags |= WebEvent::ControlKey;
        if (webEvent.altKey())
            m_modifierFlags |= WebEvent::AltKey;
        if (webEvent.metaKey())
            m_modifierFlags |= WebEvent::MetaKey;
    }
};

WebCore::PlatformMouseEvent platform(const WebMouseEvent& webEvent)
{
    return WebKit2PlatformMouseEvent(webEvent);
}

class WebKit2PlatformWheelEvent : public WebCore::PlatformWheelEvent {
public:
    WebKit2PlatformWheelEvent(const WebWheelEvent& webEvent)
    {
        // PlatformEvent
        m_type = PlatformEvent::Wheel;

        m_modifiers = 0;
        if (webEvent.shiftKey())
            m_modifiers |= ShiftKey;
        if (webEvent.controlKey())
            m_modifiers |= CtrlKey;
        if (webEvent.altKey())
            m_modifiers |= AltKey;
        if (webEvent.metaKey())
            m_modifiers |= MetaKey;

        m_timestamp = webEvent.timestamp();

        // PlatformWheelEvent
        m_position = webEvent.position();
        m_globalPosition = webEvent.globalPosition();
        m_deltaX = webEvent.delta().width();
        m_deltaY = webEvent.delta().height();
        m_wheelTicksX = webEvent.wheelTicks().width();
        m_wheelTicksY = webEvent.wheelTicks().height();
        m_granularity = (webEvent.granularity() == WebWheelEvent::ScrollByPageWheelEvent) ? WebCore::ScrollByPageWheelEvent : WebCore::ScrollByPixelWheelEvent;
        m_directionInvertedFromDevice = webEvent.directionInvertedFromDevice();
#if PLATFORM(MAC)
        m_phase = static_cast<WebCore::PlatformWheelEventPhase>(webEvent.phase());
        m_momentumPhase = static_cast<WebCore::PlatformWheelEventPhase>(webEvent.momentumPhase());
        m_hasPreciseScrollingDeltas = webEvent.hasPreciseScrollingDeltas();
        m_scrollCount = webEvent.scrollCount();
        m_unacceleratedScrollingDeltaX = webEvent.unacceleratedScrollingDelta().width();
        m_unacceleratedScrollingDeltaY = webEvent.unacceleratedScrollingDelta().height();
#endif
    }
};

WebCore::PlatformWheelEvent platform(const WebWheelEvent& webEvent)
{
    return WebKit2PlatformWheelEvent(webEvent);
}

class WebKit2PlatformKeyboardEvent : public WebCore::PlatformKeyboardEvent {
public:
    WebKit2PlatformKeyboardEvent(const WebKeyboardEvent& webEvent)
    {
        // PlatformEvent
        switch (webEvent.type()) {
        case WebEvent::KeyDown:
            m_type = WebCore::PlatformEvent::KeyDown;
            break;
        case WebEvent::KeyUp:
            m_type = WebCore::PlatformEvent::KeyUp;
            break;
        case WebEvent::RawKeyDown:
            m_type = WebCore::PlatformEvent::RawKeyDown;
            break;
        case WebEvent::Char:
            m_type = WebCore::PlatformEvent::Char;
            break;
        default:
            ASSERT_NOT_REACHED();
        }

        m_modifiers = 0;
        if (webEvent.shiftKey())
            m_modifiers |= ShiftKey;
        if (webEvent.controlKey())
            m_modifiers |= CtrlKey;
        if (webEvent.altKey())
            m_modifiers |= AltKey;
        if (webEvent.metaKey())
            m_modifiers |= MetaKey;

        m_timestamp = webEvent.timestamp();

        // PlatformKeyboardEvent
        m_text = webEvent.text();
        m_unmodifiedText = webEvent.unmodifiedText();
        m_keyIdentifier = webEvent.keyIdentifier();
        m_windowsVirtualKeyCode = webEvent.windowsVirtualKeyCode();
        m_nativeVirtualKeyCode = webEvent.nativeVirtualKeyCode();
        m_macCharCode = webEvent.macCharCode();
        m_autoRepeat = webEvent.isAutoRepeat();
        m_isKeypad = webEvent.isKeypad();
        m_isSystemKey = webEvent.isSystemKey();
    }
};

WebCore::PlatformKeyboardEvent platform(const WebKeyboardEvent& webEvent)
{
    return WebKit2PlatformKeyboardEvent(webEvent);
}

#if ENABLE(GESTURE_EVENTS)
class WebKit2PlatformGestureEvent : public WebCore::PlatformGestureEvent {
public:
    WebKit2PlatformGestureEvent(const WebGestureEvent& webEvent)
    {
        // PlatformEvent
        switch (webEvent.type()) {
        case WebEvent::GestureScrollBegin:
            m_type = WebCore::PlatformEvent::GestureScrollBegin;
            break;
        case WebEvent::GestureScrollEnd:
            m_type = WebCore::PlatformEvent::GestureScrollEnd;
            break;
        case WebEvent::GestureSingleTap:
            m_type = WebCore::PlatformEvent::GestureTap;
            break;
        default:
            ASSERT_NOT_REACHED();
        }

        m_modifiers = 0;
        if (webEvent.shiftKey())
            m_modifiers |= ShiftKey;
        if (webEvent.controlKey())
            m_modifiers |= CtrlKey;
        if (webEvent.altKey())
            m_modifiers |= AltKey;
        if (webEvent.metaKey())
            m_modifiers |= MetaKey;

        m_timestamp = webEvent.timestamp();

        // PlatformGestureEvent
        m_position = webEvent.position();
        m_globalPosition = webEvent.globalPosition();

        m_area = webEvent.area();
        m_deltaX = webEvent.delta().x();
        m_deltaY = webEvent.delta().y();
    }
};

WebCore::PlatformGestureEvent platform(const WebGestureEvent& webEvent)
{
    return WebKit2PlatformGestureEvent(webEvent);
}
#endif

#if ENABLE(TOUCH_EVENTS)
class WebKit2PlatformTouchPoint : public WebCore::PlatformTouchPoint {
public:
    WebKit2PlatformTouchPoint(const WebPlatformTouchPoint& webTouchPoint)
    {
        m_id = webTouchPoint.id();

        switch (webTouchPoint.state()) {
        case WebPlatformTouchPoint::TouchReleased:
            m_state = PlatformTouchPoint::TouchReleased;
            break;
        case WebPlatformTouchPoint::TouchPressed:
            m_state = PlatformTouchPoint::TouchPressed;
            break;
        case WebPlatformTouchPoint::TouchMoved:
            m_state = PlatformTouchPoint::TouchMoved;
            break;
        case WebPlatformTouchPoint::TouchStationary:
            m_state = PlatformTouchPoint::TouchStationary;
            break;
        case WebPlatformTouchPoint::TouchCancelled:
            m_state = PlatformTouchPoint::TouchCancelled;
            break;
        default:
            ASSERT_NOT_REACHED();
        }

        m_screenPos = webTouchPoint.screenPosition();
        m_pos = webTouchPoint.position();
        m_radiusX = webTouchPoint.radius().width();
        m_radiusY = webTouchPoint.radius().height();
        m_force = webTouchPoint.force();
        m_rotationAngle = webTouchPoint.rotationAngle();
    }
};

class WebKit2PlatformTouchEvent : public WebCore::PlatformTouchEvent {
public:
    WebKit2PlatformTouchEvent(const WebTouchEvent& webEvent)
    {
        // PlatformEvent
        switch (webEvent.type()) {
        case WebEvent::TouchStart: 
            m_type = WebCore::PlatformEvent::TouchStart;
            break;
        case WebEvent::TouchMove: 
            m_type = WebCore::PlatformEvent::TouchMove;
            break;
        case WebEvent::TouchEnd: 
            m_type = WebCore::PlatformEvent::TouchEnd;
            break;
        case WebEvent::TouchCancel:
            m_type = WebCore::PlatformEvent::TouchCancel;
            break;
        default:
            ASSERT_NOT_REACHED();
        }

        m_modifiers = 0;
        if (webEvent.shiftKey())
            m_modifiers |= ShiftKey;
        if (webEvent.controlKey())
            m_modifiers |= CtrlKey;
        if (webEvent.altKey())
            m_modifiers |= AltKey;
        if (webEvent.metaKey())
            m_modifiers |= MetaKey;

        m_timestamp = webEvent.timestamp();

        // PlatformTouchEvent
        for (size_t i = 0; i < webEvent.touchPoints().size(); ++i)
            m_touchPoints.append(WebKit2PlatformTouchPoint(webEvent.touchPoints().at(i)));
    }
};

WebCore::PlatformTouchEvent platform(const WebTouchEvent& webEvent)
{
    return WebKit2PlatformTouchEvent(webEvent);
}
#endif

} // namespace WebKit
