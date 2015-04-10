/*
 * Copyright (C) 2006, 2007 Apple Inc.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2011 Lukasz Slachciak
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "autotoolsconfig.h"
#include "LauncherInspectorWindow.h"
#include <errno.h>
#include <gdk/gdkkeysyms.h>
#ifdef WTF_USE_GSTREAMER
#include <gst/gst.h>
#endif
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <webkit/webkit.h>

static gint windowCount = 0;

static GtkWidget* createWindow(WebKitWebView** outWebView);

static void activateUriEntryCb(GtkWidget* entry, gpointer data)
{
    WebKitWebView *webView = g_object_get_data(G_OBJECT(entry), "web-view");
    const gchar* uri = gtk_entry_get_text(GTK_ENTRY(entry));
    g_assert(uri);
    gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, 0);
    webkit_web_view_load_uri(webView, uri);
}

static void updateTitle(GtkWindow* window, WebKitWebView* webView)
{
    GString *string = g_string_new(webkit_web_view_get_title(webView));
    gdouble loadProgress = webkit_web_view_get_progress(webView) * 100;
    g_string_append(string, " - WebKit Launcher");
    if (loadProgress < 100)
        g_string_append_printf(string, " (%f%%)", loadProgress);
    gchar *title = g_string_free(string, FALSE);
    gtk_window_set_title(window, title);
    g_free(title);
}

static void linkHoverCb(WebKitWebView* page, const gchar* title, const gchar* link, GtkStatusbar* statusbar)
{
    guint statusContextId =
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(statusbar), "link-hover-context"));
    /* underflow is allowed */
    gtk_statusbar_pop(statusbar, statusContextId);
    if (link)
        gtk_statusbar_push(statusbar, statusContextId, link);
}

static void notifyTitleCb(WebKitWebView* webView, GParamSpec* pspec, GtkWidget* window)
{
    updateTitle(GTK_WINDOW(window), webView);
}

static void notifyLoadStatusCb(WebKitWebView* webView, GParamSpec* pspec, GtkWidget* uriEntry)
{
    if (webkit_web_view_get_load_status(webView) == WEBKIT_LOAD_COMMITTED) {
        WebKitWebFrame *frame = webkit_web_view_get_main_frame(webView);
        const gchar *uri = webkit_web_frame_get_uri(frame);
        if (uri)
            gtk_entry_set_text(GTK_ENTRY(uriEntry), uri);
    }
}

static void notifyProgressCb(WebKitWebView* webView, GParamSpec* pspec, GtkWidget* window)
{
    updateTitle(GTK_WINDOW(window), webView);
}

static void destroyCb(GtkWidget* widget, GtkWidget* window)
{
    if (g_atomic_int_dec_and_test(&windowCount))
      gtk_main_quit();
}

static void goBackCb(GtkWidget* widget,  WebKitWebView* webView)
{
    webkit_web_view_go_back(webView);
}

static void goForwardCb(GtkWidget* widget, WebKitWebView* webView)
{
    webkit_web_view_go_forward(webView);
}

static void reloadCb(GtkWidget* widget, WebKitWebView* webView)
{
    webkit_web_view_reload(webView);
}

static WebKitWebView*
createWebViewCb(WebKitWebView* webView, WebKitWebFrame* web_frame, GtkWidget* window)
{
    WebKitWebView *newWebView;
    createWindow(&newWebView);
    webkit_web_view_set_settings(newWebView, webkit_web_view_get_settings(webView));
    return newWebView;
}

static gboolean webViewReadyCb(WebKitWebView* webView, GtkWidget* window)
{
    gtk_widget_grab_focus(GTK_WIDGET(webView));
    gtk_widget_show_all(window);
    return FALSE;
}

static gboolean closeWebViewCb(WebKitWebView* webView, GtkWidget* window)
{
    gtk_widget_destroy(window);
    return TRUE;
}

