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

#ifndef EWK2UnitTestServer_h
#define EWK2UnitTestServer_h

#include <libsoup/soup.h>
#include <wtf/text/CString.h>

class EWK2UnitTestServer {
public:
    EWK2UnitTestServer();
    virtual ~EWK2UnitTestServer();

    CString getURLForPath(const char* path) const;
    void run(SoupServerCallback);

private:
    SoupServer* m_soupServer;
    SoupURI* m_baseURL;
};

#endif // EWK2UnitTestServer_h

