/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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
#include "WebKitWebPage.h"

#include "ImageOptions.h"
#include "ImmutableDictionary.h"
#include "InjectedBundle.h"
#include "WKBundleAPICast.h"
#include "WKBundleFrame.h"
#include "WebFrame.h"
#include "WebImage.h"
#include "WebKitDOMDocumentPrivate.h"
#include "WebKitMarshal.h"
#include "WebKitPrivate.h"
#include "WebKitURIRequestPrivate.h"
#include "WebKitURIResponsePrivate.h"
#include "WebKitWebPagePrivate.h"
#include "WebProcess.h"
#include <WebCore/Document.h>
#include <WebCore/DocumentLoader.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <glib/gi18n-lib.h>
#include <wtf/text/CString.h>

using namespace WebKit;
using namespace WebCore;

enum {
    DOCUMENT_LOADED,
    SEND_REQUEST,

    LAST_SIGNAL
};

enum {
    PROP_0,

    PROP_URI
};

struct _WebKitWebPagePrivate {
    WebPage* webPage;

    CString uri;
};

static guint signals[LAST_SIGNAL] = { 0, };

WEBKIT_DEFINE_TYPE(WebKitWebPage, webkit_web_page, G_TYPE_OBJECT)

static CString getProvisionalURLForFrame(WebFrame* webFrame)
{
    DocumentLoader* documentLoader = webFrame->coreFrame()->loader()->provisionalDocumentLoader();
    if (!documentLoader->unreachableURL().isEmpty())
        return documentLoader->unreachableURL().string().utf8();

    return documentLoader->url().string().utf8();
}

static void webkitWebPageSetURI(WebKitWebPage* webPage, const CString& uri)
{
    if (webPage->priv->uri == uri)
        return;

    webPage->priv->uri = uri;
    g_object_notify(G_OBJECT(webPage), "uri");
}

static void didStartProvisionalLoadForFrame(WKBundlePageRef, WKBundleFrameRef frame, WKTypeRef*, const void *clientInfo)
{
    if (!WKBundleFrameIsMainFrame(frame))
        return;

    webkitWebPageSetURI(WEBKIT_WEB_PAGE(clientInfo), getProvisionalURLForFrame(toImpl(frame)));
}

static void didReceiveServerRedirectForProvisionalLoadForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo)
{
    if (!WKBundleFrameIsMainFrame(frame))
        return;

    webkitWebPageSetURI(WEBKIT_WEB_PAGE(clientInfo), getProvisionalURLForFrame(toImpl(frame)));
}

static void didSameDocumentNavigationForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKSameDocumentNavigationType type, WKTypeRef* userData, const void *clientInfo)
{
    if (!WKBundleFrameIsMainFrame(frame))
        return;

    webkitWebPageSetURI(WEBKIT_WEB_PAGE(clientInfo), toImpl(frame)->coreFrame()->document()->url().string().utf8());
}

static void didFinishDocumentLoadForFrame(WKBundlePageRef, WKBundleFrameRef frame, WKTypeRef*, const void *clientInfo)
{
    if (!WKBundleFrameIsMainFrame(frame))
        return;

    g_signal_emit(WEBKIT_WEB_PAGE(clientInfo), signals[DOCUMENT_LOADED], 0);
}

static void didInitiateLoadForResource(WKBundlePageRef page, WKBundleFrameRef frame, uint64_t identifier, WKURLRequestRef request, bool pageLoadIsProvisional, const void*)
{
    ImmutableDictionary::MapType message;
    message.set(String::fromUTF8("Page"), toImpl(page));
    message.set(String::fromUTF8("Frame"), toImpl(frame));
    message.set(String::fromUTF8("Identifier"), WebUInt64::create(identifier));
    message.set(String::fromUTF8("Request"), toImpl(request));
    WebProcess::shared().injectedBundle()->postMessage(String::fromUTF8("WebPage.DidInitiateLoadForResource"), ImmutableDictionary::adopt(message).get());
}

