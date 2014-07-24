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
#include "WebKitDownload.h"

#include "DownloadProxy.h"
#include "WebKitDownloadPrivate.h"
#include "WebKitMarshal.h"
#include "WebKitURIRequestPrivate.h"
#include "WebKitURIResponsePrivate.h"
#include <WebCore/ErrorsGtk.h>
#include <WebCore/ResourceResponse.h>
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>

using namespace WebKit;
using namespace WebCore;

/**
 * SECTION: WebKitDownload
 * @Short_description: Object used to communicate with the application when downloading
 * @Title: WebKitDownload
 *
 * #WebKitDownload carries information about a download request and
 * response, including a #WebKitURIRequest and a #WebKitURIResponse
 * objects. The application may use this object to control the
 * download process, or to simply figure out what is to be downloaded,
 * and handle the download process itself.
 *
 */

enum {
    RECEIVED_DATA,
    FINISHED,
    FAILED,
    DECIDE_DESTINATION,
    CREATED_DESTINATION,

    LAST_SIGNAL
};

enum {
    PROP_0,

    PROP_DESTINATION,
    PROP_RESPONSE,
    PROP_ESTIMATED_PROGRESS
};

struct _WebKitDownloadPrivate {
    ~_WebKitDownloadPrivate()
    {
        if (webView)
            g_object_remove_weak_pointer(G_OBJECT(webView), reinterpret_cast<void**>(&webView));
    }

    RefPtr<DownloadProxy> download;

    GRefPtr<WebKitURIRequest> request;
    GRefPtr<WebKitURIResponse> response;
    WebKitWebView* webView;
    CString destinationURI;
    guint64 currentSize;
    bool isCancelled;
    GOwnPtr<GTimer> timer;
    gdouble lastProgress;
    gdouble lastElapsed;
};

static guint signals[LAST_SIGNAL] = { 0, };

WEBKIT_DEFINE_TYPE(WebKitDownload, webkit_download, G_TYPE_OBJECT)

static void webkitDownloadGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitDownload* download = WEBKIT_DOWNLOAD(object);

    switch (propId) {
    case PROP_DESTINATION:
        g_value_set_string(value, webkit_download_get_destination(download));
        break;
    case PROP_RESPONSE:
        g_value_set_object(value, webkit_download_get_response(download));
        break;
    case PROP_ESTIMATED_PROGRESS:
        g_value_set_double(value, webkit_download_get_estimated_progress(download));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static gboolean webkitDownloadDecideDestination(WebKitDownload* download, const gchar* suggestedFilename)
{
    if (!download->priv->destinationURI.isNull())
        return FALSE;

    GOwnPtr<char> filename(g_strdelimit(g_strdup(suggestedFilename), G_DIR_SEPARATOR_S, '_'));
    const gchar *downloadsDir = g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD);
    if (!downloadsDir) {
        // If we don't have XDG user dirs info, set just to HOME.
        downloadsDir = g_get_home_dir();
    }
    GOwnPtr<char> destination(g_build_filename(downloadsDir, filename.get(), NULL));
    GOwnPtr<char> destinationURI(g_filename_to_uri(destination.get(), 0, 0));
    download->priv->destinationURI = destinationURI.get();
    g_object_notify(G_OBJECT(download), "destination");
    return TRUE;
}

