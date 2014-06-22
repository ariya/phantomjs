/*
 * Copyright (C) 2011 Christian Dywan <christian@lanedo.com>
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

#ifndef webkitfavicondatabase_h
#define webkitfavicondatabase_h

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_FAVICON_DATABASE             (webkit_favicon_database_get_type())
#define WEBKIT_FAVICON_DATABASE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_FAVICON_DATABASE, WebKitFaviconDatabase))
#define WEBKIT_FAVICON_DATABASE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_FAVICON_DATABASE, WebKitFaviconDatabaseClass))
#define WEBKIT_IS_FAVICON_DATABASE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_FAVICON_DATABASE))
#define WEBKIT_IS_FAVICON_DATABASE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_FAVICON_DATABASE))
#define WEBKIT_FAVICON_DATABASE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_FAVICON_DATABASE, WebKitFaviconDatabaseClass))

typedef struct _WebKitFaviconDatabasePrivate WebKitFaviconDatabasePrivate;

struct _WebKitFaviconDatabase {
    GObject parent_instance;

    /*< private >*/
    WebKitFaviconDatabasePrivate* priv;
};

struct _WebKitFaviconDatabaseClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
    void (*_webkit_reserved4) (void);
};

WEBKIT_API GType
webkit_favicon_database_get_type                  (void);

WEBKIT_API const gchar*
webkit_favicon_database_get_path                  (WebKitFaviconDatabase* database);

WEBKIT_API void
webkit_favicon_database_set_path                  (WebKitFaviconDatabase* database,
                                                   const gchar*           path);

WEBKIT_API gchar*
webkit_favicon_database_get_favicon_uri           (WebKitFaviconDatabase* database,
                                                   const gchar*           page_uri);

WEBKIT_API GdkPixbuf*
webkit_favicon_database_try_get_favicon_pixbuf    (WebKitFaviconDatabase* database,
                                                   const gchar*           page_uri,
                                                   guint                  width,
                                                   guint                  height);

WEBKIT_API void
webkit_favicon_database_get_favicon_pixbuf        (WebKitFaviconDatabase* database,
                                                   const gchar*           page_uri,
                                                   guint                  width,
                                                   guint                  height,
                                                   GCancellable*          cancellable,
                                                   GAsyncReadyCallback    callback,
                                                   gpointer               user_data);

WEBKIT_API GdkPixbuf*
webkit_favicon_database_get_favicon_pixbuf_finish (WebKitFaviconDatabase* database,
                                                   GAsyncResult*          result,
                                                   GError**               error);

WEBKIT_API void
webkit_favicon_database_clear                     (WebKitFaviconDatabase* database);

G_END_DECLS

#endif /* webkitfavicondatabase_h */
