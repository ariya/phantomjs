/*
 * Copyright (C) 2008 Holger Hans Peter Freyther
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

#include "autotoolsconfig.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

static void test_webkit_web_history_item_lifetime(void)
{
    WebKitWebView* webView;
    WebKitWebBackForwardList* backForwardList;
    WebKitWebHistoryItem* currentItem;
    WebKitWebHistoryItem* forwardItem;
    WebKitWebHistoryItem* backItem;
    WebKitWebHistoryItem* nthItem;
    WebKitWebHistoryItem* item1;
    WebKitWebHistoryItem* item2;
    WebKitWebHistoryItem* item3;
    WebKitWebHistoryItem* item4;
    GList* backList = NULL;
    GList* forwardList = NULL;
    g_test_bug("19898");

    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    backForwardList = webkit_web_view_get_back_forward_list(webView);
    g_assert_cmpint(G_OBJECT(backForwardList)->ref_count, ==, 1);

    /* add test items */
    item1 = webkit_web_history_item_new_with_data("http://example.com/1/", "Site 1");
    webkit_web_back_forward_list_add_item(backForwardList, item1);
    g_object_unref(item1);

    item2 = webkit_web_history_item_new_with_data("http://example.com/2/", "Site 2");
    webkit_web_back_forward_list_add_item(backForwardList, item2);
    g_object_unref(item2);

    item3 = webkit_web_history_item_new_with_data("http://example.com/3/", "Site 3");
    webkit_web_back_forward_list_add_item(backForwardList, item3);
    g_object_unref(item3);

    item4 = webkit_web_history_item_new_with_data("http://example.com/4/", "Site 4");
    webkit_web_back_forward_list_add_item(backForwardList, item4);
    g_object_unref(item4);

    /* make sure these functions don't add unnecessary ref to the history item */
    backItem = webkit_web_back_forward_list_get_back_item(backForwardList);
    g_object_ref(backItem);
    g_assert_cmpint(G_OBJECT(backItem)->ref_count, ==, 2);
    g_object_unref(backItem);
    g_assert_cmpint(G_OBJECT(backItem)->ref_count, ==, 1);

    currentItem = webkit_web_back_forward_list_get_current_item(backForwardList);
    g_object_ref(currentItem);
    g_assert_cmpint(G_OBJECT(currentItem)->ref_count, ==, 2);
    g_object_unref(currentItem);
    g_assert_cmpint(G_OBJECT(currentItem)->ref_count, ==, 1);

    webkit_web_back_forward_list_go_to_item(backForwardList, item2);
    forwardItem = webkit_web_back_forward_list_get_forward_item(backForwardList);
    g_object_ref(forwardItem);
    g_assert_cmpint(G_OBJECT(forwardItem)->ref_count, ==, 2);
    g_object_unref(forwardItem);
    g_assert_cmpint(G_OBJECT(forwardItem)->ref_count, ==, 1);

    nthItem = webkit_web_back_forward_list_get_nth_item(backForwardList, 1);
    g_object_ref(nthItem);
    g_assert_cmpint(G_OBJECT(nthItem)->ref_count, ==, 2);
    g_object_unref(nthItem);
    g_assert_cmpint(G_OBJECT(nthItem)->ref_count, ==, 1);

    backList = webkit_web_back_forward_list_get_back_list_with_limit(backForwardList, 5);
    for (; backList; backList = backList->next)
        g_assert_cmpint(G_OBJECT(backList->data)->ref_count, ==, 1);

    forwardList = webkit_web_back_forward_list_get_forward_list_with_limit(backForwardList, 5);
    for (; forwardList; forwardList = forwardList->next)
        g_assert_cmpint(G_OBJECT(forwardList->data)->ref_count, ==, 1);

    g_list_free(forwardList);
    g_list_free(backList);
    g_assert_cmpint(G_OBJECT(item1)->ref_count, ==, 1);
    g_assert_cmpint(G_OBJECT(item2)->ref_count, ==, 1);
    g_assert_cmpint(G_OBJECT(item3)->ref_count, ==, 1);
    g_assert_cmpint(G_OBJECT(item4)->ref_count, ==, 1);
    g_assert_cmpint(G_OBJECT(backForwardList)->ref_count, ==, 1);
    g_object_unref(webView);
}

