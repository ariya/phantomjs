/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#if ENABLE(VIDEO)

#import "WebVideoFullscreenController.h"

#import "WebVideoFullscreenHUDWindowController.h"
#import "WebWindowAnimation.h"
#import <AVFoundation/AVFoundation.h>
#import <Carbon/Carbon.h>
#import <QTKit/QTKit.h>
#import <WebCore/DisplaySleepDisabler.h>
#import <WebCore/HTMLMediaElement.h>
#import <WebCore/SoftLinking.h>
#import <objc/runtime.h>

using namespace WebCore;

SOFT_LINK_FRAMEWORK(QTKit)
SOFT_LINK_CLASS(QTKit, QTMovieLayer)

SOFT_LINK_POINTER(QTKit, QTMovieRateDidChangeNotification, NSString *)

#define QTMovieRateDidChangeNotification getQTMovieRateDidChangeNotification()

SOFT_LINK_FRAMEWORK(AVFoundation)
SOFT_LINK_CLASS(AVFoundation, AVPlayerLayer)

@interface WebVideoFullscreenWindow : NSWindow<NSAnimationDelegate>
{
    SEL _controllerActionOnAnimationEnd;
    WebWindowScaleAnimation *_fullscreenAnimation; // (retain)
}
- (void)animateFromRect:(NSRect)startRect toRect:(NSRect)endRect withSubAnimation:(NSAnimation *)subAnimation controllerAction:(SEL)controllerAction;
@end

@interface WebVideoFullscreenController(HUDWindowControllerDelegate) <WebVideoFullscreenHUDWindowControllerDelegate>
- (void)requestExitFullscreenWithAnimation:(BOOL)animation;
- (void)updateMenuAndDockForFullscreen;
- (void)updatePowerAssertions;
@end

@interface NSWindow(IsOnActiveSpaceAdditionForTigerAndLeopard)
- (BOOL)isOnActiveSpace;
@end

@implementation WebVideoFullscreenController
- (id)init
{
    // Do not defer window creation, to make sure -windowNumber is created (needed by WebWindowScaleAnimation).
    NSWindow *window = [[WebVideoFullscreenWindow alloc] initWithContentRect:NSZeroRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
    self = [super initWithWindow:window];
    [window release];
    if (!self)
        return nil;
    [self windowDidLoad];
    return self;
    
}
- (void)dealloc
{
    ASSERT(!_backgroundFullscreenWindow);
    ASSERT(!_fadeAnimation);
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (WebVideoFullscreenWindow *)fullscreenWindow
{
    return (WebVideoFullscreenWindow *)[super window];
}

- (void)setupVideoOverlay:(CALayer *)layer
{
    WebVideoFullscreenWindow *window = [self fullscreenWindow];
    [[window contentView] setLayer:layer];
    [[window contentView] setWantsLayer:YES];
}

- (void)windowDidLoad
{
    WebVideoFullscreenWindow *window = [self fullscreenWindow];
    [window setHasShadow:YES]; // This is nicer with a shadow.
    [window setLevel:NSPopUpMenuWindowLevel-1];
    [[window contentView] setLayer:[CALayer layer]];
    [[window contentView] setWantsLayer:YES];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidResignActive:) name:NSApplicationDidResignActiveNotification object:NSApp];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidChangeScreenParameters:) name:NSApplicationDidChangeScreenParametersNotification object:NSApp];
}

- (HTMLMediaElement*)mediaElement
{
    return _mediaElement.get();
}

- (void)setMediaElement:(HTMLMediaElement*)mediaElement
{
    _mediaElement = mediaElement;

    if (!_mediaElement)
        return;

    if ([self isWindowLoaded]) {
        if (_mediaElement->platformMedia().type == PlatformMedia::QTMovieType) {
            QTMovie *movie = _mediaElement->platformMedia().media.qtMovie;
            QTMovieLayer *layer = [[getQTMovieLayerClass() alloc] init];
            [layer setMovie:movie];
            [self setupVideoOverlay:layer];

            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(rateChanged:)
                                                         name:QTMovieRateDidChangeNotification
                                                       object:movie];

        } else if (_mediaElement->platformMedia().type == PlatformMedia::AVFoundationMediaPlayerType) {
            AVPlayer *player = _mediaElement->platformMedia().media.avfMediaPlayer;
            AVPlayerLayer *layer = [[getAVPlayerLayerClass() alloc] init];
            [self setupVideoOverlay:layer];
            [layer setPlayer:player];

            [player addObserver:self forKeyPath:@"rate" options:nil context:NULL];
        }
    }
}

