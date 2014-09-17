/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#import "PlatformScreen.h"

#import "FloatRect.h"
#import "Frame.h"
#import "FrameView.h"
#import "Page.h"

namespace WebCore {

int screenDepth(Widget*)
{
    return NSBitsPerPixelFromDepth([[NSScreen deepestScreen] depth]);
}

int screenDepthPerComponent(Widget*)
{
    return NSBitsPerSampleFromDepth([[NSScreen deepestScreen] depth]);
}

bool screenIsMonochrome(Widget*)
{
    return false;
}

// These functions scale between screen and page coordinates because JavaScript/DOM operations 
// assume that the screen and the page share the same coordinate system.

FloatRect screenRect(Widget* widget)
{
    NSWindow *window = widget ? [widget->platformWidget() window] : nil;
    return toUserSpace([screenForWindow(window) frame], window);
}

FloatRect screenAvailableRect(Widget* widget)
{
    NSWindow *window = widget ? [widget->platformWidget() window] : nil;
    return toUserSpace([screenForWindow(window) visibleFrame], window);
}

NSScreen *screenForWindow(NSWindow *window)
{
    NSScreen *screen = [window screen]; // nil if the window is off-screen
    if (screen)
        return screen;
    
    NSArray *screens = [NSScreen screens];
    if ([screens count] > 0)
        return [screens objectAtIndex:0]; // screen containing the menubar
    
    return nil;
}

static CGFloat windowScaleFactor(NSWindow *window)
{
    return [window backingScaleFactor];
}

FloatRect toUserSpace(const NSRect& rect, NSWindow *destination)
{
    FloatRect userRect = rect;
    userRect.setY(NSMaxY([screenForWindow(destination) frame]) - (userRect.y() + userRect.height())); // flip
    if (destination)
        userRect.scale(1 / windowScaleFactor(destination)); // scale down
    return userRect;
}

NSRect toDeviceSpace(const FloatRect& rect, NSWindow *source)
{
    FloatRect deviceRect = rect;
    if (source)
        deviceRect.scale(windowScaleFactor(source)); // scale up
    deviceRect.setY(NSMaxY([screenForWindow(source) frame]) - (deviceRect.y() + deviceRect.height())); // flip
    return deviceRect;
}

NSPoint flipScreenPoint(const NSPoint& screenPoint, NSScreen *screen)
{
    NSPoint flippedPoint = screenPoint;
    flippedPoint.y = NSMaxY([screen frame]) - flippedPoint.y;
    return flippedPoint;
}

} // namespace WebCore
