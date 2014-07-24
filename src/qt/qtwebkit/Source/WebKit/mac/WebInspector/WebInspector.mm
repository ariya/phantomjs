/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#import "WebInspector.h"

#import "WebFrameInternal.h"
#import "WebInspectorPrivate.h"
#import "WebInspectorFrontend.h"

#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/InspectorController.h>
#include <WebCore/Page.h>

using namespace WebCore;

@implementation WebInspector
- (id)initWithWebView:(WebView *)webView
{
    if (!(self = [super init]))
        return nil;
    _webView = webView; // not retained to prevent a cycle

    return self;
}

- (void)dealloc
{
    [_frontend release];
    [super dealloc];
}

- (void)webViewClosed
{
    _webView = nil;
}

- (void)showWindow
{
    if (Page* page = core(_webView))
        page->inspectorController()->show();
}

- (void)show:(id)sender
{
    [self showWindow];
}

- (void)showConsole:(id)sender
{
    [self showWindow];
    [_frontend showConsole];
}

- (void)showTimeline:(id)sender
{
    // Not used anymore. Remove when a release of Safari non-longer calls this.
}

- (BOOL)isDebuggingJavaScript
{
    return _frontend && [_frontend isDebuggingEnabled];
}

- (void)toggleDebuggingJavaScript:(id)sender
{
    [self showWindow];

    if ([self isDebuggingJavaScript])
        [_frontend setDebuggingEnabled:false];
    else
        [_frontend setDebuggingEnabled:true];
}

- (void)startDebuggingJavaScript:(id)sender
{
    if (_frontend)
        [_frontend setDebuggingEnabled:true];
}

- (void)stopDebuggingJavaScript:(id)sender
{
    if (_frontend)
        [_frontend setDebuggingEnabled:false];
}

- (BOOL)isProfilingJavaScript
{
    return _frontend && [_frontend isProfilingJavaScript];
}

- (void)toggleProfilingJavaScript:(id)sender
{
    [self showWindow];

    if ([self isProfilingJavaScript])
        [_frontend stopProfilingJavaScript];
    else
        [_frontend startProfilingJavaScript];
}

- (void)startProfilingJavaScript:(id)sender
{
    if (_frontend)
        [_frontend startProfilingJavaScript];
}

- (void)stopProfilingJavaScript:(id)sender
{
    if (_frontend)
        [_frontend stopProfilingJavaScript];
}

- (BOOL)isJavaScriptProfilingEnabled
{
    if (Page* page = core(_webView))
        return page->inspectorController()->profilerEnabled();
    return NO;
}

- (void)setJavaScriptProfilingEnabled:(BOOL)enabled
{
    Page* page = core(_webView);
    if (!page)
        return;

    page->inspectorController()->setProfilerEnabled(enabled);
}

- (BOOL)isTimelineProfilingEnabled
{
    return _frontend && [_frontend isTimelineProfilingEnabled];
}

- (void)setTimelineProfilingEnabled:(BOOL)enabled
{
    if (_frontend)
        [_frontend setTimelineProfilingEnabled:enabled];
}

- (void)close:(id)sender 
{
    if (Page* page = core(_webView))
        page->inspectorController()->close();
}

- (void)attach:(id)sender
{
    [_frontend attach];
}

- (void)detach:(id)sender
{
    [_frontend detach];
}

- (void)evaluateInFrontend:(id)sender callId:(long)callId script:(NSString *)script
{
    if (Page* page = core(_webView))
        page->inspectorController()->evaluateForTestInFrontend(callId, script);
}

- (void)setFrontend:(WebInspectorFrontend *)frontend
{
    _frontend = [frontend retain];
}

- (void)releaseFrontend
{
    [_frontend release];
    _frontend = 0;
}
@end

@implementation WebInspector (Obsolete)
+ (WebInspector *)webInspector
{
    // Safari 3.0 calls this method
    static BOOL logged = NO;
    if (!logged) {
        NSLog(@"+[WebInspector webInspector]: this method is obsolete.");
        logged = YES;
    }

    return [[[WebInspector alloc] init] autorelease];
}

- (void)setWebFrame:(WebFrame *)frame
{
    // Safari 3.0 calls this method
    static BOOL logged = NO;
    if (!logged) {
        NSLog(@"-[WebInspector setWebFrame:]: this method is obsolete.");
        logged = YES;
    }

    _webView = [frame webView];
}

- (NSWindow *)window
{
    // Shiira calls this internal method, return nil since we can't easily return the window
    static BOOL logged = NO;
    if (!logged) {
        NSLog(@"-[WebInspector window]: this method is obsolete and now returns nil.");
        logged = YES;
    }

    return nil;
}

- (void)showWindow:(id)sender
{
    // Safari 3.0 calls this method
    static BOOL logged = NO;
    if (!logged) {
        NSLog(@"-[WebInspector showWindow:]: this method is obsolete.");
        logged = YES;
    }

    [self showWindow];
}
@end
