/*
 * Copyright (C) 2009 Jan Michael C. Alonzo
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
#include "webkitwebresource.h"

#include "ArchiveResource.h"
#include "KURL.h"
#include "SharedBuffer.h"
#include "webkitenumtypes.h"
#include "webkitglobalsprivate.h"
#include "webkitmarshal.h"
#include "webkitnetworkresponse.h"
#include "webkitwebresourceprivate.h"
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <wtf/Assertions.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

/**
 * SECTION:webkitwebresource
 * @short_description: Represents a downloaded URI.
 * @see_also: #WebKitWebDataSource
 *
 * A web resource encapsulates the data of the download as well as the URI,
 * MIME type and frame name of the resource.
 */

using namespace WebCore;

enum {
    // Resource loading
    RESPONSE_RECEIVED,
    LOAD_FINISHED,
    CONTENT_LENGTH_RECEIVED,
    LOAD_FAILED,

    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_URI,
    PROP_MIME_TYPE,
    PROP_ENCODING,
    PROP_FRAME_NAME
};

static guint webkit_web_resource_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(WebKitWebResource, webkit_web_resource, G_TYPE_OBJECT);

static void webkit_web_resource_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);
static void webkit_web_resource_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);

static void webkit_web_resource_cleanup(WebKitWebResource* webResource)
{
    WebKitWebResourcePrivate* priv = webResource->priv;

    g_free(priv->uri);
    priv->uri = NULL;

    g_free(priv->mimeType);
    priv->mimeType = NULL;

    g_free(priv->textEncoding);
    priv->textEncoding = NULL;

    g_free(priv->frameName);
    priv->frameName = NULL;

    if (priv->data)
        g_string_free(priv->data, TRUE);
    priv->data = NULL;
}

static void webkit_web_resource_dispose(GObject* object)
{
    WebKitWebResource* webResource = WEBKIT_WEB_RESOURCE(object);
    WebKitWebResourcePrivate* priv = webResource->priv;

    if (priv->resource) {
        priv->resource->deref();
        priv->resource = 0;
    }

    G_OBJECT_CLASS(webkit_web_resource_parent_class)->dispose(object);
}

static void webkit_web_resource_finalize(GObject* object)
{
    WebKitWebResource* webResource = WEBKIT_WEB_RESOURCE(object);

    webkit_web_resource_cleanup(webResource);

    G_OBJECT_CLASS(webkit_web_resource_parent_class)->finalize(object);
}