static void test_webkit_web_back_forward_list_order(void)
{
    WebKitWebView* webView;
    WebKitWebBackForwardList* webBackForwardList;
    WebKitWebHistoryItem* item1;
    WebKitWebHistoryItem* item2;
    WebKitWebHistoryItem* item3;
    WebKitWebHistoryItem* item4;
    WebKitWebHistoryItem* currentItem;
    GList* backList = NULL;
    GList* forwardList = NULL;
    g_test_bug("22694");

    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);

    webkit_web_view_set_maintains_back_forward_list(webView, TRUE);
    webBackForwardList = webkit_web_view_get_back_forward_list(webView);
    g_assert(webBackForwardList);

    // Check that there is no item.
    g_assert(!webkit_web_back_forward_list_get_current_item(webBackForwardList));
    g_assert_cmpint(webkit_web_back_forward_list_get_forward_length(webBackForwardList), ==, 0);
    g_assert_cmpint(webkit_web_back_forward_list_get_back_length(webBackForwardList), ==, 0);
    g_assert(!webkit_web_view_can_go_forward(webView));
    g_assert(!webkit_web_view_can_go_back(webView));

    // Add a new items
    item1 = webkit_web_history_item_new_with_data("http://example.com/1/", "Site 1");
    webkit_web_back_forward_list_add_item(webBackForwardList, item1);
    g_object_unref(item1);
    g_assert(webkit_web_back_forward_list_contains_item(webBackForwardList, item1));

    item2 = webkit_web_history_item_new_with_data("http://example.com/2/", "Site 2");
    webkit_web_back_forward_list_add_item(webBackForwardList, item2);
    g_object_unref(item2);
    g_assert(webkit_web_back_forward_list_contains_item(webBackForwardList, item2));

    item3 = webkit_web_history_item_new_with_data("http://example.com/3/", "Site 3");
    webkit_web_back_forward_list_add_item(webBackForwardList, item3);
    g_object_unref(item3);
    g_assert(webkit_web_back_forward_list_contains_item(webBackForwardList, item3));

    item4 = webkit_web_history_item_new_with_data("http://example.com/4/", "Site 4");
    webkit_web_back_forward_list_add_item(webBackForwardList, item4);
    g_object_unref(item4);
    g_assert(webkit_web_back_forward_list_contains_item(webBackForwardList, item4));

    // check the back list order
    backList = webkit_web_back_forward_list_get_back_list_with_limit(webBackForwardList, 5);
    g_assert(backList);

    currentItem = WEBKIT_WEB_HISTORY_ITEM(backList->data);
    g_assert_cmpstr(webkit_web_history_item_get_uri(currentItem), ==, "http://example.com/3/");
    g_assert_cmpstr(webkit_web_history_item_get_title(currentItem), ==, "Site 3");
    backList = backList->next;

    currentItem = WEBKIT_WEB_HISTORY_ITEM(backList->data);
    g_assert_cmpstr(webkit_web_history_item_get_uri(currentItem), ==, "http://example.com/2/");
    g_assert_cmpstr(webkit_web_history_item_get_title(currentItem), ==, "Site 2");
    backList = backList->next;

    currentItem = WEBKIT_WEB_HISTORY_ITEM(backList->data);
    g_assert_cmpstr(webkit_web_history_item_get_uri(currentItem), ==, "http://example.com/1/");
    g_assert_cmpstr(webkit_web_history_item_get_title(currentItem), ==, "Site 1");
    g_list_free(backList);

    // check the forward list order
    g_assert(webkit_web_view_go_to_back_forward_item(webView, item1));
    forwardList = webkit_web_back_forward_list_get_forward_list_with_limit(webBackForwardList,5);
    g_assert(forwardList);

    currentItem = WEBKIT_WEB_HISTORY_ITEM(forwardList->data);
    g_assert_cmpstr(webkit_web_history_item_get_uri(currentItem), ==, "http://example.com/4/");
    g_assert_cmpstr(webkit_web_history_item_get_title(currentItem), ==, "Site 4");
    forwardList = forwardList->next;

    currentItem = WEBKIT_WEB_HISTORY_ITEM(forwardList->data);
    g_assert_cmpstr(webkit_web_history_item_get_uri(currentItem), ==, "http://example.com/3/");
    g_assert_cmpstr(webkit_web_history_item_get_title(currentItem), ==, "Site 3");
    forwardList = forwardList->next;

    currentItem = WEBKIT_WEB_HISTORY_ITEM(forwardList->data);
    g_assert_cmpstr(webkit_web_history_item_get_uri(currentItem), ==, "http://example.com/2/");
    g_assert_cmpstr(webkit_web_history_item_get_title(currentItem), ==, "Site 2");

    g_list_free(forwardList);
    g_object_unref(webView);
}