static WKURLRequestRef willSendRequestForFrame(WKBundlePageRef page, WKBundleFrameRef, uint64_t identifier, WKURLRequestRef wkRequest, WKURLResponseRef wkRedirectResponse, const void* clientInfo)
{
    GRefPtr<WebKitURIRequest> request = adoptGRef(webkitURIRequestCreateForResourceRequest(toImpl(wkRequest)->resourceRequest()));
    GRefPtr<WebKitURIResponse> redirectResponse = wkRedirectResponse ? adoptGRef(webkitURIResponseCreateForResourceResponse(toImpl(wkRedirectResponse)->resourceResponse())) : 0;

    gboolean returnValue;
    g_signal_emit(WEBKIT_WEB_PAGE(clientInfo), signals[SEND_REQUEST], 0, request.get(), redirectResponse.get(), &returnValue);
    if (returnValue)
        return 0;

    ResourceRequest resourceRequest;
    webkitURIRequestGetResourceRequest(request.get(), resourceRequest);
    RefPtr<WebURLRequest> newRequest = WebURLRequest::create(resourceRequest);

    ImmutableDictionary::MapType message;
    message.set(String::fromUTF8("Page"), toImpl(page));
    message.set(String::fromUTF8("Identifier"), WebUInt64::create(identifier));
    message.set(String::fromUTF8("Request"), newRequest.get());
    if (!toImpl(wkRedirectResponse)->resourceResponse().isNull())
        message.set(String::fromUTF8("RedirectResponse"), toImpl(wkRedirectResponse));
    WebProcess::shared().injectedBundle()->postMessage(String::fromUTF8("WebPage.DidSendRequestForResource"), ImmutableDictionary::adopt(message).get());

    return toAPI(newRequest.release().leakRef());
}

static void didReceiveResponseForResource(WKBundlePageRef page, WKBundleFrameRef, uint64_t identifier, WKURLResponseRef response, const void*)
{
    ImmutableDictionary::MapType message;
    message.set(String::fromUTF8("Page"), toImpl(page));
    message.set(String::fromUTF8("Identifier"), WebUInt64::create(identifier));
    message.set(String::fromUTF8("Response"), toImpl(response));
    WebProcess::shared().injectedBundle()->postMessage(String::fromUTF8("WebPage.DidReceiveResponseForResource"), ImmutableDictionary::adopt(message).get());
}

static void didReceiveContentLengthForResource(WKBundlePageRef page, WKBundleFrameRef, uint64_t identifier, uint64_t length, const void*)
{
    ImmutableDictionary::MapType message;
    message.set(String::fromUTF8("Page"), toImpl(page));
    message.set(String::fromUTF8("Identifier"), WebUInt64::create(identifier));
    message.set(String::fromUTF8("ContentLength"), WebUInt64::create(length));
    WebProcess::shared().injectedBundle()->postMessage(String::fromUTF8("WebPage.DidReceiveContentLengthForResource"), ImmutableDictionary::adopt(message).get());
}

static void didFinishLoadForResource(WKBundlePageRef page, WKBundleFrameRef, uint64_t identifier, const void*)
{
    ImmutableDictionary::MapType message;
    message.set(String::fromUTF8("Page"), toImpl(page));
    message.set(String::fromUTF8("Identifier"), WebUInt64::create(identifier));
    WebProcess::shared().injectedBundle()->postMessage(String::fromUTF8("WebPage.DidFinishLoadForResource"), ImmutableDictionary::adopt(message).get());
}

static void didFailLoadForResource(WKBundlePageRef page, WKBundleFrameRef, uint64_t identifier, WKErrorRef error, const void*)
{
    ImmutableDictionary::MapType message;
    message.set(String::fromUTF8("Page"), toImpl(page));
    message.set(String::fromUTF8("Identifier"), WebUInt64::create(identifier));
    message.set(String::fromUTF8("Error"), toImpl(error));
    WebProcess::shared().injectedBundle()->postMessage(String::fromUTF8("WebPage.DidFailLoadForResource"), ImmutableDictionary::adopt(message).get());
}

