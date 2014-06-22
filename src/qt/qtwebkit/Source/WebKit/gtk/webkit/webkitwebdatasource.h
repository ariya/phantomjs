/*
 * Copyright (C) 2009 Jan Michael C. Alonzo
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

#ifndef webkitwebdatasource_h
#define webkitwebdatasource_h

#include <glib.h>
#include <glib-object.h>

#include <webkit/webkitdefines.h>
#include <webkit/webkitwebframe.h>
#include <webkit/webkitnetworkrequest.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_DATA_SOURCE            (webkit_web_data_source_get_type())
#define WEBKIT_WEB_DATA_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_DATA_SOURCE, WebKitWebDataSource))
#define WEBKIT_WEB_DATA_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_DATA_SOURCE, WebKitWebDataSourceClass))
#define WEBKIT_IS_WEB_DATA_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_DATA_SOURCE))
#define WEBKIT_IS_WEB_DATA_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_DATA_SOURCE))
#define WEBKIT_WEB_DATA_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_DATA_SOURCE, WebKitWebDataSourceClass))

typedef struct _WebKitWebDataSourcePrivate WebKitWebDataSourcePrivate;

struct _WebKitWebDataSource {
    GObject parent_instance;

    /*< private >*/
    WebKitWebDataSourcePrivate *priv;
};

struct _WebKitWebDataSourceClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_web_data_source_get_type (void);

WEBKIT_API WebKitWebDataSource *
webkit_web_data_source_new                    (void);

WEBKIT_API WebKitWebDataSource *
webkit_web_data_source_new_with_request       (WebKitNetworkRequest *request);

WEBKIT_API WebKitWebFrame *
webkit_web_data_source_get_web_frame          (WebKitWebDataSource  *data_source);

WEBKIT_API WebKitNetworkRequest *
webkit_web_data_source_get_initial_request    (WebKitWebDataSource  *data_source);

WEBKIT_API WebKitNetworkRequest *
webkit_web_data_source_get_request            (WebKitWebDataSource  *data_source);

WEBKIT_API const gchar *
webkit_web_data_source_get_encoding           (WebKitWebDataSource  *data_source);

WEBKIT_API gboolean
webkit_web_data_source_is_loading             (WebKitWebDataSource  *data_source);

WEBKIT_API GString *
webkit_web_data_source_get_data               (WebKitWebDataSource  *data_source);

WEBKIT_API WebKitWebResource *
webkit_web_data_source_get_main_resource      (WebKitWebDataSource  *data_source);

WEBKIT_API const gchar *
webkit_web_data_source_get_unreachable_uri    (WebKitWebDataSource  *data_source);

WEBKIT_API GList*
webkit_web_data_source_get_subresources       (WebKitWebDataSource  *data_source);

G_END_DECLS

#endif /* webkitwebdatasource_h */
