/*
 * Copyright (C) 2009 Igalia S.L.
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

#ifndef webkitsoupauthdialog_h
#define webkitsoupauthdialog_h

#include <gtk/gtk.h>
#define LIBSOUP_I_HAVE_READ_BUG_594377_AND_KNOW_SOUP_PASSWORD_MANAGER_MIGHT_GO_AWAY
#include <libsoup/soup.h>
#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_SOUP_AUTH_DIALOG            (webkit_soup_auth_dialog_get_type ())
#define WEBKIT_SOUP_AUTH_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), WEBKIT_TYPE_SOUP_AUTH_DIALOG, WebKitSoupAuthDialog))
#define WEBKIT_SOUP_AUTH_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBKIT_TYPE_SOUP_AUTH_DIALOG, WebKitSoupAuthDialog))
#define WEBKIT_IS_SOUP_AUTH_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), WEBKIT_TYPE_SOUP_AUTH_DIALOG))
#define WEBKIT_IS_SOUP_AUTH_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBKIT_TYPE_SOUP_AUTH_DIALOG))
#define WEBKIT_SOUP_AUTH_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBKIT_TYPE_SOUP_AUTH_DIALOG, WebKitSoupAuthDialog))

typedef struct {
    GObject parent_instance;
} WebKitSoupAuthDialog;

typedef struct {
    GObjectClass parent_class;

    GtkWidget* (*current_toplevel) (WebKitSoupAuthDialog* authDialog, SoupMessage* message);
} WebKitSoupAuthDialogClass;

WEBKIT_API GType
webkit_soup_auth_dialog_get_type (void);

G_END_DECLS

#endif /* webkitsoupauthdialog_h */