- (id <WebVideoFullscreenControllerDelegate>)delegate
{
    return _delegate;
}

- (void)setDelegate:(id <WebVideoFullscreenControllerDelegate>)delegate
{
    _delegate = delegate;
}

- (CGFloat)clearFadeAnimation
{
    [_fadeAnimation stopAnimation];
    CGFloat previousAlpha = [_fadeAnimation currentAlpha];
    [_fadeAnimation setWindow:nil];
    [_fadeAnimation release];
    _fadeAnimation = nil;
    return previousAlpha;
}

- (void)windowDidExitFullscreen
{
    CALayer *layer = [[[self window] contentView] layer];
    if ([layer isKindOfClass:getAVPlayerLayerClass()])
        [[(AVPlayerLayer*)layer player] removeObserver:self forKeyPath:@"rate"];

    [self clearFadeAnimation];
    [[self window] close];
    [self setWindow:nil];
    [self updateMenuAndDockForFullscreen];   
    [self updatePowerAssertions];
    [_hudController setDelegate:nil];
    [_hudController release];
    _hudController = nil;
    [_backgroundFullscreenWindow close];
    [_backgroundFullscreenWindow release];
    _backgroundFullscreenWindow = nil;
    
    [self autorelease]; // Associated -retain is in -exitFullscreen.
    _isEndingFullscreen = NO;
}

- (void)windowDidEnterFullscreen
{
    [self clearFadeAnimation];

    ASSERT(!_hudController);
    _hudController = [[WebVideoFullscreenHUDWindowController alloc] init];
    [_hudController setDelegate:self];

    [self updateMenuAndDockForFullscreen];
    [self updatePowerAssertions];
    [NSCursor setHiddenUntilMouseMoves:YES];
    
    // Give the HUD keyboard focus initially
    [_hudController fadeWindowIn];
}

- (NSRect)mediaElementRect
{
    return _mediaElement->screenRect();
}

- (void)applicationDidResignActive:(NSNotification*)notification
{   
    UNUSED_PARAM(notification);
    // Check to see if the fullscreenWindow is on the active space; this function is available
    // on 10.6 and later, so default to YES if the function is not available:
    NSWindow* fullscreenWindow = [self fullscreenWindow];
    BOOL isOnActiveSpace = ([fullscreenWindow respondsToSelector:@selector(isOnActiveSpace)] ? [fullscreenWindow isOnActiveSpace] : YES);

    // Replicate the QuickTime Player (X) behavior when losing active application status:
    // Is the fullscreen screen the main screen? (Note: this covers the case where only a 
    // single screen is available.)  Is the fullscreen screen on the current space? IFF so, 
    // then exit fullscreen mode.    
    if ([fullscreenWindow screen] == [[NSScreen screens] objectAtIndex:0] && isOnActiveSpace)
         [self requestExitFullscreenWithAnimation:NO];
}
         
         
// MARK: -
// MARK: Exposed Interface

static void constrainFrameToRatioOfFrame(NSRect *frameToConstrain, const NSRect *frame)
{
    // Keep a constrained aspect ratio for the destination window
    CGFloat originalRatio = frame->size.width / frame->size.height;
    CGFloat newRatio = frameToConstrain->size.width / frameToConstrain->size.height;
    if (newRatio > originalRatio) {
        CGFloat newWidth = originalRatio * frameToConstrain->size.height;
        CGFloat diff = frameToConstrain->size.width - newWidth;
        frameToConstrain->size.width = newWidth;
        frameToConstrain->origin.x += diff / 2;
    } else {
        CGFloat newHeight = frameToConstrain->size.width / originalRatio;
        CGFloat diff = frameToConstrain->size.height - newHeight;
        frameToConstrain->size.height = newHeight;
        frameToConstrain->origin.y += diff / 2;
    }    
}

static NSWindow *createBackgroundFullscreenWindow(NSRect frame, int level)
{
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
    [window setOpaque:YES];
    [window setBackgroundColor:[NSColor blackColor]];
    [window setLevel:level];
    [window setReleasedWhenClosed:NO];
    return window;
}

- (void)setupFadeAnimationIfNeededAndFadeIn:(BOOL)fadeIn
{
    CGFloat initialAlpha = fadeIn ? 0 : 1;
    if (_fadeAnimation) {
        // Make sure we support queuing animation if the previous one isn't over yet
        initialAlpha = [self clearFadeAnimation];
    }
    if (!_forceDisableAnimation)
        _fadeAnimation = [[WebWindowFadeAnimation alloc] initWithDuration:0.2 window:_backgroundFullscreenWindow initialAlpha:initialAlpha finalAlpha:fadeIn ? 1 : 0];
}

