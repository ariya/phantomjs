/*
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

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#include <webkit2/webkit-web-extension.h>
#include <wtf/gobject/GOwnPtr.h>

static const char introspectionXML[] =
    "<node>"
    " <interface name='org.webkit.gtk.WebExtensionTest'>"
    "  <method name='GetTitle'>"
    "   <arg type='t' name='pageID' direction='in'/>"
    "   <arg type='s' name='title' direction='out'/>"
    "  </method>"
    "  <method name='AbortProcess'>"
    "  </method>"
    "  <signal name='DocumentLoaded'/>"
    "  <signal name='URIChanged'>"
    "   <arg type='s' name='uri' direction='out'/>"
    "  </signal>"
    " </interface>"
    "</node>";

static void documentLoadedCallback(WebKitWebPage*, gpointer userData)
{
    bool ok = g_dbus_connection_emit_signal(G_DBUS_CONNECTION(userData),
        0,
        "/org/webkit/gtk/WebExtensionTest",
        "org.webkit.gtk.WebExtensionTest",
        "DocumentLoaded",
        0,
        0);
    g_assert(ok);
}

static void uriChangedCallback(WebKitWebPage* webPage, GParamSpec* pspec, gpointer userData)
{
    bool ok = g_dbus_connection_emit_signal(
        G_DBUS_CONNECTION(userData),
        0,
        "/org/webkit/gtk/WebExtensionTest",
        "org.webkit.gtk.WebExtensionTest",
        "URIChanged",
        g_variant_new("(s)", webkit_web_page_get_uri(webPage)),
        0);
    g_assert(ok);
}

static gboolean sendRequestCallback(WebKitWebPage*, WebKitURIRequest* request, WebKitURIResponse*, gpointer)
{
    const char* requestURI = webkit_uri_request_get_uri(request);
    g_assert(requestURI);

    if (const char* suffix = g_strrstr(requestURI, "/remove-this/javascript.js")) {
        GOwnPtr<char> prefix(g_strndup(requestURI, strlen(requestURI) - strlen(suffix)));
        GOwnPtr<char> newURI(g_strdup_printf("%s/javascript.js", prefix.get()));
        webkit_uri_request_set_uri(request, newURI.get());
    } else if (g_str_has_suffix(requestURI, "/add-do-not-track-header")) {
        SoupMessageHeaders* headers = webkit_uri_request_get_http_headers(request);
        g_assert(headers);
        soup_message_headers_append(headers, "DNT", "1");
    } else if (g_str_has_suffix(requestURI, "/cancel-this.js"))
        return TRUE;

    return FALSE;
}

static void pageCreatedCallback(WebKitWebExtension*, WebKitWebPage* webPage, gpointer userData)
{
    g_signal_connect(webPage, "document-loaded", G_CALLBACK(documentLoadedCallback), userData);
    g_signal_connect(webPage, "notify::uri", G_CALLBACK(uriChangedCallback), userData);
    g_signal_connect(webPage, "send-request", G_CALLBACK(sendRequestCallback), 0);
}

static void methodCallCallback(GDBusConnection* connection, const char* sender, const char* objectPath, const char* interfaceName, const char* methodName, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer userData)
{
    if (g_strcmp0(interfaceName, "org.webkit.gtk.WebExtensionTest"))
        return;

    if (!g_strcmp0(methodName, "GetTitle")) {
        uint64_t pageID;
        g_variant_get(parameters, "(t)", &pageID);

        WebKitWebExtension* extension = WEBKIT_WEB_EXTENSION(userData);
        WebKitWebPage* page = webkit_web_extension_get_page(extension, pageID);
        if (!page) {
            g_dbus_method_invocation_return_error(
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                "Invalid page ID: %" G_GUINT64_FORMAT, pageID);
            return;
        }
        g_assert_cmpuint(webkit_web_page_get_id(page), ==, pageID);

        WebKitDOMDocument* document = webkit_web_page_get_dom_document(page);
        GOwnPtr<char> title(webkit_dom_document_get_title(document));
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", title.get()));
    } else if (!g_strcmp0(methodName, "AbortProcess")) {
        abort();
    }
}

static const GDBusInterfaceVTable interfaceVirtualTable = {
    methodCallCallback, 0, 0, { 0, }
};

static void busAcquiredCallback(GDBusConnection* connection, const char* name, gpointer userData)
{
    static GDBusNodeInfo *introspectionData = 0;
    if (!introspectionData)
        introspectionData = g_dbus_node_info_new_for_xml(introspectionXML, 0);

    GOwnPtr<GError> error;
    unsigned registrationID = g_dbus_connection_register_object(
        connection,
        "/org/webkit/gtk/WebExtensionTest",
        introspectionData->interfaces[0],
        &interfaceVirtualTable,
        g_object_ref(userData),
        static_cast<GDestroyNotify>(g_object_unref),
        &error.outPtr());
    if (!registrationID)
        g_warning("Failed to register object: %s\n", error->message);

    g_signal_connect(WEBKIT_WEB_EXTENSION(userData), "page-created", G_CALLBACK(pageCreatedCallback), connection);
}

extern "C" void webkit_web_extension_initialize(WebKitWebExtension* extension)
{
    g_bus_own_name(
        G_BUS_TYPE_SESSION,
        "org.webkit.gtk.WebExtensionTest",
        G_BUS_NAME_OWNER_FLAGS_NONE,
        busAcquiredCallback,
        0, 0,
        g_object_ref(extension),
        static_cast<GDestroyNotify>(g_object_unref));
}
