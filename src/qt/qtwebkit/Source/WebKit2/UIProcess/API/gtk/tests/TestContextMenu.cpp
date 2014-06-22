/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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
#include "WebViewTest.h"
#include <wtf/gobject/GRefPtr.h>

class ContextMenuTest: public WebViewTest {
public:
    enum ContextMenuItemStateFlags {
        Visible = 1 << 0,
        Enabled = 1 << 1,
        Checked = 1 << 2
    };

    void checkContextMenuEvent(GdkEvent* event)
    {
        g_assert(event);
        g_assert_cmpint(event->type, ==, GDK_BUTTON_PRESS);
        g_assert_cmpint(event->button.button, ==, 3);
        g_assert_cmpint(event->button.x, ==, m_menuPositionX);
        g_assert_cmpint(event->button.y, ==, m_menuPositionY);
    }

    static gboolean contextMenuCallback(WebKitWebView* webView, WebKitContextMenu* contextMenu, GdkEvent* event, WebKitHitTestResult* hitTestResult, ContextMenuTest* test)
    {
        g_assert(WEBKIT_IS_CONTEXT_MENU(contextMenu));
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(contextMenu));
        test->checkContextMenuEvent(event);
        g_assert(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult));
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(hitTestResult));

        return test->contextMenu(contextMenu, event, hitTestResult);
    }

    static void contextMenuDismissedCallback(WebKitWebView*, ContextMenuTest* test)
    {
        test->contextMenuDismissed();
    }

    ContextMenuTest()
        : m_menuPositionX(0)
        , m_menuPositionY(0)
    {
        g_signal_connect(m_webView, "context-menu", G_CALLBACK(contextMenuCallback), this);
        g_signal_connect(m_webView, "context-menu-dismissed", G_CALLBACK(contextMenuDismissedCallback), this);
    }

    ~ContextMenuTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    virtual bool contextMenu(WebKitContextMenu*, GdkEvent*, WebKitHitTestResult*) = 0;

    virtual void contextMenuDismissed()
    {
        quitMainLoop();
    }

    GtkMenu* getPopupMenu()
    {
        GOwnPtr<GList> toplevels(gtk_window_list_toplevels());
        for (GList* iter = toplevels.get(); iter; iter = g_list_next(iter)) {
            if (!GTK_IS_WINDOW(iter->data))
                continue;

            GtkWidget* child = gtk_bin_get_child(GTK_BIN(iter->data));
            if (!GTK_IS_MENU(child))
                continue;

            if (gtk_menu_get_attach_widget(GTK_MENU(child)) == GTK_WIDGET(m_webView))
                return GTK_MENU(child);
        }
        g_assert_not_reached();
        return 0;
    }

    bool shouldShowInputMethodsMenu()
    {
        GtkSettings* settings = gtk_widget_get_settings(GTK_WIDGET(m_webView));
        if (!settings)
            return true;

        gboolean showInputMethodMenu;
        g_object_get(settings, "gtk-show-input-method-menu", &showInputMethodMenu, NULL);
        return showInputMethodMenu;
    }

    void checkActionState(GtkAction* action, unsigned state)
    {
        if (state & Visible)
            g_assert(gtk_action_get_visible(action));
        else
            g_assert(!gtk_action_get_visible(action));

        if (state & Enabled)
            g_assert(gtk_action_get_sensitive(action));
        else
            g_assert(!gtk_action_get_sensitive(action));

        if (GTK_IS_TOGGLE_ACTION(action)) {
            if (state & Checked)
                g_assert(gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
            else
                g_assert(!gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
        }
    }

    GList* checkCurrentItemIsStockActionAndGetNext(GList* items, WebKitContextMenuAction stockAction, unsigned state)
    {
        g_assert(items);
        g_assert(WEBKIT_IS_CONTEXT_MENU_ITEM(items->data));

        WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(items->data);
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(item));

        GtkAction* action = webkit_context_menu_item_get_action(item);
        g_assert(GTK_IS_ACTION(action));

        g_assert_cmpint(webkit_context_menu_item_get_stock_action(item), ==, stockAction);

        checkActionState(action, state);

        return g_list_next(items);
    }

    GList* checkCurrentItemIsCustomActionAndGetNext(GList* items, const char* label, unsigned state)
    {
        g_assert(items);
        g_assert(WEBKIT_IS_CONTEXT_MENU_ITEM(items->data));

        WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(items->data);
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(item));

        GtkAction* action = webkit_context_menu_item_get_action(item);
        g_assert(GTK_IS_ACTION(action));

        g_assert_cmpint(webkit_context_menu_item_get_stock_action(item), ==, WEBKIT_CONTEXT_MENU_ACTION_CUSTOM);
        g_assert_cmpstr(gtk_action_get_label(action), ==, label);

        checkActionState(action, state);

        return g_list_next(items);
    }

    GList* checkCurrentItemIsSubMenuAndGetNext(GList* items, const char* label, unsigned state, GList** subMenuIter)
    {
        g_assert(items);
        g_assert(WEBKIT_IS_CONTEXT_MENU_ITEM(items->data));

        WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(items->data);
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(item));

        GtkAction* action = webkit_context_menu_item_get_action(item);
        g_assert(GTK_IS_ACTION(action));

        g_assert_cmpstr(gtk_action_get_label(action), ==, label);
        checkActionState(action, state);

        WebKitContextMenu* subMenu = webkit_context_menu_item_get_submenu(item);
        g_assert(WEBKIT_IS_CONTEXT_MENU(subMenu));
        if (subMenuIter)
            *subMenuIter = webkit_context_menu_get_items(subMenu);

        return g_list_next(items);
    }

    GList* checkCurrentItemIsSeparatorAndGetNext(GList* items)
    {
        g_assert(items);
        g_assert(WEBKIT_IS_CONTEXT_MENU_ITEM(items->data));

        WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(items->data);
        g_assert(webkit_context_menu_item_is_separator(item));

        return g_list_next(items);
    }

    static gboolean doRightClickIdleCallback(ContextMenuTest* test)
    {
        test->clickMouseButton(test->m_menuPositionX, test->m_menuPositionY, 3);
        return FALSE;
    }

    void showContextMenuAtPositionAndWaitUntilFinished(int x, int y)
    {
        m_menuPositionX = x;
        m_menuPositionY = y;
        g_idle_add(reinterpret_cast<GSourceFunc>(doRightClickIdleCallback), this);
        g_main_loop_run(m_mainLoop);
    }

    void showContextMenuAndWaitUntilFinished()
    {
        showContextMenuAtPositionAndWaitUntilFinished(0, 0);
    }

    static gboolean simulateEscKeyIdleCallback(ContextMenuTest* test)
    {
        test->keyStroke(GDK_KEY_Escape);
        return FALSE;
    }

    void dismissContextMenuAndWaitUntilFinished()
    {
        g_idle_add(reinterpret_cast<GSourceFunc>(simulateEscKeyIdleCallback), this);
        g_main_loop_run(m_mainLoop);
    }

    double m_menuPositionX;
    double m_menuPositionY;
};

