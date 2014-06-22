/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LP64__

#import "CarbonWindowFrame.h"
#import "CarbonWindowAdapter.h"
#import "CarbonWindowContentView.h"
#import <Foundation/NSGeometry.h>
#import <Foundation/NSString.h>
#import <HIToolbox/MacWindows.h>

#import "WebTypesInternal.h"

@interface NSView(Secret)
- (void)_setWindow:(NSWindow *)window;
@end

@class NSButton;
/*

@interface NSThemeFrame(NSHijackedClassMethods)

+ (float)_titlebarHeight:(unsigned int)style;

@end
*/

@implementation CarbonWindowFrame


- (NSRect)titlebarRect
{
    NSRect      titlebarRect;
    NSRect      boundsRect;

    boundsRect = [self bounds];

    CarbonWindowAdapter *carbonWindow;
    carbonWindow = (CarbonWindowAdapter *)[self window];
    WindowRef windowRef = [carbonWindow windowRef];
    Rect globalBounds;
    GetWindowBounds (windowRef, kWindowTitleBarRgn, &globalBounds);
    
    titlebarRect.origin.x    = boundsRect.origin.x;
    titlebarRect.size.width  = boundsRect.size.width;
    titlebarRect.size.height = globalBounds.bottom - globalBounds.top;
    titlebarRect.origin.y    = NSMaxY(boundsRect) - titlebarRect.size.height;

    return titlebarRect;
}

// Given a content rectangle and style mask, return a corresponding frame rectangle.
+ (NSRect)frameRectForContentRect:(NSRect)contentRect styleMask:(NSUInteger)style {

    // We don't bother figuring out a good value, because content rects weren't so meaningful for NSCarbonWindows in the past, but this might not be a good assumption anymore.  M.P. Warning - 12/5/00
    return contentRect;

}

+ (NSRect)contentRectForFrameRect:(NSRect)frameRect styleMask:(NSUInteger)style {

    // We don't bother figuring out a good value, because content rects weren't so meaningful for NSCarbonWindows in the past, but this might not be a good assumption anymore.  KW - copied from +frameRectForContentRect:styleMask
    return frameRect;

}

+ (NSSize)minFrameSizeForMinContentSize:(NSSize)cSize styleMask:(NSUInteger)style {
    // See comments above.  We don't make any assumptions about the relationship between content rects and frame rects
    return cSize;
}

- (NSRect)frameRectForContentRect:(NSRect)cRect styleMask:(NSUInteger)style {
    return [[self class] frameRectForContentRect: cRect styleMask:style];
}
- (NSRect)contentRectForFrameRect:(NSRect)fRect styleMask:(NSUInteger)style {
    return [[self class] contentRectForFrameRect: fRect styleMask:style];
}
- (NSSize)minFrameSizeForMinContentSize:(NSSize)cSize styleMask:(NSUInteger)style {
    return [[self class] minFrameSizeForMinContentSize:cSize styleMask: style];
}

// Initialize.
- (id)initWithFrame:(NSRect)inFrameRect styleMask:(unsigned int)inStyleMask owner:(NSWindow *)inOwningWindow {

    // Parameter check.
    if (![inOwningWindow isKindOfClass:[CarbonWindowAdapter class]]) NSLog(@"CarbonWindowFrames can only be owned by CarbonWindowAdapters.");

    // Do the standard Cocoa thing.
    self = [super initWithFrame:inFrameRect];
    if (!self) return nil;

    // Record what we'll need later.
    _styleMask = inStyleMask;

    // Do what NSFrameView's method of the same name does.
    [self _setWindow:inOwningWindow];
    [self setNextResponder:inOwningWindow];

    // Done.
    return self;

}


// Deallocate.
- (void)dealloc {

    // Simple.
    [super dealloc];
    
}


// Sink a method invocation.
- (void)_setFrameNeedsDisplay:(BOOL)needsDisplay {
    
}


