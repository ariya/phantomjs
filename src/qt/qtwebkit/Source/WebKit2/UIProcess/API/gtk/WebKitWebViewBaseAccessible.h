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

#ifndef WebKitWebViewBaseAccessible_h
#define WebKitWebViewBaseAccessible_h

#include <atk/atk.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_VIEW_BASE_ACCESSIBLE              (webkit_web_view_base_accessible_get_type())
#define WEBKIT_WEB_VIEW_BASE_ACCESSIBLE(object)           (G_TYPE_CHECK_INSTANCE_CAST((object), WEBKIT_TYPE_WEB_VIEW_BASE_ACCESSIBLE, WebKitWebViewBaseAccessible))
#define WEBKIT_WEB_VIEW_BASE_ACCESSIBLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), WEBKIT_TYPE_WEB_VIEW_BASE_ACCESSIBLE, WebKitWebViewBaseAccessibleClass))
#define WEBKIT_IS_WEB_VIEW_BASE_ACCESSIBLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE((object), WEBKIT_TYPE_WEB_VIEW_BASE_ACCESSIBLE))
#define WEBKIT_IS_WEB_VIEW_BASE_ACCESSIBLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), WEBKIT_TYPE_WEB_VIEW_BASE_ACCESSIBLE))
#define WEBKIT_WEB_VIEW_BASE_ACCESSIBLE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), WEBKIT_TYPE_WEB_VIEW_BASE_ACCESSIBLE, WebKitWebViewBaseAccessibleClass))

typedef struct _WebKitWebViewBaseAccessible WebKitWebViewBaseAccessible;
typedef struct _WebKitWebViewBaseAccessibleClass WebKitWebViewBaseAccessibleClass;
typedef struct _WebKitWebViewBaseAccessiblePrivate WebKitWebViewBaseAccessiblePrivate;


struct _WebKitWebViewBaseAccessible {
    AtkSocket parent;
    /*< private >*/
    WebKitWebViewBaseAccessiblePrivate* priv;
};

struct _WebKitWebViewBaseAccessibleClass {
    AtkSocketClass parentClass;
};

GType webkit_web_view_base_accessible_get_type();

WebKitWebViewBaseAccessible* webkitWebViewBaseAccessibleNew(GtkWidget*);

G_END_DECLS

#endif // WebKitWebViewBaseAccessible_h
