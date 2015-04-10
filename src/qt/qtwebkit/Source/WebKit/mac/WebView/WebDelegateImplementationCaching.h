/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

// This header contains WebView declarations that can be used anywhere in WebKit, but are neither SPI nor API.

#import "WebTypesInternal.h"
#import <JavaScriptCore/JSBase.h>

@class WebView;

struct WebResourceDelegateImplementationCache {
    IMP didCancelAuthenticationChallengeFunc;
    IMP didReceiveAuthenticationChallengeFunc;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    IMP canAuthenticateAgainstProtectionSpaceFunc;
#endif
    IMP identifierForRequestFunc;
    IMP willSendRequestFunc;
    IMP didReceiveResponseFunc;
    IMP didReceiveContentLengthFunc;
    IMP didFinishLoadingFromDataSourceFunc;
    IMP didFailLoadingWithErrorFromDataSourceFunc;
    IMP didLoadResourceFromMemoryCacheFunc;
    IMP willCacheResponseFunc;
    IMP plugInFailedWithErrorFunc;
    IMP shouldUseCredentialStorageFunc;
    IMP shouldPaintBrokenImageForURLFunc;
};

struct WebFrameLoadDelegateImplementationCache {
#if JSC_OBJC_API_ENABLED
    IMP didCreateJavaScriptContextForFrameFunc;
#endif
    IMP didClearWindowObjectForFrameFunc;
    IMP didClearWindowObjectForFrameInScriptWorldFunc;
    IMP didClearInspectorWindowObjectForFrameFunc;
    IMP windowScriptObjectAvailableFunc;
    IMP didHandleOnloadEventsForFrameFunc;
    IMP didReceiveServerRedirectForProvisionalLoadForFrameFunc;
    IMP didCancelClientRedirectForFrameFunc;
    IMP willPerformClientRedirectToURLDelayFireDateForFrameFunc;
    IMP didChangeLocationWithinPageForFrameFunc;
    IMP didPushStateWithinPageForFrameFunc;
    IMP didReplaceStateWithinPageForFrameFunc;
    IMP didPopStateWithinPageForFrameFunc;
    IMP willCloseFrameFunc;
    IMP didStartProvisionalLoadForFrameFunc;
    IMP didReceiveTitleForFrameFunc;
    IMP didCommitLoadForFrameFunc;
    IMP didFailProvisionalLoadWithErrorForFrameFunc;
    IMP didFailLoadWithErrorForFrameFunc;
    IMP didFinishLoadForFrameFunc;
    IMP didFirstLayoutInFrameFunc;
    IMP didFirstVisuallyNonEmptyLayoutInFrameFunc;
    IMP didLayoutFunc;
    IMP didReceiveIconForFrameFunc;
    IMP didFinishDocumentLoadForFrameFunc;
    IMP didDisplayInsecureContentFunc;
    IMP didRunInsecureContentFunc;
    IMP didDetectXSSFunc;
    IMP didRemoveFrameFromHierarchyFunc;
};

struct WebScriptDebugDelegateImplementationCache {
    BOOL didParseSourceExpectsBaseLineNumber;
    BOOL exceptionWasRaisedExpectsHasHandlerFlag;
    IMP didParseSourceFunc;
    IMP failedToParseSourceFunc;
    IMP didEnterCallFrameFunc;
    IMP willExecuteStatementFunc;
    IMP willLeaveCallFrameFunc;
    IMP exceptionWasRaisedFunc;
};

struct WebHistoryDelegateImplementationCache {
    IMP navigatedFunc;
    IMP clientRedirectFunc;
    IMP serverRedirectFunc;
    IMP deprecatedSetTitleFunc;
    IMP setTitleFunc;
    IMP populateVisitedLinksFunc;
};

