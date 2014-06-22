/*
 * Copyright (C) 2005, 2006, 2007 Apple, Inc.  All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
 *           (C) 2007 Eric Seidel <eric@webkit.org>
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
 
#import "config.h"
#import "DumpRenderTreeWindow.h"

#import "DumpRenderTree.h"

// FIXME: This file is ObjC++ only because of this include. :(
#import "TestRunner.h"
#import <WebKit/WebViewPrivate.h>
#import <WebKit/WebTypesInternal.h>

CFMutableArrayRef openWindowsRef = 0;

static CFArrayCallBacks NonRetainingArrayCallbacks = {
    0,
    NULL,
    NULL,
    CFCopyDescription,
    CFEqual
};

@implementation DumpRenderTreeWindow

+ (NSArray *)openWindows
{
    return [[(NSArray *)openWindowsRef copy] autorelease];
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)styleMask backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
    if (!openWindowsRef)
        openWindowsRef = CFArrayCreateMutable(NULL, 0, &NonRetainingArrayCallbacks);

    CFArrayAppendValue(openWindowsRef, self);
            
    return [super initWithContentRect:contentRect styleMask:styleMask backing:bufferingType defer:deferCreation];
}

- (void)close
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    CFRange arrayRange = CFRangeMake(0, CFArrayGetCount(openWindowsRef));
    CFIndex i = CFArrayGetFirstIndexOfValue(openWindowsRef, arrayRange, self);
    if (i != kCFNotFound)
        CFArrayRemoveValueAtIndex(openWindowsRef, i);

    [super close];
}

- (BOOL)isKeyWindow
{
    return gTestRunner ? gTestRunner->windowIsKey() : YES;
}

- (BOOL)_hasKeyAppearance
{
    return [self isKeyWindow];
}

- (void)keyDown:(NSEvent *)event
{
    // Do nothing, avoiding the beep we'd otherwise get from NSResponder,
    // once we get to the end of the responder chain.
}

- (WebView *)webView
{
    NSView *firstView = nil;
    if ([[[self contentView] subviews] count] > 0) {
        firstView = [[[self contentView] subviews] objectAtIndex:0];
        if ([firstView isKindOfClass:[WebView class]])
            return static_cast<WebView *>(firstView);
    }
    return nil;
}

- (void)startListeningForAcceleratedCompositingChanges
{
    [[self webView] _setPostsAcceleratedCompositingNotifications:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(webViewStartedAcceleratedCompositing:)
        name:_WebViewDidStartAcceleratedCompositingNotification object:nil];
}

- (void)webViewStartedAcceleratedCompositing:(NSNotification *)notification
{
    // If the WebView has gone into compositing mode, turn on window autodisplay. This is necessary for CA
    // to update layers and start animations.
    // We only ever turn autodisplay on here, because we turn it off before every test.
    if ([[self webView] _isUsingAcceleratedCompositing])
        [self setAutodisplay:YES];
}

- (CGFloat)backingScaleFactor
{
    return 1;
}

@end
