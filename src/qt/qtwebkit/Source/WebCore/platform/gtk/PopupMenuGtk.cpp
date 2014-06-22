/*
 * This file is part of the popup menu implementation for <select> elements in WebCore.
 *
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010 Igalia S.L.
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
 *
 */

#include "config.h"
#include "PopupMenuGtk.h"

#include "FrameView.h"
#include <wtf/gobject/GOwnPtr.h>
#include "GtkUtilities.h"
#include "HostWindow.h"
#include <gtk/gtk.h>
#include <wtf/text/CString.h>

namespace WebCore {

static const uint32_t gSearchTimeoutMs = 1000;

PopupMenuGtk::PopupMenuGtk(PopupMenuClient* client)
    : m_popupClient(client)
{
}

PopupMenuGtk::~PopupMenuGtk()
{
    if (m_popup) {
        g_signal_handlers_disconnect_matched(m_popup->platformMenu(), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        hide();
    }
}

GtkAction* PopupMenuGtk::createGtkActionForMenuItem(int itemIndex)
{
    GOwnPtr<char> actionName(g_strdup_printf("popup-menu-action-%d", itemIndex));
    GtkAction* action = gtk_action_new(actionName.get(), client()->itemText(itemIndex).utf8().data(), client()->itemToolTip(itemIndex).utf8().data(), 0);
    g_object_set_data(G_OBJECT(action), "popup-menu-action-index", GINT_TO_POINTER(itemIndex));
    g_signal_connect(action, "activate", G_CALLBACK(menuItemActivated), this);
    // FIXME: Apply the PopupMenuStyle from client()->itemStyle(i)
    gtk_action_set_visible(action, !client()->itemStyle(itemIndex).isDisplayNone());
    gtk_action_set_sensitive(action, client()->itemIsEnabled(itemIndex));

    return action;
}

void PopupMenuGtk::show(const IntRect& rect, FrameView* view, int index)
{
    ASSERT(client());

    if (!m_popup) {
        m_popup = GtkPopupMenu::create();
        g_signal_connect(m_popup->platformMenu(), "unmap", G_CALLBACK(PopupMenuGtk::menuUnmapped), this);
    } else
        m_popup->clear();

    const int size = client()->listSize();
    for (int i = 0; i < size; ++i) {
        if (client()->itemIsSeparator(i))
            m_popup->appendSeparator();
        else {
            GRefPtr<GtkAction> action = adoptGRef(createGtkActionForMenuItem(i));
            m_popup->appendItem(action.get());
        }
    }

    IntPoint menuPosition = convertWidgetPointToScreenPoint(GTK_WIDGET(view->hostWindow()->platformPageClient()), view->contentsToWindow(rect.location()));
    menuPosition.move(0, rect.height());

    m_popup->popUp(rect.size(), menuPosition, size, index, gtk_get_current_event());

    // GTK can refuse to actually open the menu when mouse grabs fails.
    // Ensure WebCore does not go into some pesky state.
    if (!gtk_widget_get_visible(m_popup->platformMenu()))
        client()->popupDidHide();
}

void PopupMenuGtk::hide()
{
    ASSERT(m_popup);
    m_popup->popDown();
}

void PopupMenuGtk::updateFromElement()
{
    client()->setTextFromItem(client()->selectedIndex());
}

void PopupMenuGtk::disconnectClient()
{
    m_popupClient = 0;
}

void PopupMenuGtk::menuItemActivated(GtkAction* action, PopupMenuGtk* that)
{
    ASSERT(that->client());
    that->client()->valueChanged(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "popup-menu-action-index")));
}

void PopupMenuGtk::menuUnmapped(GtkWidget*, PopupMenuGtk* that)
{
    ASSERT(that->client());
    that->client()->popupDidHide();
}

}

