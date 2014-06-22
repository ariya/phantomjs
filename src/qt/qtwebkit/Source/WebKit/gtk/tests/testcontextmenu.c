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

#include "autotoolsconfig.h"
#include <webkit/webkit.h>

typedef struct {
    char *data;
    guint flag;
} TestInfo;

static GMainLoop *loop;

typedef struct {
    WebKitWebView *webView;
    TestInfo *info;
} ContextMenuFixture;

static TestInfo *testInfoNew(const char *data, guint flag)
{
    TestInfo *info = g_slice_new(TestInfo);
    info->data = g_strdup(data);
    info->flag = flag;

    return info;
}

static void testInfoDestroy(TestInfo *info)
{
    g_free(info->data);
    g_slice_free(TestInfo, info);
}

static void contextMenuFixtureSetup(ContextMenuFixture *fixture, gconstpointer data)
{
    fixture->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    loop = g_main_loop_new(NULL, TRUE);
    fixture->info = (TestInfo *)data;
}

static void contextMenuFixtureTeardown(ContextMenuFixture *fixture, gconstpointer data)
{
    g_object_unref(fixture->webView);
    g_main_loop_unref(loop);
    testInfoDestroy(fixture->info);
}

static GList *checkAction(GList *iter, WebKitContextMenuAction action)
{
    GtkMenuItem *item = (GtkMenuItem *)iter->data;

    g_assert(GTK_IS_MENU_ITEM(item));
    g_assert(webkit_context_menu_item_get_action(item) == action);

    return iter->next;
}

static GList *checkActionWithSubmenu(GList *iter, WebKitContextMenuAction action)
{
    GtkMenuItem *item = (GtkMenuItem *)iter->data;

    g_assert(GTK_IS_MENU_ITEM(item));
    g_assert(webkit_context_menu_item_get_action(item) == action);
    g_assert(GTK_IS_MENU(gtk_menu_item_get_submenu(item)));

    return iter->next;
}

static GList *checkSeparator(GList *iter)
{
    GtkMenuItem *item = (GtkMenuItem *)iter->data;

    g_assert(GTK_IS_SEPARATOR_MENU_ITEM(item));

    return iter->next;
}

static gboolean contextMenuCallback(WebKitWebView *webView, GtkWidget *defaultMenu, WebKitHitTestResult *hitTestResult, gboolean keyboardMode, gpointer userData)
{
    TestInfo *info = (TestInfo *)userData;
    guint context;
    GList *items;
    GList *iter;

    /* Check signal parameters */
    g_assert(WEBKIT_IS_WEB_VIEW(webView));
    g_assert(GTK_IS_MENU(defaultMenu));
    g_assert(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult));
    g_assert(!keyboardMode);

    g_object_get(hitTestResult, "context", &context, NULL);
    g_assert(context & info->flag);

    items = gtk_container_get_children(GTK_CONTAINER(defaultMenu));
    switch (info->flag) {
    case WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT:
        iter = items;
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_GO_BACK);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_STOP);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_RELOAD);
        g_assert(!iter);

        break;
    case WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE:
        iter = items;
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD);
        g_assert(!iter);

        break;
    case WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE:
        iter = items;
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_CUT);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_PASTE);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_DELETE);
        iter = checkSeparator(iter);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL);
        iter = checkSeparator(iter);
        iter = checkActionWithSubmenu(iter, WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS);
        iter = checkActionWithSubmenu(iter, WEBKIT_CONTEXT_MENU_ACTION_UNICODE);
        g_assert(!iter);

        break;
    case WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK:
        iter = items;
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK);
        iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD);
        g_assert(!iter);

        break;
    default:
        g_assert_not_reached();
    }

    g_list_free(items);
    g_main_loop_quit(loop);

    return TRUE;
}

static void pushEvent(WebKitWebView *webView)
{
    GdkEvent *event = gdk_event_new(GDK_BUTTON_PRESS);
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkDeviceManager *deviceManager;
#endif

    event->any.window = g_object_ref(gtk_widget_get_window(GTK_WIDGET(webView)));
    event->any.send_event = FALSE;
    event->button.time = GDK_CURRENT_TIME;
    event->button.button = 3;
    event->button.x = event->button.y = 5;
    event->button.x_root = event->button.x;
    event->button.y_root = event->button.y;
#if GTK_CHECK_VERSION(3, 0, 0)
    deviceManager = gdk_display_get_device_manager(gdk_display_get_default());
    event->button.device = gdk_device_manager_get_client_pointer(deviceManager);
#endif

    gdk_event_put(event);
    gdk_event_free(event);
}

static void loadStatusCallback(WebKitWebView *webView, GParamSpec *spec, gpointer data)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    TestInfo *info = (TestInfo *)data;

    g_assert(status != WEBKIT_LOAD_FAILED);

    if (status != WEBKIT_LOAD_FINISHED)
        return;

    g_signal_connect(webView, "context-menu", G_CALLBACK(contextMenuCallback), info);
    pushEvent(webView);
}

