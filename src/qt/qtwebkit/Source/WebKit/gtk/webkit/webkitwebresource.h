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

#ifndef webkitwebresource_h
#define webkitwebresource_h

#include <glib.h>
#include <glib-object.h>

#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_RESOURCE            (webkit_web_resource_get_type())
#define WEBKIT_WEB_RESOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_RESOURCE, WebKitWebResource))
#define WEBKIT_WEB_RESOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_RESOURCE, WebKitWebResourceClass))
#define WEBKIT_IS_WEB_RESOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_RESOURCE))
#define WEBKIT_IS_WEB_RESOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_RESOURCE))
#define WEBKIT_WEB_RESOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_RESOURCE, WebKitWebResourceClass))

typedef struct _WebKitWebResourcePrivate WebKitWebResourcePrivate;

struct _WebKitWebResource {
    GObject parent_instance;

    /*< private >*/
    WebKitWebResourcePrivate *priv;
};

struct _WebKitWebResourceClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_web_resource_get_type (void);

WEBKIT_API WebKitWebResource *
webkit_web_resource_new                    (const gchar        *data,
                                            gssize             size,
                                            const gchar        *uri,
                                            const gchar        *mime_type,
                                            const gchar        *encoding,
                                            const gchar        *frame_name);

WEBKIT_API GString *
webkit_web_resource_get_data               (WebKitWebResource  *web_resource);

WEBKIT_API const gchar *
webkit_web_resource_get_uri                (WebKitWebResource  *web_resource);

WEBKIT_API const gchar *
webkit_web_resource_get_mime_type          (WebKitWebResource  *web_resource);

WEBKIT_API const gchar *
webkit_web_resource_get_encoding           (WebKitWebResource  *web_resource);

WEBKIT_API const gchar *
webkit_web_resource_get_frame_name         (WebKitWebResource  *web_resource);

G_END_DECLS

#endif /* webkitwebresource_h */
