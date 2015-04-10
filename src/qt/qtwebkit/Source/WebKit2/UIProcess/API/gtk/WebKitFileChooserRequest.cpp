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
#include "WebKitFileChooserRequest.h"

#include "ImmutableArray.h"
#include "WebKitFileChooserRequestPrivate.h"
#include "WebOpenPanelParameters.h"
#include "WebOpenPanelResultListenerProxy.h"
#include <WebCore/FileSystem.h>
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;
using namespace WebCore;

/**
 * SECTION: WebKitFileChooserRequest
 * @Short_description: A request to open a file chooser
 * @Title: WebKitFileChooserRequest
 * @See_also: #WebKitWebView
 *
 * Whenever the user interacts with an &lt;input type='file' /&gt;
 * HTML element, WebKit will need to show a dialog to choose one or
 * more files to be uploaded to the server along with the rest of the
 * form data. For that to happen in a general way, instead of just
 * opening a #GtkFileChooserDialog (which might be not desirable in
 * some cases, which could prefer to use their own file chooser
 * dialog), WebKit will fire the #WebKitWebView::run-file-chooser
 * signal with a #WebKitFileChooserRequest object, which will allow
 * the client application to specify the files to be selected, to
 * inspect the details of the request (e.g. if multiple selection
 * should be allowed) and to cancel the request, in case nothing was
 * selected.
 *
 * In case the client application does not wish to handle this signal,
 * WebKit will provide a default handler which will asynchronously run
 * a regular #GtkFileChooserDialog for the user to interact with.
 */

struct _WebKitFileChooserRequestPrivate {
    RefPtr<WebOpenPanelParameters> parameters;
    RefPtr<WebOpenPanelResultListenerProxy> listener;
    GRefPtr<GtkFileFilter> filter;
    GRefPtr<GPtrArray> mimeTypes;
    GRefPtr<GPtrArray> selectedFiles;
    bool handledRequest;
};

WEBKIT_DEFINE_TYPE(WebKitFileChooserRequest, webkit_file_chooser_request, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_FILTER,
    PROP_MIME_TYPES,
    PROP_SELECT_MULTIPLE,
    PROP_SELECTED_FILES,
};

static void webkitFileChooserRequestDispose(GObject* object)
{
    WebKitFileChooserRequest* request = WEBKIT_FILE_CHOOSER_REQUEST(object);

    // Make sure the request is always handled before finalizing.
    if (!request->priv->handledRequest)
        webkit_file_chooser_request_cancel(request);

    G_OBJECT_CLASS(webkit_file_chooser_request_parent_class)->dispose(object);
}

static void webkitFileChooserRequestGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitFileChooserRequest* request = WEBKIT_FILE_CHOOSER_REQUEST(object);
    switch (propId) {
    case PROP_FILTER:
        g_value_set_object(value, webkit_file_chooser_request_get_mime_types_filter(request));
        break;
    case PROP_MIME_TYPES:
        g_value_set_boxed(value, webkit_file_chooser_request_get_mime_types(request));
        break;
    case PROP_SELECT_MULTIPLE:
        g_value_set_boolean(value, webkit_file_chooser_request_get_select_multiple(request));
        break;
    case PROP_SELECTED_FILES:
        g_value_set_boxed(value, webkit_file_chooser_request_get_selected_files(request));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
        break;
    }
}

static void webkit_file_chooser_request_class_init(WebKitFileChooserRequestClass* requestClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(requestClass);
    objectClass->dispose = webkitFileChooserRequestDispose;
    objectClass->get_property = webkitFileChooserRequestGetProperty;

    /**
     * WebKitFileChooserRequest:filter:
     *
     * The filter currently associated with the request. See
     * webkit_file_chooser_request_get_mime_types_filter() for more
     * details.
     */
    g_object_class_install_property(objectClass,
                                    PROP_FILTER,
                                    g_param_spec_object("filter",
                                                      _("MIME types filter"),
                                                      _("The filter currently associated with the request"),
                                                      GTK_TYPE_FILE_FILTER,
                                                      WEBKIT_PARAM_READABLE));
    /**
     * WebKitFileChooserRequest:mime-types:
     *
     * A %NULL-terminated array of strings containing the list of MIME
     * types the file chooser dialog should handle. See
     * webkit_file_chooser_request_get_mime_types() for more details.
     */
    g_object_class_install_property(objectClass,
                                    PROP_MIME_TYPES,
                                    g_param_spec_boxed("mime-types",
                                                      _("MIME types"),
                                                      _("The list of MIME types associated with the request"),
                                                      G_TYPE_STRV,
                                                      WEBKIT_PARAM_READABLE));
    /**
     * WebKitFileChooserRequest:select-multiple:
     *
     * Whether the file chooser should allow selecting multiple
     * files. See
     * webkit_file_chooser_request_get_select_multiple() for
     * more details.
     */
    g_object_class_install_property(objectClass,
                                    PROP_SELECT_MULTIPLE,
                                    g_param_spec_boolean("select-multiple",
                                                       _("Select multiple files"),
                                                       _("Whether the file chooser should allow selecting multiple files"),
                                                       FALSE,
                                                       WEBKIT_PARAM_READABLE));
    /**
     * WebKitFileChooserRequest:selected-files:
     *
     * A %NULL-terminated array of strings containing the list of
     * selected files associated to the current request. See
     * webkit_file_chooser_request_get_selected_files() for more details.
     */
    g_object_class_install_property(objectClass,
                                    PROP_SELECTED_FILES,
                                    g_param_spec_boxed("selected-files",
                                                      _("Selected files"),
                                                      _("The list of selected files associated with the request"),
                                                      G_TYPE_STRV,
                                                      WEBKIT_PARAM_READABLE));
}

