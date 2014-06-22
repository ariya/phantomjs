/*
 * Copyright (C) 2006. 2007 Apple Inc. All rights reserved.
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
#import "UIDelegate.h"

#import "DumpRenderTree.h"
#import "DumpRenderTreeDraggingInfo.h"
#import "EventSendingController.h"
#import "MockWebNotificationProvider.h"
#import "TestRunner.h"
#import <WebKit/WebApplicationCache.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebHTMLViewPrivate.h>
#import <WebKit/WebQuotaManager.h>
#import <WebKit/WebSecurityOriginPrivate.h>
#import <WebKit/WebUIDelegatePrivate.h>
#import <WebKit/WebView.h>
#import <WebKit/WebViewPrivate.h>
#import <wtf/Assertions.h>

DumpRenderTreeDraggingInfo *draggingInfo = nil;

@implementation UIDelegate

- (void)webView:(WebView *)sender setFrame:(NSRect)frame
{
    m_frame = frame;
}

- (NSRect)webViewFrame:(WebView *)sender
{
    return m_frame;
}

- (void)webView:(WebView *)sender addMessageToConsole:(NSDictionary *)dictionary withSource:(NSString *)source
{
    NSString *message = [dictionary objectForKey:@"message"];
    NSNumber *lineNumber = [dictionary objectForKey:@"lineNumber"];

    NSRange range = [message rangeOfString:@"file://"];
    if (range.location != NSNotFound)
        message = [[message substringToIndex:range.location] stringByAppendingString:[[message substringFromIndex:NSMaxRange(range)] lastPathComponent]];

    printf ("CONSOLE MESSAGE: ");
    if ([lineNumber intValue])
        printf ("line %d: ", [lineNumber intValue]);
    printf ("%s\n", [message UTF8String]);
}

- (void)modalWindowWillClose:(NSNotification *)notification
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:nil];
    [NSApp abortModal];
}

- (void)webViewRunModal:(WebView *)sender
{
    gTestRunner->setWindowIsKey(false);
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(modalWindowWillClose:) name:NSWindowWillCloseNotification object:nil];
    [NSApp runModalForWindow:[sender window]];
    gTestRunner->setWindowIsKey(true);
}

- (void)webView:(WebView *)sender runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WebFrame *)frame
{
    if (!done) {
        printf("ALERT: %s\n", [message UTF8String]);
        fflush(stdout);
    }
}

- (BOOL)webView:(WebView *)sender runJavaScriptConfirmPanelWithMessage:(NSString *)message initiatedByFrame:(WebFrame *)frame
{
    if (!done)
        printf("CONFIRM: %s\n", [message UTF8String]);
    return YES;
}

- (NSString *)webView:(WebView *)sender runJavaScriptTextInputPanelWithPrompt:(NSString *)prompt defaultText:(NSString *)defaultText initiatedByFrame:(WebFrame *)frame
{
    if (!done)
        printf("PROMPT: %s, default text: %s\n", [prompt UTF8String], [defaultText UTF8String]);
    return defaultText;
}

- (BOOL)webView:(WebView *)c runBeforeUnloadConfirmPanelWithMessage:(NSString *)message initiatedByFrame:(WebFrame *)frame
{
    if (!done)
        printf("CONFIRM NAVIGATION: %s\n", [message UTF8String]);
    
    return !gTestRunner->shouldStayOnPageAfterHandlingBeforeUnload();
}


- (void)webView:(WebView *)sender dragImage:(NSImage *)anImage at:(NSPoint)viewLocation offset:(NSSize)initialOffset event:(NSEvent *)event pasteboard:(NSPasteboard *)pboard source:(id)sourceObj slideBack:(BOOL)slideFlag forView:(NSView *)view
{
     assert(!draggingInfo);
     draggingInfo = [[DumpRenderTreeDraggingInfo alloc] initWithImage:anImage offset:initialOffset pasteboard:pboard source:sourceObj];
     [sender draggingUpdated:draggingInfo];
     [EventSendingController replaySavedEvents];
}

- (void)webViewFocus:(WebView *)webView
{
    gTestRunner->setWindowIsKey(true);
}

- (void)webViewUnfocus:(WebView *)webView
{
    gTestRunner->setWindowIsKey(false);
}

- (WebView *)webView:(WebView *)sender createWebViewWithRequest:(NSURLRequest *)request
{
    if (!gTestRunner->canOpenWindows())
        return nil;
    
    // Make sure that waitUntilDone has been called.
    ASSERT(gTestRunner->waitToDump());

    WebView *webView = createWebViewAndOffscreenWindow();
    
    if (gTestRunner->newWindowsCopyBackForwardList())
        [webView _loadBackForwardListFromOtherView:sender];
    
    return [webView autorelease];
}

- (void)webViewClose:(WebView *)sender
{
    NSWindow* window = [sender window];
 
    if (gTestRunner->callCloseOnWebViews())
        [sender close];
    
    [window close];
}

- (void)webView:(WebView *)sender frame:(WebFrame *)frame exceededDatabaseQuotaForSecurityOrigin:(WebSecurityOrigin *)origin database:(NSString *)databaseIdentifier
{
    if (!done && gTestRunner->dumpDatabaseCallbacks()) {
        printf("UI DELEGATE DATABASE CALLBACK: exceededDatabaseQuotaForSecurityOrigin:{%s, %s, %i} database:%s\n", [[origin protocol] UTF8String], [[origin host] UTF8String], 
            [origin port], [databaseIdentifier UTF8String]);
    }

    static const unsigned long long defaultQuota = 5 * 1024 * 1024;    
    [[origin databaseQuotaManager] setQuota:defaultQuota];
}

- (void)webView:(WebView *)sender exceededApplicationCacheOriginQuotaForSecurityOrigin:(WebSecurityOrigin *)origin totalSpaceNeeded:(NSUInteger)totalSpaceNeeded
{
    if (!done && gTestRunner->dumpApplicationCacheDelegateCallbacks()) {
        // For example, numbers from 30000 - 39999 will output as 30000.
        // Rounding up or down not really matter for these tests. It's
        // sufficient to just get a range of 10000 to determine if we were
        // above or below a threshold.
        unsigned long truncatedSpaceNeeded = static_cast<unsigned long>((totalSpaceNeeded / 10000) * 10000);
        printf("UI DELEGATE APPLICATION CACHE CALLBACK: exceededApplicationCacheOriginQuotaForSecurityOrigin:{%s, %s, %i} totalSpaceNeeded:~%lu\n",
            [[origin protocol] UTF8String], [[origin host] UTF8String], [origin port], truncatedSpaceNeeded);
    }

    if (gTestRunner->disallowIncreaseForApplicationCacheQuota())
        return;

    static const unsigned long long defaultOriginQuota = [WebApplicationCache defaultOriginQuota];
    [[origin applicationCacheQuotaManager] setQuota:defaultOriginQuota];
}

- (void)webView:(WebView *)sender setStatusText:(NSString *)text
{
    if (gTestRunner->dumpStatusCallbacks())
        printf("UI DELEGATE STATUS CALLBACK: setStatusText:%s\n", [text UTF8String]);
}

- (void)webView:(WebView *)webView decidePolicyForGeolocationRequestFromOrigin:(WebSecurityOrigin *)origin frame:(WebFrame *)frame listener:(id<WebAllowDenyPolicyListener>)listener
{
    if (!gTestRunner->isGeolocationPermissionSet()) {
        if (!m_pendingGeolocationPermissionListeners)
            m_pendingGeolocationPermissionListeners = [[NSMutableSet set] retain];
        [m_pendingGeolocationPermissionListeners addObject:listener];
        return;
    }

    if (gTestRunner->geolocationPermission())
        [listener allow];
    else
        [listener deny];
}

- (void)didSetMockGeolocationPermission
{
    ASSERT(gTestRunner->isGeolocationPermissionSet());
    if (m_pendingGeolocationPermissionListeners && !m_timer)
        m_timer = [NSTimer scheduledTimerWithTimeInterval:0 target:self selector:@selector(timerFired) userInfo:0 repeats:NO];
}

- (int)numberOfPendingGeolocationPermissionRequests
{
    if (!m_pendingGeolocationPermissionListeners)
        return 0;
    return [m_pendingGeolocationPermissionListeners count];
}


- (void)timerFired
{
    ASSERT(gTestRunner->isGeolocationPermissionSet());
    m_timer = 0;
    NSEnumerator* enumerator = [m_pendingGeolocationPermissionListeners objectEnumerator];
    id<WebAllowDenyPolicyListener> listener;
    while ((listener = [enumerator nextObject])) {
        if (gTestRunner->geolocationPermission())
            [listener allow];
        else
            [listener deny];
    }
    [m_pendingGeolocationPermissionListeners removeAllObjects];
    [m_pendingGeolocationPermissionListeners release];
    m_pendingGeolocationPermissionListeners = nil;
}

- (BOOL)webView:(WebView *)sender shouldHaltPlugin:(DOMNode *)pluginNode
{
    return NO;
}

- (BOOL)webView:(WebView *)webView supportsFullScreenForElement:(DOMElement*)element withKeyboard:(BOOL)withKeyboard
{
    return YES;
}

- (void)enterFullScreenWithListener:(NSObject<WebKitFullScreenListener>*)listener
{
    [listener webkitWillEnterFullScreen];
    [listener webkitDidEnterFullScreen];
}

- (void)webView:(WebView *)webView enterFullScreenForElement:(DOMElement*)element listener:(NSObject<WebKitFullScreenListener>*)listener
{
    if (!gTestRunner->hasCustomFullScreenBehavior())
        [self performSelector:@selector(enterFullScreenWithListener:) withObject:listener afterDelay:0];
}

- (void)exitFullScreenWithListener:(NSObject<WebKitFullScreenListener>*)listener
{
    [listener webkitWillExitFullScreen];
    [listener webkitDidExitFullScreen];
}

- (void)webView:(WebView *)webView exitFullScreenForElement:(DOMElement*)element listener:(NSObject<WebKitFullScreenListener>*)listener
{
    if (!gTestRunner->hasCustomFullScreenBehavior())
        [self performSelector:@selector(exitFullScreenWithListener:) withObject:listener afterDelay:0];
}

- (void)webView:(WebView *)sender closeFullScreenWithListener:(NSObject<WebKitFullScreenListener>*)listener
{
    [listener webkitWillExitFullScreen];
    [listener webkitDidExitFullScreen];
}

- (BOOL)webView:(WebView *)webView didPressMissingPluginButton:(DOMElement *)element
{
    printf("MISSING PLUGIN BUTTON PRESSED\n");
    return TRUE;
}

- (void)webView:(WebView *)webView decidePolicyForNotificationRequestFromOrigin:(WebSecurityOrigin *)origin listener:(id<WebAllowDenyPolicyListener>)listener
{
    MockWebNotificationProvider *provider = (MockWebNotificationProvider *)[webView _notificationProvider];
    switch ([provider policyForOrigin:origin]) {
    case WebNotificationPermissionAllowed:
        [listener allow];
        break;
    case WebNotificationPermissionDenied:
        [listener deny];
        break;
    case WebNotificationPermissionNotAllowed:
        [provider setWebNotificationOrigin:[origin stringValue] permission:YES];
        [listener allow];
        break;
    }
}

- (void)dealloc
{
    [draggingInfo release];
    draggingInfo = nil;
    [m_pendingGeolocationPermissionListeners release];
    m_pendingGeolocationPermissionListeners = nil;

    [super dealloc];
}

@end