- (void)enterFullscreen:(NSScreen *)screen
{
    if (!screen)
        screen = [NSScreen mainScreen];

    NSRect frame = [self mediaElementRect];
    NSRect endFrame = [screen frame];
    constrainFrameToRatioOfFrame(&endFrame, &frame);

    // Create a black window if needed
    if (!_backgroundFullscreenWindow)
        _backgroundFullscreenWindow = createBackgroundFullscreenWindow([screen frame], [[self window] level]-1);
    else
        [_backgroundFullscreenWindow setFrame:[screen frame] display:NO];

    [self setupFadeAnimationIfNeededAndFadeIn:YES];
    if (_forceDisableAnimation) {
        // This will disable scale animation
        frame = NSZeroRect;
    }
    [[self fullscreenWindow] animateFromRect:frame toRect:endFrame withSubAnimation:_fadeAnimation controllerAction:@selector(windowDidEnterFullscreen)];

    [_backgroundFullscreenWindow orderWindow:NSWindowBelow relativeTo:[[self fullscreenWindow] windowNumber]];
}

- (void)exitFullscreen
{
    if (_isEndingFullscreen)
        return;
    _isEndingFullscreen = YES;
    [_hudController closeWindow];

    NSRect endFrame = [self mediaElementRect];

    [self setupFadeAnimationIfNeededAndFadeIn:NO];
    if (_forceDisableAnimation) {
        // This will disable scale animation
        endFrame = NSZeroRect;
    }
    
    // We have to retain ourselves because we want to be alive for the end of the animation.
    // If our owner releases us we could crash if this is not the case.
    // Balanced in windowDidExitFullscreen
    [self retain];    
    
    [[self fullscreenWindow] animateFromRect:[[self window] frame] toRect:endFrame withSubAnimation:_fadeAnimation controllerAction:@selector(windowDidExitFullscreen)];
}

- (void)applicationDidChangeScreenParameters:(NSNotification*)notification
{
    UNUSED_PARAM(notification);
    // The user may have changed the main screen by moving the menu bar, or they may have changed
    // the Dock's size or location, or they may have changed the fullscreen screen's dimensions.  
    // Update our presentation parameters, and ensure that the full screen window occupies the 
    // entire screen:
    [self updateMenuAndDockForFullscreen];
    [[self window] setFrame:[[[self window] screen] frame] display:YES];
}

- (void)updateMenuAndDockForFullscreen
{
    // NSApplicationPresentationOptions is available on > 10.6 only:
    NSApplicationPresentationOptions options = NSApplicationPresentationDefault;
    NSScreen* fullscreenScreen = [[self window] screen];

    if (!_isEndingFullscreen) {
        // Auto-hide the menu bar if the fullscreenScreen contains the menu bar:
        // NOTE: if the fullscreenScreen contains the menu bar but not the dock, we must still 
        // auto-hide the dock, or an exception will be thrown.
        if ([[NSScreen screens] objectAtIndex:0] == fullscreenScreen)
            options |= (NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationAutoHideDock);
        // Check if the current screen contains the dock by comparing the screen's frame to its
        // visibleFrame; if a dock is present, the visibleFrame will differ.  If the current screen
        // contains the dock, hide it.
        else if (!NSEqualRects([fullscreenScreen frame], [fullscreenScreen visibleFrame]))
            options |= NSApplicationPresentationAutoHideDock;
    }

    if ([NSApp respondsToSelector:@selector(setPresentationOptions:)])
        [NSApp setPresentationOptions:options];
    else
        SetSystemUIMode(_isEndingFullscreen ? kUIModeNormal : kUIModeAllHidden, 0);
}

- (void)updatePowerAssertions
{
    float rate = 0;
    if (_mediaElement && _mediaElement->platformMedia().type == PlatformMedia::QTMovieType)
        rate = [_mediaElement->platformMedia().media.qtMovie rate];
    
    if (rate && !_isEndingFullscreen) {
        if (!_displaySleepDisabler)
            _displaySleepDisabler = DisplaySleepDisabler::create("com.apple.WebCore - Fullscreen video");
    } else
        _displaySleepDisabler = nullptr;
}

// MARK: -
// MARK: Window callback

- (void)_requestExit
{
    if (_mediaElement)
        _mediaElement->exitFullscreen();
    _forceDisableAnimation = NO;
}

- (void)requestExitFullscreenWithAnimation:(BOOL)animation
{
    if (_isEndingFullscreen)
        return;

    _forceDisableAnimation = !animation;
    [self performSelector:@selector(_requestExit) withObject:nil afterDelay:0];

}

