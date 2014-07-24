/*
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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

#ifndef webkitwebframe_h
#define webkitwebframe_h

#include <glib-object.h>
#include <gtk/gtk.h>
#include <JavaScriptCore/JSBase.h>
#include <webkit/webkitdefines.h>
#include <webkit/webkitnetworkrequest.h>
#include <webkit/webkitwebdatasource.h>
#include <webkitdom/webkitdom.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_FRAME            (webkit_web_frame_get_type())
#define WEBKIT_WEB_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_FRAME, WebKitWebFrame))
#define WEBKIT_WEB_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_FRAME, WebKitWebFrameClass))
#define WEBKIT_IS_WEB_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_FRAME))
#define WEBKIT_IS_WEB_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_FRAME))
#define WEBKIT_WEB_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_FRAME, WebKitWebFrameClass))

typedef struct _WebKitWebFramePrivate WebKitWebFramePrivate;

struct _WebKitWebFrame {
    GObject parent_instance;

    /*< private >*/
    WebKitWebFramePrivate *priv;
};

struct _WebKitWebFrameClass {
    GObjectClass parent_class;

    /*< public >*/
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
    void (*_webkit_reserved4) (void);
    void (*_webkit_reserved5) (void);
    void (*_webkit_reserved6) (void);
};

/**
 * WebKitLoadStatus:
 * @WEBKIT_LOAD_PROVISIONAL: No data has been received yet, empty
 * structures have been allocated to perform the load; the load may
 * still fail for transport issues such as not being able to resolve a
 * name, or connect to a port.
 * @WEBKIT_LOAD_COMMITTED: The first data chunk has arrived, meaning
 * that the necessary transport requirements are stabilished, and the
 * load is being performed.
 * @WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT: The first layout with
 * actual visible content happened; one or more layouts may have
 * happened before that caused nothing to be visible on the screen,
 * because the data available at the time was not significant enough.
 * @WEBKIT_LOAD_FINISHED: This state means that everything that was
 * required to display the page has been loaded.
 * @WEBKIT_LOAD_FAILED: This state means that some error occurred
 * during the page load that prevented it from being completed. You
 * can connect to the #WebKitWebView::load-error signal if you want to
 * know precisely what kind of error occurred.
 */
typedef enum {
    WEBKIT_LOAD_PROVISIONAL,
    WEBKIT_LOAD_COMMITTED,
    WEBKIT_LOAD_FINISHED,
    WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT,
    WEBKIT_LOAD_FAILED
} WebKitLoadStatus;

WEBKIT_API GType
webkit_web_frame_get_type           (void);

#ifndef WEBKIT_DISABLE_DEPRECATED
WEBKIT_API WebKitWebFrame *
webkit_web_frame_new                (WebKitWebView        *web_view);
#endif

WEBKIT_API WebKitWebView *
webkit_web_frame_get_web_view       (WebKitWebFrame       *frame);

WEBKIT_API const gchar *
webkit_web_frame_get_name           (WebKitWebFrame       *frame);

WEBKIT_API const gchar *
webkit_web_frame_get_title          (WebKitWebFrame       *frame);

WEBKIT_API const gchar *
webkit_web_frame_get_uri            (WebKitWebFrame       *frame);

WEBKIT_API WebKitWebFrame*
webkit_web_frame_get_parent         (WebKitWebFrame       *frame);

WEBKIT_API void
webkit_web_frame_load_uri           (WebKitWebFrame       *frame,
                                     const gchar          *uri);

WEBKIT_API void
webkit_web_frame_load_string        (WebKitWebFrame       *frame,
                                     const gchar          *content,
                                     const gchar          *mime_type,
                                     const gchar          *encoding,
                                     const gchar          *base_uri);

WEBKIT_API void
webkit_web_frame_load_alternate_string (WebKitWebFrame    *frame,
                                        const gchar       *content,
                                        const gchar       *base_url,
                                        const gchar       *unreachable_url);

WEBKIT_API void
webkit_web_frame_load_request       (WebKitWebFrame       *frame,
                                     WebKitNetworkRequest *request);

WEBKIT_API void
webkit_web_frame_stop_loading       (WebKitWebFrame       *frame);

WEBKIT_API void
webkit_web_frame_reload             (WebKitWebFrame       *frame);

WEBKIT_API WebKitWebFrame *
webkit_web_frame_find_frame         (WebKitWebFrame       *frame,
                                     const gchar          *name);

WEBKIT_API JSGlobalContextRef
webkit_web_frame_get_global_context (WebKitWebFrame       *frame);

WEBKIT_API GtkPrintOperationResult
webkit_web_frame_print_full         (WebKitWebFrame       *frame,
                                     GtkPrintOperation    *operation,
                                     GtkPrintOperationAction action,
                                     GError              **error);

WEBKIT_API void
webkit_web_frame_print              (WebKitWebFrame       *frame);

WEBKIT_API WebKitLoadStatus
webkit_web_frame_get_load_status    (WebKitWebFrame       *frame);

WEBKIT_API GtkPolicyType
webkit_web_frame_get_horizontal_scrollbar_policy (WebKitWebFrame        *frame);

WEBKIT_API GtkPolicyType
webkit_web_frame_get_vertical_scrollbar_policy   (WebKitWebFrame        *frame);

WEBKIT_API WebKitWebDataSource *
webkit_web_frame_get_data_source             (WebKitWebFrame       *frame);

WEBKIT_API WebKitWebDataSource *
webkit_web_frame_get_provisional_data_source (WebKitWebFrame       *frame);

WEBKIT_API WebKitSecurityOrigin*
webkit_web_frame_get_security_origin         (WebKitWebFrame       *frame);

WEBKIT_API WebKitNetworkResponse*
webkit_web_frame_get_network_response        (WebKitWebFrame       *frame);

WEBKIT_API void
webkit_web_frame_replace_selection           (WebKitWebFrame        *frame,
                                              const char            *text);

WEBKIT_API WebKitDOMRange*
webkit_web_frame_get_range_for_word_around_caret (WebKitWebFrame    *frame);

WEBKIT_API WebKitDOMDocument*
webkit_web_frame_get_dom_document            (WebKitWebFrame        *frame);

G_END_DECLS

#endif
