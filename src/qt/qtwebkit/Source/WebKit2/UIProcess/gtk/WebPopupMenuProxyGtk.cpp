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

#include "config.h"
#include "WebPopupMenuProxyGtk.h"

#include "NativeWebMouseEvent.h"
#include "WebPopupItem.h"
#include <WebCore/GtkUtilities.h>
#include <gtk/gtk.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

WebPopupMenuProxyGtk::WebPopupMenuProxyGtk(GtkWidget* webView, WebPopupMenuProxy::Client* client)
    : WebPopupMenuProxy(client)
    , m_webView(webView)
    , m_activeItem(-1)
{
}

WebPopupMenuProxyGtk::~WebPopupMenuProxyGtk()
{
    if (m_popup) {
        g_signal_handlers_disconnect_matched(m_popup->platformMenu(), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        hidePopupMenu();
    }
}

GtkAction* WebPopupMenuProxyGtk::createGtkActionForMenuItem(const WebPopupItem& item, int itemIndex)
{
    GOwnPtr<char> actionName(g_strdup_printf("popup-menu-action-%d", itemIndex));
    GtkAction* action = gtk_action_new(actionName.get(), item.m_text.utf8().data(), item.m_toolTip.utf8().data(), 0);
    g_object_set_data(G_OBJECT(action), "popup-menu-action-index", GINT_TO_POINTER(itemIndex));
    g_signal_connect(action, "activate", G_CALLBACK(menuItemActivated), this);
    gtk_action_set_sensitive(action, item.m_isEnabled);

    return action;
}

void WebPopupMenuProxyGtk::showPopupMenu(const IntRect& rect, TextDirection textDirection, double pageScaleFactor, const Vector<WebPopupItem>& items, const PlatformPopupMenuData& data, int32_t selectedIndex)
{
    if (m_popup)
        m_popup->clear();
    else
        m_popup = GtkPopupMenu::create();

    const int size = items.size();
    for (int i = 0; i < size; i++) {
        if (items[i].m_type == WebPopupItem::Separator)
            m_popup->appendSeparator();
        else {
            GRefPtr<GtkAction> action = adoptGRef(createGtkActionForMenuItem(items[i], i));
            m_popup->appendItem(action.get());
        }
    }

    IntPoint menuPosition = convertWidgetPointToScreenPoint(m_webView, rect.location());
    menuPosition.move(0, rect.height());

    gulong unmapHandler = g_signal_connect(m_popup->platformMenu(), "unmap", G_CALLBACK(menuUnmapped), this);
    m_popup->popUp(rect.size(), menuPosition, size, selectedIndex, m_client->currentlyProcessedMouseDownEvent() ? m_client->currentlyProcessedMouseDownEvent()->nativeEvent() : 0);

    // PopupMenu can fail to open when there is no mouse grab.
    // Ensure WebCore does not go into some pesky state.
    if (!gtk_widget_get_visible(m_popup->platformMenu())) {
       m_client->failedToShowPopupMenu();
       return;
    }

    // WebPageProxy expects the menu to run in a nested run loop, since it invalidates the
    // menu right after calling WebPopupMenuProxy::showPopupMenu().
    m_runLoop = adoptGRef(g_main_loop_new(0, FALSE));

    gdk_threads_leave();
    g_main_loop_run(m_runLoop.get());
    gdk_threads_enter();

    m_runLoop.clear();

    g_signal_handler_disconnect(m_popup->platformMenu(), unmapHandler);

    if (!m_client)
        return;

    m_client->valueChangedForPopupMenu(this, m_activeItem);
}

void WebPopupMenuProxyGtk::hidePopupMenu()
{
    m_popup->popDown();
}

void WebPopupMenuProxyGtk::shutdownRunLoop()
{
    if (g_main_loop_is_running(m_runLoop.get()))
        g_main_loop_quit(m_runLoop.get());
}

void WebPopupMenuProxyGtk::menuItemActivated(GtkAction* action, WebPopupMenuProxyGtk* popupMenu)
{
    popupMenu->setActiveItem(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "popup-menu-action-index")));
    popupMenu->shutdownRunLoop();
}

void WebPopupMenuProxyGtk::menuUnmapped(GtkWidget*, WebPopupMenuProxyGtk* popupMenu)
{
    popupMenu->shutdownRunLoop();
}

} // namespace WebKit