class ContextMenuDefaultTest: public ContextMenuTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ContextMenuDefaultTest);

    enum DefaultMenuType {
        Navigation,
        Link,
        Image,
        LinkImage,
        Video,
        Editable
    };

    ContextMenuDefaultTest()
        : m_expectedMenuType(Navigation)
    {
    }

    bool contextMenu(WebKitContextMenu* contextMenu, GdkEvent* event, WebKitHitTestResult* hitTestResult)
    {
        GList* iter = webkit_context_menu_get_items(contextMenu);

        switch (m_expectedMenuType) {
        case Navigation:
            g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_GO_BACK, Visible);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD, Visible);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_STOP, Visible);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_RELOAD, Visible | Enabled);
            break;
        case Link:
            g_assert(webkit_hit_test_result_context_is_link(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD, Visible | Enabled);
            break;
        case Image:
            g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
            g_assert(webkit_hit_test_result_context_is_image(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD, Visible | Enabled);
            break;
        case LinkImage:
            g_assert(webkit_hit_test_result_context_is_link(hitTestResult));
            g_assert(webkit_hit_test_result_context_is_image(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD, Visible | Enabled);
            iter = checkCurrentItemIsSeparatorAndGetNext(iter);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD, Visible | Enabled);
            break;
        case Video:
            g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
            g_assert(webkit_hit_test_result_context_is_media(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE, Visible);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS, Visible | Enabled | Checked);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN, Visible);
            iter = checkCurrentItemIsSeparatorAndGetNext(iter);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW, Visible | Enabled);
            break;
        case Editable:
            g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
            g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
            g_assert(webkit_hit_test_result_context_is_editable(hitTestResult));
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_CUT, Visible);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY, Visible);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_PASTE, Visible | Enabled);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_DELETE, Visible);
            iter = checkCurrentItemIsSeparatorAndGetNext(iter);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL, Visible | Enabled);
            if (shouldShowInputMethodsMenu()) {
                iter = checkCurrentItemIsSeparatorAndGetNext(iter);
                iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS, Visible | Enabled);
                iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_UNICODE, Visible | Enabled);
            }
            break;
        default:
            g_assert_not_reached();
        }

        if (webkit_settings_get_enable_developer_extras(webkit_web_view_get_settings(m_webView))) {
            iter = checkCurrentItemIsSeparatorAndGetNext(iter);
            iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT, Visible | Enabled);
        }
        g_assert(!iter);

        quitMainLoop();

        return true;
    }

    DefaultMenuType m_expectedMenuType;
};