static gboolean webViewFullscreenMessageWindowClose(GtkWidget *dialog)
{
    if (GTK_IS_WIDGET(dialog))
        gtk_widget_destroy(dialog);
    return FALSE;
}

static gboolean webViewWindowStateEvent(GtkWidget *widget, GdkEventWindowState *event, WebKitWebView *webView)
{
    if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
        WebKitWebFrame *frame = webkit_web_view_get_main_frame(webView);
        const gchar *uri = webkit_web_frame_get_uri(frame);
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(webView));
        if (!gtk_widget_is_toplevel(window) || !GTK_IS_WINDOW(window) || GTK_IS_OFFSCREEN_WINDOW(window))
            window = 0;

        GtkWidget *dialog = gtk_message_dialog_new(window ? GTK_WINDOW(window) : 0,
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_INFO,
                                                    GTK_BUTTONS_CLOSE,
                                                    "%s is now full screen. Press ESC or f to exit.", uri);
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
        g_timeout_add(1500, (GSourceFunc) webViewFullscreenMessageWindowClose, dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
    }
    return TRUE;
}

static void hideWidget(GtkWidget* widget, gpointer data)
{
    if (!GTK_IS_SCROLLED_WINDOW(widget))
        gtk_widget_hide(widget);
}

static void showWidget(GtkWidget* widget, gpointer data)
{
    if (!GTK_IS_SCROLLED_WINDOW(widget))
        gtk_widget_show(widget);
}

static gboolean webViewEnteringFullScreen(WebKitWebView *webView, GObject *element, GtkWidget* vbox)
{
    WebKitWebFrame *frame = webkit_web_view_get_main_frame(webView);
    const gchar *uri = webkit_web_frame_get_uri(frame);
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(webView));
    if (!gtk_widget_is_toplevel(window) || !GTK_IS_WINDOW(window) || GTK_IS_OFFSCREEN_WINDOW(window))
        window = 0;

    GtkWidget *dialog = gtk_message_dialog_new(window ? GTK_WINDOW(window) : 0,
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_YES_NO,
                                               "Allow full screen display of %s ?", uri);
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_YES) {
        gtk_container_foreach(GTK_CONTAINER(vbox), (GtkCallback) hideWidget, NULL);
        gtk_widget_destroy(GTK_WIDGET(dialog));
        return FALSE;
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    return TRUE;
}

static gboolean webViewLeavingFullScreen(WebKitWebView *webView, GObject *element, GtkWidget* vbox)
{
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(webView));
    if (gtk_widget_is_toplevel(window) && GTK_IS_WINDOW(window) && !GTK_IS_OFFSCREEN_WINDOW(window))
        g_signal_handlers_disconnect_by_func(window, G_CALLBACK(webViewWindowStateEvent), webView);
    gtk_container_foreach(GTK_CONTAINER(vbox), (GtkCallback) showWidget, NULL);
    return FALSE;
}

static void iconLoadedCb(WebKitWebView* webView, const char* iconURI, GtkWidget* uriEntry)
{
    GdkPixbuf *icon = webkit_web_view_try_get_favicon_pixbuf(webView, 16, 16);
    if (!icon)
        return;

    gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(uriEntry), GTK_ENTRY_ICON_PRIMARY, icon);
    g_object_unref(icon);
}

static GtkWidget *inspectorInspectWebViewCb(WebKitWebInspector *inspector, WebKitWebView *webView, GtkWindow* window)
{
    GtkWidget *inspectorWindow = launcherInspectorWindowNew(inspector, window);
    return GTK_WIDGET(launcherInspectorWindowGetWebView(LAUNCHER_INSPECTOR_WINDOW(inspectorWindow)));
}