static void webkit_download_class_init(WebKitDownloadClass* downloadClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(downloadClass);
    objectClass->get_property = webkitDownloadGetProperty;

    downloadClass->decide_destination = webkitDownloadDecideDestination;

    /**
     * WebKitDownload:destination:
     *
     * The local URI to where the download will be saved.
     */
    g_object_class_install_property(objectClass,
                                    PROP_DESTINATION,
                                    g_param_spec_string("destination",
                                                        _("Destination"),
                                                        _("The local URI to where the download will be saved"),
                                                        0,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitDownload:response:
     *
     * The #WebKitURIResponse associated with this download.
     */
    g_object_class_install_property(objectClass,
                                    PROP_RESPONSE,
                                    g_param_spec_object("response",
                                                        _("Response"),
                                                        _("The response of the download"),
                                                        WEBKIT_TYPE_URI_RESPONSE,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitDownload:estimated-progress:
     *
     * An estimate of the percent completion for the download operation.
     * This value will range from 0.0 to 1.0. The value is an estimate
     * based on the total number of bytes expected to be received for
     * a download.
     * If you need a more accurate progress information you can connect to
     * #WebKitDownload::received-data signal to track the progress.
     */
    g_object_class_install_property(objectClass,
                                    PROP_ESTIMATED_PROGRESS,
                                    g_param_spec_double("estimated-progress",
                                                        _("Estimated Progress"),
                                                        _("Determines the current progress of the download"),
                                                        0.0, 1.0, 1.0,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitDownload::received-data:
     * @download: the #WebKitDownload
     * @data_length: the length of data received in bytes
     *
     * This signal is emitted after response is received,
     * every time new data has been written to the destination. It's
     * useful to know the progress of the download operation.
     */
    signals[RECEIVED_DATA] =
        g_signal_new("received-data",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     webkit_marshal_VOID__UINT64,
                     G_TYPE_NONE, 1,
                     G_TYPE_UINT64);

    /**
     * WebKitDownload::finished:
     * @download: the #WebKitDownload
     *
     * This signal is emitted when download finishes successfully or due to an error.
     * In case of errors #WebKitDownload::failed signal is emitted before this one.
     */
    signals[FINISHED] =
        g_signal_new("finished",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    /**
     * WebKitDownload::failed:
     * @download: the #WebKitDownload
     * @error: the #GError that was triggered
     *
     * This signal is emitted when an error occurs during the download
     * operation. The given @error, of the domain %WEBKIT_DOWNLOAD_ERROR,
     * contains further details of the failure. If the download is cancelled
     * with webkit_download_cancel(), this signal is emitted with error
     * %WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER. The download operation finishes
     * after an error and #WebKitDownload::finished signal is emitted after this one.
     */
    signals[FAILED] =
        g_signal_new("failed",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     g_cclosure_marshal_VOID__POINTER,
                     G_TYPE_NONE, 1,
                     G_TYPE_POINTER);

    /**
     * WebKitDownload::decide-destination:
     * @download: the #WebKitDownload
     * @suggested_filename: the filename suggested for the download
     *
     * This signal is emitted after response is received to
     * decide a destination URI for the download. If this signal is not
     * handled the file will be downloaded to %G_USER_DIRECTORY_DOWNLOAD
     * directory using @suggested_filename.
     *
     * Returns: %TRUE to stop other handlers from being invoked for the event.
     *   %FALSE to propagate the event further.
     */
    signals[DECIDE_DESTINATION] =
        g_signal_new("decide-destination",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(WebKitDownloadClass, decide_destination),
                     g_signal_accumulator_true_handled, NULL,
                     webkit_marshal_BOOLEAN__STRING,
                     G_TYPE_BOOLEAN, 1,
                     G_TYPE_STRING);

    /**
     * WebKitDownload::created-destination:
     * @download: the #WebKitDownload
     * @destination: the destination URI
     *
     * This signal is emitted after #WebKitDownload::decide-destination and before
     * #WebKitDownload::received-data to notify that destination file has been
     * created successfully at @destination.
     */
    signals[CREATED_DESTINATION] =
        g_signal_new("created-destination",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_BOOLEAN, 1,
                     G_TYPE_STRING);
}

WebKitDownload* webkitDownloadCreate(DownloadProxy* downloadProxy)
{
    ASSERT(downloadProxy);
    WebKitDownload* download = WEBKIT_DOWNLOAD(g_object_new(WEBKIT_TYPE_DOWNLOAD, NULL));
    download->priv->download = downloadProxy;
    return download;
}

WebKitDownload* webkitDownloadCreateForRequest(DownloadProxy* downloadProxy, const ResourceRequest& request)
{
    WebKitDownload* download = webkitDownloadCreate(downloadProxy);
    download->priv->request = adoptGRef(webkitURIRequestCreateForResourceRequest(request));
    return download;
}

void webkitDownloadSetResponse(WebKitDownload* download, WebKitURIResponse* response)
{
    download->priv->response = response;
    g_object_notify(G_OBJECT(download), "response");
}

void webkitDownloadSetWebView(WebKitDownload* download, WebKitWebView* webView)
{
    download->priv->webView = webView;
    g_object_add_weak_pointer(G_OBJECT(webView), reinterpret_cast<void**>(&download->priv->webView));
}

bool webkitDownloadIsCancelled(WebKitDownload* download)
{
    return download->priv->isCancelled;
}

void webkitDownloadNotifyProgress(WebKitDownload* download, guint64 bytesReceived)
{
    WebKitDownloadPrivate* priv = download->priv;
    if (priv->isCancelled)
        return;

    if (!download->priv->timer)
        download->priv->timer.set(g_timer_new());

    priv->currentSize += bytesReceived;
    g_signal_emit(download, signals[RECEIVED_DATA], 0, bytesReceived);

    // Throttle progress notification to not consume high amounts of
    // CPU on fast links, except when the last notification occured
    // more than 0.016 secs ago (60 FPS), or the last notified progress
    // is passed in 1% or we reached the end.
    gdouble currentElapsed = g_timer_elapsed(priv->timer.get(), 0);
    gdouble currentProgress = webkit_download_get_estimated_progress(download);

    if (priv->lastElapsed
        && priv->lastProgress
        && (currentElapsed - priv->lastElapsed) < 0.016
        && (currentProgress - priv->lastProgress) < 0.01
        && currentProgress < 1.0) {
        return;
    }
    priv->lastElapsed = currentElapsed;
    priv->lastProgress = currentProgress;
    g_object_notify(G_OBJECT(download), "estimated-progress");
}

void webkitDownloadFailed(WebKitDownload* download, const ResourceError& resourceError)
{
    GOwnPtr<GError> webError(g_error_new_literal(g_quark_from_string(resourceError.domain().utf8().data()),
                                                 resourceError.errorCode(),
                                                 resourceError.localizedDescription().utf8().data()));
    if (download->priv->timer)
        g_timer_stop(download->priv->timer.get());

    g_signal_emit(download, signals[FAILED], 0, webError.get());
    g_signal_emit(download, signals[FINISHED], 0, NULL);
}

void webkitDownloadCancelled(WebKitDownload* download)
{
    WebKitDownloadPrivate* priv = download->priv;
    webkitDownloadFailed(download, downloadCancelledByUserError(priv->response ? webkitURIResponseGetResourceResponse(priv->response.get()) : ResourceResponse()));
}

void webkitDownloadFinished(WebKitDownload* download)
{
    if (download->priv->isCancelled) {
        // Since cancellation is asynchronous, didFinish might be called even
        // if the download was cancelled. User cancelled the download,
        // so we should fail with cancelled error even if the download
        // actually finished successfully.
        webkitDownloadCancelled(download);
        return;
    }
    if (download->priv->timer)
        g_timer_stop(download->priv->timer.get());
    g_signal_emit(download, signals[FINISHED], 0, NULL);
}

CString webkitDownloadDecideDestinationWithSuggestedFilename(WebKitDownload* download, const CString& suggestedFilename)
{
    if (download->priv->isCancelled)
        return "";
    gboolean returnValue;
    g_signal_emit(download, signals[DECIDE_DESTINATION], 0, suggestedFilename.data(), &returnValue);
    return download->priv->destinationURI;
}

void webkitDownloadDestinationCreated(WebKitDownload* download, const CString& destinationURI)
{
    if (download->priv->isCancelled)
        return;
    gboolean returnValue;
    g_signal_emit(download, signals[CREATED_DESTINATION], 0, destinationURI.data(), &returnValue);
}

/**
 * webkit_download_get_request:
 * @download: a #WebKitDownload
 *
 * Retrieves the #WebKitURIRequest object that backs the download
 * process.
 *
 * Returns: (transfer none): the #WebKitURIRequest of @download
 */
WebKitURIRequest* webkit_download_get_request(WebKitDownload* download)
{
    g_return_val_if_fail(WEBKIT_IS_DOWNLOAD(download), 0);

    WebKitDownloadPrivate* priv = download->priv;
    if (!priv->request)
        priv->request = adoptGRef(webkitURIRequestCreateForResourceRequest(priv->download->request()));
    return priv->request.get();
}

/**
 * webkit_download_get_destination:
 * @download: a #WebKitDownload
 *
 * Obtains the URI to which the downloaded file will be written. You
 * can connect to #WebKitDownload::created-destination to make
 * sure this method returns a valid destination.
 *
 * Returns: the destination URI or %NULL
 */
const gchar* webkit_download_get_destination(WebKitDownload* download)
{
    g_return_val_if_fail(WEBKIT_IS_DOWNLOAD(download), 0);

    return download->priv->destinationURI.data();
}

/**
 * webkit_download_set_destination:
 * @download: a #WebKitDownload
 * @uri: the destination URI
 *
 * Sets the URI to which the downloaded file will be written.
 * This method should be called before the download transfer
 * starts or it will not have any effect on the ongoing download
 * operation. To set the destination using the filename suggested
 * by the server connect to #WebKitDownload::decide-destination
 * signal and call webkit_download_set_destination(). If you want to
 * set a fixed destination URI that doesn't depend on the suggested
 * filename you can connect to notify::response signal and call
 * webkit_download_set_destination().
 * If #WebKitDownload::decide-destination signal is not handled
 * and destination URI is not set when the download tranfer starts,
 * the file will be saved with the filename suggested by the server in
 * %G_USER_DIRECTORY_DOWNLOAD directory.
 */
void webkit_download_set_destination(WebKitDownload* download, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_DOWNLOAD(download));
    g_return_if_fail(uri);

    WebKitDownloadPrivate* priv = download->priv;
    if (priv->destinationURI == uri)
        return;

    priv->destinationURI = uri;
    g_object_notify(G_OBJECT(download), "destination");
}

/**
 * webkit_download_get_response:
 * @download: a #WebKitDownload
 *
 * Retrieves the #WebKitURIResponse object that backs the download
 * process. This method returns %NULL if called before the response
 * is received from the server. You can connect to notify::response
 * signal to be notified when the response is received.
 *
 * Returns: (transfer none): the #WebKitURIResponse, or %NULL if
 *     the response hasn't been received yet.
 */
WebKitURIResponse* webkit_download_get_response(WebKitDownload* download)
{
    g_return_val_if_fail(WEBKIT_IS_DOWNLOAD(download), 0);

    return download->priv->response.get();
}

/**
 * webkit_download_cancel:
 * @download: a #WebKitDownload
 *
 * Cancels the download. When the ongoing download
 * operation is effectively cancelled the signal
 * #WebKitDownload::failed is emitted with
 * %WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER error.
 */
void webkit_download_cancel(WebKitDownload* download)
{
    g_return_if_fail(WEBKIT_IS_DOWNLOAD(download));

    download->priv->isCancelled = true;
    download->priv->download->cancel();
}

/**
 * webkit_download_get_estimated_progress:
 * @download: a #WebKitDownload
 *
 * Gets the value of the #WebKitDownload:estimated-progress property.
 * You can monitor the estimated progress of the download operation by
 * connecting to the notify::estimated-progress signal of @download.
 *
 * Returns: an estimate of the of the percent complete for a download
 *     as a range from 0.0 to 1.0.
 */
gdouble webkit_download_get_estimated_progress(WebKitDownload* download)
{
    g_return_val_if_fail(WEBKIT_IS_DOWNLOAD(download), 0);

    WebKitDownloadPrivate* priv = download->priv;
    if (!priv->response)
        return 0;

    guint64 contentLength = webkit_uri_response_get_content_length(priv->response.get());
    if (!contentLength)
        return 0;

    return static_cast<gdouble>(priv->currentSize) / static_cast<gdouble>(contentLength);
}

/**
 * webkit_download_get_elapsed_time:
 * @download: a #WebKitDownload
 *
 * Gets the elapsed time in seconds, including any fractional part.
 * If the download finished, had an error or was cancelled this is
 * the time between its start and the event.
 *
 * Returns: seconds since the download was started
 */
gdouble webkit_download_get_elapsed_time(WebKitDownload* download)
{
    g_return_val_if_fail(WEBKIT_IS_DOWNLOAD(download), 0);

    WebKitDownloadPrivate* priv = download->priv;
    if (!priv->timer)
        return 0;

    return g_timer_elapsed(priv->timer.get(), 0);
}

/**
 * webkit_download_get_received_data_length:
 * @download: a #WebKitDownload
 *
 * Gets the length of the data already downloaded for @download
 * in bytes.
 *
 * Returns: the amount of bytes already downloaded.
 */
guint64 webkit_download_get_received_data_length(WebKitDownload* download)
{
    g_return_val_if_fail(WEBKIT_IS_DOWNLOAD(download), 0);

    return download->priv->currentSize;
}

/**
 * webkit_download_get_web_view:
 * @download: a #WebKitDownload
 *
 * Get the #WebKitWebView that initiated the download.
 *
 * Returns: (transfer none): the #WebKitWebView that initiated @download,
 *    or %NULL if @download was not initiated by a #WebKitWebView.
 */
WebKitWebView* webkit_download_get_web_view(WebKitDownload* download)
{
    g_return_val_if_fail(WEBKIT_IS_DOWNLOAD(download), 0);

    return download->priv->webView;
}
