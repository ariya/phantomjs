/*
 * Copyright (C) 2011 Christian Dywan <christian@lanedo.com>
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
#include "webkitfavicondatabase.h"

#include "FileSystem.h"
#include "GdkCairoUtilities.h"
#include "IconDatabase.h"
#include "IconDatabaseClient.h"
#include "Image.h"
#include "IntSize.h"
#include "webkitfavicondatabaseprivate.h"
#include "webkitglobals.h"
#include "webkitglobalsprivate.h"
#include "webkitmarshal.h"
#include "webkitsecurityoriginprivate.h"
#include "webkitwebframe.h"
#include <glib/gi18n-lib.h>
#include <wtf/MainThread.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

/**
 * SECTION:webkitfavicondatabase
 * @short_description: A WebKit favicon database
 * @Title: WebKitFaviconDatabase
 *
 * #WebKitFaviconDatabase provides access to the icons associated with
 * web sites.
 *
 * WebKit will automatically look for available icons in link elements
 * on opened pages as well as an existing favicon.ico and load the
 * images found into a memory cache if possible. That cache is frozen
 * to an on-disk database for persistence.
 *
 * The database is disabled by default. In order for icons to be
 * stored and accessed, you will need to set an icon database path
 * using webkit_favicon_database_set_path(). Disable the database
 * again passing %NULL to the previous call.
 *
 * If WebKitWebSettings::enable-private-browsing is %TRUE new icons
 * won't be added to the on-disk database and no existing icons will
 * be deleted from it. Nevertheless, WebKit will still store them in
 * the in-memory cache during the current execution.
 *
 * Since: 1.8
 */

using namespace WebKit;
using namespace WebCore;

class PendingIconRequest;

static void webkitFaviconDatabaseProcessPendingIconsForURI(WebKitFaviconDatabase*, const String& pageURI);
static void webkitFaviconDatabaseImportFinished(WebKitFaviconDatabase*);
static void webkitFaviconDatabaseGetIconPixbufCancelled(GCancellable*, PendingIconRequest*);
static void webkitFaviconDatabaseClose(WebKitFaviconDatabase* database);

class IconDatabaseClientGtk : public IconDatabaseClient {
public:
    // IconDatabaseClient interface
    virtual void didRemoveAllIcons() { };

    // Called when an icon is requested while the initial import is
    // going on.
    virtual void didImportIconURLForPageURL(const String& URL) { };

    // Called whenever a retained icon is read from database.
    virtual void didImportIconDataForPageURL(const String& URL)
    {
        WebKitFaviconDatabase* database = webkit_get_favicon_database();
        // We need to emit this here because webkitFaviconDatabaseDispatchDidReceiveIcon()
        // is only called for icons that have just being downloaded, and this is called
        // when icon data is imported from the database.
        g_signal_emit_by_name(database, "icon-loaded", URL.utf8().data());

        webkitFaviconDatabaseProcessPendingIconsForURI(database, URL);
    }

    virtual void didChangeIconForPageURL(const String& URL)
    {
        // Called when the the favicon for a particular URL changes.
        // It does not mean that the new icon data is available yet.
    }

    virtual void didFinishURLImport()
    {
        webkitFaviconDatabaseImportFinished(webkit_get_favicon_database());

        // Now that everything is imported enable pruning of old
        // icons. No icon will be removed during the import process
        // because we disable cleanups before opening the database.
        IconDatabase::allowDatabaseCleanup();
    }
};

class PendingIconRequest {
public:
    PendingIconRequest(const String& pageURL, GSimpleAsyncResult* result, GCancellable* cancellable, IntSize iconSize)
        : m_pageURL(pageURL)
        , m_asyncResult(result)
        , m_cancellable(cancellable)
        , m_cancelledId(0)
        , m_iconSize(iconSize)
    {
        if (cancellable) {
            m_cancelledId = g_cancellable_connect(cancellable, G_CALLBACK(webkitFaviconDatabaseGetIconPixbufCancelled), this, 0);
            g_object_set_data_full(G_OBJECT(result), "cancellable", g_object_ref(cancellable), static_cast<GDestroyNotify>(g_object_unref));
        }
    }

