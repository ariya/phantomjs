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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitWebResource_h
#define WebKitWebResource_h

#include <glib-object.h>
#include <gio/gio.h>
#include <webkit2/WebKitDefines.h>
#include <webkit2/WebKitURIResponse.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_RESOURCE            (webkit_web_resource_get_type())
#define WEBKIT_WEB_RESOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_RESOURCE, WebKitWebResource))
#define WEBKIT_IS_WEB_RESOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_RESOURCE))
#define WEBKIT_WEB_RESOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_RESOURCE, WebKitWebResourceClass))
#define WEBKIT_IS_WEB_RESOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_RESOURCE))
#define WEBKIT_WEB_RESOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_RESOURCE, WebKitWebResourceClass))

typedef struct _WebKitWebResource        WebKitWebResource;
typedef struct _WebKitWebResourceClass   WebKitWebResourceClass;
typedef struct _WebKitWebResourcePrivate WebKitWebResourcePrivate;

struct _WebKitWebResource {
    GObject parent;

    WebKitWebResourcePrivate *priv;
};

struct _WebKitWebResourceClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_web_resource_get_type        (void);

WEBKIT_API const gchar *
webkit_web_resource_get_uri         (WebKitWebResource  *resource);

WEBKIT_API WebKitURIResponse *
webkit_web_resource_get_response    (WebKitWebResource  *resource);

WEBKIT_API void
webkit_web_resource_get_data        (WebKitWebResource  *resource,
                                     GCancellable       *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer            user_data);

WEBKIT_API guchar *
webkit_web_resource_get_data_finish (WebKitWebResource  *resource,
                                     GAsyncResult       *result,
                                     gsize              *length,
                                     GError            **error);

G_END_DECLS

#endif
