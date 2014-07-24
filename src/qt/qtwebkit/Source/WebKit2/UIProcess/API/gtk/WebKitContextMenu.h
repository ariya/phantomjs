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

#ifndef WebKitContextMenu_h
#define WebKitContextMenu_h

#include <glib-object.h>
#include <webkit2/WebKitContextMenuItem.h>
#include <webkit2/WebKitDefines.h>
#include <webkit2/WebKitForwardDeclarations.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_CONTEXT_MENU            (webkit_context_menu_get_type())
#define WEBKIT_CONTEXT_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_CONTEXT_MENU, WebKitContextMenu))
#define WEBKIT_IS_CONTEXT_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_CONTEXT_MENU))
#define WEBKIT_CONTEXT_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_CONTEXT_MENU, WebKitContextMenuClass))
#define WEBKIT_IS_CONTEXT_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_CONTEXT_MENU))
#define WEBKIT_CONTEXT_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_CONTEXT_MENU, WebKitContextMenuClass))

typedef struct _WebKitContextMenuClass   WebKitContextMenuClass;
typedef struct _WebKitContextMenuPrivate WebKitContextMenuPrivate;

struct _WebKitContextMenu {
    GObject parent;

    WebKitContextMenuPrivate *priv;
};

struct _WebKitContextMenuClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_context_menu_get_type             (void);

WEBKIT_API WebKitContextMenu *
webkit_context_menu_new                  (void);

WEBKIT_API WebKitContextMenu *
webkit_context_menu_new_with_items       (GList                 *items);

WEBKIT_API void
webkit_context_menu_prepend              (WebKitContextMenu     *menu,
                                          WebKitContextMenuItem *item);

WEBKIT_API void
webkit_context_menu_append               (WebKitContextMenu     *menu,
                                          WebKitContextMenuItem *item);

WEBKIT_API void
webkit_context_menu_insert               (WebKitContextMenu     *menu,
                                          WebKitContextMenuItem *item,
                                          gint                   position);

WEBKIT_API void
webkit_context_menu_move_item            (WebKitContextMenu     *menu,
                                          WebKitContextMenuItem *item,
                                          gint                   position);
WEBKIT_API GList *
webkit_context_menu_get_items            (WebKitContextMenu     *menu);

WEBKIT_API guint
webkit_context_menu_get_n_items          (WebKitContextMenu     *menu);

WEBKIT_API WebKitContextMenuItem *
webkit_context_menu_first                (WebKitContextMenu     *menu);

WEBKIT_API WebKitContextMenuItem *
webkit_context_menu_last                 (WebKitContextMenu     *menu);

WEBKIT_API WebKitContextMenuItem *
webkit_context_menu_get_item_at_position (WebKitContextMenu     *menu,
                                          guint                  position);

WEBKIT_API void
webkit_context_menu_remove               (WebKitContextMenu     *menu,
                                          WebKitContextMenuItem *item);

WEBKIT_API void
webkit_context_menu_remove_all           (WebKitContextMenu     *menu);

G_END_DECLS

#endif
