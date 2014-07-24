/*
 * Copyright (C) 2011 Igalia S.L.
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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitWebContext_h
#define WebKitWebContext_h

#include <glib-object.h>
#include <webkit2/WebKitCookieManager.h>
#include <webkit2/WebKitDefines.h>
#include <webkit2/WebKitDownload.h>
#include <webkit2/WebKitFaviconDatabase.h>
#include <webkit2/WebKitSecurityManager.h>
#include <webkit2/WebKitURISchemeRequest.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_CONTEXT            (webkit_web_context_get_type())
#define WEBKIT_WEB_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_CONTEXT, WebKitWebContext))
#define WEBKIT_WEB_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_CONTEXT, WebKitWebContextClass))
#define WEBKIT_IS_WEB_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_CONTEXT))
#define WEBKIT_IS_WEB_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_CONTEXT))
#define WEBKIT_WEB_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_CONTEXT, WebKitWebContextClass))

/**
 * WebKitCacheModel:
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
 * Enum values used for determining the #WebKitWebContext cache model.
 */
typedef enum {
    WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER,
    WEBKIT_CACHE_MODEL_WEB_BROWSER,
    WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER
} WebKitCacheModel;

/**
 * WebKitTLSErrorsPolicy:
 * @WEBKIT_TLS_ERRORS_POLICY_IGNORE: Ignore TLS errors.
 * @WEBKIT_TLS_ERRORS_POLICY_FAIL: TLS errors make the load to finish with an error.
 *
 * Enum values used to denote the TLS errors policy.
 */
typedef enum {
    WEBKIT_TLS_ERRORS_POLICY_IGNORE,
    WEBKIT_TLS_ERRORS_POLICY_FAIL
} WebKitTLSErrorsPolicy;

/**
 * WebKitURISchemeRequestCallback:
 * @request: the #WebKitURISchemeRequest
 * @user_data: user data passed to the callback
 *
 * Type definition for a function that will be called back when an URI request is
 * made for a user registered URI scheme.
 */
typedef void (* WebKitURISchemeRequestCallback) (WebKitURISchemeRequest *request,
                                                 gpointer                user_data);

typedef struct _WebKitWebContext        WebKitWebContext;
typedef struct _WebKitWebContextClass   WebKitWebContextClass;
typedef struct _WebKitWebContextPrivate WebKitWebContextPrivate;

struct _WebKitWebContext {
    GObject parent;

    /*< private >*/
    WebKitWebContextPrivate *priv;
};

struct _WebKitWebContextClass {
    GObjectClass parent;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
    void (*_webkit_reserved4) (void);
    void (*_webkit_reserved5) (void);
    void (*_webkit_reserved6) (void);
    void (*_webkit_reserved7) (void);
};

WEBKIT_API GType
webkit_web_context_get_type                         (void);

WEBKIT_API WebKitWebContext *
webkit_web_context_get_default                      (void);

WEBKIT_API void
webkit_web_context_set_cache_model                  (WebKitWebContext              *context,
                                                     WebKitCacheModel               cache_model);
WEBKIT_API WebKitCacheModel
webkit_web_context_get_cache_model                  (WebKitWebContext              *context);

WEBKIT_API void
webkit_web_context_clear_cache                      (WebKitWebContext              *context);

WEBKIT_API WebKitDownload *
webkit_web_context_download_uri                     (WebKitWebContext              *context,
                                                     const gchar                   *uri);

WEBKIT_API WebKitCookieManager *
webkit_web_context_get_cookie_manager               (WebKitWebContext              *context);

WEBKIT_API WebKitFaviconDatabase *
webkit_web_context_get_favicon_database             (WebKitWebContext              *context);

WEBKIT_API void
webkit_web_context_set_favicon_database_directory   (WebKitWebContext              *context,
                                                     const gchar                   *path);
WEBKIT_API const gchar *
webkit_web_context_get_favicon_database_directory   (WebKitWebContext              *context);

WEBKIT_API WebKitSecurityManager *
webkit_web_context_get_security_manager             (WebKitWebContext              *context);

WEBKIT_API void
webkit_web_context_set_additional_plugins_directory (WebKitWebContext              *context,
                                                     const gchar                   *directory);

WEBKIT_API void
webkit_web_context_get_plugins                      (WebKitWebContext              *context,
                                                     GCancellable                  *cancellable,
                                                     GAsyncReadyCallback            callback,
                                                     gpointer                       user_data);

WEBKIT_API GList *
webkit_web_context_get_plugins_finish               (WebKitWebContext              *context,
                                                     GAsyncResult                  *result,
                                                     GError                       **error);
WEBKIT_API void
webkit_web_context_register_uri_scheme              (WebKitWebContext              *context,
                                                     const gchar                   *scheme,
                                                     WebKitURISchemeRequestCallback callback,
                                                     gpointer                       user_data,
                                                     GDestroyNotify                 user_data_destroy_func);

WEBKIT_API gboolean
webkit_web_context_get_spell_checking_enabled       (WebKitWebContext              *context);

WEBKIT_API void
webkit_web_context_set_spell_checking_enabled       (WebKitWebContext              *context,
                                                     gboolean                       enabled);
WEBKIT_API const gchar * const *
webkit_web_context_get_spell_checking_languages     (WebKitWebContext              *context);

WEBKIT_API void
webkit_web_context_set_spell_checking_languages     (WebKitWebContext              *context,
                                                     const gchar * const           *languages);

WEBKIT_API void
webkit_web_context_set_preferred_languages          (WebKitWebContext              *context,
                                                     const gchar * const           *languages);

WEBKIT_API void
webkit_web_context_set_tls_errors_policy            (WebKitWebContext              *context,
                                                     WebKitTLSErrorsPolicy          policy);

WEBKIT_API WebKitTLSErrorsPolicy
webkit_web_context_get_tls_errors_policy            (WebKitWebContext              *context);

WEBKIT_API void
webkit_web_context_set_web_extensions_directory     (WebKitWebContext              *context,
                                                     const gchar                   *directory);

WEBKIT_API void
webkit_web_context_prefetch_dns                     (WebKitWebContext              *context,
                                                     const gchar                   *hostname);

WEBKIT_API void
webkit_web_context_set_disk_cache_directory         (WebKitWebContext              *context,
                                                     const gchar                   *directory);

G_END_DECLS

#endif
