/*
 * Copyright (C) 2011 Christian Dywan <christian@lanedo.com>
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

#ifndef webkiticondatabase_h
#define webkiticondatabase_h

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_ICON_DATABASE             (webkit_icon_database_get_type())
#define WEBKIT_ICON_DATABASE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_ICON_DATABASE, WebKitIconDatabase))
#define WEBKIT_ICON_DATABASE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_ICON_DATABASE, WebKitIconDatabaseClass))
#define WEBKIT_IS_ICON_DATABASE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_ICON_DATABASE))
#define WEBKIT_IS_ICON_DATABASE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_ICON_DATABASE))
#define WEBKIT_ICON_DATABASE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_ICON_DATABASE, WebKitIconDatabaseClass))

typedef struct _WebKitIconDatabasePrivate WebKitIconDatabasePrivate;

struct _WebKitIconDatabase {
    GObject parent_instance;

    /*< private >*/
    WebKitIconDatabasePrivate* priv;
};

struct _WebKitIconDatabaseClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
    void (*_webkit_reserved4) (void);
};

WEBKIT_API GType
webkit_icon_database_get_type            (void);

#if !defined(WEBKIT_DISABLE_DEPRECATED)
WEBKIT_API const gchar*
webkit_icon_database_get_path            (WebKitIconDatabase* database);

WEBKIT_API void
webkit_icon_database_set_path            (WebKitIconDatabase* database,
                                          const gchar*        path);

WEBKIT_API gchar*
webkit_icon_database_get_icon_uri        (WebKitIconDatabase* database,
                                          const gchar*        page_uri);

WEBKIT_API GdkPixbuf*
webkit_icon_database_get_icon_pixbuf     (WebKitIconDatabase* database,
                                          const gchar*        page_uri);

WEBKIT_API void
webkit_icon_database_clear               (WebKitIconDatabase* database);
#endif

G_END_DECLS

#endif /* webkiticondatabase_h */
