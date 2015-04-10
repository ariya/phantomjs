/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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

#ifndef WebKitCookieManager_h
#define WebKitCookieManager_h

#include <gio/gio.h>
#include <glib-object.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_COOKIE_MANAGER            (webkit_cookie_manager_get_type())
#define WEBKIT_COOKIE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_COOKIE_MANAGER, WebKitCookieManager))
#define WEBKIT_IS_COOKIE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_COOKIE_MANAGER))
#define WEBKIT_COOKIE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_COOKIE_MANAGER, WebKitCookieManagerClass))
#define WEBKIT_IS_COOKIE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_COOKIE_MANAGER))
#define WEBKIT_COOKIE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_COOKIE_MANAGER, WebKitCookieManagerClass))

typedef struct _WebKitCookieManager        WebKitCookieManager;
typedef struct _WebKitCookieManagerClass   WebKitCookieManagerClass;
typedef struct _WebKitCookieManagerPrivate WebKitCookieManagerPrivate;

/**
 * WebKitCookiePersistentStorage:
 * @WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT: Cookies are stored in a text
 *  file in the Mozilla "cookies.txt" format.
 * @WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE: Cookies are stored in a SQLite
 *  file in the current Mozilla format.
 *
 * Enum values used to denote the cookie persistent storage types.
 */
typedef enum {
    WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT,
    WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE
} WebKitCookiePersistentStorage;

/**
 * WebKitCookieAcceptPolicy:
 * @WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS: Accept all cookies unconditionally.
 * @WEBKIT_COOKIE_POLICY_ACCEPT_NEVER: Reject all cookies unconditionally.
 * @WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY: Accept only cookies set by the main document loaded.
 *
 * Enum values used to denote the cookie acceptance policies.
 */
typedef enum {
    WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS,
    WEBKIT_COOKIE_POLICY_ACCEPT_NEVER,
    WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY
} WebKitCookieAcceptPolicy;

struct _WebKitCookieManager {
    GObject parent;

    WebKitCookieManagerPrivate *priv;
};

struct _WebKitCookieManagerClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_cookie_manager_get_type                        (void);

WEBKIT_API void
webkit_cookie_manager_set_persistent_storage          (WebKitCookieManager          *cookie_manager,
                                                       const gchar                  *filename,
                                                       WebKitCookiePersistentStorage storage);

WEBKIT_API void
webkit_cookie_manager_set_accept_policy               (WebKitCookieManager          *cookie_manager,
                                                       WebKitCookieAcceptPolicy      policy);

WEBKIT_API void
webkit_cookie_manager_get_accept_policy               (WebKitCookieManager          *cookie_manager,
                                                       GCancellable                 *cancellable,
                                                       GAsyncReadyCallback           callback,
                                                       gpointer                      user_data);

WEBKIT_API WebKitCookieAcceptPolicy
webkit_cookie_manager_get_accept_policy_finish        (WebKitCookieManager          *cookie_manager,
                                                       GAsyncResult                 *result,
                                                       GError                      **error);

WEBKIT_API void
webkit_cookie_manager_get_domains_with_cookies        (WebKitCookieManager          *cookie_manager,
                                                       GCancellable                 *cancellable,
                                                       GAsyncReadyCallback           callback,
                                                       gpointer                      user_data);

WEBKIT_API gchar **
webkit_cookie_manager_get_domains_with_cookies_finish (WebKitCookieManager          *cookie_manager,
                                                       GAsyncResult                 *result,
                                                       GError                      **error);

WEBKIT_API void
webkit_cookie_manager_delete_cookies_for_domain       (WebKitCookieManager          *cookie_manager,
                                                       const gchar                  *domain);

WEBKIT_API void
webkit_cookie_manager_delete_all_cookies              (WebKitCookieManager          *cookie_manager);

G_END_DECLS

#endif
