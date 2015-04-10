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

#ifndef WebKitTestServer_h
#define WebKitTestServer_h

#include <libsoup/soup.h>
#include <webkit2/webkit2.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

class WebKitTestServer {
public:

    enum ServerType {
        ServerHTTP,
        ServerHTTPS
    };

    WebKitTestServer(ServerType type = ServerHTTP);
    virtual ~WebKitTestServer();

    SoupURI* baseURI() { return m_baseURI; }

    CString getURIForPath(const char* path);
    void run(SoupServerCallback);

private:
    GRefPtr<SoupServer> m_soupServer;
    SoupURI* m_baseURI;
};

#endif // WebKitTestServer_h
