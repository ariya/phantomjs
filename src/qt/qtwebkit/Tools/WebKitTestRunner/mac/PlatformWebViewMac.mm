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
#include "PlatformWebView.h"
#include "TestController.h"

#import <WebKit2/WKImageCG.h>
#import <WebKit2/WKViewPrivate.h>
#import <wtf/RetainPtr.h>

@interface WebKitTestRunnerWindow : NSWindow {
    WTR::PlatformWebView* _platformWebView;
    NSPoint _fakeOrigin;
}
@property (nonatomic, assign) WTR::PlatformWebView* platformWebView;
@end

@interface TestRunnerWKView : WKView {
    BOOL _useTiledDrawing;
}

- (id)initWithFrame:(NSRect)frame contextRef:(WKContextRef)context pageGroupRef:(WKPageGroupRef)pageGroup relatedToPage:(WKPageRef)relatedPage useTiledDrawing:(BOOL)useTiledDrawing;

@property (nonatomic, assign) BOOL useTiledDrawing;
@end

@implementation TestRunnerWKView

@synthesize useTiledDrawing = _useTiledDrawing;

- (id)initWithFrame:(NSRect)frame contextRef:(WKContextRef)context pageGroupRef:(WKPageGroupRef)pageGroup relatedToPage:(WKPageRef)relatedPage useTiledDrawing:(BOOL)useTiledDrawing
{
    _useTiledDrawing = useTiledDrawing;
    return [super initWithFrame:frame contextRef:context pageGroupRef:pageGroup relatedToPage:relatedPage];
}

- (BOOL)_shouldUseTiledDrawingArea
{
    return _useTiledDrawing;
}

@end

@implementation WebKitTestRunnerWindow
@synthesize platformWebView = _platformWebView;

- (BOOL)isKeyWindow
{
    return _platformWebView ? _platformWebView->windowIsKey() : YES;
}

- (void)setFrameOrigin:(NSPoint)point
{
    _fakeOrigin = point;
}

- (void)setFrame:(NSRect)windowFrame display:(BOOL)displayViews animate:(BOOL)performAnimation
{
    NSRect currentFrame = [super frame];

    _fakeOrigin = windowFrame.origin;

    [super setFrame:NSMakeRect(currentFrame.origin.x, currentFrame.origin.y, windowFrame.size.width, windowFrame.size.height) display:displayViews animate:performAnimation];
}

- (void)setFrame:(NSRect)windowFrame display:(BOOL)displayViews
{
    NSRect currentFrame = [super frame];

    _fakeOrigin = windowFrame.origin;

    [super setFrame:NSMakeRect(currentFrame.origin.x, currentFrame.origin.y, windowFrame.size.width, windowFrame.size.height) display:displayViews];
}

- (NSRect)frameRespectingFakeOrigin
{
    NSRect currentFrame = [self frame];
    return NSMakeRect(_fakeOrigin.x, _fakeOrigin.y, currentFrame.size.width, currentFrame.size.height);
}

- (CGFloat)backingScaleFactor
{
    return 1;
}

@end

@interface NSWindow (Details)

- (void)_setWindowResolution:(CGFloat)resolution displayIfChanged:(BOOL)displayIfChanged;

@end

