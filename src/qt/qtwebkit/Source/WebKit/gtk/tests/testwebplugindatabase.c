/*
 * Copyright (C) 2010 Igalia S.L.
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

#include "autotoolsconfig.h"
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

/* This function is not public, so we need an extern declaration */
extern void webkit_web_settings_add_extra_plugin_directory(WebKitWebView* view, const gchar* directory);

static void test_webkit_web_plugin_database_get_plugins()
{
    WebKitWebView* view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    WebKitWebPluginDatabase* database;
    GSList* pluginList, *p;
    gboolean found = FALSE;
    gboolean enabled = FALSE;

    webkit_web_settings_add_extra_plugin_directory(view, TEST_PLUGIN_DIR);
    g_object_ref_sink(G_OBJECT(view));

    database = webkit_get_web_plugin_database();
    pluginList = webkit_web_plugin_database_get_plugins(database);
    for (p = pluginList; p; p = p->next) {
        WebKitWebPlugin* plugin = (WebKitWebPlugin*)p->data;
        if (!g_strcmp0(webkit_web_plugin_get_name(plugin), "WebKit Test PlugIn") &&
            !g_strcmp0(webkit_web_plugin_get_description(plugin), "Simple Netscape® plug-in that handles test content for WebKit")) {
            found = TRUE;
            enabled = webkit_web_plugin_get_enabled(plugin);
            webkit_web_plugin_set_enabled(plugin, FALSE);
        }
    }
    webkit_web_plugin_database_plugins_list_free(pluginList);
    g_assert(found);
    g_assert(enabled);

    webkit_web_plugin_database_refresh(database);
    pluginList = webkit_web_plugin_database_get_plugins(database);

    for (p = pluginList; p; p = p->next) {
        WebKitWebPlugin* plugin = (WebKitWebPlugin*)p->data;
        if (!g_strcmp0(webkit_web_plugin_get_name(plugin), "WebKit Test PlugIn") &&
            !g_strcmp0(webkit_web_plugin_get_description(plugin), "Simple Netscape® plug-in that handles test content for WebKit"))
            enabled = webkit_web_plugin_get_enabled(plugin);
    }
    webkit_web_plugin_database_plugins_list_free(pluginList);
    g_assert(!enabled);

    g_object_unref(view);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/webplugindatabase/getplugins", test_webkit_web_plugin_database_get_plugins);
    return g_test_run ();
}
