/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#import "config.h"
#import "EventSenderProxy.h"

#import "PlatformWebView.h"
#import "StringFunctions.h"
#import "TestController.h"
#import <wtf/RetainPtr.h>
#import <Carbon/Carbon.h>
#import <WebKit2/WKString.h>

namespace WTR {

enum MouseAction {
    MouseDown,
    MouseUp,
    MouseDragged
};

// Match the DOM spec (sadly the DOM spec does not provide an enum)
enum MouseButton {
    LeftMouseButton = 0,
    MiddleMouseButton = 1,
    RightMouseButton = 2,
    NoMouseButton = -1
};

struct KeyMappingEntry {
    int macKeyCode;
    int macNumpadKeyCode;
    unichar character;
    NSString* characterName;
};

static NSEventType eventTypeForMouseButtonAndAction(int button, MouseAction action)
{
    switch (button) {
        case LeftMouseButton:
            switch (action) {
                case MouseDown:
                    return NSLeftMouseDown;
                case MouseUp:
                    return NSLeftMouseUp;
                case MouseDragged:
                    return NSLeftMouseDragged;
            }
        case RightMouseButton:
            switch (action) {
                case MouseDown:
                    return NSRightMouseDown;
                case MouseUp:
                    return NSRightMouseUp;
                case MouseDragged:
                    return NSRightMouseDragged;
            }
        default:
            switch (action) {
                case MouseDown:
                    return NSOtherMouseDown;
                case MouseUp:
                    return NSOtherMouseUp;
                case MouseDragged:
                    return NSOtherMouseDragged;
            }
    }
    assert(0);
    return static_cast<NSEventType>(0);
}

static int buildModifierFlags(WKEventModifiers modifiers)
{
    int flags = 0;
    if (modifiers & kWKEventModifiersControlKey)
        flags |= NSControlKeyMask;
    if (modifiers & kWKEventModifiersShiftKey)
        flags |= NSShiftKeyMask;
    if (modifiers & kWKEventModifiersAltKey)
        flags |= NSAlternateKeyMask;
    if (modifiers & kWKEventModifiersMetaKey)
        flags |= NSCommandKeyMask;
    return flags;
}

static NSTimeInterval absoluteTimeForEventTime(double currentEventTime)
{
    return GetCurrentEventTime() + currentEventTime;
}

EventSenderProxy::EventSenderProxy(TestController* testController)
    : m_testController(testController)
    , m_time(0)
    , m_position()
    , m_leftMouseButtonDown(false)
    , m_clickCount(0)
    , m_clickTime(0)
    , m_clickPosition()
    , m_clickButton(kWKEventMouseButtonNoButton)
    , eventNumber(0)
{
}

EventSenderProxy::~EventSenderProxy()
{
}

void EventSenderProxy::updateClickCountForButton(int button)
{
    if (m_time - m_clickTime < 1 && m_position == m_clickPosition && button == m_clickButton) {
        ++m_clickCount;
        m_clickTime = m_time;
        return;
    }

    m_clickCount = 1;
    m_clickTime = m_time;
    m_clickPosition = m_position;
    m_clickButton = button;
}

void EventSenderProxy::mouseDown(unsigned buttonNumber, WKEventModifiers modifiers)
{
    updateClickCountForButton(buttonNumber);

    NSEventType eventType = eventTypeForMouseButtonAndAction(buttonNumber, MouseDown);
    NSEvent *event = [NSEvent mouseEventWithType:eventType
                                        location:NSMakePoint(m_position.x, m_position.y)
                                   modifierFlags:buildModifierFlags(modifiers)
                                       timestamp:absoluteTimeForEventTime(currentEventTime())
                                    windowNumber:[m_testController->mainWebView()->platformWindow() windowNumber]
                                         context:[NSGraphicsContext currentContext] 
                                     eventNumber:++eventNumber 
                                      clickCount:m_clickCount 
                                        pressure:0.0];

    NSView *targetView = [m_testController->mainWebView()->platformView() hitTest:[event locationInWindow]];
    if (targetView) {
        [targetView mouseDown:event];
        if (buttonNumber == LeftMouseButton)
            m_leftMouseButtonDown = true;
    }
}

void EventSenderProxy::mouseUp(unsigned buttonNumber, WKEventModifiers modifiers)
{
    NSEventType eventType = eventTypeForMouseButtonAndAction(buttonNumber, MouseUp);
    NSEvent *event = [NSEvent mouseEventWithType:eventType
                                        location:NSMakePoint(m_position.x, m_position.y)
                                   modifierFlags:buildModifierFlags(modifiers)
                                       timestamp:absoluteTimeForEventTime(currentEventTime())
                                    windowNumber:[m_testController->mainWebView()->platformWindow() windowNumber]
                                         context:[NSGraphicsContext currentContext] 
                                     eventNumber:++eventNumber 
                                      clickCount:m_clickCount 
                                        pressure:0.0];

    NSView *targetView = [m_testController->mainWebView()->platformView() hitTest:[event locationInWindow]];
    targetView = targetView ? targetView : m_testController->mainWebView()->platformView();
    assert(targetView);
    [targetView mouseUp:event];
    if (buttonNumber == LeftMouseButton)
        m_leftMouseButtonDown = false;
    m_clickTime = currentEventTime();
    m_clickPosition = m_position;
}

void EventSenderProxy::mouseMoveTo(double x, double y)
{
    NSView *view = m_testController->mainWebView()->platformView();
    NSPoint position = [view convertPoint:NSMakePoint(x, y) toView:nil];
    m_position.x = position.x;
    m_position.y = position.y;
    NSEvent *event = [NSEvent mouseEventWithType:(m_leftMouseButtonDown ? NSLeftMouseDragged : NSMouseMoved)
                                        location:position
                                   modifierFlags:0 
                                       timestamp:absoluteTimeForEventTime(currentEventTime())
                                    windowNumber:[[view window] windowNumber] 
                                         context:[NSGraphicsContext currentContext] 
                                     eventNumber:++eventNumber 
                                      clickCount:(m_leftMouseButtonDown ? m_clickCount : 0) 
                                        pressure:0.0];

    NSView *targetView = [m_testController->mainWebView()->platformView() hitTest:[event locationInWindow]];
    if (targetView)
        [targetView mouseMoved:event];
}

void EventSenderProxy::leapForward(int milliseconds)
{
    m_time += milliseconds / 1000.0;
}

void EventSenderProxy::keyDown(WKStringRef key, WKEventModifiers modifiers, unsigned keyLocation)
{
    NSString* character = [NSString stringWithCString:toSTD(key).c_str() 
                                   encoding:[NSString defaultCStringEncoding]];

    NSString *eventCharacter = character;
    unsigned short keyCode = 0;
    if ([character isEqualToString:@"leftArrow"]) {
        const unichar ch = NSLeftArrowFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x7B;
    } else if ([character isEqualToString:@"rightArrow"]) {
        const unichar ch = NSRightArrowFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x7C;
    } else if ([character isEqualToString:@"upArrow"]) {
        const unichar ch = NSUpArrowFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x7E;
    } else if ([character isEqualToString:@"downArrow"]) {
        const unichar ch = NSDownArrowFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x7D;
    } else if ([character isEqualToString:@"pageUp"]) {
        const unichar ch = NSPageUpFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x74;
    } else if ([character isEqualToString:@"pageDown"]) {
        const unichar ch = NSPageDownFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x79;
    } else if ([character isEqualToString:@"home"]) {
        const unichar ch = NSHomeFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x73;
    } else if ([character isEqualToString:@"end"]) {
        const unichar ch = NSEndFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x77;
    } else if ([character isEqualToString:@"insert"]) {
        const unichar ch = NSInsertFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x72;
    } else if ([character isEqualToString:@"delete"]) {
        const unichar ch = NSDeleteFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x75;
    } else if ([character isEqualToString:@"printScreen"]) {
        const unichar ch = NSPrintScreenFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x0; // There is no known virtual key code for PrintScreen.
    } else if ([character isEqualToString:@"cyrillicSmallLetterA"]) {
        const unichar ch = 0x0430;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3; // Shares key with "F" on Russian layout.
    } else if ([character isEqualToString:@"leftControl"]) {
        const unichar ch = 0xFFE3;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3B;
    } else if ([character isEqualToString:@"leftShift"]) {
        const unichar ch = 0xFFE1;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x38;
    } else if ([character isEqualToString:@"leftAlt"]) {
        const unichar ch = 0xFFE7;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3A;
    } else if ([character isEqualToString:@"rightControl"]) {
        const unichar ch = 0xFFE4;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3E;
    } else if ([character isEqualToString:@"rightShift"]) {
        const unichar ch = 0xFFE2;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3C;
    } else if ([character isEqualToString:@"rightAlt"]) {
        const unichar ch = 0xFFE8;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3D;
    }

    // Compare the input string with the function-key names defined by the DOM spec (i.e. "F1",...,"F24").
    // If the input string is a function-key name, set its key code.
    for (unsigned i = 1; i <= 24; i++) {
        if ([character isEqualToString:[NSString stringWithFormat:@"F%u", i]]) {
            const unichar ch = NSF1FunctionKey + (i - 1);
            eventCharacter = [NSString stringWithCharacters:&ch length:1];
            switch (i) {
                case 1: keyCode = 0x7A; break;
                case 2: keyCode = 0x78; break;
                case 3: keyCode = 0x63; break;
                case 4: keyCode = 0x76; break;
                case 5: keyCode = 0x60; break;
                case 6: keyCode = 0x61; break;
                case 7: keyCode = 0x62; break;
                case 8: keyCode = 0x64; break;
                case 9: keyCode = 0x65; break;
                case 10: keyCode = 0x6D; break;
                case 11: keyCode = 0x67; break;
                case 12: keyCode = 0x6F; break;
                case 13: keyCode = 0x69; break;
                case 14: keyCode = 0x6B; break;
                case 15: keyCode = 0x71; break;
                case 16: keyCode = 0x6A; break;
                case 17: keyCode = 0x40; break;
                case 18: keyCode = 0x4F; break;
                case 19: keyCode = 0x50; break;
                case 20: keyCode = 0x5A; break;
            }
        }
    }

    // FIXME: No keyCode is set for most keys.
    if ([character isEqualToString:@"\t"])
        keyCode = 0x30;
    else if ([character isEqualToString:@" "])
        keyCode = 0x31;
    else if ([character isEqualToString:@"\r"])
        keyCode = 0x24;
    else if ([character isEqualToString:@"\n"])
        keyCode = 0x4C;
    else if ([character isEqualToString:@"\x8"])
        keyCode = 0x33;
    else if ([character isEqualToString:@"a"])
        keyCode = 0x00;
    else if ([character isEqualToString:@"b"])
        keyCode = 0x0B;
    else if ([character isEqualToString:@"d"])
        keyCode = 0x02;
    else if ([character isEqualToString:@"e"])
        keyCode = 0x0E;

    KeyMappingEntry table[] = {
        {0x2F, 0x41, '.', nil},
        {0,    0x43, '*', nil},
        {0,    0x45, '+', nil},
        {0,    0x47, NSClearLineFunctionKey, @"clear"},
        {0x2C, 0x4B, '/', nil},
        {0,    0x4C, 3, @"enter" },
        {0x1B, 0x4E, '-', nil},
        {0x18, 0x51, '=', nil},
        {0x1D, 0x52, '0', nil},
        {0x12, 0x53, '1', nil},
        {0x13, 0x54, '2', nil},
        {0x14, 0x55, '3', nil},
        {0x15, 0x56, '4', nil},
        {0x17, 0x57, '5', nil},
        {0x16, 0x58, '6', nil},
        {0x1A, 0x59, '7', nil},
        {0x1C, 0x5B, '8', nil},
        {0x19, 0x5C, '9', nil},
    };
    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(table); ++i) {
        NSString* currentCharacterString = [NSString stringWithCharacters:&table[i].character length:1];
        if ([character isEqualToString:currentCharacterString] || [character isEqualToString:table[i].characterName]) {
            if (keyLocation == 0x03 /*DOM_KEY_LOCATION_NUMPAD*/)
                keyCode = table[i].macNumpadKeyCode;
            else
                keyCode = table[i].macKeyCode;
            eventCharacter = currentCharacterString;
            break;
        }
    }

