/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebKitPolicyClient.h"

#include "WebKitNavigationPolicyDecisionPrivate.h"
#include "WebKitResponsePolicyDecisionPrivate.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebKitWebViewPrivate.h"
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

static void decidePolicyForNavigationActionCallback(WKPageRef page, WKFrameRef frame, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    GRefPtr<WebKitNavigationPolicyDecision> decision =
        adoptGRef(webkitNavigationPolicyDecisionCreate(static_cast<WebKitNavigationType>(navigationType),
                                                       wkEventMouseButtonToWebKitMouseButton(mouseButton),
                                                       wkEventModifiersToGdkModifiers(modifiers),
                                                       toImpl(request),
                                                       0, /* frame name */
                                                       toImpl(listener)));
    webkitWebViewMakePolicyDecision(WEBKIT_WEB_VIEW(clientInfo),
                                    WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION,
                                    WEBKIT_POLICY_DECISION(decision.get()));
}

static void decidePolicyForNewWindowActionCallback(WKPageRef page, WKFrameRef frame, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKStringRef frameName, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    GRefPtr<WebKitNavigationPolicyDecision> decision =
        adoptGRef(webkitNavigationPolicyDecisionCreate(static_cast<WebKitNavigationType>(navigationType),
                                                       wkEventMouseButtonToWebKitMouseButton(mouseButton),
                                                       wkEventModifiersToGdkModifiers(modifiers),
                                                       toImpl(request),
                                                       toImpl(frameName)->string().utf8().data(),
                                                       toImpl(listener)));
    webkitWebViewMakePolicyDecision(WEBKIT_WEB_VIEW(clientInfo),
                                    WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION,
                                    WEBKIT_POLICY_DECISION(decision.get()));
}

static void decidePolicyForResponseCallback(WKPageRef page, WKFrameRef frame, WKURLResponseRef response, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    GRefPtr<WebKitResponsePolicyDecision> decision =
        adoptGRef(webkitResponsePolicyDecisionCreate(toImpl(request), toImpl(response), toImpl(listener)));
    webkitWebViewMakePolicyDecision(WEBKIT_WEB_VIEW(clientInfo),
                                    WEBKIT_POLICY_DECISION_TYPE_RESPONSE,
                                    WEBKIT_POLICY_DECISION(decision.get()));
}

void attachPolicyClientToView(WebKitWebView* webView)
{
    WKPagePolicyClient policyClient = {
        kWKPagePolicyClientCurrentVersion,
        webView, // clientInfo
        decidePolicyForNavigationActionCallback,
        decidePolicyForNewWindowActionCallback,
        decidePolicyForResponseCallback,
        0, // unableToImplementPolicy
    };
    WKPageSetPagePolicyClient(toAPI(webkitWebViewBaseGetPage(WEBKIT_WEB_VIEW_BASE(webView))), &policyClient);
}
