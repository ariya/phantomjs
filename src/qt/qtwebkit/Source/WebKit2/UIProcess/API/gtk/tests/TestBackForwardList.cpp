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

#include "config.h"

#include "WebKitTestServer.h"
#include "WebViewTest.h"
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <string.h>
#include <webkit2/webkit2.h>

// Back forward list limit is 100 by default.
static const int kBackForwardListLimit = 100;

static WebKitTestServer* kServer;

static void serverCallback(SoupServer* server, SoupMessage* msg, const char* path, GHashTable* query, SoupClientContext* context, gpointer data)
{
    if (msg->method != SOUP_METHOD_GET) {
        soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (g_str_has_suffix(path, "favicon.ico")) {
        soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
        return;
    }

    soup_message_set_status(msg, SOUP_STATUS_OK);

    char* body = g_strdup_printf("<html><title>%s</title><body>%s</body></html>", path + 1, path + 1);
    soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, body, strlen(body));

    soup_message_body_complete(msg->response_body);
}

class BackForwardListTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(BackForwardListTest);

    enum {
        Backward,
        Forward
    };

    enum {
        CurrentItem  = 1 << 0,
        AddedItem    = 1 << 1,
        RemovedItems = 1 << 2
    };

    static void checkItem(WebKitBackForwardListItem* item, const char* title, const char* uri, const char* originalURI)
    {
        g_assert(item);
        g_assert_cmpstr(webkit_back_forward_list_item_get_uri(item), ==, uri);
        g_assert_cmpstr(webkit_back_forward_list_item_get_title(item), == , title);
        g_assert_cmpstr(webkit_back_forward_list_item_get_original_uri(item), ==, originalURI);
    }

    static void checkItemIndex(WebKitBackForwardList* list)
    {
        g_assert(webkit_back_forward_list_get_nth_item(list, -1) == webkit_back_forward_list_get_back_item(list));
        g_assert(webkit_back_forward_list_get_nth_item(list, 0) == webkit_back_forward_list_get_current_item(list));
        g_assert(webkit_back_forward_list_get_nth_item(list, 1) == webkit_back_forward_list_get_forward_item(list));
    }

    static void checkList(WebKitBackForwardList* list, unsigned type, WebKitBackForwardListItem** items, unsigned nItems)
    {
        GList* listItems = type == BackForwardListTest::Backward ? webkit_back_forward_list_get_back_list(list) :
            webkit_back_forward_list_get_forward_list(list);
        g_assert(listItems);

        unsigned i = 0;
        for (GList* listItem = listItems; listItem; listItem = g_list_next(listItem), i++) {
            g_assert_cmpuint(i, <, nItems);
            g_assert(listItem->data == items[i]);
        }
        g_list_free(listItems);
    }

    static void backForwardListChanged(WebKitBackForwardList* list, WebKitBackForwardListItem* addedItem, GList* removedItems, BackForwardListTest* test)
    {
        test->m_hasChanged = true;

        if (test->m_changedFlags & BackForwardListTest::AddedItem) {
            g_assert(WEBKIT_IS_BACK_FORWARD_LIST_ITEM(addedItem));
            test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(addedItem));
        } else
            g_assert(!addedItem);

        if (test->m_changedFlags & BackForwardListTest::RemovedItems) {
            g_assert(removedItems);
            for (GList* iter = removedItems; iter; iter = iter->next) {
                g_assert(WEBKIT_IS_BACK_FORWARD_LIST_ITEM(iter->data));
                if (test->m_expectedRemovedItems)
                    g_assert(g_list_find(test->m_expectedRemovedItems, iter->data));
            }

        } else
            g_assert(!removedItems);
    }

    BackForwardListTest()
        : m_list(webkit_web_view_get_back_forward_list(m_webView))
        , m_changedFlags(0)
        , m_hasChanged(false)
        , m_expectedRemovedItems(0)
    {
        g_signal_connect(m_list, "changed", G_CALLBACK(backForwardListChanged), this);
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_list));
    }

    ~BackForwardListTest()
    {
        g_signal_handlers_disconnect_matched(m_list, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    void waitUntilLoadFinished()
    {
        m_hasChanged = false;
        WebViewTest::waitUntilLoadFinished();
        g_assert(m_hasChanged);
    }

    void waitUntilLoadFinishedAndCheckRemovedItems(GList* removedItems)
    {
        m_expectedRemovedItems = removedItems;
        waitUntilLoadFinished();
        m_expectedRemovedItems = 0;
    }

    WebKitBackForwardList* m_list;
    unsigned long m_changedFlags;
    bool m_hasChanged;
    GList* m_expectedRemovedItems;
};

static void testBackForwardListNavigation(BackForwardListTest* test, gconstpointer)
{
    WebKitBackForwardListItem* items[1];

    g_assert(!webkit_web_view_can_go_back(test->m_webView));
    g_assert(!webkit_web_view_can_go_forward(test->m_webView));

    g_assert_cmpuint(webkit_back_forward_list_get_length(test->m_list), ==, 0);
    g_assert(!webkit_back_forward_list_get_current_item(test->m_list));
    g_assert(!webkit_back_forward_list_get_back_item(test->m_list));
    g_assert(!webkit_back_forward_list_get_forward_item(test->m_list));
    BackForwardListTest::checkItemIndex(test->m_list);
    g_assert(!webkit_back_forward_list_get_back_list(test->m_list));
    g_assert(!webkit_back_forward_list_get_forward_list(test->m_list));

    CString uriPage1 = kServer->getURIForPath("/Page1");
    test->m_changedFlags = BackForwardListTest::CurrentItem | BackForwardListTest::AddedItem;
    test->loadURI(uriPage1.data());
    test->waitUntilLoadFinished();

    g_assert(!webkit_web_view_can_go_back(test->m_webView));
    g_assert(!webkit_web_view_can_go_forward(test->m_webView));

    g_assert_cmpuint(webkit_back_forward_list_get_length(test->m_list), ==, 1);
    WebKitBackForwardListItem* itemPage1 = webkit_back_forward_list_get_current_item(test->m_list);
    BackForwardListTest::checkItem(itemPage1, "Page1", uriPage1.data(), uriPage1.data());
    g_assert(!webkit_back_forward_list_get_back_item(test->m_list));
    g_assert(!webkit_back_forward_list_get_forward_item(test->m_list));
    BackForwardListTest::checkItemIndex(test->m_list);
    g_assert(!webkit_back_forward_list_get_back_list(test->m_list));
    g_assert(!webkit_back_forward_list_get_forward_list(test->m_list));

    CString uriPage2 = kServer->getURIForPath("/Page2");
    test->m_changedFlags = BackForwardListTest::CurrentItem | BackForwardListTest::AddedItem;
    test->loadURI(uriPage2.data());
    test->waitUntilLoadFinished();

    g_assert(webkit_web_view_can_go_back(test->m_webView));
    g_assert(!webkit_web_view_can_go_forward(test->m_webView));

    g_assert_cmpuint(webkit_back_forward_list_get_length(test->m_list), ==, 2);
    WebKitBackForwardListItem* itemPage2 = webkit_back_forward_list_get_current_item(test->m_list);
    BackForwardListTest::checkItem(itemPage2, "Page2", uriPage2.data(), uriPage2.data());
    g_assert(webkit_back_forward_list_get_back_item(test->m_list) == itemPage1);
    g_assert(!webkit_back_forward_list_get_forward_item(test->m_list));
    BackForwardListTest::checkItemIndex(test->m_list);
    items[0] = itemPage1;
    BackForwardListTest::checkList(test->m_list, BackForwardListTest::Backward, items, 1);
    g_assert(!webkit_back_forward_list_get_forward_list(test->m_list));

    test->m_changedFlags = BackForwardListTest::CurrentItem;
    test->goBack();
    test->waitUntilLoadFinished();

    g_assert(!webkit_web_view_can_go_back(test->m_webView));
    g_assert(webkit_web_view_can_go_forward(test->m_webView));

    g_assert_cmpuint(webkit_back_forward_list_get_length(test->m_list), ==, 2);
    g_assert(itemPage1 == webkit_back_forward_list_get_current_item(test->m_list));
    BackForwardListTest::checkItem(webkit_back_forward_list_get_current_item(test->m_list), "Page1", uriPage1.data(), uriPage1.data());
    g_assert(!webkit_back_forward_list_get_back_item(test->m_list));
    g_assert(webkit_back_forward_list_get_forward_item(test->m_list) == itemPage2);
    BackForwardListTest::checkItemIndex(test->m_list);
    g_assert(!webkit_back_forward_list_get_back_list(test->m_list));
    items[0] = itemPage2;
    BackForwardListTest::checkList(test->m_list, BackForwardListTest::Forward, items, 1);

    test->m_changedFlags = BackForwardListTest::CurrentItem;
    test->goForward();
    test->waitUntilLoadFinished();

    g_assert(webkit_web_view_can_go_back(test->m_webView));
    g_assert(!webkit_web_view_can_go_forward(test->m_webView));

    g_assert_cmpuint(webkit_back_forward_list_get_length(test->m_list), ==, 2);
    g_assert(itemPage2 == webkit_back_forward_list_get_current_item(test->m_list));
    BackForwardListTest::checkItem(webkit_back_forward_list_get_current_item(test->m_list), "Page2", uriPage2.data(), uriPage2.data());
    g_assert(webkit_back_forward_list_get_back_item(test->m_list) == itemPage1);
    g_assert(!webkit_back_forward_list_get_forward_item(test->m_list));
    BackForwardListTest::checkItemIndex(test->m_list);
    items[0] = itemPage1;
    BackForwardListTest::checkList(test->m_list, BackForwardListTest::Backward, items, 1);
    g_assert(!webkit_back_forward_list_get_forward_list(test->m_list));

    test->m_changedFlags = BackForwardListTest::CurrentItem;
    test->goToBackForwardListItem(itemPage1);
    test->waitUntilLoadFinished();

    g_assert(itemPage1 == webkit_back_forward_list_get_current_item(test->m_list));
}

static void testBackForwardListLimitAndCache(BackForwardListTest* test, gconstpointer)
{
    for (int i = 0; i < kBackForwardListLimit; i++) {
        GOwnPtr<char> path(g_strdup_printf("/Page%d", i));
        test->m_changedFlags = BackForwardListTest::CurrentItem | BackForwardListTest::AddedItem;
        test->loadURI(kServer->getURIForPath(path.get()).data());
        test->waitUntilLoadFinished();
    }

    g_assert_cmpuint(webkit_back_forward_list_get_length(test->m_list), ==, kBackForwardListLimit);
    WebKitBackForwardListItem* itemPageFirst = webkit_back_forward_list_get_nth_item(test->m_list, -(kBackForwardListLimit - 1));
    GOwnPtr<GList> removedItems(g_list_prepend(0, itemPageFirst));

    GOwnPtr<char> path(g_strdup_printf("/Page%d", kBackForwardListLimit));
    test->m_changedFlags = BackForwardListTest::CurrentItem | BackForwardListTest::AddedItem | BackForwardListTest::RemovedItems;
    test->loadURI(kServer->getURIForPath(path.get()).data());
    test->waitUntilLoadFinishedAndCheckRemovedItems(removedItems.get());

    g_assert_cmpuint(webkit_back_forward_list_get_length(test->m_list), ==, kBackForwardListLimit);
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    BackForwardListTest::add("BackForwardList", "navigation", testBackForwardListNavigation);
    BackForwardListTest::add("BackForwardList", "list-limit-and-cache", testBackForwardListLimitAndCache);
}

void afterAll()
{
    delete kServer;
}