static void test_webkit_web_back_forward_list_add_item(void)
{
    WebKitWebView* webView;
    WebKitWebBackForwardList* webBackForwardList;
    WebKitWebHistoryItem* addItem1;
    WebKitWebHistoryItem* addItem2;
    WebKitWebHistoryItem* backItem;
    WebKitWebHistoryItem* currentItem;
    g_test_bug("22988");

    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);

    webkit_web_view_set_maintains_back_forward_list(webView, TRUE);
    webBackForwardList = webkit_web_view_get_back_forward_list(webView);
    g_assert(webBackForwardList);

    // Check that there is no item.
    g_assert(!webkit_web_back_forward_list_get_current_item(webBackForwardList));
    g_assert_cmpint(webkit_web_back_forward_list_get_forward_length(webBackForwardList), ==, 0);
    g_assert_cmpint(webkit_web_back_forward_list_get_back_length(webBackForwardList), ==, 0);
    g_assert(!webkit_web_view_can_go_forward(webView));
    g_assert(!webkit_web_view_can_go_back(webView));

    // Add a new item
    addItem1 = webkit_web_history_item_new_with_data("http://example.com/", "Added site");
    webkit_web_back_forward_list_add_item(webBackForwardList, addItem1);
    g_object_unref(addItem1);
    g_assert(webkit_web_back_forward_list_contains_item(webBackForwardList, addItem1));

    // Check that the added item is the current item.
    currentItem = webkit_web_back_forward_list_get_current_item(webBackForwardList);
    g_assert(currentItem);
    g_assert_cmpint(webkit_web_back_forward_list_get_forward_length(webBackForwardList), ==, 0);
    g_assert_cmpint(webkit_web_back_forward_list_get_back_length(webBackForwardList), ==, 0);
    g_assert(!webkit_web_view_can_go_forward(webView));
    g_assert(!webkit_web_view_can_go_back(webView));
    g_assert_cmpstr(webkit_web_history_item_get_uri(currentItem), ==, "http://example.com/");
    g_assert_cmpstr(webkit_web_history_item_get_title(currentItem), ==, "Added site");

    // Add another item.
    addItem2 = webkit_web_history_item_new_with_data("http://example.com/2/", "Added site 2");
    webkit_web_back_forward_list_add_item(webBackForwardList, addItem2);
    g_object_unref(addItem2);
    g_assert(webkit_web_back_forward_list_contains_item(webBackForwardList, addItem2));

    // Check that the added item is new current item.
    currentItem = webkit_web_back_forward_list_get_current_item(webBackForwardList);
    g_assert(currentItem);
    g_assert_cmpint(webkit_web_back_forward_list_get_forward_length(webBackForwardList), ==, 0);
    g_assert_cmpint(webkit_web_back_forward_list_get_back_length(webBackForwardList), ==, 1);
    g_assert(!webkit_web_view_can_go_forward(webView));
    g_assert(webkit_web_view_can_go_back(webView));
    g_assert_cmpstr(webkit_web_history_item_get_uri(currentItem), ==, "http://example.com/2/");
    g_assert_cmpstr(webkit_web_history_item_get_title(currentItem), ==, "Added site 2");

    backItem = webkit_web_back_forward_list_get_back_item(webBackForwardList);
    g_assert(backItem);
    g_assert_cmpstr(webkit_web_history_item_get_uri(backItem), ==, "http://example.com/");
    g_assert_cmpstr(webkit_web_history_item_get_title(backItem), ==, "Added site");

    // Go to the first added item.
    g_assert(webkit_web_view_go_to_back_forward_item(webView, addItem1));
    g_assert_cmpint(webkit_web_back_forward_list_get_forward_length(webBackForwardList), ==, 1);
    g_assert_cmpint(webkit_web_back_forward_list_get_back_length(webBackForwardList), ==, 0);
    g_assert(webkit_web_view_can_go_forward(webView));
    g_assert(!webkit_web_view_can_go_back(webView));

    g_object_unref(webView);
}

