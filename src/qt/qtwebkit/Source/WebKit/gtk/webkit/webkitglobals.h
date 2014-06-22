/*
 * Copyright (C) 2007, 2008, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2008 Jan Michael C. Alonzo
 * Copyright (C) 2008 Collabora Ltd.
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

#ifndef webkitglobals_h
#define webkitglobals_h

#include "webkitdefines.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS

/**
 * WebKitCacheModel:
 * @WEBKIT_CACHE_MODEL_DEFAULT: The default cache model. This is
 *   WEBKIT_CACHE_MODEL_WEB_BROWSER.
 * @WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER: Disable the cache completely, which
 *   substantially reduces memory usage. Useful for applications that only
 *   access a single local file, with no navigation to other pages. No remote
 *   resources will be cached.
 * @WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER: A cache model optimized for viewing
 *   a series of local files -- for example, a documentation viewer or a website
 *   designer. WebKit will cache a moderate number of resources.
 * @WEBKIT_CACHE_MODEL_WEB_BROWSER: Improve document load speed substantially
 *   by caching a very large number of resources and previously viewed content.
 *
 * Enum values used for determining the webview cache model.
 *
 **/
typedef enum {
    WEBKIT_CACHE_MODEL_DEFAULT,
    WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER,
    WEBKIT_CACHE_MODEL_WEB_BROWSER,
    WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER,
} WebKitCacheModel;

/**
 * WebKitContextMenuAction:
 * @WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION: No action, used by separator menu items.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK: Open current link.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW: Open current link in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK: Download link destination.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD: Copy link location to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW: Open current image in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK: Download current image.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD: Copy current image to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD: Copy curent image location to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW: Open current frame in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_GO_BACK: Load the previous history item.
 * @WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD: Load the next history item.
 * @WEBKIT_CONTEXT_MENU_ACTION_STOP: Stop any ongoing loading operation.
 * @WEBKIT_CONTEXT_MENU_ACTION_RELOAD: Reload the conents of current view.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY: Copy current selection the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_CUT: Cut current selection to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_PASTE: Paste clipboard contents.
 * @WEBKIT_CONTEXT_MENU_ACTION_DELETE: Delete current selection.
 * @WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL: Select all text.
 * @WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS: Input methods menu.
 * @WEBKIT_CONTEXT_MENU_ACTION_UNICODE: Unicode menu.
 * @WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS: Guess spelling.
 * @WEBKIT_CONTEXT_MENU_ACTION_NO_GUESSES_FOUND: No guesses found.
 * @WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING: Ignore spelling.
 * @WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING: Learn spelling.
 * @WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR: Ignore grammar.
 * @WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU: Font menu.
 * @WEBKIT_CONTEXT_MENU_ACTION_BOLD: Bold.
 * @WEBKIT_CONTEXT_MENU_ACTION_ITALIC: Italic.
 * @WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE: Underline.
 * @WEBKIT_CONTEXT_MENU_ACTION_OUTLINE: Outline.
 * @WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT: Open current element in the inspector.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_MEDIA_IN_NEW_WINDOW: Open current media element in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_MEDIA_LINK_TO_CLIPBOARD: Copy media link location in to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS: Enable or disable media controls.
 * @WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP: Enable or disable media loop.
 * @WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN: Show current video element in fullscreen mode.
 * @WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY_PAUSE: Play or pause current media element.
 * @WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE: Mute current media element.
 *
 * Enum values used to denote actions of items in the default context menu.
 *
 * Since: 1.10
 */
