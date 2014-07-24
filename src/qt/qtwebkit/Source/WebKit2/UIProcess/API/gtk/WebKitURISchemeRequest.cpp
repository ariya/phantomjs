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
#include "WebKitURISchemeRequest.h"

#include "WebData.h"
#include "WebKitURISchemeRequestPrivate.h"
#include "WebKitWebContextPrivate.h"
#include "WebKitWebView.h"
#include "WebPageProxy.h"
#include "WebSoupRequestManagerProxy.h"
#include <WebCore/GOwnPtrSoup.h>
#include <WebCore/ResourceError.h>
#include <libsoup/soup.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION: WebKitURISchemeRequest
 * @Short_description: Represents a URI scheme request
 * @Title: WebKitURISchemeRequest
 *
 * If you register a particular URI scheme in a #WebKitWebContext,
 * using webkit_web_context_register_uri_scheme(), you have to provide
 * a #WebKitURISchemeRequestCallback. After that, when a URI request
 * is made with that particular scheme, your callback will be
 * called. There you will be able to access properties such as the
 * scheme, the URI and path, and the #WebKitWebView that initiated the
 * request, and also finish the request with
 * webkit_uri_scheme_request_finish().
 *
 */

static const unsigned int gReadBufferSize = 8192;

struct _WebKitURISchemeRequestPrivate {
    WebKitWebContext* webContext;
    RefPtr<WebSoupRequestManagerProxy> webRequestManager;
    RefPtr<WebPageProxy> initiatingPage;
    uint64_t requestID;
    CString uri;
    GOwnPtr<SoupURI> soupURI;

    GRefPtr<GInputStream> stream;
    uint64_t streamLength;
    GRefPtr<GCancellable> cancellable;
    char readBuffer[gReadBufferSize];
    uint64_t bytesRead;
    CString mimeType;
};

WEBKIT_DEFINE_TYPE(WebKitURISchemeRequest, webkit_uri_scheme_request, G_TYPE_OBJECT)

static void webkit_uri_scheme_request_class_init(WebKitURISchemeRequestClass* requestClass)
{
}

WebKitURISchemeRequest* webkitURISchemeRequestCreate(WebKitWebContext* webContext, WebSoupRequestManagerProxy* webRequestManager, WebURL* webURL, WebPageProxy* initiatingPage, uint64_t requestID)
{
    WebKitURISchemeRequest* request = WEBKIT_URI_SCHEME_REQUEST(g_object_new(WEBKIT_TYPE_URI_SCHEME_REQUEST, NULL));
    request->priv->webContext = webContext;
    request->priv->webRequestManager = webRequestManager;
    request->priv->uri = webURL->string().utf8();
    request->priv->initiatingPage = initiatingPage;
    request->priv->requestID = requestID;
    return request;
}

uint64_t webkitURISchemeRequestGetID(WebKitURISchemeRequest* request)
{
    return request->priv->requestID;
}

void webkitURISchemeRequestCancel(WebKitURISchemeRequest* request)
{
    if (request->priv->cancellable.get())
        g_cancellable_cancel(request->priv->cancellable.get());
}

/**
 * webkit_uri_scheme_request_get_scheme:
 * @request: a #WebKitURISchemeRequest
 *
 * Get the URI scheme of @request
 *
 * Returns: the URI scheme of @request
 */
const char* webkit_uri_scheme_request_get_scheme(WebKitURISchemeRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request), 0);

    if (!request->priv->soupURI)
        request->priv->soupURI.set(soup_uri_new(request->priv->uri.data()));
    return request->priv->soupURI->scheme;
}

/**
 * webkit_uri_scheme_request_get_uri:
 * @request: a #WebKitURISchemeRequest
 *
 * Get the URI of @request
 *
 * Returns: the full URI of @request
 */
const char* webkit_uri_scheme_request_get_uri(WebKitURISchemeRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request), 0);

    return request->priv->uri.data();
}

/**
 * webkit_uri_scheme_request_get_path:
 * @request: a #WebKitURISchemeRequest
 *
 * Get the URI path of @request
 *
 * Returns: the URI path of @request
 */
const char* webkit_uri_scheme_request_get_path(WebKitURISchemeRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request), 0);

    if (!request->priv->soupURI)
        request->priv->soupURI.set(soup_uri_new(request->priv->uri.data()));
    return request->priv->soupURI->path;
}

