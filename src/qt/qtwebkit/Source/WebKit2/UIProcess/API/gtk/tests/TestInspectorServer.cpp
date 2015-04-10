/*
 * Copyright (C) 2012 Samsung Electronics Ltd. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "WebViewTest.h"
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/WTFString.h>

// Name of the test server application creating the webView object.
static const char* gTestServerAppName = "InspectorTestServer";

// Max seconds to wait for the test server before inspecting it.
static const int gMaxWaitForChild = 5;

// The PID for the test server running, so we can kill it if needed.
static GPid gChildProcessPid = 0;

// Whether the child has replied and it's ready.
static bool gChildIsReady = false;

static void stopTestServer()
{
    // Do nothing if there's no server running.
    if (!gChildProcessPid)
        return;

    g_spawn_close_pid(gChildProcessPid);
    kill(gChildProcessPid, SIGTERM);
    gChildProcessPid = 0;
}

static void sigAbortHandler(int sigNum)
{
    // Just stop the test server if SIGABRT was received.
    stopTestServer();
}

static gpointer testServerMonitorThreadFunc(gpointer)
{
    // Wait for the specified timeout to happen.
    g_usleep(gMaxWaitForChild * G_USEC_PER_SEC);

    // Kill the child process if not ready yet.
    if (!gChildIsReady)
        stopTestServer();

    g_thread_exit(0);
    return 0;
}

static void startTestServerMonitor()
{
    gChildIsReady = false;
    g_thread_new("TestServerMonitor", testServerMonitorThreadFunc, 0);
}

static void startTestServer()
{
    // Prepare argv[] for spawning the server process.
    GOwnPtr<char> testServerPath(g_build_filename(WEBKIT_EXEC_PATH, "WebKit2APITests", gTestServerAppName, NULL));

    // We install a handler to ensure that we kill the child process
    // if the parent dies because of whatever the reason is.
    signal(SIGABRT, sigAbortHandler);

    char* testServerArgv[2];
    testServerArgv[0] = testServerPath.get();
    testServerArgv[1] = 0;

    // Spawn the server, getting its stdout file descriptor to set a
    // communication channel, so we know when it's ready.
    int childStdout = 0;
    g_assert(g_spawn_async_with_pipes(0, testServerArgv, 0, static_cast<GSpawnFlags>(0), 0, 0,
        &gChildProcessPid, 0, &childStdout, 0, 0));

    // Start monitoring the test server (in a separate thread) to
    // ensure we don't block on the child process more than a timeout.
    startTestServerMonitor();

    char msg[2];
    GIOChannel* ioChannel = g_io_channel_unix_new(childStdout);
    if (g_io_channel_read_chars(ioChannel, msg, 2, 0, 0) == G_IO_STATUS_NORMAL) {
        // Check whether the server sent a message saying it's ready
        // and store the result globally, so the monitor can see it.
        gChildIsReady = msg[0] == 'O' && msg[1] == 'K';
    }
    g_io_channel_unref(ioChannel);
    close(childStdout);

    // The timeout was reached and the server is not ready yet, so
    // stop it inmediately, and let the unit tests fail.
    if (!gChildIsReady)
        stopTestServer();
}

class InspectorServerTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(InspectorServerTest);

    InspectorServerTest()
        : WebViewTest()
    {
    }

    bool getPageList()
    {
        loadHtml("<script type=\"text/javascript\">\n"
            "var pages;\n"
            "var xhr = new XMLHttpRequest;\n"
            "xhr.open(\"GET\", \"/pagelist.json\");\n"
            "xhr.onload = function(e) {\n"
                "if (xhr.status == 200) {\n"
                    "pages = JSON.parse(xhr.responseText);\n"
                    "document.title = \"OK\";\n"
                "} else \n"
                    "document.title = \"FAIL\";\n"
                "}\n"
            "xhr.send();\n"
            "</script>\n",
            "http://127.0.0.1:2999/");

        waitUntilTitleChanged();

        if (!strcmp(webkit_web_view_get_title(m_webView), "OK"))
            return true;

        return false;
    }

    ~InspectorServerTest()
    {
    }
};

// Test to get inspector server page list from the test server.
// Should contain only one entry pointing to http://127.0.0.1:2999/webinspector/inspector.html?page=1
static void testInspectorServerPageList(InspectorServerTest* test, gconstpointer)
{
    GOwnPtr<GError> error;

    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);
    g_assert(test->getPageList());

    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished("pages.length;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert_cmpint(WebViewTest::javascriptResultToNumber(javascriptResult), ==, 1);

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("pages[0].id;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    int pageId = WebViewTest::javascriptResultToNumber(javascriptResult);

    GOwnPtr<char> valueString;
    javascriptResult = test->runJavaScriptAndWaitUntilFinished("pages[0].url;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.set(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "http://127.0.0.1:2999/");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("pages[0].inspectorUrl;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.set(WebViewTest::javascriptResultToCString(javascriptResult));
    String validInspectorURL = String("/inspector.html?page=") + String::number(pageId);
    ASSERT_CMP_CSTRING(valueString.get(), ==, validInspectorURL.utf8());
}

// Test sending a raw remote debugging message through our web socket server.
// For this specific message see: http://code.google.com/chrome/devtools/docs/protocol/tot/runtime.html#command-evaluate
static void testRemoteDebuggingMessage(InspectorServerTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);

    test->loadHtml("<script type=\"text/javascript\">\n"
        "var socket = new WebSocket('ws://127.0.0.1:2999/devtools/page/1');\n"
        "socket.onmessage = function(message) {\n"
            "var response = JSON.parse(message.data);\n"
            "if (response.id === 1)\n"
                "document.title = response.result.result.value;\n"
            "else\n"
                "document.title = \"FAIL\";\n"
            "}\n"
            "socket.onopen = function() {\n"
            "socket.send('{\"id\": 1, \"method\": \"Runtime.evaluate\", \"params\": {\"expression\": \"2 + 2\" } }');\n"
        "}\n"
        "</script>",
        "http://127.0.0.1:2999/");
    test->waitUntilTitleChanged();

    g_assert_cmpstr(webkit_web_view_get_title(test->m_webView), ==, "4");
}

static void openRemoteDebuggingSession(InspectorServerTest* test, gconstpointer)
{
    // To test the whole pipeline this exploits a behavior of the inspector front-end which won't provide any title unless the
    // debugging session was established correctly through web socket. It should be something like "Web Inspector - <Page URL>".
    // In our case page URL should be http://127.0.0.1:2999/
    // So this test case will fail if:
    // - The page list didn't return a valid inspector URL
    // - Or the front-end couldn't be loaded through the inspector HTTP server
    // - Or the web socket connection couldn't be established between the front-end and the page through the inspector server
    // Let's see if this test isn't raising too many false positives, in which case we should use a better predicate if available.

    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);

    g_assert(test->getPageList());

    GOwnPtr<GError> error;
    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished("pages[0].inspectorUrl;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());

    String resolvedURL = String("http://127.0.0.1:2999/") + String::fromUTF8(WebViewTest::javascriptResultToCString(javascriptResult));
    test->loadURI(resolvedURL.utf8().data());
    test->waitUntilTitleChanged();

    g_assert_cmpstr(webkit_web_view_get_title(test->m_webView), ==, "Web Inspector - http://127.0.0.1:2999/");
}


void beforeAll()
{
    // Overwrite WEBKIT_INSPECTOR_SERVER variable with default IP address but different port to avoid conflict with the test inspector server page.
    g_setenv("WEBKIT_INSPECTOR_SERVER", "127.0.0.1:2998", TRUE);

    startTestServer();
    InspectorServerTest::add("WebKitWebInspectorServer", "test-page-list", testInspectorServerPageList);
    InspectorServerTest::add("WebKitWebInspectorServer", "test-remote-debugging-message", testRemoteDebuggingMessage);
    InspectorServerTest::add("WebKitWebInspectorServer", "test-open-debugging-session", openRemoteDebuggingSession);

}

void afterAll()
{
    stopTestServer();
}