static void testContextMenuDefaultMenu(ContextMenuDefaultTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    const char* linksHTML =
        "<html><body>"
        " <a style='position:absolute; left:1; top:1' href='http://www.webkitgtk.org' title='WebKitGTK+ Title'>WebKitGTK+ Website</a>"
        " <img style='position:absolute; left:1; top:10' src='0xdeadbeef' width=5 height=5></img>"
        " <a style='position:absolute; left:1; top:20' href='http://www.webkitgtk.org/logo' title='WebKitGTK+ Logo'><img src='0xdeadbeef' width=5 height=5></img></a>"
        " <input style='position:absolute; left:1; top:30' size='10'></input>"
        " <video style='position:absolute; left:1; top:50' width='300' height='300' controls='controls' preload='none'><source src='movie.ogg' type='video/ogg' /></video>"
        "</body></html>";
    test->loadHtml(linksHTML, "file:///");
    test->waitUntilLoadFinished();

    // Context menu for document.
    test->m_expectedMenuType = ContextMenuDefaultTest::Navigation;
    test->showContextMenuAtPositionAndWaitUntilFinished(0, 0);

    // Context menu for link.
    test->m_expectedMenuType = ContextMenuDefaultTest::Link;
    test->showContextMenuAtPositionAndWaitUntilFinished(1, 1);

    // Context menu for image.
    test->m_expectedMenuType = ContextMenuDefaultTest::Image;
    test->showContextMenuAtPositionAndWaitUntilFinished(1, 10);

    // Enable developer extras now, so that inspector element
    // will be shown in the default context menu.
    webkit_settings_set_enable_developer_extras(webkit_web_view_get_settings(test->m_webView), TRUE);

    // Context menu for image link.
    test->m_expectedMenuType = ContextMenuDefaultTest::LinkImage;
    test->showContextMenuAtPositionAndWaitUntilFinished(1, 20);

    // Context menu for image video.
    test->m_expectedMenuType = ContextMenuDefaultTest::Video;
    test->showContextMenuAtPositionAndWaitUntilFinished(1, 50);

    // Context menu for editable.
    test->m_expectedMenuType = ContextMenuDefaultTest::Editable;
    test->showContextMenuAtPositionAndWaitUntilFinished(5, 35);
}