typedef enum {
    WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION = 0,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_GO_BACK,
    WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD,
    WEBKIT_CONTEXT_MENU_ACTION_STOP,
    WEBKIT_CONTEXT_MENU_ACTION_RELOAD,
    WEBKIT_CONTEXT_MENU_ACTION_COPY,
    WEBKIT_CONTEXT_MENU_ACTION_CUT,
    WEBKIT_CONTEXT_MENU_ACTION_PASTE,
    WEBKIT_CONTEXT_MENU_ACTION_DELETE,
    WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL,
    WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS,
    WEBKIT_CONTEXT_MENU_ACTION_UNICODE,
    WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS,
    WEBKIT_CONTEXT_MENU_ACTION_NO_GUESSES_FOUND,
    WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING,
    WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING,
    WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR,
    WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU,
    WEBKIT_CONTEXT_MENU_ACTION_BOLD,
    WEBKIT_CONTEXT_MENU_ACTION_ITALIC,
    WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE,
    WEBKIT_CONTEXT_MENU_ACTION_OUTLINE,
    WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_MEDIA_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_MEDIA_LINK_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS,
    WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP,
    WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN,
    WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY_PAUSE,
    WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE
} WebKitContextMenuAction;

/**
 * WebKitSecurityPolicy:
 * @WEBKIT_SECURITY_POLICY_LOCAL: Local URI scheme, other non-local pages
 *   cannot link to or access URIs of this scheme.
 * @WEBKIT_SECURITY_POLICY_NO_ACCESS_TO_OTHER_SCHEME: Pages loaded with this URI scheme
 *   cannot access pages loaded with any other URI scheme.
 * @WEBKIT_SECURITY_POLICY_DISPLAY_ISOLATED: Pages cannot display these URIs
 *   unless they are from the same scheme.
 * @WEBKIT_SECURITY_POLICY_SECURE: Secure URI scheme, doesn't generate mixed
 *   content warnings when included by an HTTPS page.
 * @WEBKIT_SECURITY_POLICY_CORS_ENABLED: URI scheme that can be sent
 *   CORS (Cross-origin resource sharing) requests. See W3C CORS specification
 *   http://www.w3.org/TR/cors/.
 * @WEBKIT_SECURITY_POLICY_EMPTY_DOCUMENT: Strictly empty documents allowed
 *   to commit synchronously.
 *
 * Flags used to represent the security policy of a URI scheme.
 *
 * Since: 2.0
 */
typedef enum {
    WEBKIT_SECURITY_POLICY_LOCAL                     = 1 << 1,
    WEBKIT_SECURITY_POLICY_NO_ACCESS_TO_OTHER_SCHEME = 1 << 2,
    WEBKIT_SECURITY_POLICY_DISPLAY_ISOLATED          = 1 << 3,
    WEBKIT_SECURITY_POLICY_SECURE                    = 1 << 4,
    WEBKIT_SECURITY_POLICY_CORS_ENABLED              = 1 << 5,
    WEBKIT_SECURITY_POLICY_EMPTY_DOCUMENT            = 1 << 6
} WebKitSecurityPolicy;

WEBKIT_API SoupSession*
webkit_get_default_session                      (void);

WEBKIT_API WebKitWebPluginDatabase *
webkit_get_web_plugin_database                  (void);

#if !defined(WEBKIT_DISABLE_DEPRECATED)
WEBKIT_API WebKitIconDatabase *
webkit_get_icon_database                        (void);
#endif

WEBKIT_API WebKitFaviconDatabase *
webkit_get_favicon_database                     (void);

WEBKIT_API void
webkit_set_cache_model                          (WebKitCacheModel     cache_model);

WEBKIT_API WebKitCacheModel
webkit_get_cache_model                          (void);

WEBKIT_API GObject*
webkit_get_text_checker                        (void);

WEBKIT_API void
webkit_set_text_checker                        (GObject*  checker);

WEBKIT_API WebKitContextMenuAction
webkit_context_menu_item_get_action            (GtkMenuItem* item);

WEBKIT_API void
webkit_set_security_policy_for_uri_scheme      (const gchar         *scheme,
                                                WebKitSecurityPolicy policy);

WEBKIT_API WebKitSecurityPolicy
webkit_get_security_policy_for_uri_scheme      (const gchar         *scheme);

G_END_DECLS

#endif
