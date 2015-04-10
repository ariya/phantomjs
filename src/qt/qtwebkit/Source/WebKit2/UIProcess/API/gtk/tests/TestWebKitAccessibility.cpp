/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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

#include "TestMain.h"
#include "WebViewTest.h"

// The libatspi headers don't use G_BEGIN_DECLS
extern "C" {
#include <atspi/atspi.h>
}

#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <signal.h>
#include <unistd.h>
#include <wtf/PassRefPtr.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>

// Name of the test server application creating the webView object.
static const char* kTestServerAppName = "AccessibilityTestServer";

// Max seconds to wait for the test server before inspecting it.
static const int kMaxWaitForChild = 5;

// The PID for the test server running, so we can kill it if needed.
static GPid kChildProcessPid = 0;

// Whether the child has replied and it's ready.
static bool kChildIsReady = false;

static void stopTestServer()
{
    // Do nothing if there's no server running.
    if (!kChildProcessPid)
        return;

    g_spawn_close_pid(kChildProcessPid);
    kill(kChildProcessPid, SIGTERM);
    kChildProcessPid = 0;
}

static void sigAbortHandler(int sigNum)
{
    // Just stop the test server if SIGABRT was received.
    stopTestServer();
}

static gpointer testServerMonitorThreadFunc(gpointer)
{
    // Wait for the specified timeout to happen.
    g_usleep(kMaxWaitForChild * G_USEC_PER_SEC);

    // Kill the child process if not ready yet.
    if (!kChildIsReady)
        stopTestServer();

    g_thread_exit(0);
    return 0;
}

static void startTestServerMonitor()
{
    kChildIsReady = false;
    g_thread_new("TestServerMonitor", testServerMonitorThreadFunc, 0);
}

static void startTestServer()
{
    // Prepare argv[] for spawning the server process.
    GOwnPtr<char> testServerPath(g_build_filename(WEBKIT_EXEC_PATH, "WebKit2APITests", kTestServerAppName, NULL));

    char* testServerArgv[2];
    testServerArgv[0] = testServerPath.get();
    testServerArgv[1] = 0;

    // Spawn the server, getting its stdout file descriptor to set a
    // communication channel, so we know when it's ready.
    int childStdout = 0;
    if (!g_spawn_async_with_pipes(0, testServerArgv, 0, static_cast<GSpawnFlags>(0), 0, 0,
                                  &kChildProcessPid, 0, &childStdout, 0, 0)) {
        close(childStdout);
        return;
    }

    // Start monitoring the test server (in a separate thread) to
    // ensure we don't block on the child process more than a timeout.
    startTestServerMonitor();

    char msg[2];
    GIOChannel* ioChannel = g_io_channel_unix_new(childStdout);
    if (g_io_channel_read_chars(ioChannel, msg, 2, 0, 0) == G_IO_STATUS_NORMAL) {
        // Check whether the server sent a message saying it's ready
        // and store the result globally, so the monitor can see it.
        kChildIsReady = msg[0] == 'O' && msg[1] == 'K';
    }
    g_io_channel_unref(ioChannel);
    close(childStdout);

    // The timeout was reached and the server is not ready yet, so
    // stop it inmediately, and let the unit tests fail.
    if (!kChildIsReady)
        stopTestServer();
}

static void checkAtspiAccessible(AtspiAccessible* accessible, const char* targetName, AtspiRole targetRole)
{
    g_assert(ATSPI_IS_ACCESSIBLE(accessible));

    GOwnPtr<char> name(atspi_accessible_get_name(accessible, 0));
    g_assert_cmpstr(targetName, ==, name.get());
    g_assert_cmpint(targetRole, ==, atspi_accessible_get_role(accessible, 0));
}

