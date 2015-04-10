/*
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008, 2010 Collabora Ltd.
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
#include "webkitglobals.h"

#include "ApplicationCacheStorage.h"
#include "Chrome.h"
#include "ContextMenuItem.h"
#include "FrameNetworkingContextGtk.h"
#include "IconDatabase.h"
#include "InitializeLogging.h"
#include "MemoryCache.h"
#include "Page.h"
#include "PageCache.h"
#include "PageGroup.h"
#include "PlatformStrategiesGtk.h"
#include "TextEncodingRegistry.h"
#include "Pasteboard.h"
#include "PasteboardHelperGtk.h"
#include "ResourceHandle.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceResponse.h"
#include "SchemeRegistry.h"
#include "webkitapplicationcache.h"
#include "webkitfavicondatabase.h"
#include "webkitglobalsprivate.h"
#include "webkiticondatabase.h"
#include "webkitspellchecker.h"
#include "webkitspellcheckerenchant.h"
#include "webkitwebdatabase.h"
#include "webkitwebplugindatabaseprivate.h"
#include <libintl.h>
#include <runtime/InitializeThreading.h>
#include <stdlib.h>
#include <wtf/MainThread.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>

static WebKitCacheModel cacheModel = WEBKIT_CACHE_MODEL_DEFAULT;

using namespace WebCore;

/**
 * SECTION:webkit
 * @short_description: Global functions controlling WebKit
 *
 * WebKit manages many resources which are not related to specific
 * views. These functions relate to cross-view limits, such as cache
 * sizes, database quotas, and the HTTP session management.
 */

/**
 * webkit_get_default_session:
 *
 * Retrieves the default #SoupSession used by all web views.
 * Note that the session features are added by WebKit on demand,
 * so if you insert your own #SoupCookieJar before any network
 * traffic occurs, WebKit will use it instead of the default.
 *
 * Return value: (transfer none): the default #SoupSession
 *
 * Since: 1.1.1
 */
SoupSession* webkit_get_default_session ()
{
    webkitInit();
    return ResourceHandle::defaultSession();
}

/**
 * webkit_set_cache_model:
 * @cache_model: a #WebKitCacheModel
 *
 * Specifies a usage model for WebViews, which WebKit will use to
 * determine its caching behavior. All web views follow the cache
 * model. This cache model determines the RAM and disk space to use
 * for caching previously viewed content .
 *
 * Research indicates that users tend to browse within clusters of
 * documents that hold resources in common, and to revisit previously
 * visited documents. WebKit and the frameworks below it include
 * built-in caches that take advantage of these patterns,
 * substantially improving document load speed in browsing
 * situations. The WebKit cache model controls the behaviors of all of
 * these caches, including various WebCore caches.
 *
 * Browsers can improve document load speed substantially by
 * specifying WEBKIT_CACHE_MODEL_WEB_BROWSER. Applications without a
 * browsing interface can reduce memory usage substantially by
 * specifying WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER. Default value is
 * WEBKIT_CACHE_MODEL_WEB_BROWSER.
 *
 * Since: 1.1.18
 */
