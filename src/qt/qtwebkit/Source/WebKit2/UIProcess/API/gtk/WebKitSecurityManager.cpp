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

#include "config.h"
#include "WebKitSecurityManager.h"

#include "WebContext.h"
#include "WebKitSecurityManagerPrivate.h"
#include "WebKitWebContextPrivate.h"
#include <WebCore/SchemeRegistry.h>

using namespace WebKit;

/**
 * SECTION: WebKitSecurityManager
 * @Short_description: Controls security settings in a #WebKitWebContext
 * @Title: WebKitSecurityManager
 *
 * The #WebKitSecurityManager defines security settings for URI
 * schemes in a #WebKitWebContext. Get it from the context with
 * webkit_web_context_get_security_manager(), and use it to register a
 * URI scheme with a certain security level, or to check if it already
 * has it.
 *
 */

typedef enum {
    SecurityPolicyLocal,
    SecurityPolicyNoAccess,
    SecurityPolicyDisplayIsolated,
    SecurityPolicySecure,
    SecurityPolicyCORSEnabled,
    SecurityPolicyEmptyDocument
} SecurityPolicy;

struct _WebKitSecurityManagerPrivate {
    WebKitWebContext* webContext;
};

WEBKIT_DEFINE_TYPE(WebKitSecurityManager, webkit_security_manager, G_TYPE_OBJECT)

static void webkit_security_manager_class_init(WebKitSecurityManagerClass* klass)
{
}

WebKitSecurityManager* webkitSecurityManagerCreate(WebKitWebContext* webContext)
{
    WebKitSecurityManager* manager = WEBKIT_SECURITY_MANAGER(g_object_new(WEBKIT_TYPE_SECURITY_MANAGER, NULL));
    manager->priv->webContext = webContext;
    return manager;
}

static void registerSecurityPolicyForURIScheme(WebKitSecurityManager* manager, const char* scheme, SecurityPolicy policy)
{
    String urlScheme = String::fromUTF8(scheme);
    WebContext* webContext = webkitWebContextGetContext(manager->priv->webContext);

    // We keep the WebCore::SchemeRegistry of the UI process in sync with the
    // web process one, so that we can return the SecurityPolicy for
    // a given URI scheme synchronously without blocking.
    switch (policy) {
    case SecurityPolicyLocal:
        WebCore::SchemeRegistry::registerURLSchemeAsLocal(urlScheme);
        webContext->registerURLSchemeAsLocal(urlScheme);
        break;
    case SecurityPolicyNoAccess:
        WebCore::SchemeRegistry::registerURLSchemeAsNoAccess(urlScheme);
        webContext->registerURLSchemeAsNoAccess(urlScheme);
        break;
    case SecurityPolicyDisplayIsolated:
        WebCore::SchemeRegistry::registerURLSchemeAsDisplayIsolated(urlScheme);
        webContext->registerURLSchemeAsDisplayIsolated(urlScheme);
        break;
    case SecurityPolicySecure:
        WebCore::SchemeRegistry::registerURLSchemeAsSecure(urlScheme);
        webContext->registerURLSchemeAsSecure(urlScheme);
        break;
    case SecurityPolicyCORSEnabled:
        WebCore::SchemeRegistry::registerURLSchemeAsCORSEnabled(urlScheme);
        webContext->registerURLSchemeAsCORSEnabled(urlScheme);
        break;
    case SecurityPolicyEmptyDocument:
        WebCore::SchemeRegistry::registerURLSchemeAsEmptyDocument(urlScheme);
        webContext->registerURLSchemeAsEmptyDocument(urlScheme);
        break;
    }
}

static bool checkSecurityPolicyForURIScheme(const char* scheme, SecurityPolicy policy)
{
    String urlScheme = String::fromUTF8(scheme);

    switch (policy) {
    case SecurityPolicyLocal:
        return WebCore::SchemeRegistry::shouldTreatURLSchemeAsLocal(urlScheme);
    case SecurityPolicyNoAccess:
        return WebCore::SchemeRegistry::shouldTreatURLSchemeAsNoAccess(urlScheme);
    case SecurityPolicyDisplayIsolated:
        return WebCore::SchemeRegistry::shouldTreatURLSchemeAsDisplayIsolated(urlScheme);
    case SecurityPolicySecure:
        return WebCore::SchemeRegistry::shouldTreatURLSchemeAsSecure(urlScheme);
    case SecurityPolicyCORSEnabled:
        return WebCore::SchemeRegistry::shouldTreatURLSchemeAsCORSEnabled(urlScheme);
    case SecurityPolicyEmptyDocument:
        return WebCore::SchemeRegistry::shouldLoadURLSchemeAsEmptyDocument(urlScheme);
    }

    return false;
}

/**
 * webkit_security_manager_register_uri_scheme_as_local:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Register @scheme as a local scheme. This means that other non-local pages
 * cannot link to or access URIs of this scheme.
 */
void webkit_security_manager_register_uri_scheme_as_local(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager));
    g_return_if_fail(scheme);

    registerSecurityPolicyForURIScheme(manager, scheme, SecurityPolicyLocal);
}

/**
 * webkit_security_manager_uri_scheme_is_local:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Whether @scheme is considered as a local scheme.
 * See also webkit_security_manager_register_uri_scheme_as_local().
 *
 * Returns: %TRUE if @scheme is a local scheme or %FALSE otherwise.
 */
