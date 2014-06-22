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
#include "WebKitAuthenticationWidget.h"

#include "CredentialBackingStore.h"
#include "GtkVersioning.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;

struct _WebKitAuthenticationWidgetPrivate {
    AuthenticationChallenge challenge;
    CredentialStorageMode credentialStorageMode;

    GtkWidget* loginEntry;
    GtkWidget* passwordEntry;
    GtkWidget* rememberCheckButton;
};

G_DEFINE_TYPE(WebKitAuthenticationWidget, webkit_authentication_widget, GTK_TYPE_BOX)

static const int gLayoutColumnSpacing = 12;
static const int gLayoutRowSpacing = 6;
static const int gButtonSpacing = 5;

#ifdef GTK_API_VERSION_2
static void packTwoColumnLayoutInBox(GtkWidget* box, ...)
{
    va_list argumentList;
    va_start(argumentList, box);

    GtkWidget* table = gtk_table_new(1, 2, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(table), gLayoutColumnSpacing);
    gtk_table_set_row_spacings(GTK_TABLE(table), gLayoutRowSpacing);

    GtkWidget* firstColumnWidget = va_arg(argumentList, GtkWidget*);
    int rowNumber = 0;
    while (firstColumnWidget) {
        if (rowNumber)
            gtk_table_resize(GTK_TABLE(table), rowNumber + 1, 2);

        GtkWidget* secondColumnWidget = va_arg(argumentList, GtkWidget*);
        GtkAttachOptions attachOptions = static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL);
        gtk_table_attach(
            GTK_TABLE(table), firstColumnWidget,
            0, secondColumnWidget ? 1 : 2,
            rowNumber, rowNumber + 1,
            attachOptions, attachOptions,
            0, 0);
        gtk_widget_show(firstColumnWidget);

        if (secondColumnWidget) {
            gtk_table_attach_defaults(GTK_TABLE(table), secondColumnWidget, 1, 2, rowNumber, rowNumber + 1);
            gtk_widget_show(secondColumnWidget);
        }

        firstColumnWidget = va_arg(argumentList, GtkWidget*);
        rowNumber++;
    }

    va_end(argumentList);

    gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 0);
    gtk_widget_show(table);
}
#else
static void packTwoColumnLayoutInBox(GtkWidget* box, ...)
{
    va_list argumentList;
    va_start(argumentList, box);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), gLayoutRowSpacing);
    gtk_grid_set_row_spacing(GTK_GRID(grid), gLayoutRowSpacing);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    GtkWidget* firstColumnWidget = va_arg(argumentList, GtkWidget*);
    int rowNumber = 0;
    while (firstColumnWidget) {
        GtkWidget* secondColumnWidget = va_arg(argumentList, GtkWidget*);
        int firstWidgetWidth = secondColumnWidget ? 1 : 2;

        gtk_grid_attach(GTK_GRID(grid), firstColumnWidget, 0, rowNumber, firstWidgetWidth, 1);
        gtk_widget_set_hexpand(firstColumnWidget, TRUE);
        gtk_widget_set_vexpand(firstColumnWidget, TRUE);
        gtk_widget_show(firstColumnWidget);

        if (secondColumnWidget) {
            gtk_grid_attach(GTK_GRID(grid), secondColumnWidget, 1, rowNumber, 1, 1);
            gtk_widget_set_hexpand(secondColumnWidget, TRUE);
            gtk_widget_set_vexpand(secondColumnWidget, TRUE);
            gtk_widget_show(secondColumnWidget);
        }

        firstColumnWidget = va_arg(argumentList, GtkWidget*);
        rowNumber++;
    }

    va_end(argumentList);

    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);
    gtk_widget_show(grid);
}
#endif

static GtkWidget* createLabel(const char* labelString, int horizontalPadding = 0)
{
    GtkWidget* label = gtk_label_new(labelString);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    if (horizontalPadding)
        gtk_misc_set_padding(GTK_MISC(label), 0, horizontalPadding);
    return label;
}

static GtkWidget* createEntry(GtkWidget** member)
{
    *member = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(*member), TRUE);
    return *member;
}