void webkit_set_cache_model(WebKitCacheModel model)
{
    webkitInit();

    if (cacheModel == model)
        return;

    // FIXME: Add disk cache handling when soup has the API
    guint cacheTotalCapacity;
    guint cacheMinDeadCapacity;
    guint cacheMaxDeadCapacity;
    gdouble deadDecodedDataDeletionInterval;
    guint pageCacheCapacity;

    // FIXME: The Mac port calculates these values based on the amount of physical memory that's
    // installed on the system. Currently these values match the Mac port for users with more than
    // 512 MB and less than 1024 MB of physical memory.
    switch (model) {
    case WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER:
        pageCacheCapacity = 0;
        cacheTotalCapacity = 0; // FIXME: The Mac port actually sets this to larger than 0.
        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;
        deadDecodedDataDeletionInterval = 0;
        break;
    case WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER:
        pageCacheCapacity = 2;
        cacheTotalCapacity = 16 * 1024 * 1024;
        cacheMinDeadCapacity = cacheTotalCapacity / 8;
        cacheMaxDeadCapacity = cacheTotalCapacity / 4;
        deadDecodedDataDeletionInterval = 0;
        break;
    case WEBKIT_CACHE_MODEL_WEB_BROWSER:
        // Page cache capacity (in pages). Comment from Mac port:
        // (Research indicates that value / page drops substantially after 3 pages.)
        pageCacheCapacity = 3;
        cacheTotalCapacity = 32 * 1024 * 1024;
        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;
        deadDecodedDataDeletionInterval = 60;
        break;
    default:
        g_return_if_reached();
    }

    bool disableCache = !cacheMinDeadCapacity && !cacheMaxDeadCapacity && !cacheTotalCapacity;
    memoryCache()->setDisabled(disableCache);
    memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    pageCache()->setCapacity(pageCacheCapacity);
    cacheModel = model;
}

/**
 * webkit_get_cache_model:
 *
 * Returns the current cache model. For more information about this
 * value check the documentation of the function
 * webkit_set_cache_model().
 *
 * Return value: the current #WebKitCacheModel
 *
 * Since: 1.1.18
 */
WebKitCacheModel webkit_get_cache_model()
{
    webkitInit();
    return cacheModel;
}

/**
 * webkit_get_web_plugin_database:
 *
 * Returns the current #WebKitWebPluginDatabase with information about
 * all the plugins WebKit knows about in this instance.
 *
 * Return value: (transfer none): the current #WebKitWebPluginDatabase
 *
 * Since: 1.3.8
 */
WebKitWebPluginDatabase* webkit_get_web_plugin_database()
{
    static WebKitWebPluginDatabase* database = 0;

    webkitInit();

    if (!database)
        database = webkit_web_plugin_database_new();

    return database;
}

/**
 * webkit_get_icon_database:
 *
 * Returns the #WebKitIconDatabase providing access to website icons.
 *
 * Return value: (transfer none): the current #WebKitIconDatabase
 *
 * Since: 1.3.13
 *
 * Deprecated: 1.8: Use webkit_get_favicon_database() instead
 */
WebKitIconDatabase* webkit_get_icon_database()
{
    webkitInit();

    static WebKitIconDatabase* database = 0;
    if (!database)
        database = WEBKIT_ICON_DATABASE(g_object_new(WEBKIT_TYPE_ICON_DATABASE, NULL));

    return database;
}

/**
 * webkit_get_favicon_database:
 *
 * Returns the #WebKitFaviconDatabase providing access to website
 * icons.
 *
 * Return value: (transfer none): the current #WebKitFaviconDatabase
 *
 * Since: 1.8
 */
WebKitFaviconDatabase* webkit_get_favicon_database()
{
    webkitInit();

    static WebKitFaviconDatabase* database = 0;
    if (!database)
        database = WEBKIT_FAVICON_DATABASE(g_object_new(WEBKIT_TYPE_FAVICON_DATABASE, NULL));

    return database;
}

static GRefPtr<WebKitSpellChecker> textChecker = 0;

static void webkitExit()
{
    g_object_unref(webkit_get_default_session());
#if ENABLE(ICONDATABASE)
    g_object_unref(webkit_get_favicon_database());
#endif
    textChecker = 0;
}

/**
 * webkit_get_text_checker:
 *
 * Returns: (transfer none): the #WebKitSpellChecker used by WebKit, or %NULL if spell
 * checking is not enabled
 *
 * Since: 1.5.1
 **/
GObject* webkit_get_text_checker()
{
    webkitInit();

#if ENABLE(SPELLCHECK)
    if (!textChecker)
        textChecker = adoptGRef(WEBKIT_SPELL_CHECKER(g_object_new(WEBKIT_TYPE_SPELL_CHECKER_ENCHANT, NULL)));
#endif

    return G_OBJECT(textChecker.get());
}