    ~PendingIconRequest()
    {
        if (m_cancelledId > 0)
            g_cancellable_disconnect(m_cancellable.get(), m_cancelledId);
    }

    const String& pageURL() { return m_pageURL; }
    GSimpleAsyncResult* asyncResult() { return m_asyncResult.get(); }
    const IntSize& iconSize() { return m_iconSize; }

    void asyncResultCancel()
    {
        ASSERT(m_asyncResult);
        g_simple_async_result_set_error(m_asyncResult.get(), G_IO_ERROR, G_IO_ERROR_CANCELLED, "%s", _("Operation was cancelled"));
        g_simple_async_result_complete(m_asyncResult.get());
    }

    void asyncResultCompleteInIdle(GdkPixbuf* icon)
    {
        ASSERT(m_asyncResult);
        g_simple_async_result_set_op_res_gpointer(m_asyncResult.get(), icon, 0);
        g_simple_async_result_complete_in_idle(m_asyncResult.get());
    }

    void asyncResultComplete(GdkPixbuf* icon)
    {
        ASSERT(m_asyncResult);
        g_simple_async_result_set_op_res_gpointer(m_asyncResult.get(), icon, 0);
        g_simple_async_result_complete(m_asyncResult.get());
    }

private:
    String m_pageURL;
    GRefPtr<GSimpleAsyncResult> m_asyncResult;
    GRefPtr<GCancellable> m_cancellable;
    gulong m_cancelledId;
    IntSize m_iconSize;
};


enum {
    PROP_0,

    PROP_PATH,
};

enum {
    ICON_LOADED,

    LAST_SIGNAL
};

static guint webkit_favicon_database_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(WebKitFaviconDatabase, webkit_favicon_database, G_TYPE_OBJECT)

typedef Vector<OwnPtr<PendingIconRequest> > PendingIconRequestVector;
typedef HashMap<String, PendingIconRequestVector*> PendingIconRequestMap;

struct _WebKitFaviconDatabasePrivate {
    GOwnPtr<gchar> path;
    IconDatabaseClientGtk iconDatabaseClient;
    PendingIconRequestMap pendingIconRequests;
    bool importFinished;
};

static void webkit_favicon_database_finalize(GObject* object)
{
    WebKitFaviconDatabase* database = WEBKIT_FAVICON_DATABASE(object);

    webkitFaviconDatabaseClose(database);
    database->priv->~WebKitFaviconDatabasePrivate();

    G_OBJECT_CLASS(webkit_favicon_database_parent_class)->finalize(object);
}

