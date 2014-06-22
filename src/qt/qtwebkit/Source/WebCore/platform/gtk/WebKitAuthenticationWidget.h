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

#ifndef WebKitAuthenticationWidget_h
#define WebKitAuthenticationWidget_h

#include "AuthenticationChallenge.h"
#include <gtk/gtk.h>

enum CredentialStorageMode {
    AllowPersistentStorage, // The user is asked whether to store credential information.
    DisallowPersistentStorage // Credential information is only kept in the session.
};

G_BEGIN_DECLS

#define WEBKIT_TYPE_AUTHENTICATION_WIDGET            (webkit_authentication_widget_get_type())
#define WEBKIT_AUTHENTICATION_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_AUTHENTICATION_WIDGET, WebKitAuthenticationWidget))
#define WEBKIT_IS_AUTHENTICATION_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_AUTHENTICATION_WIDGET))
#define WEBKIT_AUTHENTICATION_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_AUTHENTICATION_WIDGET, WebKitAuthenticationWidgetClass))
#define WEBKIT_IS_AUTHENTICATION_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_AUTHENTICATION_WIDGET))
#define WEBKIT_AUTHENTICATION_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_AUTHENTICATION_WIDGET, WebKitAuthenticationWidgetClass))

typedef struct _WebKitAuthenticationWidget        WebKitAuthenticationWidget;
typedef struct _WebKitAuthenticationWidgetClass   WebKitAuthenticationWidgetClass;
typedef struct _WebKitAuthenticationWidgetPrivate WebKitAuthenticationWidgetPrivate;

struct _WebKitAuthenticationWidget {
    GtkBox parent;

    WebKitAuthenticationWidgetPrivate* priv;
};

struct _WebKitAuthenticationWidgetClass {
    GtkBoxClass parentClass;
};

GType webkit_authentication_widget_get_type();
GtkWidget* webkitAuthenticationWidgetNew(const WebCore::AuthenticationChallenge&, CredentialStorageMode);
WebCore::Credential webkitAuthenticationWidgetCreateCredential(WebKitAuthenticationWidget*);
WebCore::AuthenticationChallenge& webkitAuthenticationWidgetGetChallenge(WebKitAuthenticationWidget*);

G_END_DECLS

#endif // WebKitAuthenticationWidget_h
