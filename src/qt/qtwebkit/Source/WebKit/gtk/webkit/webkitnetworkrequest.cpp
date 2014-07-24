/*
 * Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009 Gustavo Noronha Silva
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
#include "webkitnetworkrequest.h"

#include "ResourceRequest.h"
#include "webkitglobalsprivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

/**
 * SECTION:webkitnetworkrequest
 * @short_description: The target of a navigation request
 * @see_also: #WebKitWebView::navigation-policy-decision-requested
 *
 * This class represents the network related aspects of a navigation
 * request. It is used whenever WebKit wants to provide information
 * about a request that will be sent, or has been sent. Inside it you
 * can find the URI of the request, and, for valid URIs, a
 * #SoupMessage object, which provides access to further information
 * such as headers.
 *
 */

G_DEFINE_TYPE(WebKitNetworkRequest, webkit_network_request, G_TYPE_OBJECT);

struct _WebKitNetworkRequestPrivate {
    gchar* uri;
    SoupMessage* message;
};

enum {
    PROP_0,

    PROP_URI,
    PROP_MESSAGE,
};

static void webkit_network_request_dispose(GObject* object)
{
    WebKitNetworkRequest* request = WEBKIT_NETWORK_REQUEST(object);
    WebKitNetworkRequestPrivate* priv = request->priv;

    if (priv->message) {
        g_object_unref(priv->message);
        priv->message = NULL;
    }

    G_OBJECT_CLASS(webkit_network_request_parent_class)->dispose(object);
}

static void webkit_network_request_finalize(GObject* object)
{
    WebKitNetworkRequest* request = WEBKIT_NETWORK_REQUEST(object);
    WebKitNetworkRequestPrivate* priv = request->priv;

    g_free(priv->uri);

    G_OBJECT_CLASS(webkit_network_request_parent_class)->finalize(object);
}

static void webkit_network_request_get_property(GObject* object, guint propertyID, GValue* value, GParamSpec* pspec)
{
    WebKitNetworkRequest* request = WEBKIT_NETWORK_REQUEST(object);

    switch(propertyID) {
    case PROP_URI:
        g_value_set_string(value, webkit_network_request_get_uri(request));
        break;
    case PROP_MESSAGE:
        g_value_set_object(value, webkit_network_request_get_message(request));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyID, pspec);
    }
}