class ContextMenuCustomTest: public ContextMenuTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ContextMenuCustomTest);

    ContextMenuCustomTest()
        : m_itemToActivateLabel(0)
        , m_activated(false)
        , m_toggled(false)
    {
    }

    bool contextMenu(WebKitContextMenu* contextMenu, GdkEvent*, WebKitHitTestResult* hitTestResult)
    {
        // Append our custom item to the default menu.
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new(m_action.get()));
        quitMainLoop();

        return false;
    }

    GtkMenuItem* getMenuItem(GtkMenu* menu, const gchar* itemLabel)
    {
        GOwnPtr<GList> items(gtk_container_get_children(GTK_CONTAINER(menu)));
        for (GList* iter = items.get(); iter; iter = g_list_next(iter)) {
            GtkMenuItem* child = GTK_MENU_ITEM(iter->data);
            if (g_str_equal(itemLabel, gtk_menu_item_get_label(child)))
                return child;
        }
        g_assert_not_reached();
        return 0;
    }

    void activateMenuItem()
    {
        g_assert(m_itemToActivateLabel);
        GtkMenu* menu = getPopupMenu();
        GtkMenuItem* item = getMenuItem(menu, m_itemToActivateLabel);
        gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu), GTK_WIDGET(item), TRUE);
        m_itemToActivateLabel = 0;
    }

    static gboolean activateMenuItemIdleCallback(gpointer userData)
    {
        ContextMenuCustomTest* test = static_cast<ContextMenuCustomTest*>(userData);
        test->activateMenuItem();
        return FALSE;
    }

    void activateCustomMenuItemAndWaitUntilActivated(const char* actionLabel)
    {
        m_activated = m_toggled = false;
        m_itemToActivateLabel = actionLabel;
        g_idle_add(activateMenuItemIdleCallback, this);
        g_main_loop_run(m_mainLoop);
    }

    void toggleCustomMenuItemAndWaitUntilToggled(const char* actionLabel)
    {
        activateCustomMenuItemAndWaitUntilActivated(actionLabel);
    }

    static void actionActivatedCallback(GtkAction*, ContextMenuCustomTest* test)
    {
        test->m_activated = true;
    }

    static void actionToggledCallback(GtkAction*, ContextMenuCustomTest* test)
    {
        test->m_toggled = true;
    }

    void setAction(GtkAction* action)
    {
        m_action = action;
        if (GTK_IS_TOGGLE_ACTION(action))
            g_signal_connect(action, "toggled", G_CALLBACK(actionToggledCallback), this);
        else
            g_signal_connect(action, "activate", G_CALLBACK(actionActivatedCallback), this);
    }

    GRefPtr<GtkAction> m_action;
    const char* m_itemToActivateLabel;
    bool m_activated;
    bool m_toggled;
};

static void testContextMenuPopulateMenu(ContextMenuCustomTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    test->loadHtml("<html><body>WebKitGTK+ Context menu tests</body></html>", "file:///");
    test->waitUntilLoadFinished();

    // Create a custom menu item.
    GRefPtr<GtkAction> action = adoptGRef(gtk_action_new("WebKitGTK+CustomAction", "Custom _Action", 0, 0));
    test->setAction(action.get());
    test->showContextMenuAndWaitUntilFinished();
    test->activateCustomMenuItemAndWaitUntilActivated(gtk_action_get_label(action.get()));
    g_assert(test->m_activated);
    g_assert(!test->m_toggled);

    // Create a custom toggle menu item.
    GRefPtr<GtkAction> toggleAction = adoptGRef(GTK_ACTION(gtk_toggle_action_new("WebKitGTK+CustomToggleAction", "Custom _Toggle Action", 0, 0)));
    test->setAction(toggleAction.get());
    test->showContextMenuAndWaitUntilFinished();
    test->toggleCustomMenuItemAndWaitUntilToggled(gtk_action_get_label(toggleAction.get()));
    g_assert(!test->m_activated);
    g_assert(test->m_toggled);
}

