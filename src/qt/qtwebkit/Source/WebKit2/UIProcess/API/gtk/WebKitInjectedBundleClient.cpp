/*
 * Copyright (C) 2013 Igalia S.L.
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
#include "WebKitInjectedBundleClient.h"

#include "WebImage.h"
#include "WebKitURIRequestPrivate.h"
#include "WebKitURIResponsePrivate.h"
#include "WebKitWebContextPrivate.h"
#include "WebKitWebResourcePrivate.h"
#include "WebKitWebViewPrivate.h"
#include <wtf/gobject/GOwnPtr.h>

using namespace WebKit;
using namespace WebCore;

static void didReceiveWebViewMessageFromInjectedBundle(WebKitWebView* webView, const char* messageName, ImmutableDictionary& message)
{
    if (g_str_equal(messageName, "DidInitiateLoadForResource")) {
        WebFrameProxy* frame = static_cast<WebFrameProxy*>(message.get(String::fromUTF8("Frame")));
        WebUInt64* resourceIdentifier = static_cast<WebUInt64*>(message.get(String::fromUTF8("Identifier")));
        WebURLRequest* webRequest = static_cast<WebURLRequest*>(message.get(String::fromUTF8("Request")));
        GRefPtr<WebKitURIRequest> request = adoptGRef(webkitURIRequestCreateForResourceRequest(webRequest->resourceRequest()));

        webkitWebViewResourceLoadStarted(webView, frame, resourceIdentifier->value(), request.get());
    } else if (g_str_equal(messageName, "DidSendRequestForResource")) {
        WebUInt64* resourceIdentifier = static_cast<WebUInt64*>(message.get(String::fromUTF8("Identifier")));
        GRefPtr<WebKitWebResource> resource = webkitWebViewGetLoadingWebResource(webView, resourceIdentifier->value());
        if (!resource)
            return;

        WebURLRequest* webRequest = static_cast<WebURLRequest*>(message.get(String::fromUTF8("Request")));
        GRefPtr<WebKitURIRequest> request = adoptGRef(webkitURIRequestCreateForResourceRequest(webRequest->resourceRequest()));
        WebURLResponse* webRedirectResponse = static_cast<WebURLResponse*>(message.get(String::fromUTF8("RedirectResponse")));
        GRefPtr<WebKitURIResponse> redirectResponse = webRedirectResponse ? adoptGRef(webkitURIResponseCreateForResourceResponse(webRedirectResponse->resourceResponse())) : 0;

        webkitWebResourceSentRequest(resource.get(), request.get(), redirectResponse.get());
    } else if (g_str_equal(messageName, "DidReceiveResponseForResource")) {
        WebUInt64* resourceIdentifier = static_cast<WebUInt64*>(message.get(String::fromUTF8("Identifier")));
        GRefPtr<WebKitWebResource> resource = webkitWebViewGetLoadingWebResource(webView, resourceIdentifier->value());
        if (!resource)
            return;

        WebURLResponse* webResponse = static_cast<WebURLResponse*>(message.get(String::fromUTF8("Response")));
        GRefPtr<WebKitURIResponse> response = adoptGRef(webkitURIResponseCreateForResourceResponse(webResponse->resourceResponse()));

        webkitWebResourceSetResponse(resource.get(), response.get());
    } else if (g_str_equal(messageName, "DidReceiveContentLengthForResource")) {
        WebUInt64* resourceIdentifier = static_cast<WebUInt64*>(message.get(String::fromUTF8("Identifier")));
        GRefPtr<WebKitWebResource> resource = webkitWebViewGetLoadingWebResource(webView, resourceIdentifier->value());
        if (!resource)
            return;

        WebUInt64* contentLength = static_cast<WebUInt64*>(message.get(String::fromUTF8("ContentLength")));
        webkitWebResourceNotifyProgress(resource.get(), contentLength->value());
    } else if (g_str_equal(messageName, "DidFinishLoadForResource")) {
        WebUInt64* resourceIdentifier = static_cast<WebUInt64*>(message.get(String::fromUTF8("Identifier")));
        GRefPtr<WebKitWebResource> resource = webkitWebViewGetLoadingWebResource(webView, resourceIdentifier->value());
        if (!resource)
            return;

        webkitWebResourceFinished(resource.get());
        webkitWebViewRemoveLoadingWebResource(webView, resourceIdentifier->value());
    } else if (g_str_equal(messageName, "DidFailLoadForResource")) {
        WebUInt64* resourceIdentifier = static_cast<WebUInt64*>(message.get(String::fromUTF8("Identifier")));
        GRefPtr<WebKitWebResource> resource = webkitWebViewGetLoadingWebResource(webView, resourceIdentifier->value());
        if (!resource)
            return;

        WebError* webError = static_cast<WebError*>(message.get(String::fromUTF8("Error")));
        const ResourceError& platformError = webError->platformError();
        GOwnPtr<GError> resourceError(g_error_new_literal(g_quark_from_string(platformError.domain().utf8().data()),
            platformError.errorCode(), platformError.localizedDescription().utf8().data()));

        webkitWebResourceFailed(resource.get(), resourceError.get());
        webkitWebViewRemoveLoadingWebResource(webView, resourceIdentifier->value());
    } else if (g_str_equal(messageName, "DidGetSnapshot")) {
        WebUInt64* callbackID = static_cast<WebUInt64*>(message.get("CallbackID"));
        WebImage* image = static_cast<WebImage*>(message.get("Snapshot"));
        webKitWebViewDidReceiveSnapshot(webView, callbackID->value(), image);
    } else
        ASSERT_NOT_REACHED();
}

static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
{
    ASSERT(WKGetTypeID(messageBody) == WKDictionaryGetTypeID());
    ImmutableDictionary& message = *toImpl(static_cast<WKDictionaryRef>(messageBody));

    CString messageNameCString = toImpl(messageName)->string().utf8();
    const char* messageNameUTF8 = messageNameCString.data();

    if (g_str_has_prefix(messageNameUTF8, "WebPage.")) {
        WebPageProxy* page = static_cast<WebPageProxy*>(message.get(String::fromUTF8("Page")));
        WebKitWebView* webView = webkitWebContextGetWebViewForPage(WEBKIT_WEB_CONTEXT(clientInfo), page);
        if (!webView)
            return;

        didReceiveWebViewMessageFromInjectedBundle(webView, messageNameUTF8 + strlen("WebPage."), message);
    } else
        ASSERT_NOT_REACHED();
}

void attachInjectedBundleClientToContext(WebKitWebContext* webContext)
{
    WKContextInjectedBundleClient wkInjectedBundleClient = {
        kWKContextInjectedBundleClientCurrentVersion,
        webContext, // clientInfo
        didReceiveMessageFromInjectedBundle,
        0, // didReceiveSynchronousMessageFromInjectedBundle
        0 // getInjectedBundleInitializationUserData
    };
    WKContextSetInjectedBundleClient(toAPI(webkitWebContextGetContext(webContext)), &wkInjectedBundleClient);
}