- (void)requestExitFullscreen
{
    [self requestExitFullscreenWithAnimation:YES];
}

- (void)fadeHUDIn
{
    [_hudController fadeWindowIn];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(change);
    UNUSED_PARAM(context);

    if ([keyPath isEqualTo:@"rate"])
        [self rateChanged:nil];
}

- (void)rateChanged:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    [_hudController updateRate];
    [self updatePowerAssertions];
}

@end

@implementation WebVideoFullscreenWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag
{
    UNUSED_PARAM(aStyle);
    self = [super initWithContentRect:contentRect styleMask:NSBorderlessWindowMask backing:bufferingType defer:flag];
    if (!self)
        return nil;
    [self setOpaque:NO];
    [self setBackgroundColor:[NSColor clearColor]];
    [self setIgnoresMouseEvents:NO];
    [self setAcceptsMouseMovedEvents:YES];
    return self;
}

- (void)dealloc
{
    ASSERT(!_fullscreenAnimation);
    [super dealloc];
}

- (BOOL)resignFirstResponder
{
    return NO;
}

- (BOOL)canBecomeKeyWindow
{
    return NO;
}

- (void)mouseDown:(NSEvent *)theEvent
{
    UNUSED_PARAM(theEvent);
}

- (void)cancelOperation:(id)sender
{
    UNUSED_PARAM(sender);
    [[self windowController] requestExitFullscreen];
}

- (void)animatedResizeDidEnd
{
    // Call our windowController.
    if (_controllerActionOnAnimationEnd)
        [[self windowController] performSelector:_controllerActionOnAnimationEnd];
    _controllerActionOnAnimationEnd = NULL;
}

//
// This function will animate a change of frame rectangle
// We support queuing animation, that means that we'll correctly
// interrupt the running animation, and queue the next one.
//
- (void)animateFromRect:(NSRect)startRect toRect:(NSRect)endRect withSubAnimation:(NSAnimation *)subAnimation controllerAction:(SEL)controllerAction
{
    _controllerActionOnAnimationEnd = controllerAction;

    BOOL wasAnimating = NO;
    if (_fullscreenAnimation) {
        wasAnimating = YES;

        // Interrupt any running animation.
        [_fullscreenAnimation stopAnimation];

        // Save the current rect to ensure a smooth transition.
        startRect = [_fullscreenAnimation currentFrame];
        [_fullscreenAnimation release];
        _fullscreenAnimation = nil;
    }
    
    if (NSIsEmptyRect(startRect) || NSIsEmptyRect(endRect)) {
        // Fakely end the subanimation.
        [subAnimation setCurrentProgress:1];
        // And remove the weak link to the window.
        [subAnimation stopAnimation];

        [self setFrame:endRect display:NO];
        [self makeKeyAndOrderFront:self];
        [self animatedResizeDidEnd];
        return;
    }

    if (!wasAnimating) {
        // We'll downscale the window during the animation based on the higher resolution rect
        BOOL higherResolutionIsEndRect = startRect.size.width < endRect.size.width && startRect.size.height < endRect.size.height;
        [self setFrame:higherResolutionIsEndRect ? endRect : startRect display:NO];        
    }
    
    ASSERT(!_fullscreenAnimation);
    _fullscreenAnimation = [[WebWindowScaleAnimation alloc] initWithHintedDuration:0.2 window:self initalFrame:startRect finalFrame:endRect];
    [_fullscreenAnimation setSubAnimation:subAnimation];
    [_fullscreenAnimation setDelegate:self];
    
    // Make sure the animation has scaled the window before showing it.
    [_fullscreenAnimation setCurrentProgress:0];
    [self makeKeyAndOrderFront:self];

    [_fullscreenAnimation startAnimation];
}

- (void)animationDidEnd:(NSAnimation *)animation
{
    if (![NSThread isMainThread]) {
        [self performSelectorOnMainThread:@selector(animationDidEnd:) withObject:animation waitUntilDone:NO];
        return;
    }
    if (animation != _fullscreenAnimation)
        return;

    // The animation is not really over and was interrupted
    // Don't send completion events.
    if ([animation currentProgress] < 1.0)
        return;

    // Ensure that animation (and subanimation) don't keep
    // the weak reference to the window ivar that may be destroyed from
    // now on.
    [_fullscreenAnimation setWindow:nil];

    [_fullscreenAnimation autorelease];
    _fullscreenAnimation = nil;

    [self animatedResizeDidEnd];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    UNUSED_PARAM(theEvent);
    [[self windowController] fadeHUDIn];
}

@end

#endif /* ENABLE(VIDEO) */