WebResourceDelegateImplementationCache* WebViewGetResourceLoadDelegateImplementations(WebView *);
WebFrameLoadDelegateImplementationCache* WebViewGetFrameLoadDelegateImplementations(WebView *);
WebScriptDebugDelegateImplementationCache* WebViewGetScriptDebugDelegateImplementations(WebView *);
WebHistoryDelegateImplementationCache* WebViewGetHistoryDelegateImplementations(WebView *webView);

id CallFormDelegate(WebView *, SEL, id, id);
id CallFormDelegate(WebView *, SEL, id, id, id);
id CallFormDelegate(WebView *self, SEL selector, id object1, id object2, id object3, id object4, id object5);
BOOL CallFormDelegateReturningBoolean(BOOL, WebView *, SEL, id, SEL, id);

id CallUIDelegate(WebView *, SEL);
id CallUIDelegate(WebView *, SEL, id);
id CallUIDelegate(WebView *, SEL, NSRect);
id CallUIDelegate(WebView *, SEL, id, id);
id CallUIDelegate(WebView *, SEL, id, BOOL);
id CallUIDelegate(WebView *, SEL, id, id, id);
id CallUIDelegate(WebView *, SEL, id, NSUInteger);
float CallUIDelegateReturningFloat(WebView *, SEL);
BOOL CallUIDelegateReturningBoolean(BOOL, WebView *, SEL);
BOOL CallUIDelegateReturningBoolean(BOOL, WebView *, SEL, id);
BOOL CallUIDelegateReturningBoolean(BOOL, WebView *, SEL, id, id);
BOOL CallUIDelegateReturningBoolean(BOOL, WebView *, SEL, id, BOOL);
BOOL CallUIDelegateReturningBoolean(BOOL, WebView *, SEL, id, BOOL, id);

id CallFrameLoadDelegate(IMP, WebView *, SEL);
id CallFrameLoadDelegate(IMP, WebView *, SEL, NSUInteger);
id CallFrameLoadDelegate(IMP, WebView *, SEL, id);
id CallFrameLoadDelegate(IMP, WebView *, SEL, id, id);
id CallFrameLoadDelegate(IMP, WebView *, SEL, id, id, id);
id CallFrameLoadDelegate(IMP, WebView *, SEL, id, id, id, id);
id CallFrameLoadDelegate(IMP, WebView *, SEL, id, NSTimeInterval, id, id);

BOOL CallFrameLoadDelegateReturningBoolean(BOOL, IMP, WebView *, SEL);

id CallResourceLoadDelegate(IMP, WebView *, SEL, id, id);
id CallResourceLoadDelegate(IMP, WebView *, SEL, id, id, id);
id CallResourceLoadDelegate(IMP, WebView *, SEL, id, id, id, id);
id CallResourceLoadDelegate(IMP, WebView *, SEL, id, NSInteger, id);
id CallResourceLoadDelegate(IMP, WebView *, SEL, id, id, NSInteger, id);

BOOL CallResourceLoadDelegateReturningBoolean(BOOL, IMP, WebView *, SEL, id);
BOOL CallResourceLoadDelegateReturningBoolean(BOOL, IMP, WebView *, SEL, id, id);
BOOL CallResourceLoadDelegateReturningBoolean(BOOL, IMP, WebView *, SEL, id, id, id);

id CallScriptDebugDelegate(IMP, WebView *, SEL, id, id, NSInteger, id);
id CallScriptDebugDelegate(IMP, WebView *, SEL, id, NSInteger, id, NSInteger, id);
id CallScriptDebugDelegate(IMP, WebView *, SEL, id, NSInteger, id, id, id);
id CallScriptDebugDelegate(IMP, WebView *, SEL, id, NSInteger, int, id);
id CallScriptDebugDelegate(IMP, WebView *, SEL, id, BOOL, NSInteger, int, id);

id CallHistoryDelegate(IMP, WebView *, SEL);
id CallHistoryDelegate(IMP, WebView *, SEL, id, id);
id CallHistoryDelegate(IMP, WebView *, SEL, id, id, id);
