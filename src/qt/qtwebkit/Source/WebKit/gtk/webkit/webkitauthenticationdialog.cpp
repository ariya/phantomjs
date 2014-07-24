/*
 *  Copyright (C) 2013 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "webkitauthenticationdialog.h"

static void authenticationDialogResponseCallback(GtkWidget* authDialog, gint responseID, WebKitAuthenticationWidget* authWidget)
{
    WebCore::AuthenticationChallenge challenge = webkitAuthenticationWidgetGetChallenge(authWidget);
    if (responseID == GTK_RESPONSE_OK) {
        WebCore::Credential credential = webkitAuthenticationWidgetCreateCredential(authWidget);
        challenge.authenticationClient()->receivedCredential(challenge, credential);
    } else
        challenge.authenticationClient()->receivedRequestToContinueWithoutCredential(challenge);

    gtk_widget_destroy(authDialog);
}

GtkWidget* createAuthenticationDialog(GtkWindow* parentWindow, const WebCore::AuthenticationChallenge& challenge, CredentialStorageMode mode)
{
    GtkDialog* dialog = GTK_DIALOG(gtk_dialog_new());
    gtk_dialog_add_buttons(dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK);

    GtkWidget* contentArea = gtk_dialog_get_content_area(dialog);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_box_set_spacing(GTK_BOX(contentArea), 2); /* 2 * 5 + 2 = 12 */

    GtkWidget* authWidget = webkitAuthenticationWidgetNew(challenge, mode);
    gtk_box_pack_start(GTK_BOX(contentArea), authWidget, TRUE, TRUE, 0);
    gtk_widget_show(authWidget);

    GtkWidget* actionArea = gtk_dialog_get_action_area(dialog);
    gtk_container_set_border_width(GTK_CONTAINER(actionArea), 5);
    gtk_box_set_spacing(GTK_BOX(actionArea), 6);

    GtkWindow* window = GTK_WINDOW(dialog);
    gtk_window_set_resizable(window, FALSE);
    gtk_window_set_title(window, "");
    gtk_window_set_icon_name(window, GTK_STOCK_DIALOG_AUTHENTICATION);
    gtk_window_set_destroy_with_parent (window, TRUE);
    if (parentWindow)
        gtk_window_set_transient_for(window, parentWindow);

    g_signal_connect(dialog, "response", G_CALLBACK(authenticationDialogResponseCallback), authWidget);

    return GTK_WIDGET(dialog);
}
