/*
 * Copyright (C) 2009 Christian Dywan <christian@twotoasts.de>
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

#include "autotoolsconfig.h"
#include <errno.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <webkit/webkit.h>

GMainLoop* loop;
char* temporaryFilename = NULL;
WebKitDownload* theDownload = NULL;

static void
test_webkit_download_create(void)
{
    WebKitNetworkRequest* request;
    WebKitDownload* download;
    const gchar* uri = "http://example.com";
    gchar* tmpDir;

    request = webkit_network_request_new(uri);
    download = webkit_download_new(request);
    g_object_unref(request);
    g_assert_cmpstr(webkit_download_get_uri(download), ==, uri);
    g_assert(webkit_download_get_network_request(download) == request);
    g_assert(g_strrstr(uri, webkit_download_get_suggested_filename(download)));
    g_assert(webkit_download_get_status(download) == WEBKIT_DOWNLOAD_STATUS_CREATED);
    g_assert(!webkit_download_get_total_size(download));
    g_assert(!webkit_download_get_current_size(download));
    g_assert(!webkit_download_get_progress(download));
    g_assert(!webkit_download_get_elapsed_time(download));
    tmpDir = g_filename_to_uri(g_get_tmp_dir(), NULL, NULL);
    webkit_download_set_destination_uri(download, tmpDir);
    g_assert_cmpstr(tmpDir, ==, webkit_download_get_destination_uri(download));;
    g_free(tmpDir);
    g_object_unref(download);
}

static gboolean
navigation_policy_decision_requested_cb(WebKitWebView* web_view,
                                        WebKitWebFrame* web_frame,
                                        WebKitNetworkRequest* request,
                                        WebKitWebNavigationAction* action,
                                        WebKitWebPolicyDecision* decision,
                                        gpointer data)
{
    webkit_web_policy_decision_download(decision);
    return TRUE;
}

static void
notify_status_cb(GObject* object, GParamSpec* pspec, gpointer data)
{
    WebKitDownload* download = WEBKIT_DOWNLOAD(object);
    switch (webkit_download_get_status(download)) {
    case WEBKIT_DOWNLOAD_STATUS_FINISHED:
    case WEBKIT_DOWNLOAD_STATUS_ERROR:
        g_main_loop_quit(loop);
        break;
    case WEBKIT_DOWNLOAD_STATUS_CANCELLED:
        g_assert_not_reached();
        break;
    default:
        break;
    }
}

static gboolean
set_filename(gchar* filename)
{
    gchar *uri = g_filename_to_uri(filename, NULL, NULL);

    webkit_download_set_destination_uri(theDownload, uri);
    g_free(uri);

    webkit_download_start(theDownload);
    return FALSE;
}

static void
handle_download_requested_cb(WebKitDownload* download,
                             gboolean* beenThere,
                             gboolean asynch)
{
    theDownload = download;
    *beenThere = TRUE;

    if (temporaryFilename) {
        if (asynch) {
            g_idle_add((GSourceFunc)set_filename, temporaryFilename);
        } else {
            gchar *uri = g_filename_to_uri(temporaryFilename, NULL, NULL);
            if (uri)
                webkit_download_set_destination_uri(download, uri);
            g_free(uri);
        }
    }

    g_signal_connect(download, "notify::status",
                     G_CALLBACK(notify_status_cb), NULL);
}

static gboolean
download_requested_cb(WebKitWebView* web_view,
                      WebKitDownload* download,
                      gboolean* beenThere)
{
    handle_download_requested_cb(download, beenThere, FALSE);
    return TRUE;
}

static gboolean
download_requested_asynch_cb(WebKitWebView* web_view,
                             WebKitDownload* download,
                             gboolean* beenThere)
{
    handle_download_requested_cb(download, beenThere, TRUE);
    return TRUE;
}

static void
test_webkit_download_perform(gboolean asynch)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    GCallback downloadRequestCallback = NULL;

    g_object_ref_sink(G_OBJECT(webView));

    g_signal_connect(webView, "navigation-policy-decision-requested",
                     G_CALLBACK(navigation_policy_decision_requested_cb),
                     NULL);

    if (asynch)
        downloadRequestCallback = G_CALLBACK(download_requested_asynch_cb);
    else
        downloadRequestCallback = G_CALLBACK(download_requested_cb);

    gboolean beenThere = FALSE;
    g_signal_connect(webView, "download-requested",
                     downloadRequestCallback, &beenThere);

    /* Preparation; FIXME: we should move this code to a test
     * utilities file, because we have a very similar one in
     * testwebframe.c */
    GError *error = NULL;
    gchar* filename;
    int fd = g_file_open_tmp("webkit-testwebdownload-XXXXXX", &filename, &error);
    close(fd);

    if (error)
        g_critical("Failed to open a temporary file for writing: %s.", error->message);

    if (g_unlink(filename) == -1)
        g_critical("Failed to delete the temporary file: %s.", g_strerror(errno));

    theDownload = NULL;
    temporaryFilename = filename;

    loop = g_main_loop_new(NULL, TRUE);
    webkit_web_view_load_uri(webView, "http://gnome.org/");
    g_main_loop_run(loop);

    g_assert_cmpint(beenThere, ==, TRUE);

    g_assert_cmpint(g_file_test(temporaryFilename, G_FILE_TEST_IS_REGULAR), ==, TRUE);

    g_unlink(temporaryFilename);
    g_free(temporaryFilename);
    temporaryFilename = NULL;

    g_main_loop_unref(loop);
    g_object_unref(webView);
}