class ContextMenuCustomFullTest: public ContextMenuTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ContextMenuCustomFullTest);

    bool contextMenu(WebKitContextMenu* contextMenu, GdkEvent*, WebKitHitTestResult*)
    {
        // Clear proposed menu and build our own.
        webkit_context_menu_remove_all(contextMenu);
        g_assert_cmpint(webkit_context_menu_get_n_items(contextMenu), ==, 0);

        // Add actions from stock.
        webkit_context_menu_prepend(contextMenu, webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_GO_BACK));
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD));
        webkit_context_menu_insert(contextMenu, webkit_context_menu_item_new_separator(), 2);

        // Add custom actions.
        GRefPtr<GtkAction> action = adoptGRef(gtk_action_new("WebKitGTK+CustomAction", "Custom _Action", 0, 0));
        gtk_action_set_sensitive(action.get(), FALSE);
        webkit_context_menu_insert(contextMenu, webkit_context_menu_item_new(action.get()), -1);
        GRefPtr<GtkAction> toggleAction = adoptGRef(GTK_ACTION(gtk_toggle_action_new("WebKitGTK+CustomToggleAction", "Custom _Toggle Action", 0, 0)));
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toggleAction.get()), TRUE);
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new(toggleAction.get()));
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());

        // Add a submenu.
        GRefPtr<WebKitContextMenu> subMenu = adoptGRef(webkit_context_menu_new());
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(subMenu.get()));
        webkit_context_menu_insert(subMenu.get(), webkit_context_menu_item_new_from_stock_action_with_label(WEBKIT_CONTEXT_MENU_ACTION_STOP, "Stop Load"), 0);
        webkit_context_menu_insert(subMenu.get(), webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_RELOAD), -1);
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_with_submenu("Load options", subMenu.get()));
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());

        // Move Load submenu before custom actions.
        webkit_context_menu_move_item(contextMenu, webkit_context_menu_last(contextMenu), 3);
        webkit_context_menu_move_item(contextMenu, webkit_context_menu_last(contextMenu), 3);

        // If last item is a separator, remove it.
        if (webkit_context_menu_item_is_separator(webkit_context_menu_last(contextMenu)))
            webkit_context_menu_remove(contextMenu, webkit_context_menu_last(contextMenu));

        // Check the menu.
        GList* iter = webkit_context_menu_get_items(contextMenu);

        iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_GO_BACK, Visible | Enabled);
        iter = checkCurrentItemIsStockActionAndGetNext(iter, WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD, Visible | Enabled);
        iter = checkCurrentItemIsSeparatorAndGetNext(iter);

        GList* subMenuIter = 0;
        iter = checkCurrentItemIsSubMenuAndGetNext(iter, "Load options", Visible | Enabled, &subMenuIter);
        subMenuIter = checkCurrentItemIsStockActionAndGetNext(subMenuIter, WEBKIT_CONTEXT_MENU_ACTION_STOP, Visible | Enabled);
        subMenuIter = checkCurrentItemIsStockActionAndGetNext(subMenuIter, WEBKIT_CONTEXT_MENU_ACTION_RELOAD, Visible | Enabled);
        iter = checkCurrentItemIsSeparatorAndGetNext(iter);

        iter = checkCurrentItemIsCustomActionAndGetNext(iter, "Custom _Action", Visible);
        iter = checkCurrentItemIsCustomActionAndGetNext(iter, "Custom _Toggle Action", Visible | Enabled | Checked);
        g_assert(!iter);

        quitMainLoop();

        return true;
    }
};

static void testContextMenuCustomMenu(ContextMenuCustomFullTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    test->loadHtml("<html><body>WebKitGTK+ Context menu tests</body></html>", "file:///");
    test->waitUntilLoadFinished();

    test->showContextMenuAndWaitUntilFinished();
}

