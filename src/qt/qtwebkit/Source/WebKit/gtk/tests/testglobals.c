/*
 * Copyright (C) 2010 Collabora Ltd.
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
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <webkit/webkit.h>

// Make sure the session is initialized properly when webkit_get_default_session() is called.
static void test_globals_default_session()
{
    g_test_bug("36754");

    SoupSession* session = webkit_get_default_session();
    soup_session_remove_feature_by_type(session, WEBKIT_TYPE_SOUP_AUTH_DIALOG);

    // This makes sure our initialization ran.
    g_assert(soup_session_get_feature(session, SOUP_TYPE_CONTENT_DECODER) != NULL);

    // Creating a WebView should make sure the session is
    // initialized, and not mess with our changes.
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(web_view);
    g_object_unref(web_view);

    // These makes sure our modification was kept.
    g_assert(soup_session_get_feature(session, SOUP_TYPE_CONTENT_DECODER) != NULL);
    g_assert(soup_session_get_feature(session, WEBKIT_TYPE_SOUP_AUTH_DIALOG) == NULL);
}

static void test_globals_security_policy()
{
    // Check default policy for well known schemes.
    WebKitSecurityPolicy policy = webkit_get_security_policy_for_uri_scheme("http");
    guint mask = WEBKIT_SECURITY_POLICY_CORS_ENABLED;
    g_assert_cmpuint(policy & mask, ==, mask);

    policy = webkit_get_security_policy_for_uri_scheme("https");
    mask = WEBKIT_SECURITY_POLICY_SECURE | WEBKIT_SECURITY_POLICY_CORS_ENABLED;
    g_assert_cmpuint(policy & mask, ==, mask);

    policy = webkit_get_security_policy_for_uri_scheme("file");
    mask = WEBKIT_SECURITY_POLICY_LOCAL;
    g_assert_cmpuint(policy & mask, ==, mask);

    policy = webkit_get_security_policy_for_uri_scheme("data");
    mask = WEBKIT_SECURITY_POLICY_NO_ACCESS_TO_OTHER_SCHEME | WEBKIT_SECURITY_POLICY_SECURE;
    g_assert_cmpuint(policy & mask, ==, mask);

    policy = webkit_get_security_policy_for_uri_scheme("about");
    mask = WEBKIT_SECURITY_POLICY_NO_ACCESS_TO_OTHER_SCHEME | WEBKIT_SECURITY_POLICY_SECURE | WEBKIT_SECURITY_POLICY_EMPTY_DOCUMENT;
    g_assert_cmpuint(policy & mask, ==, mask);

    // Custom scheme.
    policy = webkit_get_security_policy_for_uri_scheme("foo");
    g_assert(!policy);

    policy |= WEBKIT_SECURITY_POLICY_LOCAL;
    webkit_set_security_policy_for_uri_scheme("foo", policy);
    g_assert_cmpuint(webkit_get_security_policy_for_uri_scheme("foo"), ==, policy);

    policy |= WEBKIT_SECURITY_POLICY_NO_ACCESS_TO_OTHER_SCHEME;
    webkit_set_security_policy_for_uri_scheme("foo", policy);
    g_assert_cmpuint(webkit_get_security_policy_for_uri_scheme("foo"), ==, policy);

    policy |= WEBKIT_SECURITY_POLICY_DISPLAY_ISOLATED;
    webkit_set_security_policy_for_uri_scheme("foo", policy);
    g_assert_cmpuint(webkit_get_security_policy_for_uri_scheme("foo"), ==, policy);

    policy |= WEBKIT_SECURITY_POLICY_SECURE;
    webkit_set_security_policy_for_uri_scheme("foo", policy);
    g_assert_cmpuint(webkit_get_security_policy_for_uri_scheme("foo"), ==, policy);

    policy |= WEBKIT_SECURITY_POLICY_CORS_ENABLED;
    webkit_set_security_policy_for_uri_scheme("foo", policy);
    g_assert_cmpuint(webkit_get_security_policy_for_uri_scheme("foo"), ==, policy);

    policy |= WEBKIT_SECURITY_POLICY_EMPTY_DOCUMENT;
    webkit_set_security_policy_for_uri_scheme("foo", policy);
    g_assert_cmpuint(webkit_get_security_policy_for_uri_scheme("foo"), ==, policy);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/globals/default_session",
                    test_globals_default_session);
    g_test_add_func("/webkit/globals/security-policy",
                    test_globals_security_policy);
    return g_test_run();
}