static void webkitWebPageGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitWebPage* webPage = WEBKIT_WEB_PAGE(object);

    switch (propId) {
    case PROP_URI:
        g_value_set_string(value, webkit_web_page_get_uri(webPage));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void webkit_web_page_class_init(WebKitWebPageClass* klass)
{
    GObjectClass* gObjectClass = G_OBJECT_CLASS(klass);

    gObjectClass->get_property = webkitWebPageGetProperty;

    /**
     * WebKitWebPage:uri:
     *
     * The current active URI of the #WebKitWebPage.
     */
    g_object_class_install_property(
        gObjectClass,
        PROP_URI,
        g_param_spec_string(
            "uri",
            _("URI"),
            _("The current active URI of the web page"),
            0,
            WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebPage::document-loaded:
     * @web_page: the #WebKitWebPage on which the signal is emitted
     *
     * This signal is emitted when the DOM document of a #WebKitWebPage has been
     * loaded.
     *
     * You can wait for this signal to get the DOM document with
     * webkit_web_page_get_dom_document().
     */
    signals[DOCUMENT_LOADED] = g_signal_new(
        "document-loaded",
        G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST,
        0, 0, 0,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebPage::send-request:
     * @web_page: the #WebKitWebPage on which the signal is emitted
     * @request: a #WebKitURIRequest
     * @redirected_response: a #WebKitURIResponse, or %NULL
     *
     * This signal is emitted when @request is about to be sent to
     * the server. This signal can be used to modify the #WebKitURIRequest
     * that will be sent to the server. You can also cancel the resource load
     * operation by connecting to this signal and returning %TRUE.
     *
     * In case of a server redirection this signal is
     * emitted again with the @request argument containing the new
     * request to be sent to the server due to the redirection and the
     * @redirected_response parameter containing the response
     * received by the server for the initial request.
     *
     * Returns: %TRUE to stop other handlers from being invoked for the event.
     *    %FALSE to continue emission of the event.
     */
    signals[SEND_REQUEST] = g_signal_new(
        "send-request",
        G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled, 0,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT,
        G_TYPE_BOOLEAN, 2,
        WEBKIT_TYPE_URI_REQUEST,
        WEBKIT_TYPE_URI_RESPONSE);
}

WebKitWebPage* webkitWebPageCreate(WebPage* webPage)
{
    WebKitWebPage* page = WEBKIT_WEB_PAGE(g_object_new(WEBKIT_TYPE_WEB_PAGE, NULL));
    page->priv->webPage = webPage;

    WKBundlePageLoaderClient loaderClient = {
        kWKBundlePageResourceLoadClientCurrentVersion,
        page,
        didStartProvisionalLoadForFrame,
        didReceiveServerRedirectForProvisionalLoadForFrame,
        0, // didFailProvisionalLoadWithErrorForFrame
        0, // didCommitLoadForFrame
        didFinishDocumentLoadForFrame,
        0, // didFinishLoadForFrame
        0, // didFailLoadWithErrorForFrame
        didSameDocumentNavigationForFrame,
        0, // didReceiveTitleForFrame
        0, // didFirstLayoutForFrame
        0, // didFirstVisuallyNonEmptyLayoutForFrame
        0, // didRemoveFrameFromHierarchy
        0, // didDisplayInsecureContentForFrame
        0, // didRunInsecureContentForFrame
        0, // didClearWindowObjectForFrame
        0, // didCancelClientRedirectForFrame
        0, // willPerformClientRedirectForFrame
        0, // didHandleOnloadEventsForFrame
        0, // didLayoutForFrame
        0, // didNewFirstVisuallyNonEmptyLayout
        0, // didDetectXSSForFrame
        0, // shouldGoToBackForwardListItem
        0, // globalObjectIsAvailableForFrame
        0, // willDisconnectDOMWindowExtensionFromGlobalObject
        0, // didReconnectDOMWindowExtensionToGlobalObject
        0, // willDestroyGlobalObjectForDOMWindowExtension
        0, // didFinishProgress
        0, // shouldForceUniversalAccessFromLocalURL
        0, // didReceiveIntentForFrame_unavailable
        0, // registerIntentServiceForFrame_unavailable
        0, // didLayout
        0, // featuresUsedInPage
        0, // willLoadURLRequest;
        0, // willLoadDataRequest;
    };
    WKBundlePageSetPageLoaderClient(toAPI(webPage), &loaderClient);

    WKBundlePageResourceLoadClient resourceLoadClient = {
        kWKBundlePageResourceLoadClientCurrentVersion,
        page,
        didInitiateLoadForResource,
        willSendRequestForFrame,
        didReceiveResponseForResource,
        didReceiveContentLengthForResource,
        didFinishLoadForResource,
        didFailLoadForResource,
        0, // shouldCacheResponse
        0 // shouldUseCredentialStorage
    };
    WKBundlePageSetResourceLoadClient(toAPI(webPage), &resourceLoadClient);

    return page;
}

void webkitWebPageDidReceiveMessage(WebKitWebPage* page, const String& messageName, ImmutableDictionary& message)
{
    if (messageName == String("GetSnapshot")) {
        SnapshotOptions snapshotOptions = static_cast<SnapshotOptions>(static_cast<WebUInt64*>(message.get("SnapshotOptions"))->value());
        uint64_t callbackID = static_cast<WebUInt64*>(message.get("CallbackID"))->value();
        SnapshotRegion region = static_cast<SnapshotRegion>(static_cast<WebUInt64*>(message.get("SnapshotRegion"))->value());

        RefPtr<WebImage> snapshotImage;
        WebPage* webPage = page->priv->webPage;
        if (WebCore::FrameView* frameView = webPage->mainFrameView()) {
            WebCore::IntRect snapshotRect;
            switch (region) {
            case SnapshotRegionVisible:
                snapshotRect = frameView->visibleContentRect(WebCore::ScrollableArea::ExcludeScrollbars);
                break;
            case SnapshotRegionFullDocument:
                snapshotRect = WebCore::IntRect(WebCore::IntPoint(0, 0), frameView->contentsSize());
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            if (!snapshotRect.isEmpty())
                snapshotImage = webPage->scaledSnapshotWithOptions(snapshotRect, 1, snapshotOptions | SnapshotOptionsShareable);
        }

        ImmutableDictionary::MapType messageReply;
        messageReply.set("Page", webPage);
        messageReply.set("CallbackID", WebUInt64::create(callbackID));
        messageReply.set("Snapshot", snapshotImage);
        WebProcess::shared().injectedBundle()->postMessage("WebPage.DidGetSnapshot", ImmutableDictionary::adopt(messageReply).get());
    } else
        ASSERT_NOT_REACHED();
}

/**
 * webkit_web_page_get_dom_document:
 * @web_page: a #WebKitWebPage
 *
 * Get the #WebKitDOMDocument currently loaded in @web_page
 *
 * Returns: the #WebKitDOMDocument currently loaded, or %NULL
 *    if no document is currently loaded.
 */
WebKitDOMDocument* webkit_web_page_get_dom_document(WebKitWebPage* webPage)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_PAGE(webPage), 0);

    Frame* coreFrame = webPage->priv->webPage->mainFrame();
    if (!coreFrame)
        return 0;

    return kit(coreFrame->document());
}

/**
 * webkit_web_page_get_id:
 * @web_page: a #WebKitWebPage
 *
 * Get the identifier of the #WebKitWebPage
 *
 * Returns: the identifier of @web_page
 */
guint64 webkit_web_page_get_id(WebKitWebPage* webPage)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_PAGE(webPage), 0);

    return webPage->priv->webPage->pageID();
}

/**
 * webkit_web_page_get_uri:
 * @web_page: a #WebKitWebPage
 *
 * Returns the current active URI of @web_page.
 *
 * You can monitor the active URI by connecting to the notify::uri
 * signal of @web_page.
 *
 * Returns: the current active URI of @web_view or %NULL if nothing has been
 *    loaded yet.
 */
const gchar* webkit_web_page_get_uri(WebKitWebPage* webPage)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_PAGE(webPage), 0);

    return webPage->priv->uri.data();
}