namespace WTR {

PlatformWebView::PlatformWebView(WKContextRef contextRef, WKPageGroupRef pageGroupRef, WKPageRef relatedPage, WKDictionaryRef options)
    : m_windowIsKey(true)
    , m_options(options)
{
    WKRetainPtr<WKStringRef> useTiledDrawingKey(AdoptWK, WKStringCreateWithUTF8CString("TiledDrawing"));
    WKTypeRef useTiledDrawingValue = options ? WKDictionaryGetItemForKey(options, useTiledDrawingKey.get()) : NULL;
    bool useTiledDrawing = useTiledDrawingValue && WKBooleanGetValue(static_cast<WKBooleanRef>(useTiledDrawingValue));

    NSRect rect = NSMakeRect(0, 0, TestController::viewWidth, TestController::viewHeight);
    m_view = [[TestRunnerWKView alloc] initWithFrame:rect contextRef:contextRef pageGroupRef:pageGroupRef relatedToPage:relatedPage useTiledDrawing:useTiledDrawing];
    [m_view setWindowOcclusionDetectionEnabled:NO];

    NSRect windowRect = NSOffsetRect(rect, -10000, [(NSScreen *)[[NSScreen screens] objectAtIndex:0] frame].size.height - rect.size.height + 10000);
    m_window = [[WebKitTestRunnerWindow alloc] initWithContentRect:windowRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];
    m_window.platformWebView = this;
    [m_window setColorSpace:[[NSScreen mainScreen] colorSpace]];
    [m_window setCollectionBehavior:NSWindowCollectionBehaviorStationary];
    [[m_window contentView] addSubview:m_view];
    [m_window orderBack:nil];
    [m_window setReleasedWhenClosed:NO];
    [m_window _setWindowResolution:1 displayIfChanged:YES];
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    NSRect windowFrame = [m_window frame];
    windowFrame.size.width = width;
    windowFrame.size.height = height;
    [m_window setFrame:windowFrame display:YES];
    [m_view setFrame:NSMakeRect(0, 0, width, height)];
}

PlatformWebView::~PlatformWebView()
{
    m_window.platformWebView = 0;
    [m_window close];
    [m_window release];
    [m_view release];
}

WKPageRef PlatformWebView::page()
{
    return [m_view pageRef];
}

void PlatformWebView::focus()
{
    [m_window makeFirstResponder:m_view];
    setWindowIsKey(true);
}

WKRect PlatformWebView::windowFrame()
{
    NSRect frame = [m_window frameRespectingFakeOrigin];

    WKRect wkFrame;
    wkFrame.origin.x = frame.origin.x;
    wkFrame.origin.y = frame.origin.y;
    wkFrame.size.width = frame.size.width;
    wkFrame.size.height = frame.size.height;
    return wkFrame;
}

void PlatformWebView::setWindowFrame(WKRect frame)
{
    [m_window setFrame:NSMakeRect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height) display:YES];
}

void PlatformWebView::didInitializeClients()
{
    // Set a temporary 1x1 window frame to force a WindowAndViewFramesChanged notification. <rdar://problem/13380145>
    WKRect wkFrame = windowFrame();
    [m_window setFrame:NSMakeRect(0, 0, 1, 1) display:YES];
    setWindowFrame(wkFrame);
}

void PlatformWebView::addChromeInputField()
{
    NSTextField* textField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 20)];
    textField.tag = 1;
    [[m_window contentView] addSubview:textField];
    [textField release];

    [textField setNextKeyView:m_view];
    [m_view setNextKeyView:textField];
}

void PlatformWebView::removeChromeInputField()
{
    NSView* textField = [[m_window contentView] viewWithTag:1];
    if (textField) {
        [textField removeFromSuperview];
        makeWebViewFirstResponder();
    }
}

void PlatformWebView::makeWebViewFirstResponder()
{
    [m_window makeFirstResponder:m_view];
}

WKRetainPtr<WKImageRef> PlatformWebView::windowSnapshotImage()
{
    [m_view display];
    RetainPtr<CGImageRef> windowSnapshotImage = adoptCF(CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, [m_window windowNumber], kCGWindowImageBoundsIgnoreFraming | kCGWindowImageShouldBeOpaque));

    // windowSnapshotImage will be in GenericRGB, as we've set the main display's color space to GenericRGB.
    return adoptWK(WKImageCreateFromCGImage(windowSnapshotImage.get(), 0));
}

bool PlatformWebView::viewSupportsOptions(WKDictionaryRef options) const
{
    WKRetainPtr<WKStringRef> useTiledDrawingKey(AdoptWK, WKStringCreateWithUTF8CString("TiledDrawing"));
    WKTypeRef useTiledDrawingValue = WKDictionaryGetItemForKey(options, useTiledDrawingKey.get());
    bool useTiledDrawing = useTiledDrawingValue && WKBooleanGetValue(static_cast<WKBooleanRef>(useTiledDrawingValue));

    return useTiledDrawing == [(TestRunnerWKView *)m_view useTiledDrawing];
}

} // namespace WTR