/**
 * webkit_set_text_checker:
 * @checker: a #WebKitSpellChecker or %NULL
 *
 * Sets @checker as the spell checker to be used by WebKit. The API
 * accepts GObject since in the future we might accept objects
 * implementing multiple interfaces (for example, spell checking and
 * grammar checking).
 *
 * Since: 1.5.1
 **/
void webkit_set_text_checker(GObject* checker)
{
    g_return_if_fail(!checker || WEBKIT_IS_SPELL_CHECKER(checker));

    webkitInit();

    // We need to do this because we need the cast, and casting NULL
    // is not kosher.
    textChecker = checker ? WEBKIT_SPELL_CHECKER(checker) : 0;
}

/**
 * webkit_context_menu_item_get_action:
 * @item: a #GtkMenuItem of the default context menu
 *
 * Returns the #WebKitContextMenuAction of the given @item. This function
 * can be used to determine the items present in the default context menu.
 * In order to inspect the default context menu, you should connect to
 * #WebKitWebView::context-menu signal.
 *
 * <example>
 * <title>Inspecting the default context menu</title>
 * <programlisting>
 * static gboolean context_menu_cb (WebKitWebView       *webView,
 *                                  GtkWidget           *default_menu,
 *                                  WebKitHitTestResult *hit_test_result,
 *                                  gboolean             triggered_with_keyboard,
 *                                  gpointer             user_data)
 * {
 *     GList *items = gtk_container_get_children (GTK_CONTAINER (default_menu));
 *     GList *l;
 *     GtkAction *action;
 *     GtkWidget *sub_menu;
 *
 *     for (l = items; l; l = g_list_next (l)) {
 *         GtkMenuItem *item = (GtkMenuItem *)l->data;
 *
 *         if (GTK_IS_SEPARATOR_MENU_ITEM (item)) {
 *             /&ast; It's  separator, do nothing &ast;/
 *             continue;
 *         }
 *
 *         switch (webkit_context_menu_item_get_action (item)) {
 *         case WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION:
 *             /&ast; No action for this item &ast;/
 *             break;
 *         /&ast; Don't allow to ope links from context menu &ast;/
 *         case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK:
 *         case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW:
 *             action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (item));
 *             gtk_action_set_sensitive (action, FALSE);
 *             break;
 *         default:
 *             break;
 *         }
 *
 *         sub_menu = gtk_menu_item_get_submenu (item);
 *         if (sub_menu) {
 *             GtkWidget *menu_item;
 *
 *             /&ast; Add custom action to submenu &ast;/
 *             action = gtk_action_new ("CustomItemName", "Custom Action", NULL, NULL);
 *             g_signal_connect (action, "activate", G_CALLBACK (custom_menu_item_activated), NULL);
 *
 *             menu_item = gtk_action_create_menu_item (action);
 *             g_object_unref (action);
 *             gtk_menu_shell_append (GTK_MENU_SHELL (sub_menu), menu_item);
 *             gtk_widget_show (menu_item);
 *         }
 *     }
 *
 *     g_list_free(items);
 * }
 * </programlisting>
 * </example>
 *
 * Note that you can get the #GtkAction of any item in the default context menu with
 * gtk_activatable_get_related_action().
 *
 * Returns: the #WebKitContextMenuAction of the given @item
 *
 * Since: 1.10
 */
