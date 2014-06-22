/*
 * Copyright (C) 2011 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "BrowserSettingsDialog.h"
#include "BrowserCellRendererVariant.h"

enum {
    PROP_0,

    PROP_SETTINGS
};

enum {
    SETTINGS_LIST_COLUMN_NAME,
    SETTINGS_LIST_COLUMN_NICK,
    SETTINGS_LIST_COLUMN_BLURB,
    SETTINGS_LIST_COLUMN_VALUE,
    SETTINGS_LIST_COLUMN_ADJUSTMENT,

    SETTINGS_LIST_N_COLUMNS
};

struct _BrowserSettingsDialog {
    GtkDialog parent;

    GtkWidget *settingsList;
    WebKitSettings *settings;
};

struct _BrowserSettingsDialogClass {
    GtkDialogClass parent;
};

G_DEFINE_TYPE(BrowserSettingsDialog, browser_settings_dialog, GTK_TYPE_DIALOG)

static void cellRendererChanged(GtkCellRenderer *renderer, const char *path, const GValue *value, BrowserSettingsDialog *dialog)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->settingsList));
    GtkTreePath *treePath = gtk_tree_path_new_from_string(path);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, treePath);

    char *name;
    gtk_tree_model_get(model, &iter, SETTINGS_LIST_COLUMN_NAME, &name, -1);
    g_object_set_property(G_OBJECT(dialog->settings), name, value);
    g_free(name);

    gtk_list_store_set(GTK_LIST_STORE(model), &iter, SETTINGS_LIST_COLUMN_VALUE, value, -1);
    gtk_tree_path_free(treePath);
}

static void browserSettingsDialogSetProperty(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    BrowserSettingsDialog *dialog = BROWSER_SETTINGS_DIALOG(object);

    switch (propId) {
    case PROP_SETTINGS:
        dialog->settings = g_value_get_object(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void browser_settings_dialog_init(BrowserSettingsDialog *dialog)
{
    GtkBox *contentArea = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
    gtk_box_set_spacing(contentArea, 2);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    gtk_window_set_title(GTK_WINDOW(dialog), "WebKit View Settings");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);

    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    dialog->settingsList = gtk_tree_view_new();
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(dialog->settingsList),
                                                0, "Name", renderer,
                                                "text", SETTINGS_LIST_COLUMN_NICK,
                                                NULL);
    renderer = browser_cell_renderer_variant_new();
    g_signal_connect(renderer, "changed", G_CALLBACK(cellRendererChanged), dialog);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(dialog->settingsList),
                                                1, "Value", renderer,
                                                "value", SETTINGS_LIST_COLUMN_VALUE,
                                                "adjustment", SETTINGS_LIST_COLUMN_ADJUSTMENT,
                                                NULL);
    gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(dialog->settingsList), SETTINGS_LIST_COLUMN_BLURB);

    gtk_container_add(GTK_CONTAINER(scrolledWindow), dialog->settingsList);
    gtk_widget_show(dialog->settingsList);

    gtk_box_pack_start(contentArea, scrolledWindow, TRUE, TRUE, 0);
    gtk_widget_show(scrolledWindow);

    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
}

static void browserSettingsDialogConstructed(GObject *object)
{
    BrowserSettingsDialog *dialog = BROWSER_SETTINGS_DIALOG(object);
    WebKitSettings *settings = dialog->settings;
    GtkListStore *model = gtk_list_store_new(SETTINGS_LIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                             G_TYPE_VALUE, G_TYPE_OBJECT);
    guint propertiesCount;
    GParamSpec **properties = g_object_class_list_properties(G_OBJECT_GET_CLASS(settings), &propertiesCount);
    guint i;
    for (i = 0; i < propertiesCount; i++) {
        GParamSpec *property = properties[i];
        const char *name = g_param_spec_get_name(property);
        const char *nick = g_param_spec_get_nick(property);

        GValue value = { 0, { { 0 } } };
        g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(property));
        g_object_get_property(G_OBJECT(settings), name, &value);

        GtkAdjustment *adjustment = NULL;
        if (G_PARAM_SPEC_VALUE_TYPE(property) == G_TYPE_UINT) {
            GParamSpecUInt *uIntProperty = G_PARAM_SPEC_UINT(property);
            adjustment = gtk_adjustment_new(uIntProperty->default_value, uIntProperty->minimum,
                                            uIntProperty->maximum, 1, 1, 1);
        }

        char *blurb = g_markup_escape_text(g_param_spec_get_blurb(property), -1);
        GtkTreeIter iter;
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter,
                           SETTINGS_LIST_COLUMN_NAME, name,
                           SETTINGS_LIST_COLUMN_NICK, nick,
                           SETTINGS_LIST_COLUMN_BLURB, blurb,
                           SETTINGS_LIST_COLUMN_VALUE, &value,
                           SETTINGS_LIST_COLUMN_ADJUSTMENT, adjustment,
                           -1);
        g_free(blurb);
        g_value_unset(&value);
    }
    g_free(properties);

    gtk_tree_view_set_model(GTK_TREE_VIEW(dialog->settingsList), GTK_TREE_MODEL(model));
    g_object_unref(model);
}

static void browser_settings_dialog_class_init(BrowserSettingsDialogClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);

    gobjectClass->constructed = browserSettingsDialogConstructed;
    gobjectClass->set_property = browserSettingsDialogSetProperty;

    g_object_class_install_property(gobjectClass,
                                    PROP_SETTINGS,
                                    g_param_spec_object("settings",
                                                        "Settings",
                                                        "The WebKitSettings",
                                                        WEBKIT_TYPE_SETTINGS,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

GtkWidget *browser_settings_dialog_new(WebKitSettings *settings)
{
    g_return_val_if_fail(WEBKIT_IS_SETTINGS(settings), NULL);

    return GTK_WIDGET(g_object_new(BROWSER_TYPE_SETTINGS_DIALOG, "settings", settings, NULL));
}