static GtkWidget* createBrowser(GtkWidget* window, GtkWidget* uriEntry, GtkWidget* statusbar, WebKitWebView* webView, GtkWidget* vbox)
{
    char *iconDatabasePath;
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(scrolledWindow), GTK_WIDGET(webView));

    iconDatabasePath = g_build_filename(g_get_user_data_dir(), "webkit", "icondatabase", NULL);
    webkit_favicon_database_set_path(webkit_get_favicon_database(), iconDatabasePath);
    g_free(iconDatabasePath);

    g_signal_connect(webView, "notify::title", G_CALLBACK(notifyTitleCb), window);
    g_signal_connect(webView, "notify::load-status", G_CALLBACK(notifyLoadStatusCb), uriEntry);
    g_signal_connect(webView, "notify::progress", G_CALLBACK(notifyProgressCb), window);
    g_signal_connect(webView, "icon-loaded", G_CALLBACK(iconLoadedCb), uriEntry);
    g_signal_connect(webView, "hovering-over-link", G_CALLBACK(linkHoverCb), statusbar);
    g_signal_connect(webView, "create-web-view", G_CALLBACK(createWebViewCb), window);
    g_signal_connect(webView, "web-view-ready", G_CALLBACK(webViewReadyCb), window);
    g_signal_connect(webView, "close-web-view", G_CALLBACK(closeWebViewCb), window);
    g_signal_connect(webView, "entering-fullscreen", G_CALLBACK(webViewEnteringFullScreen), vbox);
    g_signal_connect(webView, "leaving-fullscreen", G_CALLBACK(webViewLeavingFullScreen), vbox);
    g_signal_connect(webkit_web_view_get_inspector(webView), "inspect-web-view", G_CALLBACK(inspectorInspectWebViewCb), window);

    return scrolledWindow;
}

static GtkWidget* createStatusbar()
{
    GtkStatusbar *statusbar = GTK_STATUSBAR(gtk_statusbar_new());
    guint statusContextId = gtk_statusbar_get_context_id(statusbar, "Link Hover");
    g_object_set_data(G_OBJECT(statusbar), "link-hover-context",
        GUINT_TO_POINTER(statusContextId));

    return GTK_WIDGET(statusbar);
}

static GtkWidget* createToolbar(GtkWidget* window, GtkWidget* uriEntry, WebKitWebView* webView)
{
    GtkWidget *toolbar = gtk_toolbar_new();

    gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);

    GtkToolItem *item;

    /* Keyboard accelerators */
    GtkAccelGroup *accelGroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accelGroup);

    /* the back button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(goBackCb), webView);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    /* The forward button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(goForwardCb), webView);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    /* The reload button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(reloadCb), webView);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(item), "clicked", accelGroup, GDK_KEY_F5, 0, GTK_ACCEL_VISIBLE);

    /* The URL entry */
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, TRUE);
    gtk_container_add(GTK_CONTAINER(item), uriEntry);
    g_signal_connect(G_OBJECT(uriEntry), "activate", G_CALLBACK(activateUriEntryCb), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    /* The go button */
    g_object_set_data(G_OBJECT(uriEntry), "web-view", webView);
    item = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect_swapped(G_OBJECT(item), "clicked", G_CALLBACK(activateUriEntryCb), (gpointer)uriEntry);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    return toolbar;
}

static GtkWidget* createWindow(WebKitWebView** outWebView)
{
    WebKitWebView *webView;
    GtkWidget *vbox;
    GtkWidget *window;
    GtkWidget *uriEntry;
    GtkWidget *statusbar;

    g_atomic_int_inc(&windowCount);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_widget_set_name(window, "GtkLauncher");

    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    uriEntry = gtk_entry_new();

#ifdef GTK_API_VERSION_2
    vbox = gtk_vbox_new(FALSE, 0);
#else
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
    statusbar = createStatusbar(webView);
    gtk_box_pack_start(GTK_BOX(vbox), createToolbar(window, uriEntry, webView), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), createBrowser(window, uriEntry, statusbar, webView, vbox), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    g_signal_connect(window, "destroy", G_CALLBACK(destroyCb), NULL);

    if (outWebView)
        *outWebView = webView;

    return window;
}

