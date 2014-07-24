/*
 * Copyright (C) 2009 Gustavo Noronha Silva
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
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

// Not yet public API
SoupMessage* webkit_network_request_get_message(WebKitNetworkRequest* request);

static gboolean navigation_policy_decision_requested_cb(WebKitWebView* web_view,
                                                        WebKitWebFrame* web_frame,
                                                        WebKitNetworkRequest* request,
                                                        WebKitWebNavigationAction* action,
                                                        WebKitWebPolicyDecision* decision,
                                                        gpointer data)
{
    SoupMessage* message = webkit_network_request_get_message(request);

    /* 1 -> webkit_network_request_with_core_request
     *
     * The SoupMessage is created exclusively for the emission of this
     * signal.
     */
    g_assert_cmpint(G_OBJECT(message)->ref_count, ==, 1);

    return FALSE;
}

static void test_soup_message_lifetime()
{
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

    g_object_ref_sink(web_view);

    g_signal_connect(web_view, "navigation-policy-decision-requested",
                     G_CALLBACK(navigation_policy_decision_requested_cb),
                     NULL);

    /* load_uri will trigger the navigation-policy-decision-requested
     * signal emission;
     */
    webkit_web_view_load_uri(web_view, "http://127.0.0.1/");

    g_object_unref(web_view);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/soupmessage/lifetime", test_soup_message_lifetime);
    return g_test_run ();
}