    NSString *charactersIgnoringModifiers = eventCharacter;

    int modifierFlags = 0;

    if ([character length] == 1 && [character characterAtIndex:0] >= 'A' && [character characterAtIndex:0] <= 'Z') {
        modifierFlags |= NSShiftKeyMask;
        charactersIgnoringModifiers = [character lowercaseString];
    }

    modifierFlags |= buildModifierFlags(modifiers);

    if (keyLocation == 0x03 /*DOM_KEY_LOCATION_NUMPAD*/)
        modifierFlags |= NSNumericPadKeyMask;

    // FIXME: [[[mainFrame frameView] documentView] layout];

    NSEvent *event = [NSEvent keyEventWithType:NSKeyDown
                        location:NSMakePoint(5, 5)
                        modifierFlags:modifierFlags
                        timestamp:absoluteTimeForEventTime(currentEventTime())
                        windowNumber:[m_testController->mainWebView()->platformWindow() windowNumber]
                        context:[NSGraphicsContext currentContext]
                        characters:eventCharacter
                        charactersIgnoringModifiers:charactersIgnoringModifiers
                        isARepeat:NO
                        keyCode:keyCode];

    [[m_testController->mainWebView()->platformWindow() firstResponder] keyDown:event];

    event = [NSEvent keyEventWithType:NSKeyUp
                        location:NSMakePoint(5, 5)
                        modifierFlags:modifierFlags
                        timestamp:absoluteTimeForEventTime(currentEventTime())
                        windowNumber:[m_testController->mainWebView()->platformWindow() windowNumber]
                        context:[NSGraphicsContext currentContext]
                        characters:eventCharacter
                        charactersIgnoringModifiers:charactersIgnoringModifiers
                        isARepeat:NO
                        keyCode:keyCode];

    [[m_testController->mainWebView()->platformWindow() firstResponder] keyUp:event];
}

void EventSenderProxy::mouseScrollBy(int x, int y)
{
    RetainPtr<CGEventRef> cgScrollEvent = adoptCF(CGEventCreateScrollWheelEvent(0, kCGScrollEventUnitLine, 2, y, x));

    // CGEvent locations are in global display coordinates.
    CGPoint lastGlobalMousePosition = CGPointMake(m_position.x, [[NSScreen mainScreen] frame].size.height - m_position.y);
    CGEventSetLocation(cgScrollEvent.get(), lastGlobalMousePosition);

    NSEvent *event = [NSEvent eventWithCGEvent:cgScrollEvent.get()];
    if (NSView *targetView = [m_testController->mainWebView()->platformView() hitTest:[event locationInWindow]])
        [targetView scrollWheel:event];
}

void EventSenderProxy::continuousMouseScrollBy(int x, int y, bool paged)
{
    // FIXME: Implement this.
    return;
}

} // namespace WTR