static gchar* filenameToURL(const char* filename)
{
    if (!g_file_test(filename, G_FILE_TEST_EXISTS))
        return NULL;

    GFile *gfile = g_file_new_for_path(filename);
    gchar *fileURL = g_file_get_uri(gfile);
    g_object_unref(gfile);

    return fileURL;
}

static gboolean parseOptionEntryCallback(const gchar *optionNameFull, const gchar *value, WebKitWebSettings *webSettings, GError **error)
{
    if (strlen(optionNameFull) <= 2) {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED, "Invalid option %s", optionNameFull);
        return FALSE;
    }

    /* We have two -- in option name so remove them. */
    const gchar *optionName = optionNameFull + 2;
    GParamSpec *spec = g_object_class_find_property(G_OBJECT_GET_CLASS(webSettings), optionName);
    if (!spec) {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED, "Cannot find web settings for option %s", optionNameFull);
        return FALSE;
    }

    switch (G_PARAM_SPEC_VALUE_TYPE(spec)) {
    case G_TYPE_BOOLEAN: {
        gboolean propertyValue = TRUE;
        if (value && g_ascii_strcasecmp(value, "true") && strcmp(value, "1"))
            propertyValue = FALSE;
        g_object_set(G_OBJECT(webSettings), optionName, propertyValue, NULL);
        break;
    }
    case G_TYPE_STRING:
        g_object_set(G_OBJECT(webSettings), optionName, value, NULL);
        break;
    case G_TYPE_INT: {
        glong propertyValue;
        gchar *end;

        errno = 0;
        propertyValue = g_ascii_strtoll(value, &end, 0);
        if (errno == ERANGE || propertyValue > G_MAXINT || propertyValue < G_MININT) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Integer value '%s' for %s out of range", value, optionNameFull);
            return FALSE;
        }
        if (errno || value == end) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Cannot parse integer value '%s' for %s", value, optionNameFull);
            return FALSE;
        }
        g_object_set(G_OBJECT(webSettings), optionName, propertyValue, NULL);
        break;
    }
    case G_TYPE_FLOAT: {
        gdouble propertyValue;
        gchar *end;

        errno = 0;
        propertyValue = g_ascii_strtod(value, &end);
        if (errno == ERANGE || propertyValue > G_MAXFLOAT || propertyValue < G_MINFLOAT) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Float value '%s' for %s out of range", value, optionNameFull);
            return FALSE;
        }
        if (errno || value == end) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Cannot parse float value '%s' for %s", value, optionNameFull);
            return FALSE;
        }
        g_object_set(G_OBJECT(webSettings), optionName, propertyValue, NULL);
        break;
    }
    default:
        g_assert_not_reached();
    }

    return TRUE;
}

static gboolean isValidParameterType(GType gParamType)
{
    return (gParamType == G_TYPE_BOOLEAN || gParamType == G_TYPE_STRING || gParamType == G_TYPE_INT
            || gParamType == G_TYPE_FLOAT);
}

static GOptionEntry* getOptionEntriesFromWebKitWebSettings(WebKitWebSettings *webSettings)
{
    GParamSpec **propertySpecs;
    GOptionEntry *optionEntries;
    guint numProperties, numEntries, i;

    propertySpecs = g_object_class_list_properties(G_OBJECT_GET_CLASS(webSettings), &numProperties);
    if (!propertySpecs)
        return NULL;

    optionEntries = g_new0(GOptionEntry, numProperties + 1);
    numEntries = 0;
    for (i = 0; i < numProperties; i++) {
        GParamSpec *param = propertySpecs[i];

        /* Fill in structures only for writable and not construct-only properties. */
        if (!param || !(param->flags & G_PARAM_WRITABLE) || (param->flags & G_PARAM_CONSTRUCT_ONLY))
            continue;

        GType gParamType = G_PARAM_SPEC_VALUE_TYPE(param);
        if (!isValidParameterType(gParamType))
            continue;

        GOptionEntry *optionEntry = &optionEntries[numEntries++];
        optionEntry->long_name = g_param_spec_get_name(param);

        /* There is no easy way to figure our short name for generated option entries.
           optionEntry.short_name=*/
        /* For bool arguments "enable" type make option argument not required. */
        if (gParamType == G_TYPE_BOOLEAN && (strstr(optionEntry->long_name, "enable")))
            optionEntry->flags = G_OPTION_FLAG_OPTIONAL_ARG;
        optionEntry->arg = G_OPTION_ARG_CALLBACK;
        optionEntry->arg_data = parseOptionEntryCallback;
        optionEntry->description = g_param_spec_get_blurb(param);
        optionEntry->arg_description = g_type_name(gParamType);
    }
    g_free(propertySpecs);

    return optionEntries;
}

