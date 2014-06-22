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
#include "WebKitWebViewBaseAccessible.h"

#include "WebKitPrivate.h"
#include <gtk/gtk.h>

struct _WebKitWebViewBaseAccessiblePrivate {
    GtkWidget* widget;
};

WEBKIT_DEFINE_TYPE(WebKitWebViewBaseAccessible, webkit_web_view_base_accessible, ATK_TYPE_SOCKET)

static void webkitWebViewBaseAccessibleWidgetDestroyed(GtkWidget* widget, WebKitWebViewBaseAccessible* accessible)
{
    accessible->priv->widget = 0;
    atk_object_notify_state_change(ATK_OBJECT(accessible), ATK_STATE_DEFUNCT, TRUE);
}

static void webkitWebViewBaseAccessibleInitialize(AtkObject* atkObject, gpointer data)
{
    if (ATK_OBJECT_CLASS(webkit_web_view_base_accessible_parent_class)->initialize)
        ATK_OBJECT_CLASS(webkit_web_view_base_accessible_parent_class)->initialize(atkObject, data);

    if (data && GTK_IS_WIDGET(data)) {
        WebKitWebViewBaseAccessible* accessible = WEBKIT_WEB_VIEW_BASE_ACCESSIBLE(atkObject);
        accessible->priv->widget = GTK_WIDGET(data);

        g_signal_connect_after(accessible->priv->widget, "destroy",
                               G_CALLBACK(webkitWebViewBaseAccessibleWidgetDestroyed), atkObject);
    }

    atk_object_set_role(atkObject, ATK_ROLE_FILLER);
}

static AtkStateSet* webkitWebViewBaseAccessibleRefStateSet(AtkObject* atkObject)
{
    WebKitWebViewBaseAccessible* accessible = WEBKIT_WEB_VIEW_BASE_ACCESSIBLE(atkObject);

    AtkStateSet* stateSet;
    if (accessible->priv->widget) {
        // Use the implementation of AtkSocket if the widget is still alive.
        stateSet = ATK_OBJECT_CLASS(webkit_web_view_base_accessible_parent_class)->ref_state_set(atkObject);
    } else {
        // If the widget is no longer alive, save some remote calls
        // (because of AtkSocket's implementation of ref_state_set())
        // and just return that this AtkObject is defunct.
        stateSet = atk_state_set_new();
        atk_state_set_add_state(stateSet, ATK_STATE_DEFUNCT);
    }

    return stateSet;
}

static gint webkitWebViewBaseAccessibleGetIndexInParent(AtkObject* atkObject)
{
    AtkObject* atkParent = atk_object_get_parent(atkObject);
    if (!atkParent)
        return -1;

    guint count = atk_object_get_n_accessible_children(atkParent);
    for (guint i = 0; i < count; ++i) {
        AtkObject* child = atk_object_ref_accessible_child(atkParent, i);
        bool childIsObject = child == atkObject;
        g_object_unref(child);
        if (childIsObject)
            return i;
    }

    return -1;
}

static void webkit_web_view_base_accessible_class_init(WebKitWebViewBaseAccessibleClass* klass)
{
    // No need to implement get_n_children() and ref_child() here
    // since this is a subclass of AtkSocket and all the logic related
    // to those functions will be implemented by the ATK bridge.
    AtkObjectClass* atkObjectClass = ATK_OBJECT_CLASS(klass);
    atkObjectClass->initialize = webkitWebViewBaseAccessibleInitialize;
    atkObjectClass->ref_state_set = webkitWebViewBaseAccessibleRefStateSet;
    atkObjectClass->get_index_in_parent = webkitWebViewBaseAccessibleGetIndexInParent;
}

WebKitWebViewBaseAccessible* webkitWebViewBaseAccessibleNew(GtkWidget* widget)
{
    AtkObject* object = ATK_OBJECT(g_object_new(WEBKIT_TYPE_WEB_VIEW_BASE_ACCESSIBLE, NULL));
    atk_object_initialize(object, widget);
    return WEBKIT_WEB_VIEW_BASE_ACCESSIBLE(object);
}
