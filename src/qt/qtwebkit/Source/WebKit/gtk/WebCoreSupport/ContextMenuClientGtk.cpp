/*
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
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

#include "ContextMenuClientGtk.h"

#include "ContextMenu.h"
#include "ContextMenuController.h"
#include "HitTestResult.h"
#include "KURL.h"
#include "LocalizedStrings.h"
#include "NotImplemented.h"
#include "Page.h"
#include "webkitwebviewprivate.h"
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

ContextMenuClient::ContextMenuClient(WebKitWebView *webView)
    : m_webView(webView)
{
}

void ContextMenuClient::contextMenuDestroyed()
{
    delete this;
}

static GtkWidget* inputMethodsMenuItem (WebKitWebView* webView)
{
    if (gtk_major_version > 2 || (gtk_major_version == 2 && gtk_minor_version >= 10)) {
        GtkSettings* settings = webView ? gtk_widget_get_settings(GTK_WIDGET(webView)) : gtk_settings_get_default();

        gboolean showMenu = TRUE;
        if (settings)
            g_object_get(settings, "gtk-show-input-method-menu", &showMenu, NULL);
        if (!showMenu)
            return 0;
    }

    WebKitWebViewPrivate* priv = webView->priv;
    ContextMenu imContextMenu;
    gtk_im_multicontext_append_menuitems(GTK_IM_MULTICONTEXT(priv->imFilter.context()), GTK_MENU_SHELL(imContextMenu.platformDescription()));

    ContextMenuItem menuItem(ActionType, ContextMenuItemTagInputMethods, contextMenuItemTagInputMethods(), &imContextMenu);
    imContextMenu.releasePlatformDescription();

    return GTK_WIDGET(menuItem.releasePlatformDescription());
}

static int getUnicodeMenuItemPosition(GtkMenu* menu)
{
    GOwnPtr<GList> items(gtk_container_get_children(GTK_CONTAINER(menu)));
    int unicodeMenuItemPosition = -1;
    GList* iter;
    int i = 0;
    for (iter = items.get(), i = 0; iter; iter = g_list_next(iter), ++i) {
        GtkMenuItem* item = GTK_MENU_ITEM(iter->data);
        if (GTK_IS_SEPARATOR_MENU_ITEM(item))
            continue;
        if (String::fromUTF8(gtk_menu_item_get_label(item)) == contextMenuItemTagUnicode()) {
            unicodeMenuItemPosition = i;
            break;
        }
    }
    return unicodeMenuItemPosition;
}

PlatformMenuDescription ContextMenuClient::getCustomMenuFromDefaultItems(ContextMenu* menu)
{
    GtkMenu* gtkmenu = menu->releasePlatformDescription();

    WebKitWebView* webView = m_webView;
    HitTestResult result = core(webView)->contextMenuController()->hitTestResult();

    if (result.isContentEditable()) {
        GtkWidget* imContextMenu = inputMethodsMenuItem(webView);
        if (!imContextMenu)
            return gtkmenu;

        // Place the im context menu item right before the unicode menu item
        // if it's present.
        int unicodeMenuItemPosition = getUnicodeMenuItemPosition(gtkmenu);
        if (unicodeMenuItemPosition == -1) {
            GtkWidget* separator = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(gtkmenu), separator);
            gtk_widget_show(separator);
        }

        gtk_menu_shell_insert(GTK_MENU_SHELL(gtkmenu), imContextMenu, unicodeMenuItemPosition);
        gtk_widget_show(imContextMenu);
    }

    return gtkmenu;
}

void ContextMenuClient::contextMenuItemSelected(ContextMenuItem*, const ContextMenu*)
{
    notImplemented();
}

void ContextMenuClient::downloadURL(const KURL& url)
{
    WebKitNetworkRequest* networkRequest = webkit_network_request_new(url.string().utf8().data());

    webkit_web_view_request_download(m_webView, networkRequest);
    g_object_unref(networkRequest);
}

void ContextMenuClient::copyImageToClipboard(const HitTestResult&)
{
    notImplemented();
}

void ContextMenuClient::searchWithGoogle(const Frame*)
{
    notImplemented();
}

void ContextMenuClient::lookUpInDictionary(Frame*)
{
    notImplemented();
}

void ContextMenuClient::speak(const String&)
{
    notImplemented();
}

void ContextMenuClient::stopSpeaking()
{
    notImplemented();
}

bool ContextMenuClient::isSpeaking()
{
    notImplemented();
    return false;
}

}

#endif // ENABLE(CONTEXT_MENUS)