static void webkit_network_request_set_property(GObject* object, guint propertyID, const GValue* value, GParamSpec* pspec)
{
    WebKitNetworkRequest* request = WEBKIT_NETWORK_REQUEST(object);
    WebKitNetworkRequestPrivate* priv = request->priv;

    switch(propertyID) {
    case PROP_URI:
        webkit_network_request_set_uri(request, g_value_get_string(value));
        break;
    case PROP_MESSAGE:
        priv->message = SOUP_MESSAGE(g_value_dup_object(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyID, pspec);
    }
}

static void webkit_network_request_class_init(WebKitNetworkRequestClass* requestClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(requestClass);

    objectClass->dispose = webkit_network_request_dispose;
    objectClass->finalize = webkit_network_request_finalize;
    objectClass->get_property = webkit_network_request_get_property;
    objectClass->set_property = webkit_network_request_set_property;

    webkitInit();

    /**
     * WebKitNetworkRequest:uri:
     *
     * The URI to which the request will be made.
     *
     * Since: 1.1.10
     */
    g_object_class_install_property(objectClass, PROP_URI,
                                    g_param_spec_string("uri",
                                                        _("URI"),
                                                        _("The URI to which the request will be made."),
                                                        NULL,
                                                        (GParamFlags)(WEBKIT_PARAM_READWRITE)));

    /**
     * WebKitNetworkRequest:message:
     *
     * The #SoupMessage that backs the request.
     *
     * Since: 1.1.10
     */
    g_object_class_install_property(objectClass, PROP_MESSAGE,
                                    g_param_spec_object("message",
                                                        _("Message"),
                                                        _("The SoupMessage that backs the request."),
                                                        SOUP_TYPE_MESSAGE,
                                                        (GParamFlags)(WEBKIT_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY)));

    g_type_class_add_private(requestClass, sizeof(WebKitNetworkRequestPrivate));
}

static void webkit_network_request_init(WebKitNetworkRequest* request)
{
    WebKitNetworkRequestPrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(request, WEBKIT_TYPE_NETWORK_REQUEST, WebKitNetworkRequestPrivate);
    request->priv = priv;
}

/**
 * webkit_network_request_new:
 * @uri: an URI
 *
 * Creates a new #WebKitNetworkRequest initialized with an URI.
 *
 * Returns: a new #WebKitNetworkRequest, or %NULL if the URI is
 * invalid.
 */
WebKitNetworkRequest* webkit_network_request_new(const gchar* uri)
{
    g_return_val_if_fail(uri, NULL);

    return WEBKIT_NETWORK_REQUEST(g_object_new(WEBKIT_TYPE_NETWORK_REQUEST, "uri", uri, NULL));
}

/**
 * webkit_network_request_set_uri:
 * @request: a #WebKitNetworkRequest
 * @uri: an URI
 *
 * Sets the URI held and used by the given request. When the request
 * has an associated #SoupMessage, its URI will also be set by this
 * call.
 *
 */
void webkit_network_request_set_uri(WebKitNetworkRequest* request, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_NETWORK_REQUEST(request));
    g_return_if_fail(uri);

    WebKitNetworkRequestPrivate* priv = request->priv;

    if (priv->uri)
        g_free(priv->uri);
    priv->uri = g_strdup(uri);

    if (!priv->message)
        return;

    SoupURI* soupURI = soup_uri_new(uri);
    g_return_if_fail(soupURI);

    soup_message_set_uri(priv->message, soupURI);
    soup_uri_free(soupURI);
}

/**
 * webkit_network_request_get_uri:
 * @request: a #WebKitNetworkRequest
 *
 * Returns: the URI of the #WebKitNetworkRequest
 *
 * Since: 1.0.0
 */
const gchar* webkit_network_request_get_uri(WebKitNetworkRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_NETWORK_REQUEST(request), NULL);

    WebKitNetworkRequestPrivate* priv = request->priv;

    if (priv->uri)
        return priv->uri;

    SoupURI* soupURI = soup_message_get_uri(priv->message);
    priv->uri = soup_uri_to_string(soupURI, FALSE);
    return priv->uri;
}

/**
 * webkit_network_request_get_message:
 * @request: a #WebKitNetworkRequest
 *
 * Obtains the #SoupMessage held and used by the given request. Notice
 * that modification of the SoupMessage of a request by signal
 * handlers is only supported (as in, will only affect what is
 * actually sent to the server) where explicitly documented.
 *
 * Returns: (transfer none): the #SoupMessage
 * Since: 1.1.9
 */
SoupMessage* webkit_network_request_get_message(WebKitNetworkRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_NETWORK_REQUEST(request), NULL);

    WebKitNetworkRequestPrivate* priv = request->priv;

    return priv->message;
}

namespace WebKit {

WebKitNetworkRequest* kitNew(const WebCore::ResourceRequest& resourceRequest)
{
    GRefPtr<SoupMessage> soupMessage(adoptGRef(resourceRequest.toSoupMessage()));
    if (soupMessage)
        return WEBKIT_NETWORK_REQUEST(g_object_new(WEBKIT_TYPE_NETWORK_REQUEST, "message", soupMessage.get(), NULL));

    return WEBKIT_NETWORK_REQUEST(g_object_new(WEBKIT_TYPE_NETWORK_REQUEST, "uri", resourceRequest.url().string().utf8().data(), NULL));
}

WebCore::ResourceRequest core(WebKitNetworkRequest* request)
{
    SoupMessage* soupMessage = webkit_network_request_get_message(request);
    if (soupMessage)
        return WebCore::ResourceRequest(soupMessage);

    WebCore::KURL url = WebCore::KURL(WebCore::KURL(), String::fromUTF8(webkit_network_request_get_uri(request)));
    return WebCore::ResourceRequest(url);
}

}
