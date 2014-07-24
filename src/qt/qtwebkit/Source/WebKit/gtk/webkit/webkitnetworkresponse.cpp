/*
 * Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009 Gustavo Noronha Silva
 * Copyright (C) 2009 Collabora Ltd.
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
#include "webkitnetworkresponse.h"

#include "ResourceResponse.h"
#include "webkitglobalsprivate.h"
#include "webkitnetworkresponseprivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION:webkitnetworkresponse
 * @short_description: the response given to a network request
 * @see_also: #WebKitNetworkRequest
 *
 * This class represents the network related aspects of a navigation
 * response.
 *
 * Since: 1.1.14
 */

G_DEFINE_TYPE(WebKitNetworkResponse, webkit_network_response, G_TYPE_OBJECT);

struct _WebKitNetworkResponsePrivate {
    gchar* uri;
    gchar* suggestedFilename;
    SoupMessage* message;
};

#define WEBKIT_NETWORK_RESPONSE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_NETWORK_RESPONSE, WebKitNetworkResponsePrivate))

enum {
    PROP_0,

    PROP_URI,
    PROP_MESSAGE,
    PROP_SUGGESTED_FILENAME,
};

static void webkit_network_response_dispose(GObject* object)
{
    WebKitNetworkResponse* response = WEBKIT_NETWORK_RESPONSE(object);
    WebKitNetworkResponsePrivate* priv = response->priv;

    if (priv->message) {
        g_object_unref(priv->message);
        priv->message = NULL;
    }

    G_OBJECT_CLASS(webkit_network_response_parent_class)->dispose(object);
}

static void webkit_network_response_finalize(GObject* object)
{
    WebKitNetworkResponse* response = WEBKIT_NETWORK_RESPONSE(object);
    WebKitNetworkResponsePrivate* priv = response->priv;

    g_free(priv->uri);
    g_free(priv->suggestedFilename);

    G_OBJECT_CLASS(webkit_network_response_parent_class)->finalize(object);
}

static void webkit_network_response_get_property(GObject* object, guint propertyID, GValue* value, GParamSpec* pspec)
{
    WebKitNetworkResponse* response = WEBKIT_NETWORK_RESPONSE(object);

    switch(propertyID) {
    case PROP_URI:
        g_value_set_string(value, webkit_network_response_get_uri(response));
        break;
    case PROP_MESSAGE:
        g_value_set_object(value, webkit_network_response_get_message(response));
        break;
    case PROP_SUGGESTED_FILENAME:
        g_value_set_string(value, webkit_network_response_get_suggested_filename(response));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyID, pspec);
    }
}