static GRefPtr<AtspiAccessible> findTestServerApplication()
{
    // Only one desktop is supported by ATSPI at the moment.
    GRefPtr<AtspiAccessible> desktop = adoptGRef(atspi_get_desktop(0));

    // Look for the server application in the list of apps.
    GRefPtr<AtspiAccessible> current;
    int childCount = atspi_accessible_get_child_count(desktop.get(), 0);
    for (int i = 0; i < childCount; i++) {
        current = adoptGRef(atspi_accessible_get_child_at_index(desktop.get(), i, 0));
        if (!g_strcmp0(atspi_accessible_get_name(current.get(), 0), kTestServerAppName))
            return current;
    }

    return 0;
}

static void testAtspiBasicHierarchy(WebViewTest* test, gconstpointer)
{
    // The test server's accessibility object (UI Process).
    GRefPtr<AtspiAccessible> testServerApp = findTestServerApplication();
    g_assert(ATSPI_IS_ACCESSIBLE(testServerApp.get()));
    checkAtspiAccessible(testServerApp.get(), "AccessibilityTestServer", ATSPI_ROLE_APPLICATION);

    // The main window's accessibility object (UI Process).
    GRefPtr<AtspiAccessible> currentParent = testServerApp;
    GRefPtr<AtspiAccessible> currentChild = adoptGRef(atspi_accessible_get_child_at_index(currentParent.get(), 0, 0));
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));
    checkAtspiAccessible(currentChild.get(), "", ATSPI_ROLE_FRAME);

    // The WebView's accessibility object (UI Process).
    currentParent = currentChild;
    currentChild = atspi_accessible_get_child_at_index(currentParent.get(), 0, 0);
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));
    checkAtspiAccessible(currentChild.get(), "", ATSPI_ROLE_FILLER);

    // The WebPage's accessibility object (Web Process).
    currentParent = currentChild;
    currentChild = atspi_accessible_get_child_at_index(currentParent.get(), 0, 0);
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));
    checkAtspiAccessible(currentChild.get(), "", ATSPI_ROLE_FILLER);

    // HTML root element's accessible element (Web Process).
    currentParent = currentChild;
    currentChild = atspi_accessible_get_child_at_index(currentParent.get(), 0, 0);
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));

    // HTML body's accessible element (Web Process).
    currentParent = currentChild;
    currentChild = atspi_accessible_get_child_at_index(currentParent.get(), 0, 0);
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));
    checkAtspiAccessible(currentChild.get(), "", ATSPI_ROLE_DOCUMENT_FRAME);

    // HTML H1's accessible element (Web Process).
    currentParent = currentChild;
    currentChild = atspi_accessible_get_child_at_index(currentParent.get(), 0, 0);
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));
    checkAtspiAccessible(currentChild.get(), "This is a test", ATSPI_ROLE_HEADING);

    // HTML first paragraph's accessible element (Web Process).
    currentChild = atspi_accessible_get_child_at_index(currentParent.get(), 1, 0);
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));
    checkAtspiAccessible(currentChild.get(), "", ATSPI_ROLE_PARAGRAPH);

    // HTML second paragraph's accessible element (Web Process).
    currentChild = atspi_accessible_get_child_at_index(currentParent.get(), 2, 0);
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));
    checkAtspiAccessible(currentChild.get(), "", ATSPI_ROLE_PARAGRAPH);

    // HTML link's accessible element (Web Process).
    currentParent = currentChild;
    currentChild = atspi_accessible_get_child_at_index(currentParent.get(), 0, 0);
    g_assert(ATSPI_IS_ACCESSIBLE(currentChild.get()));
    checkAtspiAccessible(currentChild.get(), "a link", ATSPI_ROLE_LINK);
}

void beforeAll()
{
    // We install a handler to ensure that we kill the child process
    // if the parent dies because of whatever the reason is.
    signal(SIGABRT, sigAbortHandler);

    // Start the accessibility test server and load the tests.
    startTestServer();
    WebViewTest::add("WebKitAccessibility", "atspi-basic-hierarchy", testAtspiBasicHierarchy);
}

void afterAll()
{
    // Ensure we stop the server.
    stopTestServer();
}
