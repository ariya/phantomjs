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
#include "WebKitTestBus.h"

#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/WTFString.h>

WebKitTestBus::WebKitTestBus()
    : m_pid(-1)
{
}

bool WebKitTestBus::run()
{
    // FIXME: Use GTestDBus when we bump glib to 2.34.
    GOwnPtr<char> dbusLaunch(g_find_program_in_path("dbus-launch"));
    if (!dbusLaunch) {
        g_warning("Error starting DBUS daemon: dbus-launch not found in path");
        return false;
    }

    GOwnPtr<char> output;
    GOwnPtr<GError> error;
    if (!g_spawn_command_line_sync(dbusLaunch.get(), &output.outPtr(), 0, 0, &error.outPtr())) {
        g_warning("Error starting DBUS daemon: %s", error->message);
        return false;
    }

    String outputString = String::fromUTF8(output.get());
    Vector<String> lines;
    outputString.split(UChar('\n'), /* allowEmptyEntries */ false, lines);
    for (size_t i = 0; i < lines.size(); ++i) {
        char** keyValue = g_strsplit(lines[i].utf8().data(), "=", 2);
        g_assert_cmpuint(g_strv_length(keyValue), ==, 2);
        if (!g_strcmp0(keyValue[0], "DBUS_SESSION_BUS_ADDRESS")) {
            m_address = keyValue[1];
            g_setenv("DBUS_SESSION_BUS_ADDRESS", keyValue[1], TRUE);
        } else if (!g_strcmp0(keyValue[0], "DBUS_SESSION_BUS_PID"))
            m_pid = g_ascii_strtoll(keyValue[1], 0, 10);
        g_strfreev(keyValue);
    }

    return m_pid > 0;
}

WebKitTestBus::~WebKitTestBus()
{
    g_unsetenv("DBUS_SESSION_BUS_ADDRESS");

    if (m_pid != -1)
        kill(m_pid, SIGTERM);
}

GDBusConnection* WebKitTestBus::getOrCreateConnection()
{
    if (m_connection)
        return m_connection.get();

    g_assert(!m_address.isNull());
    m_connection = adoptGRef(g_dbus_connection_new_for_address_sync(m_address.data(),
        static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
        0, 0, 0));
    return m_connection.get();
}

static void onNameAppeared(GDBusConnection*, const char*, const char*, gpointer userData)
{
    g_main_loop_quit(static_cast<GMainLoop*>(userData));
}

GDBusProxy* WebKitTestBus::createProxy(const char* serviceName, const char* objectPath, const char* interfaceName, GMainLoop* mainLoop)
{
    unsigned watcherID = g_bus_watch_name_on_connection(getOrCreateConnection(), serviceName, G_BUS_NAME_WATCHER_FLAGS_NONE, onNameAppeared, 0, mainLoop, 0);
    g_main_loop_run(mainLoop);
    g_bus_unwatch_name(watcherID);

    GDBusProxy* proxy = g_dbus_proxy_new_sync(
        connection(),
        G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
        0, // GDBusInterfaceInfo
        serviceName,
        objectPath,
        interfaceName,
        0, // GCancellable
        0);
    g_assert(proxy);
    return proxy;
}
