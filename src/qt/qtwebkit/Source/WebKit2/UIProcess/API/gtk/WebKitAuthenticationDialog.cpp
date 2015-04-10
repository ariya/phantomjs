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

#include "config.h"
#include "WebKitAuthenticationDialog.h"

#include "AuthenticationDecisionListener.h"
#include "WebCredential.h"
#include "WebKitPrivate.h"

using namespace WebKit;

struct _WebKitAuthenticationDialogPrivate {
    RefPtr<AuthenticationChallengeProxy> authenticationChallenge;

    GtkWidget* authWidget;
    GtkWidget* defaultButton;
    GRefPtr<GtkStyleContext> styleContext;
};

WEBKIT_DEFINE_TYPE(WebKitAuthenticationDialog, webkit_authentication_dialog, GTK_TYPE_EVENT_BOX)

static void webkitAuthenticationDialogAuthenticate(WebKitAuthenticationDialog* authDialog, WebCredential* credential)
{
    WebKitAuthenticationDialogPrivate* priv = authDialog->priv;
    priv->authenticationChallenge->listener()->useCredential(credential);
    gtk_widget_destroy(GTK_WIDGET(authDialog));
}

static void okButtonClicked(GtkButton*, WebKitAuthenticationDialog* authDialog)
{
    WebKitAuthenticationDialogPrivate* priv = authDialog->priv;
    RefPtr<WebCredential> webCredential = WebCredential::create(webkitAuthenticationWidgetCreateCredential(WEBKIT_AUTHENTICATION_WIDGET(priv->authWidget)));
    webkitAuthenticationDialogAuthenticate(authDialog, webCredential.get());
}

static void cancelButtonClicked(GtkButton*, WebKitAuthenticationDialog* authDialog)
{
    webkitAuthenticationDialogAuthenticate(authDialog, 0);
}

static void webkitAuthenticationDialogInitialize(WebKitAuthenticationDialog* authDialog, CredentialStorageMode credentialStorageMode)
{
    GtkWidget* frame = gtk_frame_new(0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    GtkWidget* vBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vBox), 5);

    GtkWidget* buttonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonBox), GTK_BUTTONBOX_END);
    gtk_container_set_border_width(GTK_CONTAINER(buttonBox), 5);
    gtk_box_set_spacing(GTK_BOX(buttonBox), 6);

    GtkWidget* button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(button, "clicked", G_CALLBACK(cancelButtonClicked), authDialog);
    gtk_box_pack_end(GTK_BOX(buttonBox), button, FALSE, TRUE, 0);
    gtk_widget_show(button);

    button = gtk_button_new_from_stock(GTK_STOCK_OK);
    authDialog->priv->defaultButton = button;
    g_signal_connect(button, "clicked", G_CALLBACK(okButtonClicked), authDialog);
    gtk_widget_set_can_default(button, TRUE);
    gtk_box_pack_end(GTK_BOX(buttonBox), button, FALSE, TRUE, 0);
    gtk_widget_show(button);

    authDialog->priv->authWidget = webkitAuthenticationWidgetNew(authDialog->priv->authenticationChallenge->core(), credentialStorageMode);
    gtk_box_pack_start(GTK_BOX(vBox), authDialog->priv->authWidget, TRUE, TRUE, 0);
    gtk_widget_show(authDialog->priv->authWidget);

    gtk_box_pack_end(GTK_BOX(vBox), buttonBox, FALSE, TRUE, 0);
    gtk_widget_show(buttonBox);

    gtk_container_add(GTK_CONTAINER(frame), vBox);
    gtk_widget_show(vBox);

    gtk_container_add(GTK_CONTAINER(authDialog), frame);
    gtk_widget_show(frame);
}

static gboolean webkitAuthenticationDialogDraw(GtkWidget* widget, cairo_t* cr)
{
    WebKitAuthenticationDialogPrivate* priv = WEBKIT_AUTHENTICATION_DIALOG(widget)->priv;

    gtk_style_context_save(priv->styleContext.get());
    gtk_style_context_add_class(priv->styleContext.get(), GTK_STYLE_CLASS_BACKGROUND);
    gtk_render_background(priv->styleContext.get(), cr, 0, 0, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
    gtk_style_context_restore(priv->styleContext.get());

    GTK_WIDGET_CLASS(webkit_authentication_dialog_parent_class)->draw(widget, cr);

    return FALSE;
}

static void webkitAuthenticationDialogMap(GtkWidget* widget)
{
    WebKitAuthenticationDialogPrivate* priv = WEBKIT_AUTHENTICATION_DIALOG(widget)->priv;
    gtk_widget_grab_default(priv->defaultButton);

    GTK_WIDGET_CLASS(webkit_authentication_dialog_parent_class)->map(widget);
}

static void webkitAuthenticationDialogConstructed(GObject* object)
{
    G_OBJECT_CLASS(webkit_authentication_dialog_parent_class)->constructed(object);

    WebKitAuthenticationDialogPrivate* priv = WEBKIT_AUTHENTICATION_DIALOG(object)->priv;
    priv->styleContext = adoptGRef(gtk_style_context_new());
    GtkWidgetPath* path = gtk_widget_path_new();
    gtk_widget_path_append_type(path, GTK_TYPE_WINDOW);
    gtk_style_context_set_path(priv->styleContext.get(), path);
    gtk_widget_path_free(path);
}

static void webkit_authentication_dialog_class_init(WebKitAuthenticationDialogClass* klass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    objectClass->constructed = webkitAuthenticationDialogConstructed;

    GtkWidgetClass* widgetClass = GTK_WIDGET_CLASS(klass);
    widgetClass->draw = webkitAuthenticationDialogDraw;
    widgetClass->map = webkitAuthenticationDialogMap;
}

GtkWidget* webkitAuthenticationDialogNew(AuthenticationChallengeProxy* authenticationChallenge, CredentialStorageMode mode)
{
    WebKitAuthenticationDialog* authDialog = WEBKIT_AUTHENTICATION_DIALOG(g_object_new(WEBKIT_TYPE_AUTHENTICATION_DIALOG, NULL));
    authDialog->priv->authenticationChallenge = authenticationChallenge;
    webkitAuthenticationDialogInitialize(authDialog, mode);
    return GTK_WIDGET(authDialog);
}
