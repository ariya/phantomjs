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

#include "config.h"
#include "WebKitTestServer.h"

#include "TestMain.h"
#include <wtf/gobject/GOwnPtr.h>

WebKitTestServer::WebKitTestServer(ServerType type)
{
    GOwnPtr<char> sslCertificateFile;
    GOwnPtr<char> sslKeyFile;
    if (type == ServerHTTPS) {
        CString resourcesDir = Test::getResourcesDir();
        sslCertificateFile.set(g_build_filename(resourcesDir.data(), "test-cert.pem", NULL));
        sslKeyFile.set(g_build_filename(resourcesDir.data(), "test-key.pem", NULL));
    }

    GRefPtr<SoupAddress> address = adoptGRef(soup_address_new("127.0.0.1", SOUP_ADDRESS_ANY_PORT));
    soup_address_resolve_sync(address.get(), 0);

    m_soupServer = adoptGRef(soup_server_new(SOUP_SERVER_INTERFACE, address.get(),
                                             SOUP_SERVER_SSL_CERT_FILE, sslCertificateFile.get(),
                                             SOUP_SERVER_SSL_KEY_FILE, sslKeyFile.get(), NULL));
    m_baseURI = type == ServerHTTPS ? soup_uri_new("https://127.0.0.1/") : soup_uri_new("http://127.0.0.1/");
    soup_uri_set_port(m_baseURI, soup_server_get_port(m_soupServer.get()));
}

WebKitTestServer::~WebKitTestServer()
{
    soup_uri_free(m_baseURI);
}

void WebKitTestServer::run(SoupServerCallback serverCallback)
{
    soup_server_run_async(m_soupServer.get());
    soup_server_add_handler(m_soupServer.get(), 0, serverCallback, 0, 0);
}

CString WebKitTestServer::getURIForPath(const char* path)
{
    SoupURI* uri = soup_uri_new_with_base(m_baseURI, path);
    GOwnPtr<gchar> uriString(soup_uri_to_string(uri, FALSE));
    soup_uri_free(uri);
    return uriString.get();
}

