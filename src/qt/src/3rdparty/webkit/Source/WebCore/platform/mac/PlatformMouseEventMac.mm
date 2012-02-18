/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "PlatformMouseEvent.h"

#import "PlatformScreen.h"

namespace WebCore {

static MouseButton mouseButtonForEvent(NSEvent *event)
{
    switch ([event type]) {
        case NSLeftMouseDown:
        case NSLeftMouseUp:
        case NSLeftMouseDragged:
            return LeftButton;
        case NSRightMouseDown:
        case NSRightMouseUp:
        case NSRightMouseDragged:
            return RightButton;
        case NSOtherMouseDown:
        case NSOtherMouseUp:
        case NSOtherMouseDragged:
            return MiddleButton;
        default:
            return NoButton;
    }
}

static int clickCountForEvent(NSEvent *event)
{
    switch ([event type]) {
        case NSLeftMouseDown:
        case NSLeftMouseUp:
        case NSLeftMouseDragged:
        case NSRightMouseDown:
        case NSRightMouseUp:
        case NSRightMouseDragged:
        case NSOtherMouseDown:
        case NSOtherMouseUp:
        case NSOtherMouseDragged:
            return [event clickCount];
        default:
            return 0;
    }
}

IntPoint globalPoint(const NSPoint& windowPoint, NSWindow *window)
{
    return IntPoint(flipScreenPoint([window convertBaseToScreen:windowPoint], screenForWindow(window)));
}

IntPoint pointForEvent(NSEvent *event, NSView *windowView)
{
    switch ([event type]) {
        case NSLeftMouseDown:
        case NSLeftMouseUp:
        case NSLeftMouseDragged:
        case NSRightMouseDown:
        case NSRightMouseUp:
        case NSRightMouseDragged:
        case NSOtherMouseDown:
        case NSOtherMouseUp:
        case NSOtherMouseDragged:
        case NSMouseMoved:
        case NSScrollWheel: {
            // Note: This will have its origin at the bottom left of the window unless windowView is flipped.
            // In those cases, the Y coordinate gets flipped by Widget::convertFromContainingWindow.
            NSPoint location = [event locationInWindow];
            if (windowView)
                location = [windowView convertPoint:location fromView:nil];
            return IntPoint(location);
        }
        default:
            return IntPoint();
    }
}

IntPoint globalPointForEvent(NSEvent *event)
{
    switch ([event type]) {
        case NSLeftMouseDown:
        case NSLeftMouseUp:
        case NSLeftMouseDragged:
        case NSRightMouseDown:
        case NSRightMouseUp:
        case NSRightMouseDragged:
        case NSOtherMouseDown:
        case NSOtherMouseUp:
        case NSOtherMouseDragged:
        case NSMouseMoved:
        case NSScrollWheel:
            return globalPoint([event locationInWindow], [event window]);
        default:
            return IntPoint();
    }
}

static MouseEventType mouseEventForNSEvent(NSEvent* event) 
{
    switch ([event type]) {
    case NSScrollWheel:
        return MouseEventScroll;
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged:
    case NSMouseMoved:
        return MouseEventMoved;
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
        return MouseEventPressed;
    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
        return MouseEventReleased;
    default:
        return MouseEventMoved;
    }
}

PlatformMouseEvent::PlatformMouseEvent(NSEvent* event, NSView *windowView)
    : m_position(pointForEvent(event, windowView))
    , m_globalPosition(globalPointForEvent(event))
    , m_button(mouseButtonForEvent(event))
    , m_eventType(mouseEventForNSEvent(event))
    , m_clickCount(clickCountForEvent(event))
    , m_shiftKey([event modifierFlags] & NSShiftKeyMask)
    , m_ctrlKey([event modifierFlags] & NSControlKeyMask)
    , m_altKey([event modifierFlags] & NSAlternateKeyMask)
    , m_metaKey([event modifierFlags] & NSCommandKeyMask)
    , m_timestamp([event timestamp])
    , m_modifierFlags([event modifierFlags])
    , m_eventNumber([event eventNumber])
{
}

PlatformMouseEvent::PlatformMouseEvent(int x, int y, int globalX, int globalY, MouseButton button, MouseEventType eventType,
                   int clickCount, bool shiftKey, bool ctrlKey, bool altKey, bool metaKey, double timestamp,
                   unsigned modifierFlags, int eventNumber)
    : m_position(IntPoint(x, y))
    , m_globalPosition(IntPoint(globalX, globalY))
    , m_button(button)
    , m_eventType(eventType)
    , m_clickCount(clickCount)
    , m_shiftKey(shiftKey)
    , m_ctrlKey(ctrlKey)
    , m_altKey(altKey)
    , m_metaKey(metaKey)
    , m_timestamp(timestamp)
    , m_modifierFlags(modifierFlags)
    , m_eventNumber(eventNumber)
{
}

}