WebKitFileChooserRequest* webkitFileChooserRequestCreate(WebOpenPanelParameters* parameters, WebOpenPanelResultListenerProxy* listener)
{
    WebKitFileChooserRequest* request = WEBKIT_FILE_CHOOSER_REQUEST(g_object_new(WEBKIT_TYPE_FILE_CHOOSER_REQUEST, NULL));
    request->priv->parameters = parameters;
    request->priv->listener = listener;
    return request;
}

/**
 * webkit_file_chooser_request_get_mime_types:
 * @request: a #WebKitFileChooserRequest
 *
 * Get the list of MIME types the file chooser dialog should handle,
 * in the format specified in RFC 2046 for "media types". Its contents
 * depend on the value of the 'accept' attribute for HTML input
 * elements. This function should normally be called before presenting
 * the file chooser dialog to the user, to decide whether to allow the
 * user to select multiple files at once or only one.
 *
 * Returns: (array zero-terminated=1) (transfer none): a
 * %NULL-terminated array of strings if a list of accepted MIME types
 * is defined or %NULL otherwise, meaning that any MIME type should be
 * accepted. This array and its contents are owned by WebKitGTK+ and
 * should not be modified or freed.
 */
const gchar* const* webkit_file_chooser_request_get_mime_types(WebKitFileChooserRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_FILE_CHOOSER_REQUEST(request), 0);
    if (request->priv->mimeTypes)
        return reinterpret_cast<gchar**>(request->priv->mimeTypes->pdata);

    RefPtr<ImmutableArray> mimeTypes = request->priv->parameters->acceptMIMETypes();
    size_t numOfMimeTypes = mimeTypes->size();
    if (!numOfMimeTypes)
        return 0;

    request->priv->mimeTypes = adoptGRef(g_ptr_array_new_with_free_func(g_free));
    for (size_t i = 0; i < numOfMimeTypes; ++i) {
        WebString* webMimeType = static_cast<WebString*>(mimeTypes->at(i));
        String mimeTypeString = webMimeType->string();
        if (mimeTypeString.isEmpty())
            continue;
        g_ptr_array_add(request->priv->mimeTypes.get(), g_strdup(mimeTypeString.utf8().data()));
    }
    g_ptr_array_add(request->priv->mimeTypes.get(), 0);

    return reinterpret_cast<gchar**>(request->priv->mimeTypes->pdata);
}

/**
 * webkit_file_chooser_request_get_mime_types_filter:
 * @request: a #WebKitFileChooserRequest
 *
 * Get the filter currently associated with the request, ready to be
 * used by #GtkFileChooser. This function should normally be called
 * before presenting the file chooser dialog to the user, to decide
 * whether to apply a filter so the user would not be allowed to
 * select files with other MIME types.
 *
 * See webkit_file_chooser_request_get_mime_types() if you are
 * interested in getting the list of accepted MIME types.
 *
 * Returns: (transfer none): a #GtkFileFilter if a list of accepted
 * MIME types is defined or %NULL otherwise. The returned object is
 * owned by WebKitGTK+ should not be modified or freed.
 */
GtkFileFilter* webkit_file_chooser_request_get_mime_types_filter(WebKitFileChooserRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_FILE_CHOOSER_REQUEST(request), 0);
    if (request->priv->filter)
        return request->priv->filter.get();

    RefPtr<ImmutableArray> mimeTypes = request->priv->parameters->acceptMIMETypes();
    size_t numOfMimeTypes = mimeTypes->size();
    if (!numOfMimeTypes)
        return 0;

    // Do not use adoptGRef here, since we want to sink the floating
    // reference for the new instance of GtkFileFilter, so we make
    // sure we keep the ownership during the lifetime of the request.
    request->priv->filter = gtk_file_filter_new();
    for (size_t i = 0; i < numOfMimeTypes; ++i) {
        WebString* webMimeType = static_cast<WebString*>(mimeTypes->at(i));
        String mimeTypeString = webMimeType->string();
        if (mimeTypeString.isEmpty())
            continue;
        gtk_file_filter_add_mime_type(request->priv->filter.get(), mimeTypeString.utf8().data());
    }

    return request->priv->filter.get();
}