static void webkit_favicon_database_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec)
{
    WebKitFaviconDatabase* database = WEBKIT_FAVICON_DATABASE(object);

    switch (propId) {
    case PROP_PATH:
        webkit_favicon_database_set_path(database, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webkit_favicon_database_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec)
{
    WebKitFaviconDatabase* database = WEBKIT_FAVICON_DATABASE(object);

    switch (propId) {
    case PROP_PATH:
        g_value_set_string(value, webkit_favicon_database_get_path(database));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webkit_favicon_database_class_init(WebKitFaviconDatabaseClass* klass)
{
    webkitInit();

    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = webkit_favicon_database_finalize;
    gobjectClass->set_property = webkit_favicon_database_set_property;
    gobjectClass->get_property = webkit_favicon_database_get_property;

    /**
     * WebKitFaviconDatabase:path:
     *
     * The absolute path of the icon database folder.
     *
     * Since: 1.8
     */
    g_object_class_install_property(gobjectClass, PROP_PATH,
                                     g_param_spec_string("path",
                                                         _("Path"),
                                                         _("The absolute path of the icon database folder"),
                                                         NULL,
                                                         WEBKIT_PARAM_READWRITE));


    /**
     * WebKitFaviconDatabase::icon-loaded:
     * @database: the object on which the signal is emitted
     * @frame_uri: the URI of the main frame of a Web page containing
     * the icon
     *
     * This signal is fired if an icon is loaded on any
     * #WebKitWebView. If you are only interested in a particular
     * #WebKitWebView see #WebKitWebView::icon-loaded.
     *
     * Note that this signal carries the URI of the frame that loads
     * the icon, while #WebKitWebView::icon-loaded provides the URI
     * of the favicon.
     *
     * Since: 1.8
     */
    webkit_favicon_database_signals[ICON_LOADED] = g_signal_new("icon-loaded",
        G_TYPE_FROM_CLASS(klass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0, 0, 0,
        webkit_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    g_type_class_add_private(klass, sizeof(WebKitFaviconDatabasePrivate));
}

static void webkit_favicon_database_init(WebKitFaviconDatabase* database)
{
    database->priv = G_TYPE_INSTANCE_GET_PRIVATE(database, WEBKIT_TYPE_FAVICON_DATABASE, WebKitFaviconDatabasePrivate);
    new (database->priv) WebKitFaviconDatabasePrivate();
}

// Called from FrameLoaderClient::dispatchDidReceiveIcon()
void webkitFaviconDatabaseDispatchDidReceiveIcon(WebKitFaviconDatabase* database, const char* frameURI)
{
    g_signal_emit(database, webkit_favicon_database_signals[ICON_LOADED], 0, frameURI);

    // Retain the new icon.
    iconDatabase().retainIconForPageURL(String::fromUTF8(frameURI));
}

/**
 * webkit_favicon_database_get_path:
 * @database: a #WebKitFaviconDatabase
 *
 * Determines the absolute path to the database folder on disk.
 *
 * Returns: the absolute path of the database folder, or %NULL
 *
 * Since: 1.8
 */
const gchar* webkit_favicon_database_get_path(WebKitFaviconDatabase* database)
{
    g_return_val_if_fail(WEBKIT_IS_FAVICON_DATABASE(database), 0);

    return database->priv->path.get();
}

static void webkitFaviconDatabaseClose(WebKitFaviconDatabase* database)
{
    if (iconDatabase().isEnabled()) {
        iconDatabase().setEnabled(false);
        iconDatabase().close();
    }
}

/**
 * webkit_favicon_database_set_path:
 * @database: a #WebKitFaviconDatabase
 * @path: (allow-none): an absolute path to the icon database folder
 * or %NULL to disable the database
 *
 * Specifies the absolute path to the database folder on disk. The
 * icon database will only be enabled after a call to this method.
 *
 * Passing %NULL or "" as path disables the icon database.
 *
 * Since: 1.8
 */
void webkit_favicon_database_set_path(WebKitFaviconDatabase* database, const gchar* path)
{
    g_return_if_fail(WEBKIT_IS_FAVICON_DATABASE(database));

    // Always try to close because the deprecated icondatabase is opened by default.
    webkitFaviconDatabaseClose(database);

    database->priv->importFinished = false;
    if (!path || !path[0]) {
        database->priv->path.set(0);
        iconDatabase().setEnabled(false);
        return;
    }

    iconDatabase().setClient(&database->priv->iconDatabaseClient);
    IconDatabase::delayDatabaseCleanup();
    iconDatabase().setEnabled(true);
    if (!iconDatabase().open(filenameToString(path), IconDatabase::defaultDatabaseFilename())) {
        IconDatabase::allowDatabaseCleanup();
        return;
    }

    database->priv->path.set(g_strdup(path));
}

/**
 * webkit_favicon_database_get_favicon_uri:
 * @database: a #WebKitFaviconDatabase
 * @page_uri: URI of the page containing the icon
 *
 * Obtains the URI for the favicon for the given page URI.
 * See also webkit_web_view_get_icon_uri().
 *
 * Returns: a newly allocated URI for the favicon, or %NULL
 *
 * Since: 1.8
 */
gchar* webkit_favicon_database_get_favicon_uri(WebKitFaviconDatabase* database, const gchar* pageURI)
{
    g_return_val_if_fail(WEBKIT_IS_FAVICON_DATABASE(database), 0);
    g_return_val_if_fail(pageURI, 0);
    ASSERT(isMainThread());

    String iconURI = iconDatabase().synchronousIconURLForPageURL(String::fromUTF8(pageURI));
    if (iconURI.isEmpty())
        return 0;

    return g_strdup(iconURI.utf8().data());
}

static GdkPixbuf* getIconPixbufSynchronously(WebKitFaviconDatabase* database, const String& pageURL, const IntSize& iconSize)
{
    ASSERT(isMainThread());

    // The exact size we pass is irrelevant to the iconDatabase code.
    // We must pass something greater than 0x0 to get a pixbuf.
    RefPtr<cairo_surface_t> surface = iconDatabase().synchronousNativeIconForPageURL(pageURL, !iconSize.isZero() ? iconSize : IntSize(1, 1));
    if (!surface)
        return 0;

    GRefPtr<GdkPixbuf> pixbuf = adoptGRef(cairoImageSurfaceToGdkPixbuf(surface.get()));
    if (!pixbuf)
        return 0;

    // A size of (0, 0) means the maximum available size.
    int pixbufWidth = gdk_pixbuf_get_width(pixbuf.get());
    int pixbufHeight = gdk_pixbuf_get_height(pixbuf.get());
    if (!iconSize.isZero() && (pixbufWidth != iconSize.width() || pixbufHeight != iconSize.height()))
        pixbuf = adoptGRef(gdk_pixbuf_scale_simple(pixbuf.get(), iconSize.width(), iconSize.height(), GDK_INTERP_BILINEAR));
    return pixbuf.leakRef();
}

/**
 * webkit_favicon_database_try_get_favicon_pixbuf:
 * @database: a #WebKitFaviconDatabase
 * @page_uri: URI of the page containing the icon
 * @width: the desired width for the icon
 * @height: the desired height for the icon
 *
 * Obtains a #GdkPixbuf of the favicon for the given page URI, or
 * %NULL if there is no icon for the given page or it hasn't been
 * loaded from disk yet. Use webkit_favicon_database_get_favicon_uri()
 * if you need to distinguish these cases.  To make sure this method
 * will return a valid icon when the given URI has one, you should
 * connect to #WebKitFaviconDatabase::icon-loaded and use this function
 * in the callback.
 *
 * If @width and @height ar both 0 then this method will return the
 * maximum available size for the icon. Note that if you specify a
 * different size the icon will be scaled each time you call this
 * function.
 *
 * Returns: (transfer full): a new reference to a #GdkPixbuf, or %NULL
 * if the given URI doesn't have an icon or it hasn't been loaded yet.
 *
 * Since: 1.8
 */
GdkPixbuf* webkit_favicon_database_try_get_favicon_pixbuf(WebKitFaviconDatabase* database, const gchar* pageURI, guint width, guint height)
{
    g_return_val_if_fail(WEBKIT_IS_FAVICON_DATABASE(database), 0);
    g_return_val_if_fail(pageURI, 0);
    g_return_val_if_fail((width && height) || (!width && !height), 0);

    return getIconPixbufSynchronously(database, String::fromUTF8(pageURI), IntSize(width, height));
}

static PendingIconRequestVector* webkitFaviconDatabaseGetOrCreateRequests(WebKitFaviconDatabase* database, const String& pageURL)
{
    PendingIconRequestVector* icons = database->priv->pendingIconRequests.get(pageURL);
    if (!icons) {
        icons = new PendingIconRequestVector;
        database->priv->pendingIconRequests.set(pageURL, icons);
    }

    return icons;
}

static void webkitfavicondatabaseDeleteRequests(WebKitFaviconDatabase* database, PendingIconRequestVector* requests, const String& pageURL)
{
    database->priv->pendingIconRequests.remove(pageURL);
    delete requests;
}

static void getIconPixbufCancelled(void* userData)
{
    PendingIconRequest* request = static_cast<PendingIconRequest*>(userData);
    request->asyncResultCancel();

    const String& pageURL = request->pageURL();
    WebKitFaviconDatabase* database = webkit_get_favicon_database();
    PendingIconRequestVector* icons = database->priv->pendingIconRequests.get(pageURL);
    if (!icons)
        return;

    size_t itemIndex = icons->find(request);
    if (itemIndex != notFound)
        icons->remove(itemIndex);
    if (icons->isEmpty())
        webkitfavicondatabaseDeleteRequests(database, icons, pageURL);
}

static void webkitFaviconDatabaseGetIconPixbufCancelled(GCancellable* cancellable, PendingIconRequest* request)
{
    // Handle cancelled in a in idle since it might be called from any thread.
    callOnMainThread(getIconPixbufCancelled, request);
}

/**
 * webkit_favicon_database_get_favicon_pixbuf:
 * @database: a #WebKitFaviconDatabase
 * @page_uri: URI of the page containing the icon
 * @width: the desired width for the icon
 * @height: the desired height for the icon
 * @cancellable: (allow-none): A #GCancellable or %NULL.
 * @callback: (allow-none): A #GAsyncReadyCallback to call when the request is
 *            satisfied or %NULL if you don't care about the result.
 * @user_data: The data to pass to @callback.
 *
 * Asynchronously obtains a #GdkPixbuf of the favicon for the given
 * page URI. The advantage of this method over
 * webkit_favicon_database_try_get_favicon_pixbuf() is that it always returns the
 * cached icon if it's in the database asynchronously waiting for the
 * icon to be read from the database.
 *
 * This is an asynchronous method. When the operation is finished, callback will
 * be invoked. You can then call webkit_favicon_database_get_favicon_pixbuf_finish()
 * to get the result of the operation.
 * See also webkit_favicon_database_try_get_favicon_pixbuf().
 *
 * If @width and @height are both 0 then this method will return the
 * maximum available size for the icon. Note that if you specify a
 * different size the icon will be scaled each time you call this
 * function.
 *
 * Since: 1.8
 */
void webkit_favicon_database_get_favicon_pixbuf(WebKitFaviconDatabase* database, const gchar* pageURI, guint width, guint height, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer userData)
{
    g_return_if_fail(WEBKIT_IS_FAVICON_DATABASE(database));
    g_return_if_fail(pageURI);
    g_return_if_fail((width && height) || (!width && !height));

    GRefPtr<GSimpleAsyncResult> result = adoptGRef(g_simple_async_result_new(G_OBJECT(database), callback, userData,
                                                                             reinterpret_cast<gpointer>(webkit_favicon_database_get_favicon_pixbuf)));

    // If we don't have an icon for the given URI or the database is not opened then return ASAP. We have to check that
    // because if the database is not opened it will skip (and not notify about) every single icon load request
    if ((database->priv->importFinished && iconDatabase().synchronousIconURLForPageURL(String::fromUTF8(pageURI)).isEmpty())
        || !iconDatabase().isOpen()) {
        g_simple_async_result_set_op_res_gpointer(result.get(), 0, 0);
        g_simple_async_result_complete_in_idle(result.get());
        return;
    }

    String pageURL = String::fromUTF8(pageURI);
    PendingIconRequest* request = new PendingIconRequest(pageURL, result.get(), cancellable, IntSize(width, height));

    // Register icon request before asking for the icon to avoid race conditions.
    PendingIconRequestVector* icons = webkitFaviconDatabaseGetOrCreateRequests(database, pageURL);
    ASSERT(icons);
    icons->append(adoptPtr(request));

    // We ask for the icon directly. If we don't get the icon data now,
    // we'll be notified later (even if the database is still importing icons).
    GdkPixbuf* pixbuf = getIconPixbufSynchronously(database, pageURL, IntSize(width, height));
    if (!pixbuf)
        return;

    request->asyncResultCompleteInIdle(pixbuf);

    // Remove the request we have just created as it isn't pending
    // anymore because we already have the pixbuf.
    ASSERT(icons->last().get() == request);
    icons->removeLast();
    if (icons->isEmpty())
        webkitfavicondatabaseDeleteRequests(database, icons, pageURL);
}

/**
 * webkit_favicon_database_get_favicon_pixbuf_finish:
 * @database: a #WebKitFaviconDatabase
 * @result: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to webkit_favicon_database_get_favicon_pixbuf()
 * @error: (allow-none): Return location for error or %NULL.
 *
 * Finishes an operation started with webkit_favicon_database_get_favicon_pixbuf().
 *
 * Returns: (transfer full): a new reference to a #GdkPixbuf, or %NULL.
 *
 * Since: 1.8
 */
GdkPixbuf* webkit_favicon_database_get_favicon_pixbuf_finish(WebKitFaviconDatabase* database, GAsyncResult* result, GError** error)
{
    GSimpleAsyncResult* simpleResult = G_SIMPLE_ASYNC_RESULT(result);
    g_return_val_if_fail(g_simple_async_result_get_source_tag(simpleResult) == webkit_favicon_database_get_favicon_pixbuf, 0);

    if (g_simple_async_result_propagate_error(simpleResult, error))
        return 0;

    GCancellable* cancellable = static_cast<GCancellable*>(g_object_get_data(G_OBJECT(simpleResult), "cancellable"));
    if (cancellable && g_cancellable_is_cancelled(cancellable)) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_CANCELLED, _("Operation was cancelled"));
        return 0;
    }

    GdkPixbuf* icon = static_cast<GdkPixbuf*>(g_simple_async_result_get_op_res_gpointer(simpleResult));
    if (!icon)
        return 0;

    return static_cast<GdkPixbuf*>(icon);
}

static void webkitFaviconDatabaseProcessPendingIconsForURI(WebKitFaviconDatabase* database, const String& pageURL)
{
    PendingIconRequestVector* icons = database->priv->pendingIconRequests.get(pageURL);
    if (!icons)
        return;

    for (size_t i = 0; i < icons->size(); ++i) {
        PendingIconRequest* request = icons->at(i).get();
        if (request->asyncResult())
            request->asyncResultComplete(getIconPixbufSynchronously(database, pageURL, request->iconSize()));
    }
    webkitfavicondatabaseDeleteRequests(database, icons, pageURL);
}

static void webkitFaviconDatabaseImportFinished(WebKitFaviconDatabase* database)
{
    ASSERT(isMainThread());
    database->priv->importFinished = true;

    // Import is complete, process pending requests for pages that are not in the database,
    // since didImportIconDataForPageURL() will never be called for them.
    Vector<String> toDeleteURLs;
    PendingIconRequestMap::const_iterator end = database->priv->pendingIconRequests.end();
    for (PendingIconRequestMap::const_iterator iter = database->priv->pendingIconRequests.begin(); iter != end; ++iter) {
        String iconURL = iconDatabase().synchronousIconURLForPageURL(iter->key);
        if (!iconURL.isEmpty())
            continue;

        PendingIconRequestVector* icons = iter->value;
        for (size_t i = 0; i < icons->size(); ++i) {
            PendingIconRequest* request = icons->at(i).get();
            if (request->asyncResult())
                request->asyncResultComplete(0);
        }

        toDeleteURLs.append(iter->key);
    }

    for (size_t i = 0; i < toDeleteURLs.size(); ++i)
        webkitfavicondatabaseDeleteRequests(database, database->priv->pendingIconRequests.get(toDeleteURLs[i]), toDeleteURLs[i]);
}

/**
 * webkit_favicon_database_clear:
 * @database: a #WebKitFaviconDatabase
 *
 * Clears all icons from the database.
 *
 * Since: 1.8
 */
void webkit_favicon_database_clear(WebKitFaviconDatabase* database)
{
    g_return_if_fail(WEBKIT_IS_FAVICON_DATABASE(database));

    iconDatabase().removeAllIcons();
}
