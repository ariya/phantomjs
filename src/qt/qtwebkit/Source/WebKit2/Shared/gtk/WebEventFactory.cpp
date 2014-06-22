/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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

#include "PlatformKeyboardEvent.h"
#include "Scrollbar.h"
#include "WindowsKeyboardCodes.h"
#include <WebCore/GtkVersioning.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <wtf/ASCIICType.h>

using namespace WebCore;

namespace WebKit {

static inline bool isGdkKeyCodeFromKeyPad(unsigned keyval)
{
    return keyval >= GDK_KP_Space && keyval <= GDK_KP_9;
}

static inline WebEvent::Modifiers modifiersForEvent(const GdkEvent* event)
{
    unsigned modifiers = 0;
    GdkModifierType state;

    // Check for a valid state in GdkEvent.
    if (!gdk_event_get_state(event, &state))
        return static_cast<WebEvent::Modifiers>(0);

    if (state & GDK_CONTROL_MASK)
        modifiers |= WebEvent::ControlKey;
    if (state & GDK_SHIFT_MASK)
        modifiers |= WebEvent::ShiftKey;
    if (state & GDK_MOD1_MASK)
        modifiers |= WebEvent::AltKey;
    if (state & GDK_META_MASK)
        modifiers |= WebEvent::MetaKey;

    return static_cast<WebEvent::Modifiers>(modifiers);
}

static inline WebMouseEvent::Button buttonForEvent(const GdkEvent* event)
{
    unsigned button = 0;

    switch (event->type) {
    case GDK_MOTION_NOTIFY:
        button = WebMouseEvent::NoButton;
        if (event->motion.state & GDK_BUTTON1_MASK)
            button = WebMouseEvent::LeftButton;
        else if (event->motion.state & GDK_BUTTON2_MASK)
            button = WebMouseEvent::MiddleButton;
        else if (event->motion.state & GDK_BUTTON3_MASK)
            button = WebMouseEvent::RightButton;
        break;
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
        if (event->button.button == 1)
            button = WebMouseEvent::LeftButton;
        else if (event->button.button == 2)
            button = WebMouseEvent::MiddleButton;
        else if (event->button.button == 3)
            button = WebMouseEvent::RightButton;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return static_cast<WebMouseEvent::Button>(button);
}

WebMouseEvent WebEventFactory::createWebMouseEvent(const GdkEvent* event, int currentClickCount)
{
    double x, y, xRoot, yRoot;
    gdk_event_get_coords(event, &x, &y);
    gdk_event_get_root_coords(event, &xRoot, &yRoot);

    WebEvent::Type type = static_cast<WebEvent::Type>(0);
    switch (event->type) {
    case GDK_MOTION_NOTIFY:
        type = WebEvent::MouseMove;
        break;
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
        type = WebEvent::MouseDown;
        break;
    case GDK_BUTTON_RELEASE:
        type = WebEvent::MouseUp;
        break;
    default :
        ASSERT_NOT_REACHED();
    }

    return WebMouseEvent(type,
                         buttonForEvent(event),
                         IntPoint(x, y),
                         IntPoint(xRoot, yRoot),
                         0 /* deltaX */,
                         0 /* deltaY */,
                         0 /* deltaZ */,
                         currentClickCount,
                         modifiersForEvent(event),
                         gdk_event_get_time(event));
}

WebWheelEvent WebEventFactory::createWebWheelEvent(const GdkEvent* event)
{
    double x, y, xRoot, yRoot;
    gdk_event_get_coords(event, &x, &y);
    gdk_event_get_root_coords(event, &xRoot, &yRoot);

    FloatSize wheelTicks;
    switch (event->scroll.direction) {
    case GDK_SCROLL_UP:
        wheelTicks = FloatSize(0, 1);
        break;
    case GDK_SCROLL_DOWN:
        wheelTicks = FloatSize(0, -1);
        break;
    case GDK_SCROLL_LEFT:
        wheelTicks = FloatSize(1, 0);
        break;
    case GDK_SCROLL_RIGHT:
        wheelTicks = FloatSize(-1, 0);
        break;
#if GTK_CHECK_VERSION(3, 3, 18)
    case GDK_SCROLL_SMOOTH: {
            double deltaX, deltaY;
            gdk_event_get_scroll_deltas(event, &deltaX, &deltaY);
            wheelTicks = FloatSize(-deltaX, -deltaY);
        }
        break;
#endif
    default:
        ASSERT_NOT_REACHED();
    }

    // FIXME: [GTK] Add a setting to change the pixels per line used for scrolling
    // https://bugs.webkit.org/show_bug.cgi?id=54826
    float step = static_cast<float>(Scrollbar::pixelsPerLineStep());
    FloatSize delta(wheelTicks.width() * step, wheelTicks.height() * step);

    return WebWheelEvent(WebEvent::Wheel,
                         IntPoint(x, y),
                         IntPoint(xRoot, yRoot),
                         delta,
                         wheelTicks,
                         WebWheelEvent::ScrollByPixelWheelEvent,
                         modifiersForEvent(event),
                         gdk_event_get_time(event));
}

WebKeyboardEvent WebEventFactory::createWebKeyboardEvent(const GdkEvent* event, const WebCore::CompositionResults& compositionResults)
{
    unsigned int keyValue = event->key.keyval;
    String text = compositionResults.simpleString.length() ?
         compositionResults.simpleString : PlatformKeyboardEvent::singleCharacterString(keyValue);

    int windowsVirtualKeyCode = compositionResults.compositionUpdated() ?
         VK_PROCESSKEY : PlatformKeyboardEvent::windowsKeyCodeForGdkKeyCode(event->key.keyval);

    return WebKeyboardEvent((event->type == GDK_KEY_RELEASE) ? WebEvent::KeyUp : WebEvent::KeyDown,
                            text,
                            text,
                            PlatformKeyboardEvent::keyIdentifierForGdkKeyCode(keyValue),
                            windowsVirtualKeyCode,
                            static_cast<int>(keyValue),
                            0 /* macCharCode */,
                            false /* isAutoRepeat */,
                            isGdkKeyCodeFromKeyPad(keyValue),
                            false /* isSystemKey */,
                            modifiersForEvent(event),
                            gdk_event_get_time(event));
}

} // namespace WebKit
