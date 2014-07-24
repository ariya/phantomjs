/*
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation
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
#include "WebEventFactory.h"

#include "EflKeyboardUtilities.h"
#include <WebCore/AffineTransform.h>
#include <WebCore/Scrollbar.h>

#if ENABLE(TOUCH_EVENTS)
#include "EwkTouchEvent.h"
#include "EwkTouchPoint.h"
#include "ImmutableArray.h"
#include "WKAPICast.h"
#endif

using namespace WebCore;

namespace WebKit {

enum {
    VerticalScrollDirection = 0,
    HorizontalScrollDirection = 1
};

enum {
    LeftButton = 1,
    MiddleButton = 2,
    RightButton = 3
};

static const char keyPadPrefix[] = "KP_";

static inline WebEvent::Modifiers modifiersForEvent(const Evas_Modifier* modifiers)
{
    unsigned result = 0;

    if (evas_key_modifier_is_set(modifiers, "Shift"))
        result |= WebEvent::ShiftKey;
    if (evas_key_modifier_is_set(modifiers, "Control"))
        result |= WebEvent::ControlKey;
    if (evas_key_modifier_is_set(modifiers, "Alt"))
        result |= WebEvent::AltKey;
    if (evas_key_modifier_is_set(modifiers, "Meta"))
        result |= WebEvent::MetaKey;

    return static_cast<WebEvent::Modifiers>(result);
}

static inline WebMouseEvent::Button buttonForEvent(int button)
{
    if (button == LeftButton)
        return WebMouseEvent::LeftButton;
    if (button == MiddleButton)
        return WebMouseEvent::MiddleButton;
    if (button == RightButton)
        return WebMouseEvent::RightButton;

    return WebMouseEvent::NoButton;
}

static inline int clickCountForEvent(const Evas_Button_Flags flags)
{
    if (flags & EVAS_BUTTON_TRIPLE_CLICK)
        return 3;
    if (flags & EVAS_BUTTON_DOUBLE_CLICK)
        return 2;

    return 1;
}

static inline double convertMillisecondToSecond(unsigned timestamp)
{
    return static_cast<double>(timestamp) / 1000;
}

WebMouseEvent WebEventFactory::createWebMouseEvent(const Evas_Event_Mouse_Down* event, const AffineTransform& toWebContent, const AffineTransform& toDeviceScreen)
{
    IntPoint pos(event->canvas.x, event->canvas.y);
    return WebMouseEvent(WebEvent::MouseDown,
        buttonForEvent(event->button),
        toWebContent.mapPoint(pos),
        toDeviceScreen.mapPoint(pos),
        0 /* deltaX */,
        0 /* deltaY */,
        0 /* deltaZ */,
        clickCountForEvent(event->flags),
        modifiersForEvent(event->modifiers),
        convertMillisecondToSecond(event->timestamp));
}

WebMouseEvent WebEventFactory::createWebMouseEvent(const Evas_Event_Mouse_Up* event, const AffineTransform& toWebContent, const AffineTransform& toDeviceScreen)
{
    IntPoint pos(event->canvas.x, event->canvas.y);
    return WebMouseEvent(WebEvent::MouseUp,
        buttonForEvent(event->button),
        toWebContent.mapPoint(pos),
        toDeviceScreen.mapPoint(pos),
        0 /* deltaX */,
        0 /* deltaY */,
        0 /* deltaZ */,
        clickCountForEvent(event->flags),
        modifiersForEvent(event->modifiers),
        convertMillisecondToSecond(event->timestamp));
}

WebMouseEvent WebEventFactory::createWebMouseEvent(const Evas_Event_Mouse_Move* event, const AffineTransform& toWebContent, const AffineTransform& toDeviceScreen)
{
    IntPoint pos(event->cur.canvas.x, event->cur.canvas.y);
    return WebMouseEvent(WebEvent::MouseMove,
        buttonForEvent(event->buttons),
        toWebContent.mapPoint(pos),
        toDeviceScreen.mapPoint(pos),
        0 /* deltaX */,
        0 /* deltaY */,
        0 /* deltaZ */,
        0 /* clickCount */,
        modifiersForEvent(event->modifiers),
        convertMillisecondToSecond(event->timestamp));
}

WebWheelEvent WebEventFactory::createWebWheelEvent(const Evas_Event_Mouse_Wheel* event, const AffineTransform& toWebContent, const AffineTransform& toDeviceScreen)
{
    float deltaX = 0;
    float deltaY = 0;
    float wheelTicksX = 0;
    float wheelTicksY = 0;

    // A negative z value means (in EFL) that we are scrolling down, so we need
    // to invert the value.
    if (event->direction == VerticalScrollDirection) {
        deltaX = 0;
        deltaY = - event->z;
    } else if (event->direction == HorizontalScrollDirection) {
        deltaX = - event->z;
        deltaY = 0;
    }

    wheelTicksX = deltaX;
    wheelTicksY = deltaY;
    deltaX *= static_cast<float>(Scrollbar::pixelsPerLineStep());
    deltaY *= static_cast<float>(Scrollbar::pixelsPerLineStep());

    IntPoint pos(event->canvas.x, event->canvas.y);

    return WebWheelEvent(WebEvent::Wheel,
        toWebContent.mapPoint(pos),
        toDeviceScreen.mapPoint(pos),
        FloatSize(deltaX, deltaY),
        FloatSize(wheelTicksX, wheelTicksY),
        WebWheelEvent::ScrollByPixelWheelEvent,
        modifiersForEvent(event->modifiers),
        convertMillisecondToSecond(event->timestamp));
}