class ContextMenuDisabledTest: public ContextMenuTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ContextMenuDisabledTest);

    enum DisableMode {
        IgnoreClicks,
        IgnoreDefaultMenu
    };

    static gboolean buttonPressEventCallback(GtkWidget*, GdkEvent* event, ContextMenuDisabledTest* test)
    {
        if (event->button.button != 3)
            return FALSE;
        return test->rightButtonPressed();
    }

    ContextMenuDisabledTest()
        : m_disableMode(IgnoreClicks)
    {
        g_signal_connect(m_webView, "button-press-event", G_CALLBACK(buttonPressEventCallback), this);
    }

    bool contextMenu(WebKitContextMenu* contextMenu, GdkEvent*, WebKitHitTestResult*)
    {
        if (m_disableMode == IgnoreClicks)
            g_assert_not_reached();
        else
            quitMainLoop();

        return true;
    }

    bool rightButtonPressed()
    {
        if (m_disableMode == IgnoreClicks) {
            quitMainLoopAfterProcessingPendingEvents();
            return true;
        }
        return false;
    }

    DisableMode m_disableMode;
};

static void testContextMenuDisableMenu(ContextMenuDisabledTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    test->loadHtml("<html><body>WebKitGTK+ Context menu tests</body></html>", "file:///");
    test->waitUntilLoadFinished();

    test->m_disableMode = ContextMenuDisabledTest::IgnoreDefaultMenu;
    test->showContextMenuAndWaitUntilFinished();

    test->m_disableMode = ContextMenuDisabledTest::IgnoreClicks;
    test->showContextMenuAndWaitUntilFinished();
}

class ContextMenuSubmenuTest: public ContextMenuTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ContextMenuSubmenuTest);

    bool contextMenu(WebKitContextMenu* contextMenu, GdkEvent*, WebKitHitTestResult*)
    {
        size_t menuSize = webkit_context_menu_get_n_items(contextMenu);
        GRefPtr<WebKitContextMenu> subMenu = adoptGRef(webkit_context_menu_new());
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_with_submenu("SubMenuItem", subMenu.get()));
        g_assert_cmpuint(webkit_context_menu_get_n_items(contextMenu), ==, menuSize + 1);

        GRefPtr<WebKitContextMenu> subMenu2 = adoptGRef(webkit_context_menu_new());
        GRefPtr<WebKitContextMenuItem> item = webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK);

        // Add submenu to newly created item.
        g_assert(!webkit_context_menu_item_get_submenu(item.get()));
        webkit_context_menu_item_set_submenu(item.get(), subMenu2.get());
        g_assert(webkit_context_menu_item_get_submenu(item.get()) == subMenu2.get());

        // Replace the submenu.
        webkit_context_menu_item_set_submenu(item.get(), 0);
        g_assert(!webkit_context_menu_item_get_submenu(item.get()));

        // Try to add a submenu already added to another item.
        removeLogFatalFlag(G_LOG_LEVEL_WARNING);
        webkit_context_menu_item_set_submenu(item.get(), subMenu.get());
        addLogFatalFlag(G_LOG_LEVEL_WARNING);
        g_assert(!webkit_context_menu_item_get_submenu(item.get()));

        // A removed submenu shouldn't have a parent.
        webkit_context_menu_item_set_submenu(item.get(), subMenu2.get());
        g_assert(webkit_context_menu_item_get_submenu(item.get()) == subMenu2.get());

        quitMainLoop();

        return true;
    }
};

static void testContextMenuSubMenu(ContextMenuSubmenuTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    test->loadHtml("<html><body>WebKitGTK+ Context menu tests</body></html>", "file:///");
    test->waitUntilLoadFinished();

    test->showContextMenuAndWaitUntilFinished();
}

class ContextMenuDismissedTest: public ContextMenuTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ContextMenuDismissedTest);

    ContextMenuDismissedTest()
        : m_dismissed(false)
    {
    }

    bool contextMenu(WebKitContextMenu* contextMenu, GdkEvent*, WebKitHitTestResult*)
    {
        quitMainLoop();
        // Show the default context menu.
        return false;
    }

    void contextMenuDismissed()
    {
        m_dismissed = true;
        ContextMenuTest::contextMenuDismissed();
    }

    void showContextMenuAndWaitUntilDismissed()
    {
        showContextMenuAndWaitUntilFinished();
        dismissContextMenuAndWaitUntilFinished();
    }

    bool m_dismissed;
};

