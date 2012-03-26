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

#if ENABLE(VIDEO)

#import <Cocoa/Cocoa.h>
#import <wtf/RefPtr.h>

namespace WebCore {
    class HTMLMediaElement;
}

@protocol WebVideoFullscreenControllerDelegate;
@class WebVideoFullscreenHUDWindowController;
@class WebWindowFadeAnimation;
@class QTMovieLayer;

@interface WebVideoFullscreenController : NSWindowController {
@private
    RefPtr<WebCore::HTMLMediaElement> _mediaElement; // (retain)
    id <WebVideoFullscreenControllerDelegate> _delegate; // (assign)

    NSWindow *_backgroundFullscreenWindow; // (retain)
    WebVideoFullscreenHUDWindowController *_hudController; // (retain)

    WebWindowFadeAnimation *_fadeAnimation; // (retain)

    BOOL _isEndingFullscreen;
    BOOL _isWindowLoaded;
    BOOL _forceDisableAnimation;
    uint32_t _idleDisplaySleepAssertion;
    uint32_t _idleSystemSleepAssertion;
    NSTimer *_tickleTimer;
    uint32_t _savedUIMode;
    uint32_t _savedUIOptions;
}

- (id <WebVideoFullscreenControllerDelegate>)delegate;
- (void)setDelegate:(id <WebVideoFullscreenControllerDelegate>)delegate;

- (void)setupVideoOverlay:(QTMovieLayer*)layer;
- (void)setMediaElement:(WebCore::HTMLMediaElement*)mediaElement;
- (WebCore::HTMLMediaElement*)mediaElement;

- (void)enterFullscreen:(NSScreen *)screen;
- (void)exitFullscreen;

@end

#endif // ENABLE(VIDEO)
