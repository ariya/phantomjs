/*
 * Copyright (C) 2011 Igalia S.L.
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

#ifndef WebPopupMenuProxyGtk_h
#define WebPopupMenuProxyGtk_h

#include "WebPopupMenuProxy.h"
#include <WebCore/GtkPopupMenu.h>
#include <WebCore/IntRect.h>
#include <wtf/gobject/GRefPtr.h>

typedef struct _GMainLoop GMainLoop;

namespace WebKit {

class WebPageProxy;

class WebPopupMenuProxyGtk : public WebPopupMenuProxy {
public:
    static PassRefPtr<WebPopupMenuProxyGtk> create(GtkWidget* webView, WebPopupMenuProxy::Client* client)
    {
        return adoptRef(new WebPopupMenuProxyGtk(webView, client));
    }
    ~WebPopupMenuProxyGtk();

    virtual void showPopupMenu(const WebCore::IntRect&, WebCore::TextDirection, double pageScaleFactor, const Vector<WebPopupItem>&, const PlatformPopupMenuData&, int32_t selectedIndex);
    virtual void hidePopupMenu();

private:
    WebPopupMenuProxyGtk(GtkWidget*, WebPopupMenuProxy::Client*);
    void shutdownRunLoop();
    void setActiveItem(int activeItem) { m_activeItem = activeItem; }
    GtkAction* createGtkActionForMenuItem(const WebPopupItem&, int itemIndex);

    static void menuItemActivated(GtkAction*, WebPopupMenuProxyGtk*);
    static void menuUnmapped(GtkWidget*, WebPopupMenuProxyGtk*);

    GtkWidget* m_webView;
    OwnPtr<WebCore::GtkPopupMenu> m_popup;
    int m_activeItem;
    GRefPtr<GMainLoop> m_runLoop;
};

} // namespace WebKit


#endif // WebPopupMenuProxyGtk_h
