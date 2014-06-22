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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitWindowProperties_h
#define WebKitWindowProperties_h

#include <glib-object.h>
#include <gtk/gtk.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WINDOW_PROPERTIES            (webkit_window_properties_get_type())
#define WEBKIT_WINDOW_PROPERTIES(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WINDOW_PROPERTIES, WebKitWindowProperties))
#define WEBKIT_IS_WINDOW_PROPERTIES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WINDOW_PROPERTIES))
#define WEBKIT_WINDOW_PROPERTIES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WINDOW_PROPERTIES, WebKitWindowPropertiesClass))
#define WEBKIT_IS_WINDOW_PROPERTIES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WINDOW_PROPERTIES))
#define WEBKIT_WINDOW_PROPERTIES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WINDOW_PROPERTIES, WebKitWindowPropertiesClass))

typedef struct _WebKitWindowProperties WebKitWindowProperties;
typedef struct _WebKitWindowPropertiesClass WebKitWindowPropertiesClass;
typedef struct _WebKitWindowPropertiesPrivate WebKitWindowPropertiesPrivate;

struct _WebKitWindowProperties {
    GObject parent;

    /*< private >*/
    WebKitWindowPropertiesPrivate *priv;
};

struct _WebKitWindowPropertiesClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_window_properties_get_type                (void);

WEBKIT_API void
webkit_window_properties_get_geometry            (WebKitWindowProperties *window_properties,
                                                  GdkRectangle           *geometry);
WEBKIT_API gboolean
webkit_window_properties_get_toolbar_visible     (WebKitWindowProperties *window_properties);

WEBKIT_API gboolean
webkit_window_properties_get_statusbar_visible   (WebKitWindowProperties *window_properties);

WEBKIT_API gboolean
webkit_window_properties_get_scrollbars_visible  (WebKitWindowProperties *window_properties);

WEBKIT_API gboolean
webkit_window_properties_get_menubar_visible     (WebKitWindowProperties *window_properties);

WEBKIT_API gboolean
webkit_window_properties_get_locationbar_visible (WebKitWindowProperties *window_properties);

WEBKIT_API gboolean
webkit_window_properties_get_resizable           (WebKitWindowProperties *window_properties);

WEBKIT_API gboolean
webkit_window_properties_get_fullscreen          (WebKitWindowProperties *window_properties);

G_END_DECLS

#endif
