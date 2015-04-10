/*
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

#ifndef webkitnewtorkresponse_h
#define webkitnewtorkresponse_h

#include <glib-object.h>
#include <libsoup/soup.h>

#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_NETWORK_RESPONSE            (webkit_network_response_get_type())
#define WEBKIT_NETWORK_RESPONSE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_NETWORK_RESPONSE, WebKitNetworkResponse))
#define WEBKIT_NETWORK_RESPONSE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_NETWORK_RESPONSE, WebKitNetworkResponseClass))
#define WEBKIT_IS_NETWORK_RESPONSE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_NETWORK_RESPONSE))
#define WEBKIT_IS_NETWORK_RESPONSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_NETWORK_RESPONSE))
#define WEBKIT_NETWORK_RESPONSE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_NETWORK_RESPONSE, WebKitNetworkResponseClass))

typedef struct _WebKitNetworkResponsePrivate WebKitNetworkResponsePrivate;

struct _WebKitNetworkResponse {
    GObject parent_instance;

    /*< private >*/
    WebKitNetworkResponsePrivate *priv;
};

struct _WebKitNetworkResponseClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_network_response_get_type (void);

WEBKIT_API WebKitNetworkResponse *
webkit_network_response_new      (const gchar          *uri);

WEBKIT_API void
webkit_network_response_set_uri  (WebKitNetworkResponse *response,
                                  const gchar*          uri);

WEBKIT_API const gchar *
webkit_network_response_get_uri  (WebKitNetworkResponse *response);

WEBKIT_API SoupMessage *
webkit_network_response_get_message(WebKitNetworkResponse* response);

WEBKIT_API const char *
webkit_network_response_get_suggested_filename(WebKitNetworkResponse *response);

G_END_DECLS

#endif
