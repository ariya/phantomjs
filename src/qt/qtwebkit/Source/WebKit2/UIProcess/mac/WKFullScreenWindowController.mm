/*
 * Copyright (C) 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#if ENABLE(FULLSCREEN_API)

#import "WKFullScreenWindowController.h"

#import "LayerTreeContext.h"
#import "WKAPICast.h"
#import "WKViewInternal.h"
#import "WKViewPrivate.h"
#import "WebFullScreenManagerProxy.h"
#import "WebPageProxy.h"
#import <QuartzCore/QuartzCore.h>
#import <WebCore/DisplaySleepDisabler.h>
#import <WebCore/FloatRect.h>
#import <WebCore/IntRect.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/WebCoreFullScreenPlaceholderView.h>
#import <WebCore/WebCoreFullScreenWindow.h>
#import <WebCore/WebWindowAnimation.h>
#import <WebKit/WebNSWindowExtras.h>
#import <WebKitSystemInterface.h>

using namespace WebKit;
using namespace WebCore;

static RetainPtr<NSWindow> createBackgroundFullscreenWindow(NSRect frame);

static const CFTimeInterval defaultAnimationDuration = 0.5;
static const NSTimeInterval DefaultWatchdogTimerInterval = 1;

enum FullScreenState : NSInteger {
    NotInFullScreen,
    WaitingToEnterFullScreen,
    EnteringFullScreen,
    InFullScreen,
    WaitingToExitFullScreen,
    ExitingFullScreen,
};

@interface NSWindow (WebNSWindowDetails)
- (void)exitFullScreenMode:(id)sender;
- (void)enterFullScreenMode:(id)sender;
@end

@interface WKFullScreenWindowController(Private)<NSAnimationDelegate>
- (void)_replaceView:(NSView*)view with:(NSView*)otherView;
- (WebPageProxy*)_page;
- (WebFullScreenManagerProxy*)_manager;
- (void)_startEnterFullScreenAnimationWithDuration:(NSTimeInterval)duration;
- (void)_startExitFullScreenAnimationWithDuration:(NSTimeInterval)duration;
@end

static NSRect convertRectToScreen(NSWindow *window, NSRect rect)
{
    return [window convertRectToScreen:rect];
}

@interface NSWindow(IsOnActiveSpaceAdditionForTigerAndLeopard)
- (BOOL)isOnActiveSpace;
@end

@implementation WKFullScreenWindowController

#pragma mark -
#pragma mark Initialization
- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (!self)
        return nil;
    [window setDelegate:self];
    [window setCollectionBehavior:([window collectionBehavior] | NSWindowCollectionBehaviorFullScreenPrimary)];
    [self windowDidLoad];
    
    return self;
}

- (void)dealloc
{
    [self setWebView:nil];
    [[self window] setDelegate:nil];
    
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)windowDidLoad
{
    [super windowDidLoad];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidChangeScreenParameters:) name:NSApplicationDidChangeScreenParametersNotification object:NSApp];
}

#pragma mark -
#pragma mark Accessors

- (WKView*)webView
{
    return _webView;
}

- (void)setWebView:(WKView *)webView
{
    [webView retain];
    [_webView release];
    _webView = webView;
}

- (BOOL)isFullScreen
{
    return _fullScreenState == WaitingToEnterFullScreen
        || _fullScreenState == EnteringFullScreen
        || _fullScreenState == InFullScreen;
}

- (WebCoreFullScreenPlaceholderView*)webViewPlaceholder
{
    return _webViewPlaceholder.get();
}

#pragma mark -
#pragma mark NSWindowController overrides

- (void)cancelOperation:(id)sender
{
    // If the page doesn't respond in DefaultWatchdogTimerInterval seconds, it could be because
    // the WebProcess has hung, so exit anyway.
    if (!_watchdogTimer) {
        [self _manager]->requestExitFullScreen();
        _watchdogTimer = adoptNS([[NSTimer alloc] initWithFireDate:nil interval:DefaultWatchdogTimerInterval target:self selector:@selector(exitFullScreen) userInfo:nil repeats:NO]);
        [[NSRunLoop mainRunLoop] addTimer:_watchdogTimer.get() forMode:NSDefaultRunLoopMode];
    }
}

#pragma mark -
#pragma mark Notifications

- (void)applicationDidChangeScreenParameters:(NSNotification*)notification
{
    // The user may have changed the main screen by moving the menu bar, or they may have changed
    // the Dock's size or location, or they may have changed the fullScreen screen's dimensions. 
    // Update our presentation parameters, and ensure that the full screen window occupies the 
    // entire screen:
    NSWindow* window = [self window];
    NSRect screenFrame = [[window screen] frame];
    [window setFrame:screenFrame display:YES];
    [_backgroundWindow.get() setFrame:screenFrame display:YES];
}

#pragma mark -
#pragma mark Exposed Interface

static RetainPtr<CGDataProviderRef> createImageProviderWithCopiedData(CGDataProviderRef sourceProvider)
{
    RetainPtr<CFDataRef> data = adoptCF(CGDataProviderCopyData(sourceProvider));
    return adoptCF(CGDataProviderCreateWithCFData(data.get()));
}

static RetainPtr<CGImageRef> createImageWithCopiedData(CGImageRef sourceImage)
{
    size_t width = CGImageGetWidth(sourceImage);
    size_t height = CGImageGetHeight(sourceImage);
    size_t bitsPerComponent = CGImageGetBitsPerComponent(sourceImage);
    size_t bitsPerPixel = CGImageGetBitsPerPixel(sourceImage);
    size_t bytesPerRow = CGImageGetBytesPerRow(sourceImage);
    CGColorSpaceRef colorSpace = CGImageGetColorSpace(sourceImage);
    CGBitmapInfo bitmapInfo = CGImageGetBitmapInfo(sourceImage);
    RetainPtr<CGDataProviderRef> provider = createImageProviderWithCopiedData(CGImageGetDataProvider(sourceImage));
    bool shouldInterpolate = CGImageGetShouldInterpolate(sourceImage);
    CGColorRenderingIntent intent = CGImageGetRenderingIntent(sourceImage);

    return adoptCF(CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow, colorSpace, bitmapInfo, provider.get(), 0, shouldInterpolate, intent));
}

- (void)enterFullScreen:(NSScreen *)screen
{
    if ([self isFullScreen])
        return;
    _fullScreenState = WaitingToEnterFullScreen;

    if (!screen)
        screen = [NSScreen mainScreen];
    NSRect screenFrame = [screen frame];

    NSRect webViewFrame = convertRectToScreen([_webView window], [_webView convertRect:[_webView frame] toView:nil]);

    // Flip coordinate system:
    webViewFrame.origin.y = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - NSMaxY(webViewFrame);

    CGWindowID windowID = [[_webView window] windowNumber];
    RetainPtr<CGImageRef> webViewContents = adoptCF(CGWindowListCreateImage(NSRectToCGRect(webViewFrame), kCGWindowListOptionIncludingWindow, windowID, kCGWindowImageShouldBeOpaque));

    // Using the returned CGImage directly would result in calls to the WindowServer every time
    // the image was painted. Instead, copy the image data into our own process to eliminate that
    // future overhead.
    webViewContents = createImageWithCopiedData(webViewContents.get());

    // Screen updates to be re-enabled in _startEnterFullScreenAnimationWithDuration:
    NSDisableScreenUpdates();
    [[self window] setAutodisplay:NO];

    NSResponder *webWindowFirstResponder = [[_webView window] firstResponder];
    [self _manager]->saveScrollPosition();
    [[self window] setFrame:screenFrame display:NO];

    // Painting is normally suspended when the WKView is removed from the window, but this is
    // unnecessary in the full-screen animation case, and can cause bugs; see
    // https://bugs.webkit.org/show_bug.cgi?id=88940 and https://bugs.webkit.org/show_bug.cgi?id=88374
    // We will resume the normal behavior in _startEnterFullScreenAnimationWithDuration:
    [_webView _setSuppressVisibilityUpdates:YES];

    // Swap the webView placeholder into place.
    if (!_webViewPlaceholder) {
        _webViewPlaceholder = adoptNS([[WebCoreFullScreenPlaceholderView alloc] initWithFrame:[_webView frame]]);
        [_webViewPlaceholder.get() setAction:@selector(cancelOperation:)];
    }
    [_webViewPlaceholder.get() setTarget:nil];
    [_webViewPlaceholder.get() setContents:(id)webViewContents.get()];
    [self _replaceView:_webView with:_webViewPlaceholder.get()];
    
    // Then insert the WebView into the full screen window
    NSView* contentView = [[self window] contentView];
    [contentView addSubview:_webView positioned:NSWindowBelow relativeTo:nil];
    [_webView setFrame:[contentView bounds]];

    [[self window] makeResponder:webWindowFirstResponder firstResponderIfDescendantOfView:_webView];

    [self _manager]->setAnimatingFullScreen(true);
    [self _manager]->willEnterFullScreen();
    _savedScale = [self _page]->pageScaleFactor();
    [self _page]->scalePage(1, IntPoint());
}

- (void)beganEnterFullScreenWithInitialFrame:(const WebCore::IntRect&)initialFrame finalFrame:(const WebCore::IntRect&)finalFrame
{
    if (_fullScreenState != WaitingToEnterFullScreen)
        return;
    _fullScreenState = EnteringFullScreen;

    _initialFrame = initialFrame;
    _finalFrame = finalFrame;

    if (!_backgroundWindow)
        _backgroundWindow = createBackgroundFullscreenWindow(NSZeroRect);

    [[self window] enterFullScreenMode:self];
}

- (void)finishedEnterFullScreenAnimation:(bool)completed
{
    if (_fullScreenState != EnteringFullScreen)
        return;
    _fullScreenState = InFullScreen;

    if (completed) {
        // Screen updates to be re-enabled ta the end of the current block.
        NSDisableScreenUpdates();
        [self _manager]->didEnterFullScreen();
        [self _manager]->setAnimatingFullScreen(false);

        NSRect windowBounds = [[self window] frame];
        windowBounds.origin = NSZeroPoint;
        WKWindowSetClipRect([self window], windowBounds);

        [_fadeAnimation.get() stopAnimation];
        [_fadeAnimation.get() setWindow:nil];
        _fadeAnimation = nullptr;
        
        [_backgroundWindow.get() orderOut:self];
        [_backgroundWindow.get() setFrame:NSZeroRect display:YES];

        [_webViewPlaceholder.get() setExitWarningVisible:YES];
        [_webViewPlaceholder.get() setTarget:self];
        NSEnableScreenUpdates();
    } else
        [_scaleAnimation.get() stopAnimation];
}

- (void)exitFullScreen
{
    if (_watchdogTimer) {
        [_watchdogTimer.get() invalidate];
        _watchdogTimer.clear();
    }

    if (![self isFullScreen])
        return;
    _fullScreenState = WaitingToExitFullScreen;

    [_webViewPlaceholder.get() setExitWarningVisible:NO];

    // Screen updates to be re-enabled in _startExitFullScreenAnimationWithDuration: or beganExitFullScreenWithInitialFrame:finalFrame:
    NSDisableScreenUpdates();
    [[self window] setAutodisplay:NO];

    // See the related comment in enterFullScreen:
    // We will resume the normal behavior in _startExitFullScreenAnimationWithDuration:
    [_webView _setSuppressVisibilityUpdates:YES];
    [_webViewPlaceholder.get() setTarget:nil];

    [self _manager]->setAnimatingFullScreen(true);
    [self _manager]->willExitFullScreen();
}

- (void)beganExitFullScreenWithInitialFrame:(const WebCore::IntRect&)initialFrame finalFrame:(const WebCore::IntRect&)finalFrame
{

    if (_fullScreenState != WaitingToExitFullScreen)
        return;
    _fullScreenState = ExitingFullScreen;

    if (![[self window] isOnActiveSpace]) {
        // If the full screen window is not in the active space, the NSWindow full screen animation delegate methods
        // will never be called. So call finishedExitFullScreenAnimation explicitly.
        [self finishedExitFullScreenAnimation:YES];

        // Because we are breaking the normal animation pattern, re-enable screen updates
        // as exitFullScreen has disabled them, but _startExitFullScreenAnimationWithDuration:
        // will never be called.
        NSEnableScreenUpdates();
    }

    [[self window] exitFullScreenMode:self];
}

static void completeFinishExitFullScreenAnimationAfterRepaint(WKErrorRef, void*);

- (void)finishedExitFullScreenAnimation:(bool)completed
{
    if (_fullScreenState != ExitingFullScreen)
        return;
    _fullScreenState = NotInFullScreen;

    // Screen updates to be re-enabled in completeFinishExitFullScreenAnimationAfterRepaint.
    NSDisableScreenUpdates();
    [[_webViewPlaceholder.get() window] setAutodisplay:NO];

    NSResponder *firstResponder = [[self window] firstResponder];
    [self _replaceView:_webViewPlaceholder.get() with:_webView];
    [[_webView window] makeResponder:firstResponder firstResponderIfDescendantOfView:_webView];

    NSRect windowBounds = [[self window] frame];
    windowBounds.origin = NSZeroPoint;
    WKWindowSetClipRect([self window], windowBounds);

    [[self window] orderOut:self];
    [[self window] setFrame:NSZeroRect display:YES];

    [_fadeAnimation.get() stopAnimation];
    [_fadeAnimation.get() setWindow:nil];
    _fadeAnimation = nullptr;

    [_backgroundWindow.get() orderOut:self];
    [_backgroundWindow.get() setFrame:NSZeroRect display:YES];

    [[_webView window] makeKeyAndOrderFront:self];

    // These messages must be sent after the swap or flashing will occur during forceRepaint:
    [self _manager]->didExitFullScreen();
    [self _manager]->setAnimatingFullScreen(false);
    [self _page]->scalePage(_savedScale, IntPoint());
    [self _manager]->restoreScrollPosition();
    [self _page]->forceRepaint(VoidCallback::create(self, completeFinishExitFullScreenAnimationAfterRepaint));
}

- (void)completeFinishExitFullScreenAnimationAfterRepaint
{
    [[_webView window] setAutodisplay:YES];
    [[_webView window] displayIfNeeded];
    NSEnableScreenUpdates();
}

static void completeFinishExitFullScreenAnimationAfterRepaint(WKErrorRef, void* _self)
{
    [(WKFullScreenWindowController*)_self completeFinishExitFullScreenAnimationAfterRepaint];
}

- (void)performClose:(id)sender
{
    if ([self isFullScreen])
        [self cancelOperation:sender];
}

- (void)close
{
    // We are being asked to close rapidly, most likely because the page 
    // has closed or the web process has crashed.  Just walk through our
    // normal exit full screen sequence, but don't wait to be called back
    // in response.
    if ([self isFullScreen])
        [self exitFullScreen];
    
    if (_fullScreenState == ExitingFullScreen)
        [self finishedExitFullScreenAnimation:YES];

    [super close];
}

#pragma mark -
#pragma mark Custom NSWindow Full Screen Animation

- (NSArray *)customWindowsToEnterFullScreenForWindow:(NSWindow *)window
{
    return [NSArray arrayWithObjects:[self window], _backgroundWindow.get(), nil];
}

- (NSArray *)customWindowsToExitFullScreenForWindow:(NSWindow *)window
{
    return [NSArray arrayWithObjects:[self window], _backgroundWindow.get(), nil];
}

- (void)window:(NSWindow *)window startCustomAnimationToEnterFullScreenWithDuration:(NSTimeInterval)duration
{
    [self _startEnterFullScreenAnimationWithDuration:duration];
}

- (void)window:(NSWindow *)window startCustomAnimationToExitFullScreenWithDuration:(NSTimeInterval)duration
{
    [self _startExitFullScreenAnimationWithDuration:duration];
}

- (void)windowDidFailToEnterFullScreen:(NSWindow *)window
{
    [self finishedEnterFullScreenAnimation:NO];
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification
{
    [self finishedEnterFullScreenAnimation:YES];
}

- (void)windowDidFailToExitFullScreen:(NSWindow *)window
{
    [self finishedExitFullScreenAnimation:NO];
}

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
    [self finishedExitFullScreenAnimation:YES];
}

#pragma mark -
#pragma mark Internal Interface

- (WebPageProxy*)_page
{
    return toImpl([_webView pageRef]);
}

- (WebFullScreenManagerProxy*)_manager
{
    WebPageProxy* webPage = [self _page];
    if (!webPage)
        return 0;
    return webPage->fullScreenManager();
}

- (void)_replaceView:(NSView*)view with:(NSView*)otherView
{
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    [otherView setFrame:[view frame]];        
    [otherView setAutoresizingMask:[view autoresizingMask]];
    [otherView removeFromSuperview];
    [[view superview] addSubview:otherView positioned:NSWindowAbove relativeTo:view];
    [view removeFromSuperview];
    [CATransaction commit];
}

static RetainPtr<NSWindow> createBackgroundFullscreenWindow(NSRect frame)
{
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
    [window setOpaque:YES];
    [window setBackgroundColor:[NSColor blackColor]];
    [window setReleasedWhenClosed:NO];
    return adoptNS(window);
}

static NSRect windowFrameFromApparentFrames(NSRect screenFrame, NSRect initialFrame, NSRect finalFrame)
{
    NSRect initialWindowFrame;
    if (!NSWidth(initialFrame) || !NSWidth(finalFrame) || !NSHeight(initialFrame) || !NSHeight(finalFrame))
        return screenFrame;

    CGFloat xScale = NSWidth(screenFrame) / NSWidth(finalFrame);
    CGFloat yScale = NSHeight(screenFrame) / NSHeight(finalFrame);
    CGFloat xTrans = NSMinX(screenFrame) - NSMinX(finalFrame);
    CGFloat yTrans = NSMinY(screenFrame) - NSMinY(finalFrame);
    initialWindowFrame.size = NSMakeSize(NSWidth(initialFrame) * xScale, NSHeight(initialFrame) * yScale);
    initialWindowFrame.origin = NSMakePoint
        ( NSMinX(initialFrame) + xTrans / (NSWidth(finalFrame) / NSWidth(initialFrame))
        , NSMinY(initialFrame) + yTrans / (NSHeight(finalFrame) / NSHeight(initialFrame)));
    return initialWindowFrame;
}

- (void)_startEnterFullScreenAnimationWithDuration:(NSTimeInterval)duration
{
    NSRect screenFrame = [[[self window] screen] frame];
    NSRect initialWindowFrame = windowFrameFromApparentFrames(screenFrame, _initialFrame, _finalFrame);
    
    _scaleAnimation = adoptNS([[WebWindowScaleAnimation alloc] initWithHintedDuration:duration window:[self window] initalFrame:initialWindowFrame finalFrame:screenFrame]);
    
    [_scaleAnimation.get() setAnimationBlockingMode:NSAnimationNonblocking];
    [_scaleAnimation.get() setCurrentProgress:0];
    [_scaleAnimation.get() startAnimation];

    // WKWindowSetClipRect takes window coordinates, so convert from screen coordinates here:
    NSRect finalBounds = _finalFrame;
    finalBounds.origin = [[self window] convertScreenToBase:finalBounds.origin];
    WKWindowSetClipRect([self window], finalBounds);

    NSWindow* window = [self window];
    NSWindowCollectionBehavior behavior = [window collectionBehavior];
    [window setCollectionBehavior:(behavior | NSWindowCollectionBehaviorCanJoinAllSpaces)];
    [window makeKeyAndOrderFront:self];
    [window setCollectionBehavior:behavior];


    if (!_backgroundWindow)
        _backgroundWindow = createBackgroundFullscreenWindow(screenFrame);
    else
        [_backgroundWindow.get() setFrame:screenFrame display:NO];

    CGFloat currentAlpha = 0;
    if (_fadeAnimation) {
        currentAlpha = [_fadeAnimation.get() currentAlpha];
        [_fadeAnimation.get() stopAnimation];
        [_fadeAnimation.get() setWindow:nil];
    }

    _fadeAnimation = adoptNS([[WebWindowFadeAnimation alloc] initWithDuration:duration 
                                                                     window:_backgroundWindow.get() 
                                                               initialAlpha:currentAlpha 
                                                                 finalAlpha:1]);
    [_fadeAnimation.get() setAnimationBlockingMode:NSAnimationNonblocking];
    [_fadeAnimation.get() setCurrentProgress:0];
    [_fadeAnimation.get() startAnimation];

    [_backgroundWindow.get() orderWindow:NSWindowBelow relativeTo:[[self window] windowNumber]];

    [_webView _setSuppressVisibilityUpdates:NO];
    [[self window] setAutodisplay:YES];
    [[self window] displayIfNeeded];
    NSEnableScreenUpdates();
}

- (void)_startExitFullScreenAnimationWithDuration:(NSTimeInterval)duration
{
    if ([self isFullScreen]) {
        // We still believe we're in full screen mode, so we must have been asked to exit full
        // screen by the system full screen button.
        [self _manager]->requestExitFullScreen();
        [self exitFullScreen];
        _fullScreenState = ExitingFullScreen;
    }

    NSRect screenFrame = [[[self window] screen] frame];
    NSRect initialWindowFrame = windowFrameFromApparentFrames(screenFrame, _initialFrame, _finalFrame);

    NSRect currentFrame = _scaleAnimation ? [_scaleAnimation.get() currentFrame] : [[self window] frame];
    _scaleAnimation = adoptNS([[WebWindowScaleAnimation alloc] initWithHintedDuration:duration window:[self window] initalFrame:currentFrame finalFrame:initialWindowFrame]);

    [_scaleAnimation.get() setAnimationBlockingMode:NSAnimationNonblocking];
    [_scaleAnimation.get() setCurrentProgress:0];
    [_scaleAnimation.get() startAnimation];

    if (!_backgroundWindow)
        _backgroundWindow = createBackgroundFullscreenWindow(screenFrame);
    else
        [_backgroundWindow.get() setFrame:screenFrame display:NO];

    CGFloat currentAlpha = 1;
    if (_fadeAnimation) {
        currentAlpha = [_fadeAnimation.get() currentAlpha];
        [_fadeAnimation.get() stopAnimation];
        [_fadeAnimation.get() setWindow:nil];
    }
    _fadeAnimation = adoptNS([[WebWindowFadeAnimation alloc] initWithDuration:duration 
                                                                     window:_backgroundWindow.get() 
                                                               initialAlpha:currentAlpha 
                                                                 finalAlpha:0]);
    [_fadeAnimation.get() setAnimationBlockingMode:NSAnimationNonblocking];
    [_fadeAnimation.get() setCurrentProgress:0];
    [_fadeAnimation.get() startAnimation];

    [_backgroundWindow.get() orderWindow:NSWindowBelow relativeTo:[[self window] windowNumber]];

    // WKWindowSetClipRect takes window coordinates, so convert from screen coordinates here:
    NSRect finalBounds = _finalFrame;
    finalBounds.origin = [[self window] convertScreenToBase:finalBounds.origin];
    WKWindowSetClipRect([self window], finalBounds);

    [_webView _setSuppressVisibilityUpdates:NO];
    [[self window] setAutodisplay:YES];
    [[self window] displayIfNeeded];
    NSEnableScreenUpdates();
}
@end

#endif
