/*
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2012 Intel Corporation
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
#include "EWK2UnitTestServer.h"

EWK2UnitTestServer::EWK2UnitTestServer()
{
    SoupAddress* address = soup_address_new("127.0.0.1", SOUP_ADDRESS_ANY_PORT);
    soup_address_resolve_sync(address, 0);

    m_soupServer = soup_server_new(SOUP_SERVER_INTERFACE, address, static_cast<char*>(0));
    m_baseURL = soup_uri_new("http://127.0.0.1/");
    soup_uri_set_port(m_baseURL, soup_server_get_port(m_soupServer));
    g_object_unref(address);
}

EWK2UnitTestServer::~EWK2UnitTestServer()
{
    soup_uri_free(m_baseURL);
    g_object_unref(m_soupServer);
}

void EWK2UnitTestServer::run(SoupServerCallback serverCallback)
{
    soup_server_run_async(m_soupServer);
    soup_server_add_handler(m_soupServer, 0, serverCallback, 0, 0);
}

CString EWK2UnitTestServer::getURLForPath(const char* path) const
{
    SoupURI* soupURL = soup_uri_new_with_base(m_baseURL, path);
    char* url = soup_uri_to_string(soupURL, false);
    CString urlString = url;
    free(url);
    soup_uri_free(soupURL);

    return urlString;
}