/**
 * webkit_file_chooser_request_get_select_multiple:
 * @request: a #WebKitFileChooserRequest
 *
 * Determine whether the file chooser associated to this
 * #WebKitFileChooserRequest should allow selecting multiple files,
 * which depends on the HTML input element having a 'multiple'
 * attribute defined.
 *
 * Returns: %TRUE if the file chooser should allow selecting multiple files or %FALSE otherwise.
 */
gboolean webkit_file_chooser_request_get_select_multiple(WebKitFileChooserRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_FILE_CHOOSER_REQUEST(request), FALSE);
    return request->priv->parameters->allowMultipleFiles();
}

/**
 * webkit_file_chooser_request_select_files:
 * @request: a #WebKitFileChooserRequest
 * @files: (array zero-terminated=1) (transfer none): a
 * %NULL-terminated array of strings, containing paths to local files.
 *
 * Ask WebKit to select local files for upload and complete the
 * request.
 */
void webkit_file_chooser_request_select_files(WebKitFileChooserRequest* request, const gchar* const* files)
{
    g_return_if_fail(WEBKIT_IS_FILE_CHOOSER_REQUEST(request));
    g_return_if_fail(files);

    GRefPtr<GPtrArray> selectedFiles = adoptGRef(g_ptr_array_new_with_free_func(g_free));
    Vector<RefPtr<APIObject> > choosenFiles;
    for (int i = 0; files[i]; i++) {
        GRefPtr<GFile> filename = adoptGRef(g_file_new_for_path(files[i]));

        // Make sure the file path is presented as an URI (escaped
        // string, with the 'file://' prefix) to WebCore otherwise the
        // FileChooser won't actually choose it.
        GOwnPtr<char> uri(g_file_get_uri(filename.get()));
        choosenFiles.append(WebURL::create(String::fromUTF8(uri.get())));

        // Do not use the URI here because this won't reach WebCore.
        g_ptr_array_add(selectedFiles.get(), g_strdup(files[i]));
    }
    g_ptr_array_add(selectedFiles.get(), 0);

    // Select the files in WebCore and update local private attributes.
    request->priv->listener->chooseFiles(ImmutableArray::adopt(choosenFiles).get());
    request->priv->selectedFiles = selectedFiles;
    request->priv->handledRequest = true;
}

/**
 * webkit_file_chooser_request_get_selected_files:
 * @request: a #WebKitFileChooserRequest
 *
 * Get the list of selected files currently associated to the
 * request. Initially, the return value of this method contains any
 * files selected in previous file chooser requests for this HTML
 * input element. Once webkit_file_chooser_request_select_files, the
 * value will reflect whatever files are given.
 *
 * This function should normally be called only before presenting the
 * file chooser dialog to the user, to decide whether to perform some
 * extra action, like pre-selecting the files from a previous request.
 *
 * Returns: (array zero-terminated=1) (transfer none): a
 * %NULL-terminated array of strings if there are selected files
 * associated with the request or %NULL otherwise. This array and its
 * contents are owned by WebKitGTK+ and should not be modified or
 * freed.
 */
const gchar* const* webkit_file_chooser_request_get_selected_files(WebKitFileChooserRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_FILE_CHOOSER_REQUEST(request), 0);
    if (request->priv->selectedFiles)
        return reinterpret_cast<gchar**>(request->priv->selectedFiles->pdata);

    RefPtr<ImmutableArray> selectedFileNames = request->priv->parameters->selectedFileNames();
    size_t numOfFiles = selectedFileNames->size();
    if (!numOfFiles)
        return 0;

    request->priv->selectedFiles = adoptGRef(g_ptr_array_new_with_free_func(g_free));
    for (size_t i = 0; i < numOfFiles; ++i) {
        WebString* webFileName = static_cast<WebString*>(selectedFileNames->at(i));
        if (webFileName->isEmpty())
            continue;
        CString filename = fileSystemRepresentation(webFileName->string());
        g_ptr_array_add(request->priv->selectedFiles.get(), g_strdup(filename.data()));
    }
    g_ptr_array_add(request->priv->selectedFiles.get(), 0);

    return reinterpret_cast<gchar**>(request->priv->selectedFiles->pdata);
}

/**
 * webkit_file_chooser_request_cancel:
 * @request: a #WebKitFileChooserRequest
 *
 * Ask WebKit to cancel the request. It's important to do this in case
 * no selection has been made in the client, otherwise the request
 * won't be properly completed and the browser will keep the request
 * pending forever, which might cause the browser to hang.
 */
void webkit_file_chooser_request_cancel(WebKitFileChooserRequest* request)
{
    g_return_if_fail(WEBKIT_IS_FILE_CHOOSER_REQUEST(request));
    request->priv->listener->cancel();
    request->priv->handledRequest = true;
}