/**
 * webkit_uri_scheme_request_get_web_view:
 * @request: a #WebKitURISchemeRequest
 *
 * Get the #WebKitWebView that initiated the request.
 *
 * Returns: (transfer none): the #WebKitWebView that initiated @request.
 */
WebKitWebView* webkit_uri_scheme_request_get_web_view(WebKitURISchemeRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request), 0);

    return WEBKIT_WEB_VIEW(request->priv->initiatingPage->viewWidget());
}

static void webkitURISchemeRequestReadCallback(GInputStream* inputStream, GAsyncResult* result, WebKitURISchemeRequest* schemeRequest)
{
    GRefPtr<WebKitURISchemeRequest> request = adoptGRef(schemeRequest);
    GOwnPtr<GError> error;
    gssize bytesRead = g_input_stream_read_finish(inputStream, result, &error.outPtr());
    if (bytesRead == -1) {
        webkit_uri_scheme_request_finish_error(request.get(), error.get());
        return;
    }

    WebKitURISchemeRequestPrivate* priv = request->priv;
    RefPtr<WebData> webData = WebData::create(reinterpret_cast<const unsigned char*>(priv->readBuffer), bytesRead);
    if (!priv->bytesRead) {
        // First chunk read. In case of empty reply an empty WebData is sent to the WebProcess.
        priv->webRequestManager->didHandleURIRequest(webData.get(), priv->streamLength, String::fromUTF8(priv->mimeType.data()), priv->requestID);
    } else if (bytesRead || (!bytesRead && !priv->streamLength)) {
        // Subsequent chunk read. We only send an empty WebData to the WebProcess when stream length is unknown.
        priv->webRequestManager->didReceiveURIRequestData(webData.get(), priv->requestID);
    }

    if (!bytesRead) {
        webkitWebContextDidFinishURIRequest(request->priv->webContext, request->priv->requestID);
        return;
    }

    priv->bytesRead += bytesRead;
    g_input_stream_read_async(inputStream, priv->readBuffer, gReadBufferSize, G_PRIORITY_DEFAULT, priv->cancellable.get(),
                              reinterpret_cast<GAsyncReadyCallback>(webkitURISchemeRequestReadCallback), g_object_ref(request.get()));
}

/**
 * webkit_uri_scheme_request_finish:
 * @request: a #WebKitURISchemeRequest
 * @stream: a #GInputStream to read the contents of the request
 * @stream_length: the length of the stream or -1 if not known
 * @mime_type: (allow-none): the content type of the stream or %NULL if not known
 *
 * Finish a #WebKitURISchemeRequest by setting the contents of the request and its mime type.
 */
void webkit_uri_scheme_request_finish(WebKitURISchemeRequest* request, GInputStream* inputStream, gint64 streamLength, const gchar* mimeType)
{
    g_return_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request));
    g_return_if_fail(G_IS_INPUT_STREAM(inputStream));
    g_return_if_fail(streamLength == -1 || streamLength >= 0);

    request->priv->stream = inputStream;
    // We use -1 in the API for consistency with soup when the content length is not known, but 0 internally.
    request->priv->streamLength = streamLength == -1 ? 0 : streamLength;
    request->priv->cancellable = adoptGRef(g_cancellable_new());
    request->priv->bytesRead = 0;
    request->priv->mimeType = mimeType;
    g_input_stream_read_async(inputStream, request->priv->readBuffer, gReadBufferSize, G_PRIORITY_DEFAULT, request->priv->cancellable.get(),
                              reinterpret_cast<GAsyncReadyCallback>(webkitURISchemeRequestReadCallback), g_object_ref(request));
}

/**
 * webkit_uri_scheme_request_finish_error:
 * @request: a #WebKitURISchemeRequest
 * @error: a #GError that will be passed to the #WebKitWebView
 *
 * Finish a #WebKitURISchemeRequest with a #GError.
 *
 * Since: 2.2
 */
void webkit_uri_scheme_request_finish_error(WebKitURISchemeRequest* request, GError* error)
{
    g_return_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request));
    g_return_if_fail(error);

    WebKitURISchemeRequestPrivate* priv = request->priv;

    WebCore::ResourceError resourceError(g_quark_to_string(error->domain), error->code, priv->uri.data(), String::fromUTF8(error->message));
    priv->webRequestManager->didFailURIRequest(resourceError, priv->requestID);

    webkitWebContextDidFinishURIRequest(priv->webContext, priv->requestID);
}
