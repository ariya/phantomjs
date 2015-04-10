/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#if ENABLE(FULLSCREEN_API)

#import <wtf/OwnPtr.h>
#import <wtf/RetainPtr.h>

namespace WebKit { 
class LayerTreeContext;
}

namespace WebCore {
class DisplaySleepDisabler;
class IntRect;
}

@class WKView;
@class WebCoreFullScreenPlaceholderView;
@class WebWindowScaleAnimation;
@class WebWindowFadeAnimation;

typedef enum FullScreenState : NSInteger FullScreenState;

@interface WKFullScreenWindowController : NSWindowController<NSWindowDelegate> {
@private
    WKView *_webView;
    RetainPtr<WebCoreFullScreenPlaceholderView> _webViewPlaceholder;
    RetainPtr<WebWindowScaleAnimation> _scaleAnimation;
    RetainPtr<WebWindowFadeAnimation> _fadeAnimation;
    RetainPtr<NSWindow> _backgroundWindow;
    NSRect _initialFrame;
    NSRect _finalFrame;
    RetainPtr<NSTimer> _watchdogTimer;

    FullScreenState _fullScreenState;

    double _savedScale;
}

- (WKView*)webView;
- (void)setWebView:(WKView*)webView;

- (WebCoreFullScreenPlaceholderView*)webViewPlaceholder;

- (BOOL)isFullScreen;

- (void)enterFullScreen:(NSScreen *)screen;
- (void)exitFullScreen;
- (void)close;
- (void)beganEnterFullScreenWithInitialFrame:(const WebCore::IntRect&)initialFrame finalFrame:(const WebCore::IntRect&)finalFrame;
- (void)beganExitFullScreenWithInitialFrame:(const WebCore::IntRect&)initialFrame finalFrame:(const WebCore::IntRect&)finalFrame;

@end

#endif