// Sink a method invocation.
- (void)_setSheet:(BOOL)sheetFlag {

}


// Sink a method invocation.
- (void)_updateButtonState {

}

#if 0
// Sink a method invocation.
- (void)_windowChangedKeyState {
}
#endif

// Toolbar methods that NSWindow expects to be there.
- (BOOL)_canHaveToolbar { return NO; }
- (BOOL)_toolbarIsInTransition { return NO; }
- (BOOL)_toolbarIsShown { return NO; }
- (BOOL)_toolbarIsHidden { return NO; }
- (void)_showToolbarWithAnimation:(BOOL)animate {}
- (void)_hideToolbarWithAnimation:(BOOL)animate {}
- (float)_distanceFromToolbarBaseToTitlebar { return 0; }


// Refuse to admit there's a close button on the window.
- (NSButton *)closeButton {

    // Simple.
    return nil;
}


// Return what's asked for.
- (unsigned int)styleMask {

    // Simple.
    return _styleMask;

}


// Return what's asked for.
- (NSRect)dragRectForFrameRect:(NSRect)frameRect {

    // Do what NSThemeFrame would do.
    // If we just return NSZeroRect here, _NXMakeWindowVisible() gets all befuddled in the sheet-showing case, a window-moving loop is entered, and the sheet gets moved right off of the screen.  M.P. Warning - 3/23/01
    NSRect dragRect;
    dragRect.size.height = 27;//[NSThemeFrame _titlebarHeight:[self styleMask]];
    dragRect.origin.y = NSMaxY(frameRect) - dragRect.size.height;
    dragRect.size.width = frameRect.size.width;
    dragRect.origin.x = frameRect.origin.x;
    return dragRect;
    
}


// Return what's asked for.
- (BOOL)isOpaque {

    // Return a value that will make -[NSWindow displayIfNeeded] on our Carbon window actually work.
    return YES;

}


// Refuse to admit there's a minimize button on the window.
- (NSButton *)minimizeButton {

    // Simple.
    return nil;

}


// Do the right thing for a Carbon window.
- (void)setTitle:(NSString *)title {

    CarbonWindowAdapter *carbonWindow;
    OSStatus osStatus;
    WindowRef windowRef;

    // Set the Carbon window's title.
    carbonWindow = (CarbonWindowAdapter *)[self window];
    windowRef = [carbonWindow windowRef];
    osStatus = SetWindowTitleWithCFString(windowRef, (CFStringRef)title);
    if (osStatus!=noErr) NSLog(@"A Carbon window's title couldn't be set.");

}


// Return what's asked for.
- (NSString *)title {

    CFStringRef windowTitle;
    CarbonWindowAdapter *carbonWindow;
    NSString *windowTitleAsNSString;
    OSStatus osStatus;
    WindowRef windowRef;

    // Return the Carbon window's title.
    carbonWindow = (CarbonWindowAdapter *)[self window];
    windowRef = [carbonWindow windowRef];
    osStatus = CopyWindowTitleAsCFString(windowRef, &windowTitle);
    if (osStatus==noErr) {
        windowTitleAsNSString = (NSString *)windowTitle;
    } else {
        NSLog(@"A Carbon window's title couldn't be gotten.");
        windowTitleAsNSString = @"";
    }
    return [windowTitleAsNSString autorelease];

}


// Return what's asked for.
- (float)_sheetHeightAdjustment {

    // Do what NSThemeFrame would do.
    return 22;//[NSThemeFrame _titlebarHeight:([self styleMask] & ~NSDocModalWindowMask)];
    
}

// Return what's asked for.
- (float)_maxTitlebarTitleRect {

    // Do what NSThemeFrame would do.
    return 22;//[NSThemeFrame _titlebarHeight:([self styleMask] & ~NSDocModalWindowMask)];
    
}

- (void)_clearDragMargins {
}

- (void)_resetDragMargins {
}


@end // implementation NSCarbonWindowFrame

#endif
