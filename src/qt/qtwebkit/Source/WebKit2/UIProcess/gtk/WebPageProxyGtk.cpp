/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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
#include "WebPageProxy.h"

#include "NativeWebKeyboardEvent.h"
#include "NotImplemented.h"
#include "PageClientImpl.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebPageMessages.h"
#include "WebProcessProxy.h"
#include <WebCore/UserAgentGtk.h>
#include <gtk/gtkx.h>

namespace WebKit {

GtkWidget* WebPageProxy::viewWidget()
{
    return static_cast<PageClientImpl*>(m_pageClient)->viewWidget();
}

String WebPageProxy::standardUserAgent(const String& applicationNameForUserAgent)
{
    return WebCore::standardUserAgent(applicationNameForUserAgent);
}

void WebPageProxy::getEditorCommandsForKeyEvent(const AtomicString& eventType, Vector<WTF::String>& commandsList)
{
    // When the keyboard event is started in the WebProcess side (e.g. from the Inspector)
    // it will arrive without a GdkEvent associated, so the keyEventQueue will be empty.
    if (!m_keyEventQueue.isEmpty())
        m_pageClient->getEditorCommandsForKeyEvent(m_keyEventQueue.first(), eventType, commandsList);
}

void WebPageProxy::bindAccessibilityTree(const String& plugID)
{
    m_accessibilityPlugID = plugID;
}

void WebPageProxy::saveRecentSearches(const String&, const Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::loadRecentSearches(const String&, Vector<String>&)
{
    notImplemented();
}

typedef HashMap<uint64_t, GtkWidget* > PluginWindowMap;
static PluginWindowMap& pluginWindowMap()
{
    DEFINE_STATIC_LOCAL(PluginWindowMap, map, ());
    return map;
}

static gboolean pluginContainerPlugRemoved(GtkSocket* socket)
{
    uint64_t windowID = static_cast<uint64_t>(gtk_socket_get_id(socket));
    pluginWindowMap().remove(windowID);
    return FALSE;
}

void WebPageProxy::createPluginContainer(uint64_t& windowID)
{
    GtkWidget* socket = gtk_socket_new();
    g_signal_connect(socket, "plug-removed", G_CALLBACK(pluginContainerPlugRemoved), 0);
    gtk_container_add(GTK_CONTAINER(viewWidget()), socket);
    gtk_widget_show(socket);

    windowID = static_cast<uint64_t>(gtk_socket_get_id(GTK_SOCKET(socket)));
    pluginWindowMap().set(windowID, socket);
}

void WebPageProxy::windowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID)
{
    GtkWidget* plugin = pluginWindowMap().get(windowID);
    if (!plugin)
        return;

    if (gtk_widget_get_realized(plugin)) {
        GdkRectangle clip = clipRect;
        cairo_region_t* clipRegion = cairo_region_create_rectangle(&clip);
        gdk_window_shape_combine_region(gtk_widget_get_window(plugin), clipRegion, 0, 0);
        cairo_region_destroy(clipRegion);
    }

    webkitWebViewBaseChildMoveResize(WEBKIT_WEB_VIEW_BASE(viewWidget()), plugin, frameRect);
}

void WebPageProxy::setInputMethodState(bool enabled)
{
    webkitWebViewBaseSetInputMethodState(WEBKIT_WEB_VIEW_BASE(viewWidget()), enabled);
}

#if USE(TEXTURE_MAPPER_GL)
void WebPageProxy::setAcceleratedCompositingWindowId(uint64_t nativeWindowId)
{
    process()->send(Messages::WebPage::SetAcceleratedCompositingWindowId(nativeWindowId), m_pageID);
}
#endif

} // namespace WebKit
