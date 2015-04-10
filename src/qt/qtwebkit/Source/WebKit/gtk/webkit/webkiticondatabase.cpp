/*
 * Copyright (C) 2011 Christian Dywan <christian@lanedo.com>
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
#include "webkiticondatabase.h"

#include "FileSystem.h"
#include "IconDatabase.h"
#include "Image.h"
#include "IntSize.h"
#include "webkitglobalsprivate.h"
#include "webkitmarshal.h"
#include "webkitsecurityoriginprivate.h"
#include "webkitwebframe.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

/**
 * SECTION:webkiticondatabase
 * @short_description: A WebKit web application database
 *
 * #WebKitIconDatabase provides access to website icons, as shown
 * in tab labels, window captions or bookmarks. All views share
 * the same icon database.
 *
 * The icon database is enabled by default and stored in
 * ~/.local/share/webkit/icondatabase, depending on XDG_DATA_HOME.
 *
 * WebKit will automatically look for available icons in link elements
 * on opened pages as well as an existing favicon.ico and load the
 * images found into the memory cache if possible. The signal "icon-loaded"
 * will be emitted when any icon is found and loaded.
 * Old Icons are automatically cleaned up after 4 days.
 *
 * webkit_icon_database_set_path() can be used to change the location
 * of the database and also to disable it by passing %NULL.
 *
 * If WebKitWebSettings::enable-private-browsing is %TRUE new icons
 * won't be added to the database on disk and no existing icons will
 * be deleted from it.
 *
 * Since: 1.3.13
 *
 * Deprecated: 1.8: Use WebKitFaviconDatabase instead.
 */

using namespace WebKit;

enum {
    PROP_0,

    PROP_PATH,
};

enum {
    ICON_LOADED,

    LAST_SIGNAL
};

static guint webkit_icon_database_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(WebKitIconDatabase, webkit_icon_database, G_TYPE_OBJECT);

struct _WebKitIconDatabasePrivate {
    GOwnPtr<gchar> path;
};

static void webkit_icon_database_finalize(GObject* object)
{
    // Call C++ destructors, the reverse of 'placement new syntax'
    WEBKIT_ICON_DATABASE(object)->priv->~WebKitIconDatabasePrivate();

    G_OBJECT_CLASS(webkit_icon_database_parent_class)->finalize(object);
}

static void webkit_icon_database_dispose(GObject* object)
{
    G_OBJECT_CLASS(webkit_icon_database_parent_class)->dispose(object);
}