gboolean webkit_security_manager_uri_scheme_is_local(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager), FALSE);
    g_return_val_if_fail(scheme, FALSE);

    return checkSecurityPolicyForURIScheme(scheme, SecurityPolicyLocal);
}

/**
 * webkit_security_manager_register_uri_scheme_as_no_access:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Register @scheme as a no-access scheme. This means that pages loaded
 * with this URI scheme cannot access pages loaded with any other URI scheme.
 */
void webkit_security_manager_register_uri_scheme_as_no_access(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager));
    g_return_if_fail(scheme);

    registerSecurityPolicyForURIScheme(manager, scheme, SecurityPolicyNoAccess);
}

/**
 * webkit_security_manager_uri_scheme_is_no_access:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Whether @scheme is considered as a no-access scheme.
 * See also webkit_security_manager_register_uri_scheme_as_no_access().
 *
 * Returns: %TRUE if @scheme is a no-access scheme or %FALSE otherwise.
 */
gboolean webkit_security_manager_uri_scheme_is_no_access(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager), FALSE);
    g_return_val_if_fail(scheme, FALSE);

    return checkSecurityPolicyForURIScheme(scheme, SecurityPolicyNoAccess);
}

/**
 * webkit_security_manager_register_uri_scheme_as_display_isolated:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Register @scheme as a display isolated scheme. This means that pages cannot
 * display these URIs unless they are from the same scheme.
 */
void webkit_security_manager_register_uri_scheme_as_display_isolated(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager));
    g_return_if_fail(scheme);

    registerSecurityPolicyForURIScheme(manager, scheme, SecurityPolicyDisplayIsolated);
}

/**
 * webkit_security_manager_uri_scheme_is_display_isolated:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Whether @scheme is considered as a display isolated scheme.
 * See also webkit_security_manager_register_uri_scheme_as_display_isolated().
 *
 * Returns: %TRUE if @scheme is a display isolated scheme or %FALSE otherwise.
 */
gboolean webkit_security_manager_uri_scheme_is_display_isolated(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager), FALSE);
    g_return_val_if_fail(scheme, FALSE);

    return checkSecurityPolicyForURIScheme(scheme, SecurityPolicyDisplayIsolated);
}

/**
 * webkit_security_manager_register_uri_scheme_as_secure:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Register @scheme as a secure scheme. This means that mixed
 * content warnings won't be generated for this scheme when
 * included by an HTTPS page.
 */
void webkit_security_manager_register_uri_scheme_as_secure(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager));
    g_return_if_fail(scheme);

    registerSecurityPolicyForURIScheme(manager, scheme, SecurityPolicySecure);
}

/**
 * webkit_security_manager_uri_scheme_is_secure:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Whether @scheme is considered as a secure scheme.
 * See also webkit_security_manager_register_uri_scheme_as_secure().
 *
 * Returns: %TRUE if @scheme is a secure scheme or %FALSE otherwise.
 */
gboolean webkit_security_manager_uri_scheme_is_secure(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager), FALSE);
    g_return_val_if_fail(scheme, FALSE);

    return checkSecurityPolicyForURIScheme(scheme, SecurityPolicySecure);
}

/**
 * webkit_security_manager_register_uri_scheme_as_cors_enabled:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Register @scheme as a CORS (Cross-origin resource sharing) enabled scheme.
 * This means that CORS requests are allowed. See W3C CORS specification
 * http://www.w3.org/TR/cors/.
 */
void webkit_security_manager_register_uri_scheme_as_cors_enabled(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager));
    g_return_if_fail(scheme);

    registerSecurityPolicyForURIScheme(manager, scheme, SecurityPolicyCORSEnabled);
}

/**
 * webkit_security_manager_uri_scheme_is_cors_enabled:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Whether @scheme is considered as a CORS enabled scheme.
 * See also webkit_security_manager_register_uri_scheme_as_cors_enabled().
 *
 * Returns: %TRUE if @scheme is a CORS enabled scheme or %FALSE otherwise.
 */
gboolean webkit_security_manager_uri_scheme_is_cors_enabled(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager), FALSE);
    g_return_val_if_fail(scheme, FALSE);

    return checkSecurityPolicyForURIScheme(scheme, SecurityPolicyCORSEnabled);
}

/**
 * webkit_security_manager_register_uri_scheme_as_empty_document:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Register @scheme as an empty document scheme. This means that
 * they are allowd to commit synchronously.
 */
void webkit_security_manager_register_uri_scheme_as_empty_document(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager));
    g_return_if_fail(scheme);

    registerSecurityPolicyForURIScheme(manager, scheme, SecurityPolicyEmptyDocument);
}

/**
 * webkit_security_manager_uri_scheme_is_empty_document:
 * @security_manager: a #WebKitSecurityManager
 * @scheme: a URI scheme
 *
 * Whether @scheme is considered as an empty document scheme.
 * See also webkit_security_manager_register_uri_scheme_as_empty_document().
 *
 * Returns: %TRUE if @scheme is a an empty document scheme or %FALSE otherwise.
 */
gboolean webkit_security_manager_uri_scheme_is_empty_document(WebKitSecurityManager* manager, const char* scheme)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_MANAGER(manager), FALSE);
    g_return_val_if_fail(scheme, FALSE);

    return checkSecurityPolicyForURIScheme(scheme, SecurityPolicyEmptyDocument);
}
