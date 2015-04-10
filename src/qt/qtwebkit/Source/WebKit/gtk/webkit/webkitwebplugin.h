/*
 *  Copyright (C) 2010 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef webkitwebplugin_h
#define webkitwebplugin_h

#include <glib-object.h>

#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_PLUGIN            (webkit_web_plugin_get_type())
#define WEBKIT_WEB_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_PLUGIN, WebKitWebPlugin))
#define WEBKIT_WEB_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), WEBKIT_TYPE_WEB_PLUGIN, WebKitWebPluginClass))
#define WEBKIT_IS_WEB_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_PLUGIN))
#define WEBKIT_IS_WEB_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), WEBKIT_TYPE_WEB_PLUGIN))
#define WEBKIT_WEB_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), WEBKIT_TYPE_WEB_PLUGIN, WebKitWebPluginClass))

typedef struct _WebKitWebPluginPrivate WebKitWebPluginPrivate;

/**
 * WebKitWebPluginMIMEType:
 * @name: the name of the MIME type.
 * @description: the description of the MIME type.
 * @extensions: a %NULL-terminated array with the extensions
 * associated with this MIME type.
 *
 * A structure representing one of the MIME types associated with a
 * plugin. A #GSList of these objects will be returned by
 * #webkit_web_plugin_get_mimetypes, use
 * #webkit_web_plugin_mime_type_list_free to free it.
 *
 * Since: 1.3.8
 */
typedef struct _WebKitWebPluginMIMEType {
    char* name;
    char* description;
    char** extensions;
} WebKitWebPluginMIMEType;

struct _WebKitWebPluginClass {
    GObjectClass parentClass;
};

struct _WebKitWebPlugin {
    GObject parentInstance;

    WebKitWebPluginPrivate* priv;
};

WEBKIT_API GType
webkit_web_plugin_get_type        (void) G_GNUC_CONST;

WEBKIT_API const char*
webkit_web_plugin_get_name        (WebKitWebPlugin *plugin);

WEBKIT_API const char*
webkit_web_plugin_get_description (WebKitWebPlugin *plugin);

WEBKIT_API const char*
webkit_web_plugin_get_path        (WebKitWebPlugin *plugin);

WEBKIT_API GSList*
webkit_web_plugin_get_mimetypes   (WebKitWebPlugin *plugin);

WEBKIT_API void
webkit_web_plugin_set_enabled     (WebKitWebPlugin *plugin,
                                   gboolean        enabled);

WEBKIT_API gboolean
webkit_web_plugin_get_enabled     (WebKitWebPlugin *plugin);

G_END_DECLS

#endif