WebKitContextMenuAction webkit_context_menu_item_get_action(GtkMenuItem* item)
{
#if ENABLE(CONTEXT_MENUS)
    g_return_val_if_fail(GTK_IS_MENU_ITEM(item), WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION);

    ContextMenuItem menuItem(item);
    switch (menuItem.action()) {
    case ContextMenuItemTagNoAction:
        return WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION;
    case ContextMenuItemTagOpenLink:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK;
    case ContextMenuItemTagOpenLinkInNewWindow:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW;
    case ContextMenuItemTagDownloadLinkToDisk:
        return WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK;
    case ContextMenuItemTagCopyLinkToClipboard:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD;
    case ContextMenuItemTagOpenImageInNewWindow:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW;
    case ContextMenuItemTagDownloadImageToDisk:
        return WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK;
    case ContextMenuItemTagCopyImageToClipboard:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD;
    case ContextMenuItemTagCopyImageUrlToClipboard:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD;
    case ContextMenuItemTagOpenFrameInNewWindow:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW;
    case ContextMenuItemTagGoBack:
        return WEBKIT_CONTEXT_MENU_ACTION_GO_BACK;
    case ContextMenuItemTagGoForward:
        return WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD;
    case ContextMenuItemTagStop:
        return WEBKIT_CONTEXT_MENU_ACTION_STOP;
    case ContextMenuItemTagReload:
        return WEBKIT_CONTEXT_MENU_ACTION_RELOAD;
    case ContextMenuItemTagCopy:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY;
    case ContextMenuItemTagCut:
        return WEBKIT_CONTEXT_MENU_ACTION_CUT;
    case ContextMenuItemTagPaste:
        return WEBKIT_CONTEXT_MENU_ACTION_PASTE;
    case ContextMenuItemTagDelete:
        return WEBKIT_CONTEXT_MENU_ACTION_DELETE;
    case ContextMenuItemTagSelectAll:
        return WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL;
    case ContextMenuItemTagInputMethods:
        return WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS;
    case ContextMenuItemTagUnicode:
        return WEBKIT_CONTEXT_MENU_ACTION_UNICODE;
    case ContextMenuItemTagSpellingGuess:
        return WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS;
    case ContextMenuItemTagIgnoreSpelling:
        return WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING;
    case ContextMenuItemTagLearnSpelling:
        return WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING;
    case ContextMenuItemTagIgnoreGrammar:
        return WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR;
    case ContextMenuItemTagFontMenu:
        return WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU;
    case ContextMenuItemTagBold:
        return WEBKIT_CONTEXT_MENU_ACTION_BOLD;
    case ContextMenuItemTagItalic:
        return WEBKIT_CONTEXT_MENU_ACTION_ITALIC;
    case ContextMenuItemTagUnderline:
        return WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE;
    case ContextMenuItemTagOutline:
        return WEBKIT_CONTEXT_MENU_ACTION_OUTLINE;
    case ContextMenuItemTagInspectElement:
        return WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT;
    case ContextMenuItemTagOpenMediaInNewWindow:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_MEDIA_IN_NEW_WINDOW;
    case ContextMenuItemTagCopyMediaLinkToClipboard:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY_MEDIA_LINK_TO_CLIPBOARD;
    case ContextMenuItemTagToggleMediaControls:
        return WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS;
    case ContextMenuItemTagToggleMediaLoop:
        return WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP;
    case ContextMenuItemTagEnterVideoFullscreen:
        return WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN;
    case ContextMenuItemTagMediaPlayPause:
        return WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY_PAUSE;
    case ContextMenuItemTagMediaMute:
        return WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE;
    default:
        g_assert_not_reached();
    }
#else
    return WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION;
#endif
}

/**
 * webkit_set_security_policy_for_uri_scheme:
 * @scheme: a URI scheme
 * @policy: a #WebKitSecurityPolicy
 *
 * Set the security policy for the given URI scheme.
 *
 * Since: 2.0
 */
