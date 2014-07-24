/*
 * Copyright (C) 2007 Holger Hans Peter Freyther
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

#ifndef webkitnetworkrequest_h
#define webkitnetworkrequest_h

#include <glib-object.h>
#include <libsoup/soup.h>

#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_NETWORK_REQUEST            (webkit_network_request_get_type())
#define WEBKIT_NETWORK_REQUEST(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_NETWORK_REQUEST, WebKitNetworkRequest))
#define WEBKIT_NETWORK_REQUEST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_NETWORK_REQUEST, WebKitNetworkRequestClass))
#define WEBKIT_IS_NETWORK_REQUEST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_NETWORK_REQUEST))
#define WEBKIT_IS_NETWORK_REQUEST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_NETWORK_REQUEST))
#define WEBKIT_NETWORK_REQUEST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_NETWORK_REQUEST, WebKitNetworkRequestClass))

typedef struct _WebKitNetworkRequestPrivate WebKitNetworkRequestPrivate;

struct _WebKitNetworkRequest {
    GObject parent_instance;

    /*< private >*/
    WebKitNetworkRequestPrivate *priv;
};

struct _WebKitNetworkRequestClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_network_request_get_type (void);

WEBKIT_API WebKitNetworkRequest *
webkit_network_request_new      (const gchar          *uri);

WEBKIT_API void
webkit_network_request_set_uri  (WebKitNetworkRequest *request,
                                 const gchar*          uri);

WEBKIT_API const gchar *
webkit_network_request_get_uri  (WebKitNetworkRequest *request);

WEBKIT_API SoupMessage *
webkit_network_request_get_message(WebKitNetworkRequest* request);

G_END_DECLS

#endif