static gboolean addWebSettingsGroupToContext(GOptionContext *context, WebKitWebSettings* webkitSettings)
{
    GOptionEntry *optionEntries = getOptionEntriesFromWebKitWebSettings(webkitSettings);
    if (!optionEntries)
        return FALSE;

    GOptionGroup *webSettingsGroup = g_option_group_new("websettings",
                                                        "WebKitWebSettings writable properties for default WebKitWebView",
                                                        "WebKitWebSettings properties",
                                                        webkitSettings,
                                                        NULL);
    g_option_group_add_entries(webSettingsGroup, optionEntries);
    g_free(optionEntries);

    /* Option context takes ownership of the group. */
    g_option_context_add_group(context, webSettingsGroup);

    return TRUE;
}

int main(int argc, char* argv[])
{
    WebKitWebSettings *webkitSettings = 0;
    const gchar **uriArguments = 0;
    const GOptionEntry commandLineOptions[] =
    {
        { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &uriArguments, 0, "[URL]" },
        { 0, 0, 0, 0, 0, 0, 0 }
    };

    gtk_init(&argc, &argv);

    GOptionContext *context = g_option_context_new(0);
    g_option_context_add_main_entries(context, commandLineOptions, 0);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
#ifdef WTF_USE_GSTREAMER
    g_option_context_add_group(context, gst_init_get_option_group());
#endif
    webkitSettings = webkit_web_settings_new();
    g_object_set(webkitSettings, "enable-developer-extras", TRUE, NULL);
    if (!addWebSettingsGroupToContext(context, webkitSettings)) {
        g_object_unref(webkitSettings);
        webkitSettings = 0;
    }

    GError *error = 0;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("Cannot parse arguments: %s\n", error->message);
        g_error_free(error);
        g_option_context_free(context);

        return 1;
    }
    g_option_context_free(context);

#ifdef SOUP_TYPE_PROXY_RESOLVER_DEFAULT
    soup_session_add_feature_by_type(webkit_get_default_session(), SOUP_TYPE_PROXY_RESOLVER_DEFAULT);
#else
    const char *httpProxy = g_getenv("http_proxy");
    if (httpProxy) {
        SoupURI *proxyUri = soup_uri_new(httpProxy);
        g_object_set(webkit_get_default_session(), SOUP_SESSION_PROXY_URI, proxyUri, NULL);
        soup_uri_free(proxyUri);
    }
#endif

#ifdef WEBKIT_EXEC_PATH
    g_setenv("WEBKIT_INSPECTOR_PATH", WEBKIT_EXEC_PATH "resources/inspector", FALSE);
#endif /* WEBKIT_EXEC_PATH */

    WebKitWebView *webView;
    GtkWidget *main_window = createWindow(&webView);

    if (webkitSettings) {
        webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView), webkitSettings);
        g_object_unref(webkitSettings);
    }

    const gchar *uri = (uriArguments ? uriArguments[0] : "http://www.google.com/");
    gchar *fileURL = filenameToURL(uri);

    webkit_web_view_load_uri(webView, fileURL ? fileURL : uri);
    g_free(fileURL);

    gtk_widget_grab_focus(GTK_WIDGET(webView));
    gtk_widget_show_all(main_window);
    gtk_main();

    return 0;
}
