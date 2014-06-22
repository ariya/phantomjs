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

#ifndef webkitwebplugindatabase_h
#define webkitwebplugindatabase_h

#include <glib-object.h>

#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_PLUGIN_DATABASE            (webkit_web_plugin_database_get_type())
#define WEBKIT_WEB_PLUGIN_DATABASE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_PLUGIN_DATABASE, WebKitWebPluginDatabase))
#define WEBKIT_WEB_PLUGIN_DATABASE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), WEBKIT_TYPE_WEB_PLUGIN_DATABASE, WebKitWebPluginDatabaseClass))
#define WEBKIT_IS_WEB_PLUGIN_DATABASE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_PLUGIN_DATABASE))
#define WEBKIT_IS_WEB_PLUGIN_DATABASE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), WEBKIT_TYPE_WEB_PLUGIN_DATABASE))
#define WEBKIT_WEB_PLUGIN_DATABASE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), WEBKIT_TYPE_WEB_PLUGIN_DATABASE, WebKitWebPluginDatabaseClass))

typedef struct _WebKitWebPluginDatabasePrivate WebKitWebPluginDatabasePrivate;

struct _WebKitWebPluginDatabaseClass {
    GObjectClass parentClass;
};

struct _WebKitWebPluginDatabase {
    GObject parentInstance;

    WebKitWebPluginDatabasePrivate* priv;
};

WEBKIT_API GType
webkit_web_plugin_database_get_type                (void) G_GNUC_CONST;

WEBKIT_API void
webkit_web_plugin_database_plugins_list_free       (GSList                  *list);

WEBKIT_API GSList*
webkit_web_plugin_database_get_plugins             (WebKitWebPluginDatabase *database);

WEBKIT_API WebKitWebPlugin*
webkit_web_plugin_database_get_plugin_for_mimetype (WebKitWebPluginDatabase *database,
                                                    const char              *mime_type);

WEBKIT_API void
webkit_web_plugin_database_refresh                 (WebKitWebPluginDatabase *database);

G_END_DECLS

#endif
