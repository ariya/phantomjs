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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS AS IS''
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

#include "config.h"
#include "WebContextMenuProxyGtk.h"

#if ENABLE(CONTEXT_MENUS)

#include "NativeWebMouseEvent.h"
#include "WebContextMenuItemData.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebPageProxy.h"
#include <WebCore/GtkUtilities.h>
#include <gtk/gtk.h>
#include <wtf/text/CString.h>


static const char* gContextMenuActionId = "webkit-context-menu-action";

using namespace WebCore;

namespace WebKit {

static void contextMenuItemActivatedCallback(GtkAction* action, WebPageProxy* page)
{
    gboolean isToggle = GTK_IS_TOGGLE_ACTION(action);
    WebKit::WebContextMenuItemData item(isToggle ? WebCore::CheckableActionType : WebCore::ActionType,
        static_cast<WebCore::ContextMenuAction>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), gContextMenuActionId))),
        String::fromUTF8(gtk_action_get_label(action)), gtk_action_get_sensitive(action),
        isToggle ? gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)) : false);
    page->contextMenuItemSelected(item);
}

static void contextMenuItemVisibilityChanged(GtkAction* action, GParamSpec*, WebContextMenuProxyGtk* contextMenuProxy)
{
    GtkMenu* menu = contextMenuProxy->gtkMenu();
    if (!menu)
        return;

    GOwnPtr<GList> items(gtk_container_get_children(GTK_CONTAINER(menu)));
    bool previousVisibleItemIsNotASeparator = false;
    GtkWidget* lastItemVisibleSeparator = 0;
    for (GList* iter = items.get(); iter; iter = g_list_next(iter)) {
        GtkWidget* widget = GTK_WIDGET(iter->data);

        if (GTK_IS_SEPARATOR_MENU_ITEM(widget)) {
            if (previousVisibleItemIsNotASeparator) {
                gtk_widget_show(widget);
                lastItemVisibleSeparator = widget;
                previousVisibleItemIsNotASeparator = false;
            } else
                gtk_widget_hide(widget);
        } else if (gtk_widget_get_visible(widget)) {
            lastItemVisibleSeparator = 0;
            previousVisibleItemIsNotASeparator = true;
        }
    }

    if (lastItemVisibleSeparator)
        gtk_widget_hide(lastItemVisibleSeparator);
}

void WebContextMenuProxyGtk::append(ContextMenuItem& menuItem)
{
    GtkAction* action = menuItem.gtkAction();
    if (action) {
        switch (menuItem.type()) {
        case ActionType:
        case CheckableActionType:
            g_object_set_data(G_OBJECT(action), gContextMenuActionId, GINT_TO_POINTER(menuItem.action()));
            g_signal_connect(action, "activate", G_CALLBACK(contextMenuItemActivatedCallback), m_page);
            // Fall through.
        case SubmenuType:
            g_signal_connect(action, "notify::visible", G_CALLBACK(contextMenuItemVisibilityChanged), this);
            break;
        case SeparatorType:
            break;
        }
    }

    m_menu.appendItem(menuItem);
}

// Populate the context menu ensuring that:
//  - There aren't separators next to each other.
//  - There aren't separators at the beginning of the menu.
//  - There aren't separators at the end of the menu.
void WebContextMenuProxyGtk::populate(Vector<ContextMenuItem>& items)
{
    bool previousIsSeparator = false;
    bool isEmpty = true;
    for (size_t i = 0; i < items.size(); i++) {
        ContextMenuItem& menuItem = items.at(i);
        if (menuItem.type() == SeparatorType) {
            previousIsSeparator = true;
            continue;
        }

        if (previousIsSeparator && !isEmpty)
            append(items.at(i - 1));
        previousIsSeparator = false;

        append(menuItem);
        isEmpty = false;
    }
}

void WebContextMenuProxyGtk::populate(const Vector<WebContextMenuItemData>& items)
{
    for (size_t i = 0; i < items.size(); i++) {
        ContextMenuItem menuitem = items.at(i).core();
        append(menuitem);
    }
}

void WebContextMenuProxyGtk::showContextMenu(const WebCore::IntPoint& position, const Vector<WebContextMenuItemData>& items)
{
    if (!items.isEmpty())
        populate(items);

    if (!m_menu.itemCount())
        return;

    m_popupPosition = convertWidgetPointToScreenPoint(m_webView, position);

    // Display menu initiated by right click (mouse button pressed = 3).
    NativeWebMouseEvent* mouseEvent = m_page->currentlyProcessedMouseDownEvent();
    const GdkEvent* event = mouseEvent ? mouseEvent->nativeEvent() : 0;
    gtk_menu_attach_to_widget(m_menu.platformDescription(), GTK_WIDGET(m_webView), 0);
    gtk_menu_popup(m_menu.platformDescription(), 0, 0, reinterpret_cast<GtkMenuPositionFunc>(menuPositionFunction), this,
                   event ? event->button.button : 3, event ? event->button.time : GDK_CURRENT_TIME);
}

void WebContextMenuProxyGtk::hideContextMenu()
{
    gtk_menu_popdown(m_menu.platformDescription());
}

WebContextMenuProxyGtk::WebContextMenuProxyGtk(GtkWidget* webView, WebPageProxy* page)
    : m_webView(webView)
    , m_page(page)
{
    webkitWebViewBaseSetActiveContextMenuProxy(WEBKIT_WEB_VIEW_BASE(m_webView), this);
}

WebContextMenuProxyGtk::~WebContextMenuProxyGtk()
{
    webkitWebViewBaseSetActiveContextMenuProxy(WEBKIT_WEB_VIEW_BASE(m_webView), 0);
}

void WebContextMenuProxyGtk::menuPositionFunction(GtkMenu* menu, gint* x, gint* y, gboolean* pushIn, WebContextMenuProxyGtk* popupMenu)
{
    GtkRequisition menuSize;
    gtk_widget_get_preferred_size(GTK_WIDGET(menu), &menuSize, 0);

    GdkScreen* screen = gtk_widget_get_screen(popupMenu->m_webView);
    *x = popupMenu->m_popupPosition.x();
    if ((*x + menuSize.width) >= gdk_screen_get_width(screen))
        *x -= menuSize.width;

    *y = popupMenu->m_popupPosition.y();
    if ((*y + menuSize.height) >= gdk_screen_get_height(screen))
        *y -= menuSize.height;

    *pushIn = FALSE;
}

} // namespace WebKit
#endif // ENABLE(CONTEXT_MENUS)