static void webkit_network_response_set_property(GObject* object, guint propertyID, const GValue* value, GParamSpec* pspec)
{
    WebKitNetworkResponse* response = WEBKIT_NETWORK_RESPONSE(object);
    WebKitNetworkResponsePrivate* priv = response->priv;

    switch(propertyID) {
    case PROP_URI:
        webkit_network_response_set_uri(response, g_value_get_string(value));
        break;
    case PROP_MESSAGE:
        priv->message = SOUP_MESSAGE(g_value_dup_object(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyID, pspec);
    }
}

static void webkit_network_response_class_init(WebKitNetworkResponseClass* responseClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(responseClass);

    objectClass->dispose = webkit_network_response_dispose;
    objectClass->finalize = webkit_network_response_finalize;
    objectClass->get_property = webkit_network_response_get_property;
    objectClass->set_property = webkit_network_response_set_property;

    webkitInit();

    /**
     * WebKitNetworkResponse:uri:
     *
     * The URI to which the response will be made.
     *
     * Since: 1.1.14
     */
    g_object_class_install_property(objectClass, PROP_URI,
                                    g_param_spec_string("uri",
                                                        _("URI"),
                                                        _("The URI to which the response will be made."),
                                                        NULL,
                                                        (GParamFlags)(WEBKIT_PARAM_READWRITE)));

    /**
     * WebKitNetworkResponse:message:
     *
     * The #SoupMessage that backs the response.
     *
     * Since: 1.1.14
     */
    g_object_class_install_property(objectClass, PROP_MESSAGE,
                                    g_param_spec_object("message",
                                                        _("Message"),
                                                        _("The SoupMessage that backs the response."),
                                                        SOUP_TYPE_MESSAGE,
                                                        (GParamFlags)(WEBKIT_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY)));

    /**
     * WebKitNetworkResponse:suggested-filename:
     *
     * The suggested filename for the response.
     *
     * Since: 1.10
     */
    g_object_class_install_property(objectClass, PROP_SUGGESTED_FILENAME,
                                    g_param_spec_string("suggested-filename",
                                                        _("Suggested filename"),
                                                        _("The suggested filename for the response."),
                                                        0,
                                                        WEBKIT_PARAM_READABLE));

    g_type_class_add_private(responseClass, sizeof(WebKitNetworkResponsePrivate));
}

static void webkit_network_response_init(WebKitNetworkResponse* response)
{
    response->priv = WEBKIT_NETWORK_RESPONSE_GET_PRIVATE(response);
}

/**
 * webkit_network_response_new:
 * @uri: an URI
 *
 * Creates a new #WebKitNetworkResponse initialized with an URI.
 *
 * Returns: a new #WebKitNetworkResponse, or %NULL if the URI is
 * invalid.
 *
 * Since: 1.1.14
 */
WebKitNetworkResponse* webkit_network_response_new(const gchar* uri)
{
    g_return_val_if_fail(uri, NULL);

    return WEBKIT_NETWORK_RESPONSE(g_object_new(WEBKIT_TYPE_NETWORK_RESPONSE, "uri", uri, NULL));
}

/**
 * webkit_network_response_set_uri:
 * @response: a #WebKitNetworkResponse
 * @uri: an URI
 *
 * Sets the URI held and used by the given response. When the response
 * has an associated #SoupMessage, its URI will also be set by this
 * call.
 *
 * Since: 1.1.14
 */
void webkit_network_response_set_uri(WebKitNetworkResponse* response, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_NETWORK_RESPONSE(response));
    g_return_if_fail(uri);

    WebKitNetworkResponsePrivate* priv = response->priv;

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
 * webkit_network_response_get_uri:
 * @response: a #WebKitNetworkResponse
 *
 * Returns: the URI of the #WebKitNetworkResponse
 *
 * Since: 1.1.14
 */
const gchar* webkit_network_response_get_uri(WebKitNetworkResponse* response)
{
    g_return_val_if_fail(WEBKIT_IS_NETWORK_RESPONSE(response), NULL);

    WebKitNetworkResponsePrivate* priv = response->priv;

    if (priv->uri)
        return priv->uri;

    SoupURI* soupURI = soup_message_get_uri(priv->message);
    priv->uri = soup_uri_to_string(soupURI, FALSE);
    return priv->uri;
}

/**
 * webkit_network_response_get_message:
 * @response: a #WebKitNetworkResponse
 *
 * Obtains the #SoupMessage that represents the given response. Notice
 * that only the response side of the HTTP conversation is
 * represented.
 *
 * Returns: (transfer none): the #SoupMessage
 * Since: 1.1.14
 */
SoupMessage* webkit_network_response_get_message(WebKitNetworkResponse* response)
{
    g_return_val_if_fail(WEBKIT_IS_NETWORK_RESPONSE(response), NULL);

    WebKitNetworkResponsePrivate* priv = response->priv;

    return priv->message;
}

/**
 * webkit_network_response_get_suggested_filename:
 * @response: a #WebKitNetworkResponse
 *
 * Obtains the suggested filename for the given network response. The
 * suggested filename is taken from the 'Content-Disposition' HTTP
 * header, but this is not always present, and this method will return
 * %NULL in such case.
 *
 * Returns: (transfer none): the suggested filename or %NULL if not present
 * Since: 1.10
 **/
const gchar* webkit_network_response_get_suggested_filename(WebKitNetworkResponse* response)
{
    g_return_val_if_fail(WEBKIT_IS_NETWORK_RESPONSE(response), 0);

    WebKitNetworkResponsePrivate* priv = response->priv;

    if (priv->suggestedFilename)
        return priv->suggestedFilename;

    WebCore::ResourceResponse coreResponse = core(response);
    priv->suggestedFilename = g_strdup(coreResponse.suggestedFilename().utf8().data());
    return priv->suggestedFilename;
}

namespace WebKit {

WebCore::ResourceResponse core(WebKitNetworkResponse* response)
{
    SoupMessage* soupMessage = webkit_network_response_get_message(response);
    if (soupMessage)
        return WebCore::ResourceResponse(soupMessage);

    return WebCore::ResourceResponse();
}

WebKitNetworkResponse* kitNew(const WebCore::ResourceResponse& resourceResponse)
{
    GRefPtr<SoupMessage> soupMessage(adoptGRef(resourceResponse.toSoupMessage()));
    if (soupMessage)
        return WEBKIT_NETWORK_RESPONSE(g_object_new(WEBKIT_TYPE_NETWORK_RESPONSE, "message", soupMessage.get(), NULL));

    return WEBKIT_NETWORK_RESPONSE(g_object_new(WEBKIT_TYPE_NETWORK_RESPONSE, "uri", resourceResponse.url().string().utf8().data(), NULL));
}

}
