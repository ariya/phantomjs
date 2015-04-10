/*
 * Copyright (C) 2011 Igalia S.L.
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

#ifndef WebKitBackForwardList_h
#define WebKitBackForwardList_h

#include <glib-object.h>
#include <webkit2/WebKitBackForwardListItem.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_BACK_FORWARD_LIST            (webkit_back_forward_list_get_type())
#define WEBKIT_BACK_FORWARD_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_BACK_FORWARD_LIST, WebKitBackForwardList))
#define WEBKIT_BACK_FORWARD_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_BACK_FORWARD_LIST, WebKitBackForwardListClass))
#define WEBKIT_IS_BACK_FORWARD_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_BACK_FORWARD_LIST))
#define WEBKIT_IS_BACK_FORWARD_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_BACK_FORWARD_LIST))
#define WEBKIT_BACK_FORWARD_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_BACK_FORWARD_LIST, WebKitBackForwardListClass))

typedef struct _WebKitBackForwardList        WebKitBackForwardList;
typedef struct _WebKitBackForwardListClass   WebKitBackForwardListClass;
typedef struct _WebKitBackForwardListPrivate WebKitBackForwardListPrivate;

struct _WebKitBackForwardList {
    GObject parent;

    WebKitBackForwardListPrivate *priv;
};

struct _WebKitBackForwardListClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_back_forward_list_get_type                    (void);

WEBKIT_API WebKitBackForwardListItem *
webkit_back_forward_list_get_current_item            (WebKitBackForwardList *back_forward_list);

WEBKIT_API WebKitBackForwardListItem *
webkit_back_forward_list_get_back_item               (WebKitBackForwardList *back_forward_list);

WEBKIT_API WebKitBackForwardListItem *
webkit_back_forward_list_get_forward_item            (WebKitBackForwardList *back_forward_list);

WEBKIT_API WebKitBackForwardListItem *
webkit_back_forward_list_get_nth_item                (WebKitBackForwardList *back_forward_list,
                                                      gint                   index);
WEBKIT_API guint
webkit_back_forward_list_get_length                  (WebKitBackForwardList *back_forward_list);

WEBKIT_API GList *
webkit_back_forward_list_get_back_list               (WebKitBackForwardList *back_forward_list);

WEBKIT_API GList *
webkit_back_forward_list_get_back_list_with_limit    (WebKitBackForwardList *back_forward_list,
                                                      guint                  limit);

WEBKIT_API GList *
webkit_back_forward_list_get_forward_list            (WebKitBackForwardList *back_forward_list);

WEBKIT_API GList *
webkit_back_forward_list_get_forward_list_with_limit (WebKitBackForwardList *back_forward_list,
                                                      guint                  limit);

G_END_DECLS

#endif