static gboolean mapEventCallback(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_widget_grab_focus(widget);
    ContextMenuFixture *fixture = (ContextMenuFixture *)data;
    webkit_web_view_load_string(fixture->webView,
                                fixture->info->data,
                                "text/html",
                                "utf-8",
                                "file://");
    g_signal_connect(fixture->webView, "notify::load-status", G_CALLBACK(loadStatusCallback), fixture->info);
    return FALSE;
}

static void testContextMenu(ContextMenuFixture *fixture, gconstpointer data)
{
    GtkAllocation allocation = { 0, 0, 50, 50 };
    GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);

    gtk_window_resize(GTK_WINDOW(window), 50, 50);
    gtk_window_move(GTK_WINDOW(window), 0, 0);
    gtk_widget_size_allocate(GTK_WIDGET(fixture->webView), &allocation);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(fixture->webView));
    g_signal_connect(window, "map-event", G_CALLBACK(mapEventCallback), fixture);
    gtk_widget_show_all(window);

    g_main_loop_run(loop);
}

static gboolean contextMenuCustomItemCallback(WebKitWebView *webView, GtkWidget *defaultMenu, WebKitHitTestResult *hitTestResult, gboolean keyboardMode, gpointer userData)
{
    TestInfo *info = (TestInfo *)userData;
    guint context;
    GList *items;
    GList *iter;
    GtkWidget *menuItem;
    GtkAction *action;

    /* Check signal parameters */
    g_assert(WEBKIT_IS_WEB_VIEW(webView));
    g_assert(GTK_IS_MENU(defaultMenu));
    g_assert(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult));
    g_assert(!keyboardMode);

    g_object_get(hitTestResult, "context", &context, NULL);
    g_assert(context & info->flag);

    action = gtk_action_new("TestAction", "Custom Action", "Custom Action Tooltip", NULL);
    menuItem = gtk_action_create_menu_item(action);
    g_object_unref(action);

    gtk_menu_shell_append(GTK_MENU_SHELL(defaultMenu), menuItem);
    gtk_widget_show(menuItem);

    items = gtk_container_get_children(GTK_CONTAINER(defaultMenu));
    iter = items;
    iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_GO_BACK);
    iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD);
    iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_STOP);
    iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_RELOAD);
    iter = checkAction(iter, WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION);
    g_assert(!iter);

    g_list_free(items);
    g_main_loop_quit(loop);

    return TRUE;
}

static void testContextMenuCustomItem(ContextMenuFixture *fixture, gconstpointer data)
{
    GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(fixture->webView));
    gtk_widget_show_all(window);
    gtk_widget_grab_focus(GTK_WIDGET(fixture->webView));

    webkit_web_view_load_string(fixture->webView,
                                fixture->info->data,
                                "text/html",
                                "utf-8",
                                "file://");
    g_signal_connect(fixture->webView, "context-menu", G_CALLBACK(contextMenuCustomItemCallback), fixture->info);
    pushEvent(fixture->webView);
}

int main(int argc, char **argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");

    g_test_add("/webkit/testcontextmenu/document", ContextMenuFixture,
               testInfoNew("<html><body><h1>WebKitGTK+!</h1></body></html>",
                           WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT),
               contextMenuFixtureSetup, testContextMenu, contextMenuFixtureTeardown);
    /* We hardcode all elements to be at 0,0 so that we know where to generate the button events */
    g_test_add("/webkit/testcontextmenu/image", ContextMenuFixture,
               testInfoNew("<html><body><img style='position:absolute; left:0; top:0' src='0xdeadbeef' width=50 height=50></img></body></html>",
                           WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE),
               contextMenuFixtureSetup, testContextMenu, contextMenuFixtureTeardown);
    g_test_add("/webkit/testcontextmenu/editable", ContextMenuFixture,
               testInfoNew("<html><body><input style='position:absolute; left:0; top:0' size='35'></input>></body></html>",
                           WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE),
               contextMenuFixtureSetup, testContextMenu, contextMenuFixtureTeardown);
    g_test_add("/webkit/testcontextmenu/link", ContextMenuFixture,
               testInfoNew("<html><body><a style='position:absolute; left:0; top:0' href='http://www.example.com'>HELLO WORLD</a></body></html>",
                           WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK),
               contextMenuFixtureSetup, testContextMenu, contextMenuFixtureTeardown);
    g_test_add("/webkit/testcontextmenu/customitem", ContextMenuFixture,
               testInfoNew("<html><body><h1>WebKitGTK+!</h1></body></html>",
                           WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT),
               contextMenuFixtureSetup, testContextMenuCustomItem, contextMenuFixtureTeardown);

    return g_test_run();
}