static void webkitAuthenticationWidgetInitialize(WebKitAuthenticationWidget* authWidget)
{
    gtk_orientable_set_orientation(GTK_ORIENTABLE(authWidget), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_set_spacing(GTK_BOX(authWidget), gLayoutColumnSpacing);
    gtk_container_set_border_width(GTK_CONTAINER(authWidget), gButtonSpacing);

    GtkWidget* icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
    gtk_misc_set_alignment(GTK_MISC(icon), 0.5, 0.0);
    gtk_box_pack_start(GTK_BOX(authWidget), icon, FALSE, FALSE, 0);
    gtk_widget_show(icon);

    WebKitAuthenticationWidgetPrivate* priv = authWidget->priv;
    GOwnPtr<char> prompt(g_strdup_printf(
        _("The site %s:%i requests a username and password"),
        priv->challenge.protectionSpace().host().utf8().data(),
        priv->challenge.protectionSpace().port()));

    priv->rememberCheckButton = gtk_check_button_new_with_mnemonic(_("_Remember password"));
    gtk_label_set_line_wrap(GTK_LABEL(gtk_bin_get_child(GTK_BIN(priv->rememberCheckButton))), TRUE);

    String realm = authWidget->priv->challenge.protectionSpace().realm();
    if (!realm.isEmpty()) {
        packTwoColumnLayoutInBox(
            GTK_WIDGET(authWidget),
            createLabel(prompt.get(), gLayoutRowSpacing), NULL,
            createLabel(_("Server message:")), createLabel(realm.utf8().data()),
            createLabel(_("Username:")), createEntry(&priv->loginEntry),
            createLabel(_("Password:")), createEntry(&priv->passwordEntry),
            priv->rememberCheckButton, NULL,
            NULL);

    } else {
        packTwoColumnLayoutInBox(
            GTK_WIDGET(authWidget),
            createLabel(prompt.get(), gLayoutRowSpacing), NULL,
            createLabel(_("Username:")), createEntry(&priv->loginEntry),
            createLabel(_("Password:")), createEntry(&priv->passwordEntry),
            priv->rememberCheckButton, NULL, NULL,
            NULL);
    }
    gtk_entry_set_visibility(GTK_ENTRY(priv->passwordEntry), FALSE);
    gtk_widget_set_visible(priv->rememberCheckButton, priv->credentialStorageMode != DisallowPersistentStorage);

    const Credential& credentialFromPersistentStorage = priv->challenge.proposedCredential();
    if (!credentialFromPersistentStorage.isEmpty()) {
        gtk_entry_set_text(GTK_ENTRY(priv->loginEntry), credentialFromPersistentStorage.user().utf8().data());
        gtk_entry_set_text(GTK_ENTRY(priv->passwordEntry), credentialFromPersistentStorage.password().utf8().data());
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->rememberCheckButton), TRUE);
    }

    gtk_widget_grab_focus(priv->loginEntry);
}

static void webkitAuthenticationWidgetFinalize(GObject* object)
{
    WEBKIT_AUTHENTICATION_WIDGET(object)->priv->~WebKitAuthenticationWidgetPrivate();
    G_OBJECT_CLASS(webkit_authentication_widget_parent_class)->finalize(object);
}

static void webkit_authentication_widget_init(WebKitAuthenticationWidget* authWidget)
{
    WebKitAuthenticationWidgetPrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(authWidget, WEBKIT_TYPE_AUTHENTICATION_WIDGET, WebKitAuthenticationWidgetPrivate);
    new (priv) WebKitAuthenticationWidgetPrivate();
    authWidget->priv = priv;
}

static void webkit_authentication_widget_class_init(WebKitAuthenticationWidgetClass* klass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    objectClass->finalize = webkitAuthenticationWidgetFinalize;
    g_type_class_add_private(objectClass, sizeof(WebKitAuthenticationWidgetPrivate));
}

GtkWidget* webkitAuthenticationWidgetNew(const AuthenticationChallenge& challenge, CredentialStorageMode mode)
{
    WebKitAuthenticationWidget* authWidget = WEBKIT_AUTHENTICATION_WIDGET(g_object_new(WEBKIT_TYPE_AUTHENTICATION_WIDGET, NULL));
    authWidget->priv->challenge = challenge;
    authWidget->priv->credentialStorageMode = mode;
    webkitAuthenticationWidgetInitialize(authWidget);
    return GTK_WIDGET(authWidget);
}

Credential webkitAuthenticationWidgetCreateCredential(WebKitAuthenticationWidget* authWidget)
{
    const char* username = gtk_entry_get_text(GTK_ENTRY(authWidget->priv->loginEntry));
    const char* password = gtk_entry_get_text(GTK_ENTRY(authWidget->priv->passwordEntry));
    bool rememberPassword = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(authWidget->priv->rememberCheckButton));

    CredentialPersistence persistence;
    if (rememberPassword && authWidget->priv->credentialStorageMode == AllowPersistentStorage)
        persistence = CredentialPersistencePermanent;
    else
        persistence = CredentialPersistenceForSession;
    return Credential(String::fromUTF8(username), String::fromUTF8(password), persistence);
}

AuthenticationChallenge& webkitAuthenticationWidgetGetChallenge(WebKitAuthenticationWidget* authWidget)
{
    return authWidget->priv->challenge;
}
