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

#include "config.h"
#include "WebKitPlugin.h"

#include "WebKitMimeInfoPrivate.h"
#include "WebKitPluginPrivate.h"
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION: WebKitPlugin
 * @Short_description: Represents a plugin, enabling fine-grained control
 * @Title: WebKitPlugin
 *
 * This object represents a single plugin, found while scanning the
 * various platform plugin directories. This object can be used to get
 * more information about a plugin, and enable/disable it, allowing
 * fine-grained control of plugins. The list of available plugins can
 * be obtained from the #WebKitWebContext, with
 * webkit_web_context_get_plugins().
 *
 */

struct _WebKitPluginPrivate {
    ~_WebKitPluginPrivate()
    {
        g_list_free_full(mimeInfoList, reinterpret_cast<GDestroyNotify>(webkit_mime_info_unref));
    }

    PluginModuleInfo pluginInfo;
    CString name;
    CString description;
    CString path;
    GList* mimeInfoList;
};

WEBKIT_DEFINE_TYPE(WebKitPlugin, webkit_plugin, G_TYPE_OBJECT)

static void webkit_plugin_class_init(WebKitPluginClass* pluginClass)
{
}

WebKitPlugin* webkitPluginCreate(const PluginModuleInfo& pluginInfo)
{
    WebKitPlugin* plugin = WEBKIT_PLUGIN(g_object_new(WEBKIT_TYPE_PLUGIN, NULL));
    plugin->priv->pluginInfo = pluginInfo;
    return plugin;
}

/**
 * webkit_plugin_get_name:
 * @plugin: a #WebKitPlugin
 *
 * Returns: the name of the plugin.
 */
const char* webkit_plugin_get_name(WebKitPlugin* plugin)
{
    g_return_val_if_fail(WEBKIT_IS_PLUGIN(plugin), 0);

    if (!plugin->priv->name.isNull())
        return plugin->priv->name.data();

    if (plugin->priv->pluginInfo.info.name.isEmpty())
        return 0;

    plugin->priv->name = plugin->priv->pluginInfo.info.name.utf8();
    return plugin->priv->name.data();
}

/**
 * webkit_plugin_get_description:
 * @plugin: a #WebKitPlugin
 *
 * Returns: the description of the plugin.
 */
const char* webkit_plugin_get_description(WebKitPlugin* plugin)
{
    g_return_val_if_fail(WEBKIT_IS_PLUGIN(plugin), 0);

    if (!plugin->priv->description.isNull())
        plugin->priv->description.data();

    if (plugin->priv->pluginInfo.info.desc.isEmpty())
        return 0;

    plugin->priv->description = plugin->priv->pluginInfo.info.desc.utf8();
    return plugin->priv->description.data();
}

/**
 * webkit_plugin_get_path:
 * @plugin: a #WebKitPlugin
 *
 * Returns: the absolute path where the plugin is installed.
 */
const char* webkit_plugin_get_path(WebKitPlugin* plugin)
{
    g_return_val_if_fail(WEBKIT_IS_PLUGIN(plugin), 0);

    if (!plugin->priv->path.isNull())
        return plugin->priv->path.data();

    if (plugin->priv->pluginInfo.path.isEmpty())
        return 0;

    plugin->priv->path = plugin->priv->pluginInfo.path.utf8();
    return plugin->priv->path.data();
}

/**
 * webkit_plugin_get_mime_info_list:
 * @plugin: a #WebKitPlugin
 *
 * Get information about MIME types handled by the plugin,
 * as a list of #WebKitMimeInfo.
 *
 * Returns: (element-type WebKitMimeInfo) (transfer none): a #GList of #WebKitMimeInfo.
 */
GList* webkit_plugin_get_mime_info_list(WebKitPlugin* plugin)
{
    g_return_val_if_fail(WEBKIT_IS_PLUGIN(plugin), 0);

    if (plugin->priv->mimeInfoList)
        return plugin->priv->mimeInfoList;

    if (plugin->priv->pluginInfo.info.mimes.isEmpty())
        return 0;

    for (size_t i = 0; i < plugin->priv->pluginInfo.info.mimes.size(); ++i)
        plugin->priv->mimeInfoList = g_list_prepend(plugin->priv->mimeInfoList, webkitMimeInfoCreate(plugin->priv->pluginInfo.info.mimes[i]));
    return plugin->priv->mimeInfoList;
}