static void
test_webkit_download_synch(void)
{
    test_webkit_download_perform(FALSE);
}

static void
test_webkit_download_asynch(void)
{
    test_webkit_download_perform(TRUE);
}

static gboolean mime_type_policy_decision_requested_cb(WebKitWebView* view, WebKitWebFrame* frame,
                                                       WebKitNetworkRequest* request, const char* mime_type,
                                                       WebKitWebPolicyDecision* decision, gpointer data)
{
    webkit_web_policy_decision_download(decision);
    return TRUE;
}

static void idle_quit_loop_cb(WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    if (webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FINISHED ||
        webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FAILED)
        g_main_loop_quit(loop);
}

static void
test_webkit_download_data(void)
{
    gboolean beenThere = FALSE;
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);

    g_signal_connect(webView, "download-requested",
                     G_CALLBACK(download_requested_cb),
                     &beenThere);

    g_signal_connect(webView, "notify::load-status",
                     G_CALLBACK(idle_quit_loop_cb),
                     NULL);

    g_signal_connect(webView, "mime-type-policy-decision-requested",
                     G_CALLBACK(mime_type_policy_decision_requested_cb),
                     NULL);

    loop = g_main_loop_new(NULL, TRUE);

    /* We're testing for a crash, so just not crashing is a pass */
    webkit_web_view_load_uri(webView, "data:application/octect-stream,");
    g_main_loop_run(loop);

    g_assert_cmpint(beenThere, ==, TRUE);

    g_main_loop_unref(loop);
    g_object_unref(webView);
}

static void notifyDownloadStatusCallback(GObject *object, GParamSpec *pspec, gpointer data)
{
    WebKitDownload *download = WEBKIT_DOWNLOAD(object);
    WebKitNetworkResponse *response = webkit_download_get_network_response(download);
    SoupMessage *message = webkit_network_response_get_message(response);

    switch (webkit_download_get_status(download)) {
    case WEBKIT_DOWNLOAD_STATUS_ERROR:
        g_assert_cmpint(message->status_code, ==, 404);
        g_main_loop_quit(loop);
        break;
    case WEBKIT_DOWNLOAD_STATUS_FINISHED:
    case WEBKIT_DOWNLOAD_STATUS_CANCELLED:
        g_assert_not_reached();
        break;
    default:
        break;
    }
}

static void serverCallback(SoupServer *server, SoupMessage *message, const char *path, GHashTable *query, SoupClientContext *context, gpointer userData)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
    soup_message_body_complete(message->response_body);
}

static void test_webkit_download_not_found(void)
{
    SoupServer *server = soup_server_new(SOUP_SERVER_PORT, 0, NULL);
    soup_server_run_async(server);
    soup_server_add_handler(server, NULL, serverCallback, NULL, NULL);
    SoupURI *baseURI = soup_uri_new("http://127.0.0.1/");
    soup_uri_set_port(baseURI, soup_server_get_port(server));

    SoupURI *uri = soup_uri_new_with_base(baseURI, "/foo");
    char *uriString = soup_uri_to_string(uri, FALSE);
    soup_uri_free(uri);

    loop = g_main_loop_new(NULL, TRUE);
    WebKitNetworkRequest *request = webkit_network_request_new(uriString);
    g_free (uriString);
    WebKitDownload *download = webkit_download_new(request);
    g_object_unref(request);

    webkit_download_set_destination_uri(download, "file:///tmp/foo");
    g_signal_connect(download, "notify::status", G_CALLBACK(notifyDownloadStatusCallback), NULL);

    webkit_download_start(download);
    g_main_loop_run(loop);

    g_object_unref(download);
    g_main_loop_unref(loop);
    soup_uri_free(baseURI);
    g_object_unref(server);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/download/create", test_webkit_download_create);
    g_test_add_func("/webkit/download/synch", test_webkit_download_synch);
    g_test_add_func("/webkit/download/asynch", test_webkit_download_asynch);
    g_test_add_func("/webkit/download/data", test_webkit_download_data);
    g_test_add_func("/webkit/download/not-found", test_webkit_download_not_found);
    return g_test_run ();
}
