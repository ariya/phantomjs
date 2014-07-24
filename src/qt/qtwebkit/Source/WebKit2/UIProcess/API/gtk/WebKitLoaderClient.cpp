/*
 * Copyright (C) 2011 Igalia S.L.
 * Portions Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitLoaderClient.h"

#include "WebKitBackForwardListPrivate.h"
#include "WebKitURIResponsePrivate.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebKitWebViewPrivate.h"
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;
using namespace WebCore;

static void didStartProvisionalLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    webkitWebViewLoadChanged(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_LOAD_STARTED);
}

static void didReceiveServerRedirectForProvisionalLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    webkitWebViewLoadChanged(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_LOAD_REDIRECTED);
}

static void didFailProvisionalLoadWithErrorForFrame(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    const ResourceError& resourceError = toImpl(error)->platformError();
    GOwnPtr<GError> webError(g_error_new_literal(g_quark_from_string(resourceError.domain().utf8().data()),
                                                 resourceError.errorCode(),
                                                 resourceError.localizedDescription().utf8().data()));
    if (resourceError.tlsErrors()) {
        webkitWebViewLoadFailedWithTLSErrors(WEBKIT_WEB_VIEW(clientInfo), resourceError.failingURL().utf8().data(), webError.get(),
            static_cast<GTlsCertificateFlags>(resourceError.tlsErrors()), resourceError.certificate());
    } else
        webkitWebViewLoadFailed(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_LOAD_STARTED, resourceError.failingURL().utf8().data(), webError.get());
}

static void didCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    webkitWebViewLoadChanged(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_LOAD_COMMITTED);
}

static void didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    webkitWebViewLoadChanged(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_LOAD_FINISHED);
}

static void didFailLoadWithErrorForFrame(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    const ResourceError& resourceError = toImpl(error)->platformError();
    GOwnPtr<GError> webError(g_error_new_literal(g_quark_from_string(resourceError.domain().utf8().data()),
                                                 resourceError.errorCode(),
                                                 resourceError.localizedDescription().utf8().data()));
    webkitWebViewLoadFailed(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_LOAD_COMMITTED,
                            resourceError.failingURL().utf8().data(), webError.get());
}

static void didSameDocumentNavigationForFrame(WKPageRef page, WKFrameRef frame, WKSameDocumentNavigationType, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    webkitWebViewUpdateURI(WEBKIT_WEB_VIEW(clientInfo));
}

static void didReceiveTitleForFrame(WKPageRef page, WKStringRef titleRef, WKFrameRef frameRef, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frameRef))
        return;

    webkitWebViewSetTitle(WEBKIT_WEB_VIEW(clientInfo), toImpl(titleRef)->string().utf8());
}

static void didDisplayInsecureContentForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    webkitWebViewInsecureContentDetected(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_INSECURE_CONTENT_DISPLAYED);
}

static void didRunInsecureContentForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    webkitWebViewInsecureContentDetected(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_INSECURE_CONTENT_RUN);
}

static void didChangeProgress(WKPageRef page, const void* clientInfo)
{
    webkitWebViewSetEstimatedLoadProgress(WEBKIT_WEB_VIEW(clientInfo), WKPageGetEstimatedProgress(page));
}

static void didChangeBackForwardList(WKPageRef page, WKBackForwardListItemRef addedItem, WKArrayRef removedItems, const void* clientInfo)
{
    webkitBackForwardListChanged(webkit_web_view_get_back_forward_list(WEBKIT_WEB_VIEW(clientInfo)), toImpl(addedItem), toImpl(removedItems));
}

static void didReceiveAuthenticationChallengeInFrame(WKPageRef page, WKFrameRef frame, WKAuthenticationChallengeRef authenticationChallenge, const void *clientInfo)
{
    webkitWebViewHandleAuthenticationChallenge(WEBKIT_WEB_VIEW(clientInfo), toImpl(authenticationChallenge));
}

static void processDidCrash(WKPageRef page, const void* clientInfo)
{
    webkitWebViewWebProcessCrashed(WEBKIT_WEB_VIEW(clientInfo));
}

void attachLoaderClientToView(WebKitWebView* webView)
{
    WKPageLoaderClient wkLoaderClient = {
        kWKPageLoaderClientCurrentVersion,
        webView, // clientInfo
        didStartProvisionalLoadForFrame,
        didReceiveServerRedirectForProvisionalLoadForFrame,
        didFailProvisionalLoadWithErrorForFrame,
        didCommitLoadForFrame,
        0, // didFinishDocumentLoadForFrame
        didFinishLoadForFrame,
        didFailLoadWithErrorForFrame,
        didSameDocumentNavigationForFrame,
        didReceiveTitleForFrame,
        0, // didFirstLayoutForFrame
        0, // didFirstVisuallyNonEmptyLayoutForFrame
        0, // didRemoveFrameFromHierarchy
        didDisplayInsecureContentForFrame,
        didRunInsecureContentForFrame,
        0, // canAuthenticateAgainstProtectionSpaceInFrame
        didReceiveAuthenticationChallengeInFrame,
        didChangeProgress, // didStartProgress
        didChangeProgress,
        didChangeProgress, // didFinishProgress
        0, // didBecomeUnresponsive
        0, // didBecomeResponsive
        processDidCrash,
        didChangeBackForwardList,
        0, // shouldGoToBackForwardListItem
        0, // didFailToInitializePlugin
        0, // didDetectXSSForFrame
        0, // didFirstVisuallyNonEmptyLayoutForFrame
        0, // willGoToBackForwardListItem
        0, // interactionOccurredWhileProcessUnresponsive
        0, // pluginDidFail_deprecatedForUseWithV1
        0, // didReceiveIntentForFrame
        0, // registerIntentServiceForFrame
        0, // didLayout
        0, // pluginLoadPolicy
        0, // pluginDidFail
    };
    WKPageRef wkPage = toAPI(webkitWebViewBaseGetPage(WEBKIT_WEB_VIEW_BASE(webView)));
    WKPageSetPageLoaderClient(wkPage, &wkLoaderClient);
}

