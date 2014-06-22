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

#ifndef WebKitTestBus_h
#define WebKitTestBus_h

#include <gio/gio.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

class WebKitTestBus {
public:
    WebKitTestBus();
    virtual ~WebKitTestBus();

    bool run();
    GDBusProxy* createProxy(const char* serviceName, const char* objectPath, const char* interfaceName, GMainLoop*);
    GDBusConnection* connection() const { return m_connection.get(); }

private:
    GDBusConnection* getOrCreateConnection();

    pid_t m_pid;
    CString m_address;
    GRefPtr<GDBusConnection> m_connection;
};

#endif // WebKitTestBus_h
