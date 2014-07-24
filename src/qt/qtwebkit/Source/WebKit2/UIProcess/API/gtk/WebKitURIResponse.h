/*
 * Copyright (C) 2011 Igalia S.L.
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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION) && !defined(__WEBKIT_WEB_EXTENSION_H_INSIDE__)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitURIResponse_h
#define WebKitURIResponse_h

#include <gio/gio.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_URI_RESPONSE            (webkit_uri_response_get_type())
#define WEBKIT_URI_RESPONSE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_URI_RESPONSE, WebKitURIResponse))
#define WEBKIT_IS_URI_RESPONSE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_URI_RESPONSE))
#define WEBKIT_URI_RESPONSE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_URI_RESPONSE, WebKitURIResponseClass))
#define WEBKIT_IS_URI_RESPONSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_URI_RESPONSE))
#define WEBKIT_URI_RESPONSE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_URI_RESPONSE, WebKitURIResponseClass))

typedef struct _WebKitURIResponse WebKitURIResponse;
typedef struct _WebKitURIResponseClass WebKitURIResponseClass;
typedef struct _WebKitURIResponsePrivate WebKitURIResponsePrivate;

struct _WebKitURIResponse {
    GObject parent;

    /*< private >*/
    WebKitURIResponsePrivate *priv;
};

struct _WebKitURIResponseClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_uri_response_get_type               (void);

WEBKIT_API const gchar *
webkit_uri_response_get_uri                (WebKitURIResponse    *response);

WEBKIT_API guint
webkit_uri_response_get_status_code        (WebKitURIResponse    *response);

WEBKIT_API guint64
webkit_uri_response_get_content_length     (WebKitURIResponse    *response);

WEBKIT_API const gchar *
webkit_uri_response_get_mime_type          (WebKitURIResponse    *response);

WEBKIT_API const gchar *
webkit_uri_response_get_suggested_filename (WebKitURIResponse    *response);

G_END_DECLS

#endif