static void test_webkit_web_back_forward_list_clear(void)
{
    WebKitWebView* webView;
    WebKitWebBackForwardList* webBackForwardList;
    WebKitWebHistoryItem* addItem;
    g_test_bug("36173");

    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);

    webBackForwardList = webkit_web_view_get_back_forward_list(webView);
    g_assert(webBackForwardList);

    // Check that there is no item.
    g_assert_cmpint(webkit_web_back_forward_list_get_forward_length(webBackForwardList), ==, 0);
    g_assert_cmpint(webkit_web_back_forward_list_get_back_length(webBackForwardList), ==, 0);
    g_assert(!webkit_web_back_forward_list_get_current_item(webBackForwardList));
    g_assert(!webkit_web_view_can_go_forward(webView));
    g_assert(!webkit_web_view_can_go_back(webView));

    // Check that clearing the empty list does not modify counters
    webkit_web_back_forward_list_clear(webBackForwardList);
    g_assert_cmpint(webkit_web_back_forward_list_get_forward_length(webBackForwardList), ==, 0);
    g_assert_cmpint(webkit_web_back_forward_list_get_back_length(webBackForwardList), ==, 0);
    g_assert(!webkit_web_back_forward_list_get_current_item(webBackForwardList));
    g_assert(!webkit_web_view_can_go_forward(webView));
    g_assert(!webkit_web_view_can_go_back(webView));

    // Add a new item
    addItem = webkit_web_history_item_new_with_data("http://example.com/", "Added site");
    webkit_web_back_forward_list_add_item(webBackForwardList, addItem);
    g_object_unref(addItem);
    g_assert(webkit_web_back_forward_list_contains_item(webBackForwardList, addItem));

    // Check that after clearing the list the added item is no longer in the list
    webkit_web_back_forward_list_clear(webBackForwardList);
    g_assert(!webkit_web_back_forward_list_contains_item(webBackForwardList, addItem));

    // Check that after clearing it, the list is empty
    g_assert_cmpint(webkit_web_back_forward_list_get_forward_length(webBackForwardList), ==, 0);
    g_assert_cmpint(webkit_web_back_forward_list_get_back_length(webBackForwardList), ==, 0);
    g_assert(!webkit_web_back_forward_list_get_current_item(webBackForwardList));
    g_assert(!webkit_web_view_can_go_forward(webView));
    g_assert(!webkit_web_view_can_go_back(webView));

    g_object_unref(webView);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/webbackforwardlist/add_item", test_webkit_web_back_forward_list_add_item);
    g_test_add_func("/webkit/webbackforwardlist/list_order", test_webkit_web_back_forward_list_order);
    g_test_add_func("/webkit/webhistoryitem/lifetime", test_webkit_web_history_item_lifetime);
    g_test_add_func("/webkit/webbackforwardlist/clear", test_webkit_web_back_forward_list_clear);
    return g_test_run ();
}
