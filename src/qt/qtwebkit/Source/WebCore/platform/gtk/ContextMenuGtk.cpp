/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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

#if ENABLE(CONTEXT_MENUS)

#include "ContextMenu.h"

#include <gtk/gtk.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>

namespace WebCore {

ContextMenu::ContextMenu()
{
    m_platformDescription = GTK_MENU(gtk_menu_new());
}

ContextMenu::ContextMenu(const PlatformMenuDescription menu)
    : m_platformDescription(menu)
{
}

ContextMenu::~ContextMenu()
{
    if (m_platformDescription)
        gtk_widget_destroy(GTK_WIDGET(m_platformDescription));
}

void ContextMenu::appendItem(ContextMenuItem& item)
{
    ASSERT(m_platformDescription);

    GRefPtr<GtkWidget> platformItem = GTK_WIDGET(item.releasePlatformDescription());
    ASSERT(platformItem);

    if (GtkWidget* parent = gtk_widget_get_parent(platformItem.get()))
        gtk_container_remove(GTK_CONTAINER(parent), platformItem.get());

    gtk_menu_shell_append(GTK_MENU_SHELL(m_platformDescription), platformItem.get());
    gtk_widget_show(platformItem.get());
}

void ContextMenu::setPlatformDescription(PlatformMenuDescription menu)
{
    ASSERT(menu);
    if (m_platformDescription == menu)
        return;
    if (m_platformDescription)
        gtk_widget_destroy(GTK_WIDGET(m_platformDescription));

    m_platformDescription = menu;
}

PlatformMenuDescription ContextMenu::platformDescription() const
{
    return m_platformDescription;
}

PlatformMenuDescription ContextMenu::releasePlatformDescription()
{
    PlatformMenuDescription description = m_platformDescription;
    m_platformDescription = 0;

    return description;
}

unsigned ContextMenu::itemCount() const
{
    ASSERT(m_platformDescription);

    GOwnPtr<GList> children(gtk_container_get_children(GTK_CONTAINER(m_platformDescription)));
    return g_list_length(children.get());
}

Vector<ContextMenuItem> contextMenuItemVector(const PlatformMenuDescription menu)
{
    Vector<ContextMenuItem> menuItemVector;

    GOwnPtr<GList> children(gtk_container_get_children(GTK_CONTAINER(menu)));
    int itemCount = g_list_length(children.get());
    menuItemVector.reserveCapacity(itemCount);

    for (GList* item = children.get(); item; item = g_list_next(item)) {
        GtkWidget* widget = static_cast<GtkWidget*>(item->data);
        if (!GTK_IS_MENU_ITEM(widget))
            continue;
        menuItemVector.append(GTK_MENU_ITEM(widget));
    }

    return menuItemVector;
}

PlatformMenuDescription platformMenuDescription(Vector<ContextMenuItem>& subMenuItems)
{
    GtkMenu* menu = GTK_MENU(gtk_menu_new());
    for (size_t i = 0; i < subMenuItems.size(); i++) {
        GtkWidget* platformItem = GTK_WIDGET(subMenuItems[i].releasePlatformDescription());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), platformItem);
        gtk_widget_show(platformItem);
    }
    return menu;
}

}

#endif // ENABLE(CONTEXT_MENUS)
