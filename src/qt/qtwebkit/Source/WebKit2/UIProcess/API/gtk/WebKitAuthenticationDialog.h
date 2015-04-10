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

#ifndef WebKitAuthenticationDialog_h
#define WebKitAuthenticationDialog_h

#include "AuthenticationChallengeProxy.h"
#include "WebKitAuthenticationWidget.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_AUTHENTICATION_DIALOG            (webkit_authentication_dialog_get_type())
#define WEBKIT_AUTHENTICATION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_AUTHENTICATION_DIALOG, WebKitAuthenticationDialog))
#define WEBKIT_IS_AUTHENTICATION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_AUTHENTICATION_DIALOG))
#define WEBKIT_AUTHENTICATION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_AUTHENTICATION_DIALOG, WebKitAuthenticationDialogClass))
#define WEBKIT_IS_AUTHENTICATION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_AUTHENTICATION_DIALOG))
#define WEBKIT_AUTHENTICATION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_AUTHENTICATION_DIALOG, WebKitAuthenticationDialogClass))

typedef struct _WebKitAuthenticationDialog        WebKitAuthenticationDialog;
typedef struct _WebKitAuthenticationDialogClass   WebKitAuthenticationDialogClass;
typedef struct _WebKitAuthenticationDialogPrivate WebKitAuthenticationDialogPrivate;

struct _WebKitAuthenticationDialog {
    GtkEventBox parent;

    WebKitAuthenticationDialogPrivate* priv;
};

struct _WebKitAuthenticationDialogClass {
    GtkEventBoxClass parentClass;
};

GType webkit_authentication_dialog_get_type();
GtkWidget* webkitAuthenticationDialogNew(WebKit::AuthenticationChallengeProxy*, CredentialStorageMode);

G_END_DECLS

#endif // WebKitAuthenticationDialog_h
