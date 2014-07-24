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

#ifndef WebKitPlugin_h
#define WebKitPlugin_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_PLUGIN            (webkit_plugin_get_type())
#define WEBKIT_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_PLUGIN, WebKitPlugin))
#define WEBKIT_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_PLUGIN))
#define WEBKIT_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_PLUGIN, WebKitPluginClass))
#define WEBKIT_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_PLUGIN))
#define WEBKIT_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_PLUGIN, WebKitPluginClass))

typedef struct _WebKitPlugin        WebKitPlugin;
typedef struct _WebKitPluginClass   WebKitPluginClass;
typedef struct _WebKitPluginPrivate WebKitPluginPrivate;

struct _WebKitPlugin {
    GObject parent;

    WebKitPluginPrivate *priv;
};

struct _WebKitPluginClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_plugin_get_type           (void);

WEBKIT_API const gchar *
webkit_plugin_get_name           (WebKitPlugin *plugin);

WEBKIT_API const gchar *
webkit_plugin_get_description    (WebKitPlugin *plugin);

WEBKIT_API const gchar *
webkit_plugin_get_path           (WebKitPlugin *plugin);

WEBKIT_API GList *
webkit_plugin_get_mime_info_list (WebKitPlugin *plugin);

G_END_DECLS

#endif