static void webkit_icon_database_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec)
{
    WebKitIconDatabase* database = WEBKIT_ICON_DATABASE(object);

    switch (propId) {
    case PROP_PATH:
        webkit_icon_database_set_path(database, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webkit_icon_database_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec)
{
    WebKitIconDatabase* database = WEBKIT_ICON_DATABASE(object);

    switch (propId) {
    case PROP_PATH:
        g_value_set_string(value, webkit_icon_database_get_path(database));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webkit_icon_database_class_init(WebKitIconDatabaseClass* klass)
{
    webkitInit();

    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->dispose = webkit_icon_database_dispose;
    gobjectClass->finalize = webkit_icon_database_finalize;
    gobjectClass->set_property = webkit_icon_database_set_property;
    gobjectClass->get_property = webkit_icon_database_get_property;

     /**
      * WebKitIconDatabase:path:
      *
      * The absolute path of the icon database folder.
      *
      * Since: 1.3.13
      *
      * Deprecated: 1.8: Use WebKitFaviconDatabase::path instead.
      */
     g_object_class_install_property(gobjectClass, PROP_PATH,
                                     g_param_spec_string("path",
                                                         _("Path"),
                                                         _("The absolute path of the icon database folder"),
                                                         NULL,
                WEBKIT_PARAM_READWRITE));

    /**
     * WebKitIconDatabase::icon-loaded:
     * @database: the object on which the signal is emitted
     * @frame: the frame containing the icon
     * @frame_uri: the URI of the frame containing the icon
     *
     * This signal is emitted when a favicon is available for a page,
     * or a child frame.
     * See WebKitWebView::icon-loaded if you only need the favicon for
     * the main frame of a particular #WebKitWebView.
     *
     * Since: 1.3.13
     *
     * Deprecated: 1.8: Use WebKitFaviconDatabase::icon-loaded instead.
     */
    webkit_icon_database_signals[ICON_LOADED] = g_signal_new("icon-loaded",
        G_TYPE_FROM_CLASS(klass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        webkit_marshal_VOID__OBJECT_STRING,
        G_TYPE_NONE, 2,
        WEBKIT_TYPE_WEB_FRAME,
        G_TYPE_STRING);

    g_type_class_add_private(klass, sizeof(WebKitIconDatabasePrivate));
}

static void webkit_icon_database_init(WebKitIconDatabase* database)
{
    database->priv = G_TYPE_INSTANCE_GET_PRIVATE(database, WEBKIT_TYPE_ICON_DATABASE, WebKitIconDatabasePrivate);
    // 'placement new syntax', see webkitwebview.cpp
    new (database->priv) WebKitIconDatabasePrivate();
}

/**
 * webkit_icon_database_get_path:
 * @database: a #WebKitIconDatabase
 *
 * Determines the absolute path to the database folder on disk.
 *
 * Returns: the absolute path of the database folder, or %NULL
 *
 * Since: 1.3.13
 *
 * Deprecated: 1.8: Use webkit_favicon_database_get_path() instead.
 **/
const gchar* webkit_icon_database_get_path(WebKitIconDatabase* database)
{
    g_return_val_if_fail(WEBKIT_IS_ICON_DATABASE(database), 0);

    return database->priv->path.get();
}

static void closeIconDatabaseOnExit()
{
    if (WebCore::iconDatabase().isEnabled()) {
        WebCore::iconDatabase().setEnabled(false);
        WebCore::iconDatabase().close();
    }
}

/**
 * webkit_icon_database_set_path:
 * @database: a #WebKitIconDatabase
 * @path: an absolute path to the icon database folder
 *
 * Specifies the absolute path to the database folder on disk.
 *
 * Passing %NULL or "" disables the icon database.
 *
 * Since: 1.3.13
 *
 * Deprecated: 1.8: Use webkit_favicon_database_set_path() instead.
 **/
void webkit_icon_database_set_path(WebKitIconDatabase* database, const gchar* path)
{
    g_return_if_fail(WEBKIT_IS_ICON_DATABASE(database));

    if (database->priv->path.get())
        WebCore::iconDatabase().close();

    if (!(path && path[0])) {
        database->priv->path.set(0);
        WebCore::iconDatabase().setEnabled(false);
        return;
    }

    database->priv->path.set(g_strdup(path));

    WebCore::iconDatabase().setEnabled(true);
    WebCore::iconDatabase().open(WebCore::filenameToString(database->priv->path.get()), WebCore::IconDatabase::defaultDatabaseFilename());

    static bool initialized = false;
    if (!initialized) {
        atexit(closeIconDatabaseOnExit);
        initialized = true;
    }
}

/**
 * webkit_icon_database_get_icon_uri:
 * @database: a #WebKitIconDatabase
 * @page_uri: URI of the page containing the icon
 *
 * Obtains the URI for the favicon for the given page URI.
 * See also webkit_web_view_get_icon_uri().
 *
 * Returns: a newly allocated URI for the favicon, or %NULL
 *
 * Since: 1.3.13
 *
 * Deprecated: 1.8: Use webkit_favicon_database_get_favicon_uri() instead.
 **/
gchar* webkit_icon_database_get_icon_uri(WebKitIconDatabase* database, const gchar* pageURI)
{
    g_return_val_if_fail(WEBKIT_IS_ICON_DATABASE(database), 0);
    g_return_val_if_fail(pageURI, 0);

    String pageURL = String::fromUTF8(pageURI);
    return g_strdup(WebCore::iconDatabase().synchronousIconURLForPageURL(pageURL).utf8().data());
}

/**
 * webkit_icon_database_get_icon_pixbuf:
 * @database: a #WebKitIconDatabase
 * @page_uri: URI of the page containing the icon
 *
 * Obtains a #GdkPixbuf of the favicon for the given page URI, or
 * a default icon if there is no icon for the given page. Use
 * webkit_icon_database_get_icon_uri() if you need to distinguish these cases.
 * Usually you want to connect to WebKitIconDatabase::icon-loaded and call this
 * method in the callback.
 *
 * The pixbuf will have the largest size provided by the server and should
 * be resized before it is displayed.
 * See also webkit_web_view_get_icon_pixbuf().
 *
 * Returns: (transfer full): a new reference to a #GdkPixbuf, or %NULL
 *
 * Since: 1.3.13
 *
 * Deprecated: 1.8: Use webkit_favicon_database_try_get_favicon_pixbuf() instead.
 **/
GdkPixbuf* webkit_icon_database_get_icon_pixbuf(WebKitIconDatabase* database, const gchar* pageURI)
{
    g_return_val_if_fail(WEBKIT_IS_ICON_DATABASE(database), 0);
    g_return_val_if_fail(pageURI, 0);

    String pageURL = String::fromUTF8(pageURI);
    // The exact size we pass is irrelevant to the WebCore::iconDatabase code.
    // We must pass something greater than 0, 0 to get a pixbuf.
    WebCore::Image* icon = WebCore::iconDatabase().synchronousIconForPageURL(pageURL, WebCore::IntSize(16, 16));
    if (!icon)
        return 0;
    GdkPixbuf* pixbuf = icon->getGdkPixbuf();
    if (!pixbuf)
        return 0;
    return static_cast<GdkPixbuf*>(g_object_ref(pixbuf));
}

/**
 * webkit_icon_database_clear:
 * @database: a #WebKitIconDatabase
 *
 * Clears all icons from the database.
 *
 * Since: 1.3.13
 *
 * Deprecated: 1.8: Use webkit_favicon_database_clear() instead.
 **/
void webkit_icon_database_clear(WebKitIconDatabase* database)
{
    g_return_if_fail(WEBKIT_IS_ICON_DATABASE(database));

    WebCore::iconDatabase().removeAllIcons();
}