WebKeyboardEvent WebEventFactory::createWebKeyboardEvent(const Evas_Event_Key_Down* event)
{
    const String keyName(event->key);
    return WebKeyboardEvent(WebEvent::KeyDown,
        String::fromUTF8(event->string),
        String::fromUTF8(event->string),
        keyIdentifierForEvasKeyName(keyName),
        windowsKeyCodeForEvasKeyName(keyName),
        0 /* FIXME: nativeVirtualKeyCode */,
        0 /* macCharCode */,
        false /* FIXME: isAutoRepeat */,
        keyName.startsWith(keyPadPrefix),
        false /* isSystemKey */,
        modifiersForEvent(event->modifiers),
        convertMillisecondToSecond(event->timestamp));
}

WebKeyboardEvent WebEventFactory::createWebKeyboardEvent(const Evas_Event_Key_Up* event)
{
    const String keyName(event->key);
    return WebKeyboardEvent(WebEvent::KeyUp,
        String::fromUTF8(event->string),
        String::fromUTF8(event->string),
        keyIdentifierForEvasKeyName(keyName),
        windowsKeyCodeForEvasKeyName(keyName),
        0 /* FIXME: nativeVirtualKeyCode */,
        0 /* macCharCode */,
        false /* FIXME: isAutoRepeat */,
        keyName.startsWith(keyPadPrefix),
        false /* isSystemKey */,
        modifiersForEvent(event->modifiers),
        convertMillisecondToSecond(event->timestamp));
}

#if ENABLE(TOUCH_EVENTS)
static inline WebPlatformTouchPoint::TouchPointState toWebPlatformTouchPointState(WKTouchPointState state)
{
    switch (state) {
    case kWKTouchPointStateTouchReleased:
        return WebPlatformTouchPoint::TouchReleased;
    case kWKTouchPointStateTouchMoved:
        return WebPlatformTouchPoint::TouchMoved;
    case kWKTouchPointStateTouchPressed:
        return WebPlatformTouchPoint::TouchPressed;
    case kWKTouchPointStateTouchStationary:
        return WebPlatformTouchPoint::TouchStationary;
    case kWKTouchPointStateTouchCancelled:
    default:
        return WebPlatformTouchPoint::TouchCancelled;
    }
}

static inline WebEvent::Type toWebEventType(WKEventType type)
{
    switch (type) {
    case kWKEventTypeTouchStart:
        return WebEvent::TouchStart;
    case kWKEventTypeTouchMove:
        return WebEvent::TouchMove;
    case kWKEventTypeTouchEnd:
        return WebEvent::TouchEnd;
    case kWKEventTypeTouchCancel:
        return WebEvent::TouchCancel;
    default:
        return WebEvent::NoType;
    }

}

static inline WebEvent::Modifiers toWebEventModifiers(unsigned modifiers)
{
    unsigned result = 0;

    if (modifiers & kWKEventModifiersShiftKey)
        result |= WebEvent::ShiftKey;
    if (modifiers & kWKEventModifiersControlKey)
        result |= WebEvent::ControlKey;
    if (modifiers & kWKEventModifiersAltKey)
        result |= WebEvent::AltKey;
    if (modifiers & kWKEventModifiersMetaKey)
        result |= WebEvent::MetaKey;

    return static_cast<WebEvent::Modifiers>(result);
}

WebTouchEvent WebEventFactory::createWebTouchEvent(const EwkTouchEvent* event, const AffineTransform& toWebContent)
{
    ImmutableArray* touchPointsArray = toImpl(event->touchPoints());
    size_t size = touchPointsArray->size();

    Vector<WebPlatformTouchPoint> touchPoints;
    touchPoints.reserveInitialCapacity(size);

    for (size_t i = 0; i < size; ++i) {
        if (EwkTouchPoint* point = touchPointsArray->at<EwkTouchPoint>(i))
            touchPoints.uncheckedAppend(WebPlatformTouchPoint(point->id(), toWebPlatformTouchPointState(point->state()), toIntPoint(point->screenPosition()), toWebContent.mapPoint(toIntPoint(point->position())), toIntSize(point->radius()), point->rotationAngle(), point->forceFactor()));
    }

    return WebTouchEvent(toWebEventType(event->eventType()), touchPoints, toWebEventModifiers(event->modifiers()), event->timestamp());
}
#endif

} // namespace WebKit
