/*
 * Copyright (C) 2010 Joone Hur <joone@kldp.org>
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

#ifndef webkitviewportattributes_h
#define webkitviewportattributes_h

#include <glib-object.h>
#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_VIEWPORT_ATTRIBUTES            (webkit_viewport_attributes_get_type())
#define WEBKIT_VIEWPORT_ATTRIBUTES(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_VIEWPORT_ATTRIBUTES, WebKitViewportAttributes))
#define WEBKIT_VIEWPORT_ATTRIBUTES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_VIEWPORT_ATTRIBUTES, WebKitViewportAttributesClass))
#define WEBKIT_IS_VIEWPORT_ATTRIBUTES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_VIEWPORT_ATTRIBUTES))
#define WEBKIT_IS_VIEWPORT_ATTRIBUTES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_VIEWPORT_ATTRIBUTES))
#define WEBKIT_VIEWPORT_ATTRIBUTES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_VIEWPORT_ATTRIBUTES, WebKitViewportAttributesClass))

typedef struct _WebKitViewportAttributesPrivate WebKitViewportAttributesPrivate;

struct _WebKitViewportAttributes {
    GObject parent_instance;

    /*< private >*/
    WebKitViewportAttributesPrivate *priv;
};

struct _WebKitViewportAttributesClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_viewport_attributes_get_type                 (void);

WEBKIT_API void
webkit_viewport_attributes_recompute(WebKitViewportAttributes* viewportAttributes);

G_END_DECLS

#endif
