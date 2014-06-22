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

#ifndef webkitfilechooserrequest_h
#define webkitfilechooserrequest_h

#include <gtk/gtk.h>
#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_FILE_CHOOSER_REQUEST            (webkit_file_chooser_request_get_type())
#define WEBKIT_FILE_CHOOSER_REQUEST(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_FILE_CHOOSER_REQUEST, WebKitFileChooserRequest))
#define WEBKIT_FILE_CHOOSER_REQUEST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_FILE_CHOOSER_REQUEST, WebKitFileChooserRequestClass))
#define WEBKIT_IS_FILE_CHOOSER_REQUEST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_FILE_CHOOSER_REQUEST))
#define WEBKIT_IS_FILE_CHOOSER_REQUEST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_FILE_CHOOSER_REQUEST))
#define WEBKIT_FILE_CHOOSER_REQUEST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_FILE_CHOOSER_REQUEST, WebKitFileChooserRequestClass))

typedef struct _WebKitFileChooserRequest        WebKitFileChooserRequest;
typedef struct _WebKitFileChooserRequestClass   WebKitFileChooserRequestClass;
typedef struct _WebKitFileChooserRequestPrivate WebKitFileChooserRequestPrivate;

struct _WebKitFileChooserRequest {
    GObject parent;

    /*< private >*/
    WebKitFileChooserRequestPrivate *priv;
};

struct _WebKitFileChooserRequestClass {
    GObjectClass parent_class;
};

WEBKIT_API GType
webkit_file_chooser_request_get_type                  (void);

WEBKIT_API const gchar * const *
webkit_file_chooser_request_get_mime_types        (WebKitFileChooserRequest *request);

WEBKIT_API GtkFileFilter *
webkit_file_chooser_request_get_mime_types_filter (WebKitFileChooserRequest *request);

WEBKIT_API gboolean
webkit_file_chooser_request_get_select_multiple   (WebKitFileChooserRequest *request);

WEBKIT_API void
webkit_file_chooser_request_select_files          (WebKitFileChooserRequest *request,
                                                   const gchar * const      *files);

WEBKIT_API const gchar * const *
webkit_file_chooser_request_get_selected_files    (WebKitFileChooserRequest *request);

G_END_DECLS

#endif
