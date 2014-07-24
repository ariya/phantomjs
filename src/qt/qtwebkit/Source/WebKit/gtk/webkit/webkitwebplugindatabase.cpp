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

#include "config.h"
#include "webkitwebplugindatabase.h"

#include "PluginDatabase.h"
#include "webkitglobalsprivate.h"
#include "webkitwebplugindatabaseprivate.h"
#include "webkitwebpluginprivate.h"

/**
 * SECTION:webkitwebplugindatabase
 * @short_description: Provides information about the plugins the engine knows about
 * @see_also: #WebKitWebPlugin
 *
 * This object allows you to query information about the plugins found
 * by the engine while scanning the usual directories. You can then
 * use the #WebKitWebPlugin objects to get more information or
 * enable/disable individual plugins.
 */

using namespace WebKit;
using namespace WebCore;

G_DEFINE_TYPE(WebKitWebPluginDatabase, webkit_web_plugin_database, G_TYPE_OBJECT)

static void webkit_web_plugin_database_dispose(GObject* object)
{
    G_OBJECT_CLASS(webkit_web_plugin_database_parent_class)->dispose(object);
}

static void webkit_web_plugin_database_class_init(WebKitWebPluginDatabaseClass* klass)
{
    webkitInit();

    GObjectClass* gobjectClass = reinterpret_cast<GObjectClass*>(klass);

    gobjectClass->dispose = webkit_web_plugin_database_dispose;

    g_type_class_add_private(klass, sizeof(WebKitWebPluginDatabasePrivate));
}

static void webkit_web_plugin_database_init(WebKitWebPluginDatabase* database)
{
    WebKitWebPluginDatabasePrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(database, WEBKIT_TYPE_WEB_PLUGIN_DATABASE, WebKitWebPluginDatabasePrivate);
    database->priv = priv;

    priv->coreDatabase = PluginDatabase::installedPlugins();
}

/**
 * webkit_web_plugin_database_plugins_list_free:
 * @list: (element-type WebKitWebPlugin): a #GSList of #WebKitWebPlugin
 *
 * Frees @list.
 *
 * Since: 1.3.8
 */
void webkit_web_plugin_database_plugins_list_free(GSList* list)
{
    if (!list)
        return;

    for (GSList* p = list; p; p = p->next)
        g_object_unref(p->data);

    g_slist_free(list);
}

/**
 * webkit_web_plugin_database_get_plugins:
 * @database: a #WebKitWebPluginDatabase
 *
 * Returns all #WebKitWebPlugin available in @database.
 * The returned list must be freed with webkit_web_plugin_database_plugins_list_free()
 *
 * Returns: (transfer full) (element-type WebKitWebPlugin): a #GSList of #WebKitWebPlugin
 *
 * Since: 1.3.8
 */
GSList* webkit_web_plugin_database_get_plugins(WebKitWebPluginDatabase* database)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_PLUGIN_DATABASE(database), 0);

    GSList* gPlugins = 0;
    const Vector<PluginPackage*>& plugins = database->priv->coreDatabase->plugins();

    for (unsigned int i = 0; i < plugins.size(); ++i) {
        PluginPackage* plugin = plugins[i];
        gPlugins = g_slist_append(gPlugins, kitNew(plugin));
    }

    return gPlugins;
}

/**
 * webkit_web_plugin_database_get_plugin_for_mimetype:
 * @database: a #WebKitWebPluginDatabase
 * @mime_type: a mime type
 *
 * Returns the #WebKitWebPlugin that is handling @mimeType in the
 * @database, or %NULL if there's none doing so.
 *
 * Returns: (transfer full): a #WebKitWebPlugin
 *
 * Since: 1.3.8
 */
WebKitWebPlugin* webkit_web_plugin_database_get_plugin_for_mimetype(WebKitWebPluginDatabase* database, const char* mimeType)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_PLUGIN_DATABASE(database), 0);
    g_return_val_if_fail(mimeType, 0);

    return kitNew(database->priv->coreDatabase->pluginForMIMEType(mimeType));
}

/**
 * webkit_web_plugin_database_refresh:
 * @database: a #WebKitWebPluginDatabase
 *
 * Refreshes @database adding new plugins that are now in use and
 * removing those that have been disabled or are otherwise no longer
 * available.
 *
 * Since: 1.3.8
 */
void webkit_web_plugin_database_refresh(WebKitWebPluginDatabase* database)
{
    g_return_if_fail(WEBKIT_IS_WEB_PLUGIN_DATABASE(database));

    database->priv->coreDatabase->refresh();
}

WebKitWebPluginDatabase* webkit_web_plugin_database_new(void)
{
    return WEBKIT_WEB_PLUGIN_DATABASE(g_object_new(WEBKIT_TYPE_WEB_PLUGIN_DATABASE, 0));
}