static void testContextMenuDismissed(ContextMenuDismissedTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    test->loadHtml("<html><body>WebKitGTK+ Context menu tests</body></html>", "file:///");
    test->waitUntilLoadFinished();

    test->showContextMenuAndWaitUntilDismissed();
    g_assert(test->m_dismissed);
}

class ContextMenuSmartSeparatorsTest: public ContextMenuTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ContextMenuSmartSeparatorsTest);

    bool contextMenu(WebKitContextMenu* contextMenu, GdkEvent*, WebKitHitTestResult*)
    {
        webkit_context_menu_remove_all(contextMenu);

        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_GO_BACK));
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD));
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_COPY));
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_from_stock_action(WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT));
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());
        webkit_context_menu_append(contextMenu, webkit_context_menu_item_new_separator());

        quitMainLoop();

        return false;
    }

    GtkMenu* showContextMenuAndGetGtkMenu()
    {
        showContextMenuAndWaitUntilFinished();
        return getPopupMenu();
    }
};

static void testContextMenuSmartSeparators(ContextMenuSmartSeparatorsTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    test->loadHtml("<html><body>WebKitGTK+ Context menu tests</body></html>", "file:///");
    test->waitUntilLoadFinished();

    GtkMenu* menu = test->showContextMenuAndGetGtkMenu();
    g_assert(menu);

    // Leading and trailing separators are not added to the context menu.
    GOwnPtr<GList> menuItems(gtk_container_get_children(GTK_CONTAINER(menu)));
    g_assert_cmpuint(g_list_length(menuItems.get()), ==, 6);
    GtkWidget* item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 0));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 1));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 2));
    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 3));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 4));
    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 5));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));

    // Hiding a menu item between two separators hides the following separator.
    GtkAction* action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(g_list_nth_data(menuItems.get(), 3)));
    gtk_action_set_visible(action, FALSE);
    menuItems.set(gtk_container_get_children(GTK_CONTAINER(menu)));
    g_assert_cmpuint(g_list_length(menuItems.get()), ==, 6);
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 0));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 1));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 2));
    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 3));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && !gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 4));
    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item) && !gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 5));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    gtk_action_set_visible(action, TRUE);

    // Showing an action between two separators shows the hidden separator.
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 0));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 1));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 2));
    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 3));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 4));
    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 5));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));

    // Trailing separators are hidden too.
    action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(g_list_nth_data(menuItems.get(), 5)));
    gtk_action_set_visible(action, FALSE);
    menuItems.set(gtk_container_get_children(GTK_CONTAINER(menu)));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 0));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 1));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 2));
    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 3));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 4));
    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item) && !gtk_widget_get_visible(item));
    item = GTK_WIDGET(g_list_nth_data(menuItems.get(), 5));
    g_assert(!GTK_IS_SEPARATOR_MENU_ITEM(item) && !gtk_widget_get_visible(item));
}

void beforeAll()
{
    ContextMenuDefaultTest::add("WebKitWebView", "default-menu", testContextMenuDefaultMenu);
    ContextMenuCustomTest::add("WebKitWebView", "populate-menu", testContextMenuPopulateMenu);
    ContextMenuCustomFullTest::add("WebKitWebView", "custom-menu", testContextMenuCustomMenu);
    ContextMenuDisabledTest::add("WebKitWebView", "disable-menu", testContextMenuDisableMenu);
    ContextMenuSubmenuTest::add("WebKitWebView", "submenu", testContextMenuSubMenu);
    ContextMenuDismissedTest::add("WebKitWebView", "menu-dismissed", testContextMenuDismissed);
    ContextMenuSmartSeparatorsTest::add("WebKitWebView", "smart-separators", testContextMenuSmartSeparators);
}

void afterAll()
{
}
