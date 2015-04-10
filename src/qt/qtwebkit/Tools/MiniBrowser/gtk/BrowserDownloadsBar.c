/*
 * Copyright (C) 2011 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "BrowserDownloadsBar.h"

#include <glib/gi18n.h>

#define BROWSER_TYPE_DOWNLOAD (browser_download_get_type())
#define BROWSER_DOWNLOAD(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), BROWSER_TYPE_DOWNLOAD, BrowserDownload))

typedef struct _BrowserDownload BrowserDownload;
typedef struct _BrowserDownloadClass BrowserDownloadClass;

static GType browser_download_get_type();

struct _BrowserDownloadsBar {
    GtkInfoBar parent;
};

struct _BrowserDownloadsBarClass {
    GtkInfoBarClass parentClass;
};

G_DEFINE_TYPE(BrowserDownloadsBar, browser_downloads_bar, GTK_TYPE_INFO_BAR)

static void
browserDownloadsBarChildRemoved(GtkContainer *infoBar,  GtkWidget *widget, BrowserDownloadsBar *downloadsBar)
{
    GList *children = gtk_container_get_children(infoBar);
    if (g_list_length(children) == 1)
        gtk_info_bar_response(GTK_INFO_BAR(downloadsBar), GTK_RESPONSE_CLOSE);
    g_list_free(children);
}

static void browserDownloadsBarResponse(GtkInfoBar *infoBar, gint responseId)
{
    gtk_widget_destroy(GTK_WIDGET(infoBar));
}

static void browser_downloads_bar_init(BrowserDownloadsBar *downloadsBar)
{
    GtkWidget *contentBox = gtk_info_bar_get_content_area(GTK_INFO_BAR(downloadsBar));
    g_signal_connect_after(contentBox, "remove", G_CALLBACK(browserDownloadsBarChildRemoved), downloadsBar);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(contentBox), GTK_ORIENTATION_VERTICAL);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold'>Downloads</span>");
    gtk_misc_set_alignment(GTK_MISC(title), 0., 0.5);
    gtk_box_pack_start(GTK_BOX(contentBox), title, FALSE, FALSE, 12);
    gtk_widget_show(title);
}

static void browser_downloads_bar_class_init(BrowserDownloadsBarClass *klass)
{
    GtkInfoBarClass *infoBarClass = GTK_INFO_BAR_CLASS(klass);
    infoBarClass->response = browserDownloadsBarResponse;
}

GtkWidget *browser_downloads_bar_new()
{
    GtkInfoBar *downloadsBar = GTK_INFO_BAR(g_object_new(BROWSER_TYPE_DOWNLOADS_BAR, NULL));
    gtk_info_bar_add_buttons(downloadsBar, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
    return GTK_WIDGET(downloadsBar);
}

struct _BrowserDownload {
    GtkBox parent;

    WebKitDownload *download;
    guint64 contentLength;
    guint64 downloadedSize;
    gboolean finished;

    GtkWidget *statusLabel;
    GtkWidget *remainingLabel;
    GtkWidget *progressBar;
    GtkWidget *actionButton;
};

struct _BrowserDownloadClass {
    GtkBoxClass parentClass;
};

G_DEFINE_TYPE(BrowserDownload, browser_download, GTK_TYPE_BOX)

static void actionButtonClicked(GtkButton *button, BrowserDownload *browserDownload)
{
    if (!browserDownload->finished) {
        webkit_download_cancel(browserDownload->download);
        return;
    }

    gtk_show_uri(gtk_widget_get_screen(GTK_WIDGET(browserDownload)),
                 webkit_download_get_destination(browserDownload->download),
                 gtk_get_current_event_time(), NULL);
    gtk_widget_destroy(GTK_WIDGET(browserDownload));
}

static void browserDownloadFinalize(GObject *object)
{
    BrowserDownload *browserDownload = BROWSER_DOWNLOAD(object);

    if (browserDownload->download) {
        g_object_unref(browserDownload->download);
        browserDownload->download = NULL;
    }

    G_OBJECT_CLASS(browser_download_parent_class)->finalize(object);
}

static void browser_download_init(BrowserDownload *download)
{
    GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(download), mainBox, FALSE, FALSE, 0);
    gtk_widget_show(mainBox);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), vbox, TRUE, TRUE, 0);
    gtk_widget_show(vbox);

    GtkWidget *statusBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), statusBox, TRUE, TRUE, 0);
    gtk_widget_show(statusBox);

    download->statusLabel = gtk_label_new("Starting Download");
    gtk_label_set_ellipsize(GTK_LABEL(download->statusLabel), PANGO_ELLIPSIZE_END);
    gtk_misc_set_alignment(GTK_MISC(download->statusLabel), 0., 0.5);
    gtk_box_pack_start(GTK_BOX(statusBox), download->statusLabel, TRUE, TRUE, 0);
    gtk_widget_show(download->statusLabel);

    download->remainingLabel = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(download->remainingLabel), 1., 0.5);
    gtk_box_pack_end(GTK_BOX(statusBox), download->remainingLabel, TRUE, TRUE, 0);
    gtk_widget_show(download->remainingLabel);

    download->progressBar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), download->progressBar, FALSE, FALSE, 0);
    gtk_widget_show(download->progressBar);

    download->actionButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(download->actionButton, "clicked", G_CALLBACK(actionButtonClicked), download);
    gtk_box_pack_end(GTK_BOX(mainBox), download->actionButton, FALSE, FALSE, 0);
    gtk_widget_show(download->actionButton);
}

static void browser_download_class_init(BrowserDownloadClass *klass)
{
    GObjectClass *objectClass = G_OBJECT_CLASS(klass);

    objectClass->finalize = browserDownloadFinalize;
}

static void downloadReceivedResponse(WebKitDownload *download, GParamSpec *paramSpec, BrowserDownload *browserDownload)
{
    WebKitURIResponse *response = webkit_download_get_response(download);
    browserDownload->contentLength = webkit_uri_response_get_content_length(response);
    char *text = g_strdup_printf("Downloading %s", webkit_uri_response_get_uri(response));
    gtk_label_set_text(GTK_LABEL(browserDownload->statusLabel), text);
    g_free(text);
}

static gchar *remainingTime(BrowserDownload *browserDownload)
{
    guint64 total = browserDownload->contentLength;
    guint64 current = browserDownload->downloadedSize;
    gdouble elapsedTime = webkit_download_get_elapsed_time(browserDownload->download);

    if (current <= 0)
        return NULL;

    gdouble perByteTime = elapsedTime / current;
    gdouble interval = perByteTime * (total - current);

    int hours = (int) (interval / 3600);
    interval -= hours * 3600;
    int mins = (int) (interval / 60);
    interval -= mins * 60;
    int secs = (int) interval;

    if (hours > 0) {
        if (mins > 0)
            return g_strdup_printf (ngettext ("%u:%02u hour left", "%u:%02u hours left", hours), hours, mins);
        return g_strdup_printf (ngettext ("%u hour left", "%u hours left", hours), hours);
    }

    if (mins > 0)
        return g_strdup_printf (ngettext ("%u:%02u minute left", "%u:%02u minutes left", mins), mins, secs);
    return g_strdup_printf (ngettext ("%u second left", "%u seconds left", secs), secs);
}

static void downloadProgress(WebKitDownload *download, GParamSpec *paramSpec, BrowserDownload *browserDownload)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(browserDownload->progressBar),
                                  webkit_download_get_estimated_progress(download));
    char *remaining = remainingTime(browserDownload);
    gtk_label_set_text(GTK_LABEL(browserDownload->remainingLabel), remaining);
    g_free(remaining);
}

static void downloadReceivedData(WebKitDownload *download, guint64 dataLength, BrowserDownload *browserDownload)
{
    browserDownload->downloadedSize += dataLength;
}

static void downloadFinished(WebKitDownload *download, BrowserDownload *browserDownload)
{
    gchar *text = g_strdup_printf("Download completed: %s", webkit_download_get_destination(download));
    gtk_label_set_text(GTK_LABEL(browserDownload->statusLabel), text);
    g_free(text);
    gtk_label_set_text(GTK_LABEL(browserDownload->remainingLabel), NULL);
    gtk_button_set_image(GTK_BUTTON(browserDownload->actionButton),
                         gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON));
    gtk_button_set_label(GTK_BUTTON(browserDownload->actionButton), "Open ...");
    browserDownload->finished = TRUE;
}

static void downloadFailed(WebKitDownload *download, GError *error, BrowserDownload *browserDownload)
{
    g_signal_handlers_disconnect_by_func(browserDownload->download, downloadFinished, browserDownload);
    if (g_error_matches(error, WEBKIT_DOWNLOAD_ERROR, WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER)) {
        gtk_widget_destroy(GTK_WIDGET(browserDownload));
        return;
    }

    char *errorMessage = g_strdup_printf("Download failed: %s", error->message);
    gtk_label_set_text(GTK_LABEL(browserDownload->statusLabel), errorMessage);
    g_free(errorMessage);
    gtk_label_set_text(GTK_LABEL(browserDownload->remainingLabel), NULL);
    gtk_widget_set_sensitive(browserDownload->actionButton, FALSE);
}

GtkWidget *browserDownloadNew(WebKitDownload *download)
{
    BrowserDownload *browserDownload = BROWSER_DOWNLOAD(g_object_new(BROWSER_TYPE_DOWNLOAD,
                                                                     "orientation", GTK_ORIENTATION_VERTICAL,
                                                                     NULL));

    browserDownload->download = g_object_ref(download);
    g_signal_connect(browserDownload->download, "notify::response", G_CALLBACK(downloadReceivedResponse), browserDownload);
    g_signal_connect(browserDownload->download, "notify::estimated-progress", G_CALLBACK(downloadProgress), browserDownload);
    g_signal_connect(browserDownload->download, "received-data", G_CALLBACK(downloadReceivedData), browserDownload);
    g_signal_connect(browserDownload->download, "finished", G_CALLBACK(downloadFinished), browserDownload);
    g_signal_connect(browserDownload->download, "failed", G_CALLBACK(downloadFailed), browserDownload);

    return GTK_WIDGET(browserDownload);
}

void browser_downloads_bar_add_download(BrowserDownloadsBar *downloadsBar, WebKitDownload *download)
{
    GtkWidget *browserDownload = browserDownloadNew(download);
    GtkWidget *contentBox = gtk_info_bar_get_content_area(GTK_INFO_BAR(downloadsBar));
    gtk_box_pack_start(GTK_BOX(contentBox), browserDownload, FALSE, TRUE, 0);
    gtk_widget_show(browserDownload);
}
