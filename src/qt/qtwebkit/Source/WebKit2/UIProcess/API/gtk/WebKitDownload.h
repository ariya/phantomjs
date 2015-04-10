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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitDownload_h
#define WebKitDownload_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>
#include <webkit2/WebKitForwardDeclarations.h>
#include <webkit2/WebKitURIRequest.h>
#include <webkit2/WebKitURIResponse.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_DOWNLOAD            (webkit_download_get_type())
#define WEBKIT_DOWNLOAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_DOWNLOAD, WebKitDownload))
#define WEBKIT_IS_DOWNLOAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_DOWNLOAD))
#define WEBKIT_DOWNLOAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_DOWNLOAD, WebKitDownloadClass))
#define WEBKIT_IS_DOWNLOAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_DOWNLOAD))
#define WEBKIT_DOWNLOAD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_DOWNLOAD, WebKitDownloadClass))

typedef struct _WebKitDownload        WebKitDownload;
typedef struct _WebKitDownloadClass   WebKitDownloadClass;
typedef struct _WebKitDownloadPrivate WebKitDownloadPrivate;

struct _WebKitDownload {
    GObject parent;

    WebKitDownloadPrivate *priv;
};

struct _WebKitDownloadClass {
    GObjectClass parent_class;

    gboolean (* decide_destination)  (WebKitDownload *download,
                                      const gchar    *suggested_filename);

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_download_get_type                 (void);

WEBKIT_API WebKitURIRequest *
webkit_download_get_request              (WebKitDownload *download);

WEBKIT_API const gchar *
webkit_download_get_destination          (WebKitDownload *download);

WEBKIT_API void
webkit_download_set_destination          (WebKitDownload *download,
                                          const gchar    *uri);

WEBKIT_API WebKitURIResponse*
webkit_download_get_response             (WebKitDownload *download);

WEBKIT_API void
webkit_download_cancel                   (WebKitDownload *download);

WEBKIT_API gdouble
webkit_download_get_estimated_progress   (WebKitDownload *download);

WEBKIT_API gdouble
webkit_download_get_elapsed_time         (WebKitDownload *download);

WEBKIT_API guint64
webkit_download_get_received_data_length (WebKitDownload *download);

WEBKIT_API WebKitWebView *
webkit_download_get_web_view             (WebKitDownload *download);

G_END_DECLS

#endif