void webkit_set_security_policy_for_uri_scheme(const char *scheme, WebKitSecurityPolicy policy)
{
    g_return_if_fail(scheme);

    if (!policy)
        return;

    String urlScheme = String::fromUTF8(scheme);

    if (policy & WEBKIT_SECURITY_POLICY_LOCAL)
        SchemeRegistry::registerURLSchemeAsLocal(urlScheme);
    if (policy & WEBKIT_SECURITY_POLICY_NO_ACCESS_TO_OTHER_SCHEME)
        SchemeRegistry::registerURLSchemeAsNoAccess(urlScheme);
    if (policy & WEBKIT_SECURITY_POLICY_DISPLAY_ISOLATED)
        SchemeRegistry::registerURLSchemeAsDisplayIsolated(urlScheme);
    if (policy & WEBKIT_SECURITY_POLICY_SECURE)
        SchemeRegistry::registerURLSchemeAsSecure(urlScheme);
    if (policy & WEBKIT_SECURITY_POLICY_CORS_ENABLED)
        SchemeRegistry::registerURLSchemeAsCORSEnabled(urlScheme);
    if (policy & WEBKIT_SECURITY_POLICY_EMPTY_DOCUMENT)
        SchemeRegistry::registerURLSchemeAsEmptyDocument(urlScheme);
}

/**
 * webkit_get_security_policy_for_uri_scheme:
 * @scheme: a URI scheme
 *
 * Get the security policy for the given URI scheme.
 *
 * Returns: a #WebKitSecurityPolicy
 *
 * Since: 2.0
 */
WebKitSecurityPolicy webkit_get_security_policy_for_uri_scheme(const char *scheme)
{
    g_return_val_if_fail(scheme, static_cast<WebKitSecurityPolicy>(0));

    guint policy = 0;
    String urlScheme = String::fromUTF8(scheme);

    if (SchemeRegistry::shouldTreatURLSchemeAsLocal(urlScheme))
        policy |= WEBKIT_SECURITY_POLICY_LOCAL;
    if (SchemeRegistry::shouldTreatURLSchemeAsNoAccess(urlScheme))
        policy |= WEBKIT_SECURITY_POLICY_NO_ACCESS_TO_OTHER_SCHEME;
    if (SchemeRegistry::shouldTreatURLSchemeAsDisplayIsolated(urlScheme))
        policy |= WEBKIT_SECURITY_POLICY_DISPLAY_ISOLATED;
    if (SchemeRegistry::shouldTreatURLSchemeAsSecure(urlScheme))
        policy |= WEBKIT_SECURITY_POLICY_SECURE;
    if (SchemeRegistry::shouldTreatURLSchemeAsCORSEnabled(urlScheme))
        policy |= WEBKIT_SECURITY_POLICY_CORS_ENABLED;
    if (SchemeRegistry::shouldLoadURLSchemeAsEmptyDocument(urlScheme))
        policy |= WEBKIT_SECURITY_POLICY_EMPTY_DOCUMENT;

    return static_cast<WebKitSecurityPolicy>(policy);
}

void webkitInit()
{
    static bool isInitialized = false;
    if (isInitialized)
        return;
    isInitialized = true;

    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    JSC::initializeThreading();
    WTF::initializeMainThread();

#if !LOG_DISABLED
    WebCore::initializeLoggingChannelsIfNecessary();
#endif // !LOG_DISABLED
    PlatformStrategiesGtk::initialize();

    // We make sure the text codecs have been initialized, because
    // that may only be done by the main thread.
    atomicCanonicalTextEncodingName("UTF-8");

    GOwnPtr<gchar> databaseDirectory(g_build_filename(g_get_user_data_dir(), "webkit", "databases", NULL));
    webkit_set_web_database_directory_path(databaseDirectory.get());

    GOwnPtr<gchar> cacheDirectory(g_build_filename(g_get_user_cache_dir(), "webkitgtk", "applications", NULL));
    WebCore::cacheStorage().setCacheDirectory(cacheDirectory.get());

    PageGroup::setShouldTrackVisitedLinks(true);

    GOwnPtr<gchar> iconDatabasePath(g_build_filename(g_get_user_data_dir(), "webkit", "icondatabase", NULL));
    webkit_icon_database_set_path(webkit_get_icon_database(), iconDatabasePath.get());

    WebCore::ResourceHandle::setIgnoreSSLErrors(true);

    atexit(webkitExit);
}

const char* webkitPageGroupName()
{
    return "org.webkit.gtk.WebKitGTK";
}
