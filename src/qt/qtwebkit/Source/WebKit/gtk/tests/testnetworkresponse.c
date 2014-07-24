/*
 * Copyright (C) 2009 Gustavo Noronha Silva
 * Copyright (C) 2009 Collabora Ltd.
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
#include <stdlib.h>
#include <webkit/webkit.h>

static void test_network_response_create_destroy()
{
    WebKitNetworkResponse* response;
    SoupMessage* message;

    /* Test creation with URI */
    response = WEBKIT_NETWORK_RESPONSE(g_object_new(WEBKIT_TYPE_NETWORK_RESPONSE, "uri", "http://debian.org/", NULL));
    g_assert(WEBKIT_IS_NETWORK_RESPONSE(response));
    message = webkit_network_response_get_message(response);
    g_assert(!message);
    g_object_unref(response);

    /* Test creation with SoupMessage */
    message = soup_message_new("GET", "http://debian.org/");
    response = WEBKIT_NETWORK_RESPONSE(g_object_new(WEBKIT_TYPE_NETWORK_RESPONSE, "message", message, NULL));
    g_assert(WEBKIT_IS_NETWORK_RESPONSE(response));
    g_assert_cmpint(G_OBJECT(message)->ref_count, ==, 2);
    g_object_unref(response);
    g_assert_cmpint(G_OBJECT(message)->ref_count, ==, 1);
    g_object_unref(message);

    /* Test creation with both SoupMessage and URI */
    message = soup_message_new("GET", "http://debian.org/");
    response = WEBKIT_NETWORK_RESPONSE(g_object_new(WEBKIT_TYPE_NETWORK_RESPONSE, "message", message, "uri", "http://gnome.org/", NULL));
    g_assert(WEBKIT_IS_NETWORK_RESPONSE(response));
    g_assert_cmpint(G_OBJECT(message)->ref_count, ==, 2);
    g_assert_cmpstr(webkit_network_response_get_uri(response), ==, "http://gnome.org/");
    g_object_unref(response);
    g_assert_cmpint(G_OBJECT(message)->ref_count, ==, 1);
    g_object_unref(message);
}

static void test_network_response_properties()
{
    WebKitNetworkResponse* response;
    SoupMessage* message;
    gchar* soupURI;

    /* Test URI is set correctly when creating with URI */
    response = webkit_network_response_new("http://debian.org/");
    g_assert(WEBKIT_IS_NETWORK_RESPONSE(response));
    g_assert_cmpstr(webkit_network_response_get_uri(response), ==, "http://debian.org/");
    g_object_unref(response);

    /* Test URI is set correctly when creating with Message */
    message = soup_message_new("GET", "http://debian.org/");
    response = WEBKIT_NETWORK_RESPONSE(g_object_new(WEBKIT_TYPE_NETWORK_RESPONSE, "message", message, NULL));
    g_assert(WEBKIT_IS_NETWORK_RESPONSE(response));
    g_object_unref(message);

    message = webkit_network_response_get_message(response);
    soupURI = soup_uri_to_string(soup_message_get_uri(message), FALSE);
    g_assert_cmpstr(soupURI, ==, "http://debian.org/");
    g_free(soupURI);

    g_assert_cmpstr(webkit_network_response_get_uri(response), ==, "http://debian.org/");
    g_object_unref(response);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/networkresponse/createdestroy", test_network_response_create_destroy);
    g_test_add_func("/webkit/networkresponse/properties", test_network_response_properties);
    return g_test_run ();
}