static void webkit_web_resource_class_init(WebKitWebResourceClass* webResourceClass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(webResourceClass);

    gobject_class->dispose = webkit_web_resource_dispose;
    gobject_class->finalize = webkit_web_resource_finalize;
    gobject_class->get_property = webkit_web_resource_get_property;
    gobject_class->set_property = webkit_web_resource_set_property;

    /**
     * WebKitWebResource::response-received:
     * @web_resource: the #WebKitWebResource being loaded
     * @response: the #WebKitNetworkResponse that was received
     *
     * Emitted when the response is received from the server.
     *
     * Since: 1.7.5
     */
    webkit_web_resource_signals[RESPONSE_RECEIVED] = g_signal_new("response-received",
        G_TYPE_FROM_CLASS(webResourceClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_NETWORK_RESPONSE);

    /**
     * WebKitWebResource::load-failed:
     * @web_resource: the #WebKitWebResource that was loaded
     * @error: the #GError that was triggered
     *
     * Invoked when the @web_resource failed to load
     *
     * Since: 1.7.5
     */
    webkit_web_resource_signals[LOAD_FAILED] = g_signal_new("load-failed",
        G_TYPE_FROM_CLASS(webResourceClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1,
        G_TYPE_POINTER);

    /**
     * WebKitWebResource::load-finished:
     * @web_resource: the #WebKitWebResource being loaded
     *
     * Emitted when all the data for the resource was loaded
     *
     * Since: 1.7.5
     */
    webkit_web_resource_signals[LOAD_FINISHED] = g_signal_new("load-finished",
        G_TYPE_FROM_CLASS(webResourceClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebResource::content-length-received:
     * @web_resource: the #WebKitWebResource that was loaded
     * @length_received: the amount of data received since the last signal emission
     *
     * Emitted when new resource data has been received. The
     * @length_received variable stores the amount of bytes received
     * since the last time this signal was emitted. This is useful to
     * provide progress information about the resource load operation.
     *
     * Since: 1.7.5
     */
    webkit_web_resource_signals[CONTENT_LENGTH_RECEIVED] = g_signal_new("content-length-received",
        G_TYPE_FROM_CLASS(webResourceClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1,
        G_TYPE_INT);

    /**
     * WebKitWebResource:uri:
     *
     * The URI of the web resource
     *
     * Since: 1.1.14
     */
    g_object_class_install_property(gobject_class,
                                    PROP_URI,
                                    g_param_spec_string(
                                    "uri",
                                    _("URI"),
                                    _("The URI of the resource"),
                                    NULL,
                                    (GParamFlags)(WEBKIT_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY)));
    /**
     * WebKitWebResource:mime-type:
     *
     * The MIME type of the web resource.
     *
     * Since: 1.1.14
     */
    g_object_class_install_property(gobject_class,
                                    PROP_MIME_TYPE,
                                    g_param_spec_string(
                                    "mime-type",
                                    _("MIME Type"),
                                    _("The MIME type of the resource"),
                                    NULL,
                                    WEBKIT_PARAM_READABLE));
    /**
     * WebKitWebResource:encoding:
     *
     * The encoding name to which the web resource was encoded in.
     *
     * Since: 1.1.14
     */
    g_object_class_install_property(gobject_class,
                                    PROP_ENCODING,
                                    g_param_spec_string(
                                    "encoding",
                                    _("Encoding"),
                                    _("The text encoding name of the resource"),
                                    NULL,
                                    WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebResource:frame-name:
     *
     * The frame name for the web resource.
     *
     * Since: 1.1.14
     */
    g_object_class_install_property(gobject_class,
                                    PROP_FRAME_NAME,
                                    g_param_spec_string(
                                    "frame-name",
                                    _("Frame Name"),
                                    _("The frame name of the resource"),
                                    NULL,
                                    WEBKIT_PARAM_READABLE));

    g_type_class_add_private(gobject_class, sizeof(WebKitWebResourcePrivate));
}

static void webkit_web_resource_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitWebResource* webResource = WEBKIT_WEB_RESOURCE(object);

    switch (prop_id) {
    case PROP_URI:
        g_value_set_string(value, webkit_web_resource_get_uri(webResource));
        break;
    case PROP_MIME_TYPE:
        g_value_set_string(value, webkit_web_resource_get_mime_type(webResource));
        break;
    case PROP_ENCODING:
        g_value_set_string(value, webkit_web_resource_get_encoding(webResource));
        break;
    case PROP_FRAME_NAME:
        g_value_set_string(value, webkit_web_resource_get_frame_name(webResource));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_web_resource_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    WebKitWebResource* webResource = WEBKIT_WEB_RESOURCE(object);

    switch (prop_id) {
    case PROP_URI:
        g_free(webResource->priv->uri);
        webResource->priv->uri = g_value_dup_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_web_resource_init(WebKitWebResource* webResource)
{
    webResource->priv = G_TYPE_INSTANCE_GET_PRIVATE(webResource, WEBKIT_TYPE_WEB_RESOURCE, WebKitWebResourcePrivate);
}

// internal use only
WebKitWebResource* webkit_web_resource_new_with_core_resource(PassRefPtr<ArchiveResource> resource)
{
    WebKitWebResource* webResource = WEBKIT_WEB_RESOURCE(g_object_new(WEBKIT_TYPE_WEB_RESOURCE, NULL));
    WebKitWebResourcePrivate* priv = webResource->priv;
    priv->resource = resource.leakRef();

    return webResource;
}

void webkit_web_resource_init_with_core_resource(WebKitWebResource* webResource, PassRefPtr<ArchiveResource> resource)
{
    ASSERT(resource);

    WebKitWebResourcePrivate* priv = webResource->priv;

    if (priv->resource)
        priv->resource->deref();

    priv->resource = resource.leakRef();
}

/**
 * webkit_web_resource_new:
 * @data: the data to initialize the #WebKitWebResource
 * @size: the length of @data
 * @uri: the URI of the #WebKitWebResource
 * @mime_type: the MIME type of the #WebKitWebResource
 * @encoding: the text encoding name of the #WebKitWebResource
 * @frame_name: the frame name of the #WebKitWebResource
 *
 * Returns a new #WebKitWebResource. The @encoding can be %NULL. The
 * @frame_name argument can be used if the resource represents contents of an
 * entire HTML frame, otherwise pass %NULL.
 *
 * Return value: a new #WebKitWebResource
 *
 * Since: 1.1.14
 */
WebKitWebResource* webkit_web_resource_new(const gchar* data,
                                           gssize size,
                                           const gchar* uri,
                                           const gchar* mimeType,
                                           const gchar* encoding,
                                           const gchar* frameName)
{
    g_return_val_if_fail(data, NULL);
    g_return_val_if_fail(uri, NULL);
    g_return_val_if_fail(mimeType, NULL);

    if (size < 0)
        size = strlen(data);

    RefPtr<SharedBuffer> buffer = SharedBuffer::create(data, size);
    WebKitWebResource* webResource = webkit_web_resource_new_with_core_resource(ArchiveResource::create(buffer, KURL(KURL(), String::fromUTF8(uri)), String::fromUTF8(mimeType), String::fromUTF8(encoding), String::fromUTF8(frameName)));

    return webResource;
}

/**
 * webkit_web_resource_get_data:
 * @web_resource: a #WebKitWebResource
 *
 * Returns the data of the @webResource.
 *
 * Return value: (transfer none): a #GString containing the character
 * data of the @webResource.  The string is owned by WebKit and should
 * not be freed or destroyed.
 *
 * Since: 1.1.14
 */
GString* webkit_web_resource_get_data(WebKitWebResource* webResource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_RESOURCE(webResource), NULL);

    WebKitWebResourcePrivate* priv = webResource->priv;

    if (!priv->resource)
        return NULL;

    if (!priv->data)
        priv->data = g_string_new_len(priv->resource->data()->data(), priv->resource->data()->size());

    return priv->data;
}

/**
 * webkit_web_resource_get_uri:
 * @web_resource: a #WebKitWebResource
 *
 * Return value: the URI of the resource
 *
 * Since: 1.1.14
 */
const gchar* webkit_web_resource_get_uri(WebKitWebResource* webResource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_RESOURCE(webResource), NULL);

    WebKitWebResourcePrivate* priv = webResource->priv;


    // We may have an URI without having a resource assigned to us (e.g., if the
    // FrameLoaderClient only had a ResourceRequest when we got created
    if (priv->uri)
        return priv->uri;

    if (!priv->resource)
        return NULL;

    priv->uri = g_strdup(priv->resource->url().string().utf8().data());

    return priv->uri;
}

/**
 * webkit_web_resource_get_mime_type:
 * @web_resource: a #WebKitWebResource
 *
 * Return value: the MIME type of the resource
 *
 * Since: 1.1.14
 */
const gchar* webkit_web_resource_get_mime_type(WebKitWebResource* webResource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_RESOURCE(webResource), NULL);

    WebKitWebResourcePrivate* priv = webResource->priv;
    if (!priv->resource)
        return NULL;

    if (!priv->mimeType)
        priv->mimeType = g_strdup(priv->resource->mimeType().utf8().data());

    return priv->mimeType;
}

/**
 * webkit_web_resource_get_encoding:
 * @web_resource: a #WebKitWebResource
 *
 * Return value: the encoding name of the resource
 *
 * Since: 1.1.14
 */
const gchar* webkit_web_resource_get_encoding(WebKitWebResource* webResource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_RESOURCE(webResource), NULL);

    WebKitWebResourcePrivate* priv = webResource->priv;
    if (!priv->resource)
        return NULL;

    if (!priv->textEncoding)
        priv->textEncoding = g_strdup(priv->resource->textEncoding().utf8().data());

    return priv->textEncoding;
}

/**
 * webkit_web_resource_get_frame_name:
 * @web_resource: a #WebKitWebResource
 *
 * Return value: the frame name of the resource.
 *
 * Since: 1.1.14
 */
const gchar* webkit_web_resource_get_frame_name(WebKitWebResource* webResource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_RESOURCE(webResource), NULL);

    WebKitWebResourcePrivate* priv = webResource->priv;
    if (!priv->resource)
        return NULL;

    if (!priv->frameName)
        priv->frameName = g_strdup(priv->resource->frameName().utf8().data());

    return priv->frameName;
}

