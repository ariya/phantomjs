/*
 * This file is part of the popup menu implementation for <select> elements in WebCore.
 *
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010-2011 Igalia S.L.
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
#include "GtkPopupMenu.h"

#include <wtf/gobject/GOwnPtr.h>
#include "GtkVersioning.h"
#include <gtk/gtk.h>
#include <wtf/text/CString.h>

namespace WebCore {

static const uint32_t gSearchTimeoutMs = 1000;

GtkPopupMenu::GtkPopupMenu()
    : m_popup(gtk_menu_new())
    , m_previousKeyEventCharacter(0)
    , m_currentlySelectedMenuItem(0)
{
    m_keyPressHandlerID = g_signal_connect(m_popup.get(), "key-press-event", G_CALLBACK(GtkPopupMenu::keyPressEventCallback), this);
}

GtkPopupMenu::~GtkPopupMenu()
{
    g_signal_handler_disconnect(m_popup.get(), m_keyPressHandlerID);
}

void GtkPopupMenu::clear()
{
    gtk_container_foreach(GTK_CONTAINER(m_popup.get()), reinterpret_cast<GtkCallback>(menuRemoveItem), this);
}

void GtkPopupMenu::appendSeparator()
{
    GtkWidget* menuItem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(m_popup.get()), menuItem);
    gtk_widget_show(menuItem);
}

void GtkPopupMenu::appendItem(GtkAction* action)
{
    GtkWidget* menuItem = gtk_action_create_menu_item(action);
    gtk_widget_set_tooltip_text(menuItem, gtk_action_get_tooltip(action));
    g_signal_connect(menuItem, "select", G_CALLBACK(GtkPopupMenu::selectItemCallback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(m_popup.get()), menuItem);

    if (gtk_action_is_visible(action))
        gtk_widget_show(menuItem);
}

void GtkPopupMenu::popUp(const IntSize& menuSize, const IntPoint& menuPosition, int itemCount, int selectedItem, const GdkEvent* event)
{
    resetTypeAheadFindState();
    m_menuPosition = menuPosition;
    gtk_menu_set_active(GTK_MENU(m_popup.get()), selectedItem);

    // This approach follows the one in gtkcombobox.c.
    GtkRequisition requisition;
    gtk_widget_set_size_request(m_popup.get(), -1, -1);
#ifdef GTK_API_VERSION_2
    gtk_widget_size_request(m_popup.get(), &requisition);
#else
    gtk_widget_get_preferred_size(m_popup.get(), &requisition, 0);
#endif

    gtk_widget_set_size_request(m_popup.get(), std::max(menuSize.width(), requisition.width), -1);

    GList* children = gtk_container_get_children(GTK_CONTAINER(m_popup.get()));
    GList* p = children;
    if (itemCount) {
        for (int i = 0; i < itemCount; i++) {
            if (i > selectedItem)
                break;

            GtkWidget* item = reinterpret_cast<GtkWidget*>(p->data);
            GtkRequisition itemRequisition;
#ifdef GTK_API_VERSION_2
            gtk_widget_get_child_requisition(item, &itemRequisition);
#else
            gtk_widget_get_preferred_size(item, &itemRequisition, 0);
#endif
            m_menuPosition.setY(m_menuPosition.y() - itemRequisition.height);

            p = g_list_next(p);
        }
    } else {
        // Center vertically the empty popup in the combo box area.
        m_menuPosition.setY(m_menuPosition.y() - menuSize.height() / 2);
    }
    g_list_free(children);

    guint button;
    guint32 activateTime;
    if (event) {
        button = event->type == GDK_BUTTON_PRESS ? event->button.button : 1;
        activateTime = gdk_event_get_time(event);
    } else {
        button = 1;
        activateTime = GDK_CURRENT_TIME;
    }

#ifdef GTK_API_VERSION_2
    gtk_menu_popup(GTK_MENU(m_popup.get()), 0, 0, reinterpret_cast<GtkMenuPositionFunc>(menuPositionFunction), this, button, activateTime);
#else
    gtk_menu_popup_for_device(GTK_MENU(m_popup.get()), event ? gdk_event_get_device(event) : 0, 0, 0,
                              reinterpret_cast<GtkMenuPositionFunc>(menuPositionFunction), this, 0, button, activateTime);
#endif
}

void GtkPopupMenu::popDown()
{
    gtk_menu_popdown(GTK_MENU(m_popup.get()));
    resetTypeAheadFindState();
}

void GtkPopupMenu::menuRemoveItem(GtkWidget* widget, GtkPopupMenu* popupMenu)
{
    ASSERT(popupMenu->m_popup);
    gtk_container_remove(GTK_CONTAINER(popupMenu->m_popup.get()), widget);
}

void GtkPopupMenu::menuPositionFunction(GtkMenu*, gint* x, gint* y, gboolean* pushIn, GtkPopupMenu* popupMenu)
{
    *x = popupMenu->m_menuPosition.x();
    *y = popupMenu->m_menuPosition.y();
    *pushIn = true;
}

void GtkPopupMenu::resetTypeAheadFindState()
{
    m_currentlySelectedMenuItem = 0;
    m_previousKeyEventCharacter = 0;
    m_currentSearchString = "";
}

bool GtkPopupMenu::typeAheadFind(GdkEventKey* event)
{
    // If we were given a non-printable character just skip it.
    gunichar unicodeCharacter = gdk_keyval_to_unicode(event->keyval);
    if (!g_unichar_isprint(unicodeCharacter)) {
        resetTypeAheadFindState();
        return false;
    }

    glong charactersWritten;
    GOwnPtr<gunichar2> utf16String(g_ucs4_to_utf16(&unicodeCharacter, 1, 0, &charactersWritten, 0));
    if (!utf16String) {
        resetTypeAheadFindState();
        return false;
    }

    // If the character is the same as the last character, the user is probably trying to
    // cycle through the menulist entries. This matches the WebCore behavior for collapsed
    // menulists.
    bool repeatingCharacter = unicodeCharacter != m_previousKeyEventCharacter;
    if (event->time - m_previousKeyEventTimestamp > gSearchTimeoutMs)
        m_currentSearchString = String(reinterpret_cast<UChar*>(utf16String.get()), charactersWritten);
    else if (repeatingCharacter)
        m_currentSearchString.append(String(reinterpret_cast<UChar*>(utf16String.get()), charactersWritten));

    m_previousKeyEventTimestamp = event->time;
    m_previousKeyEventCharacter = unicodeCharacter;

    // Like the Chromium port, we case fold before searching, because 
    // strncmp does not handle non-ASCII characters.
    GOwnPtr<gchar> searchStringWithCaseFolded(g_utf8_casefold(m_currentSearchString.utf8().data(), -1));
    size_t prefixLength = strlen(searchStringWithCaseFolded.get());

    GList* children = gtk_container_get_children(GTK_CONTAINER(m_popup.get()));
    if (!children)
        return true;

    // If a menu item has already been selected, start searching from the current
    // item down the list. This will make multiple key presses of the same character
    // advance the selection.
    GList* currentChild = children;
    if (m_currentlySelectedMenuItem) {
        currentChild = g_list_find(children, m_currentlySelectedMenuItem);
        if (!currentChild) {
            m_currentlySelectedMenuItem = 0;
            currentChild = children;
        }

        // Repeating characters should iterate.
        if (repeatingCharacter) {
            if (GList* nextChild = g_list_next(currentChild))
                currentChild = nextChild;
        }
    }

    GList* firstChild = currentChild;
    do {
        currentChild = g_list_next(currentChild);
        if (!currentChild)
            currentChild = children;

        GOwnPtr<gchar> itemText(g_utf8_casefold(gtk_menu_item_get_label(GTK_MENU_ITEM(currentChild->data)), -1));
        if (!strncmp(searchStringWithCaseFolded.get(), itemText.get(), prefixLength)) {
            gtk_menu_shell_select_item(GTK_MENU_SHELL(m_popup.get()), GTK_WIDGET(currentChild->data));
            break;
        }
    } while (currentChild != firstChild);

    g_list_free(children);
    return true;
}

void GtkPopupMenu::selectItemCallback(GtkMenuItem* item, GtkPopupMenu* popupMenu)
{
    popupMenu->m_currentlySelectedMenuItem = GTK_WIDGET(item);
}

gboolean GtkPopupMenu::keyPressEventCallback(GtkWidget* widget, GdkEventKey* event, GtkPopupMenu* popupMenu)
{
    return popupMenu->typeAheadFind(event);
}

} // namespace WebCore
