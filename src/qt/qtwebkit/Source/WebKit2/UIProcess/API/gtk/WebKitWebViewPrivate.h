/*
 * Copyright (C) 2011 Igalia S.L.
 * Portions Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
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

#ifndef WebKitWebViewPrivate_h
#define WebKitWebViewPrivate_h

#include "WebImage.h"
#include "WebKitWebView.h"
#include <wtf/text/CString.h>

void webkitWebViewLoadChanged(WebKitWebView*, WebKitLoadEvent);
void webkitWebViewLoadFailed(WebKitWebView*, WebKitLoadEvent, const char* failingURI, GError*);
void webkitWebViewLoadFailedWithTLSErrors(WebKitWebView*, const char* failingURI, GError *, GTlsCertificateFlags, GTlsCertificate*);
void webkitWebViewSetEstimatedLoadProgress(WebKitWebView*, double estimatedLoadProgress);
void webkitWebViewSetTitle(WebKitWebView*, const CString&);
void webkitWebViewUpdateURI(WebKitWebView*);
WebKit::WebPageProxy* webkitWebViewCreateNewPage(WebKitWebView*, WebKit::ImmutableDictionary* windowFeatures);
void webkitWebViewReadyToShowPage(WebKitWebView*);
void webkitWebViewRunAsModal(WebKitWebView*);
void webkitWebViewClosePage(WebKitWebView*);
void webkitWebViewRunJavaScriptAlert(WebKitWebView*, const CString& message);
bool webkitWebViewRunJavaScriptConfirm(WebKitWebView*, const CString& message);
CString webkitWebViewRunJavaScriptPrompt(WebKitWebView*, const CString& message, const CString& defaultText);
void webkitWebViewMakePermissionRequest(WebKitWebView*, WebKitPermissionRequest*);
void webkitWebViewMakePolicyDecision(WebKitWebView*, WebKitPolicyDecisionType, WebKitPolicyDecision*);
void webkitWebViewMouseTargetChanged(WebKitWebView*, WebKit::WebHitTestResult*, unsigned modifiers);
void webkitWebViewPrintFrame(WebKitWebView*, WebKit::WebFrameProxy*);
void webkitWebViewResourceLoadStarted(WebKitWebView*, WebKit::WebFrameProxy*, uint64_t resourceIdentifier, WebKitURIRequest*);
void webkitWebViewRunFileChooserRequest(WebKitWebView*, WebKitFileChooserRequest*);
WebKitWebResource* webkitWebViewGetLoadingWebResource(WebKitWebView*, uint64_t resourceIdentifier);
void webKitWebViewDidReceiveSnapshot(WebKitWebView*, uint64_t callbackID, WebKit::WebImage*);
void webkitWebViewRemoveLoadingWebResource(WebKitWebView*, uint64_t resourceIdentifier);
bool webkitWebViewEnterFullScreen(WebKitWebView*);
bool webkitWebViewLeaveFullScreen(WebKitWebView*);
void webkitWebViewPopulateContextMenu(WebKitWebView*, WebKit::ImmutableArray* proposedMenu, WebKit::WebHitTestResult*);
void webkitWebViewSubmitFormRequest(WebKitWebView*, WebKitFormSubmissionRequest*);
void webkitWebViewHandleAuthenticationChallenge(WebKitWebView*, WebKit::AuthenticationChallengeProxy*);
void webkitWebViewInsecureContentDetected(WebKitWebView*, WebKitInsecureContentEvent);
void webkitWebViewWebProcessCrashed(WebKitWebView*);

#endif // WebKitWebViewPrivate_h
