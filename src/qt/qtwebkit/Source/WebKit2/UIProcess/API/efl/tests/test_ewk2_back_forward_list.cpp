/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#include "UnitTestUtils/EWK2UnitTestBase.h"
#include "UnitTestUtils/EWK2UnitTestServer.h"
#include "WKEinaSharedString.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

using namespace EWK2UnitTest;
using namespace WTF;

extern EWK2UnitTestEnvironment* environment;

static const char title1[] = "Page1";
static const char title2[] = "Page2";
static const char title3[] = "Page3";

static void serverCallbackNavigation(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);

    Eina_Strbuf* body = eina_strbuf_new();
    eina_strbuf_append_printf(body, "<html><title>%s</title><body>%s</body></html>", path + 1, path + 1);
    const size_t bodyLength = eina_strbuf_length_get(body);
    soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, eina_strbuf_string_steal(body), bodyLength);
    eina_strbuf_free(body);

    soup_message_body_complete(message->response_body);
}

class EWK2BackForwardListTest : public EWK2UnitTestBase {
protected:
    void checkItem(Ewk_Back_Forward_List_Item* item, const char* title, const char* url, const char* originalURL)
    {
        ASSERT_TRUE(item);
        EXPECT_STREQ(url, ewk_back_forward_list_item_url_get(item));
        EXPECT_STREQ(title, ewk_back_forward_list_item_title_get(item));
        EXPECT_STREQ(originalURL, ewk_back_forward_list_item_original_url_get(item));
    }

    WKEinaSharedString urlFromTitle(EWK2UnitTestServer* httpServer, const char* title)
    {
        Eina_Strbuf* path = eina_strbuf_new();
        eina_strbuf_append_printf(path, "/%s", title);
        WKEinaSharedString res = httpServer->getURLForPath(eina_strbuf_string_get(path)).data();
        eina_strbuf_free(path);

        return res;
    }

    void freeEinaList(Eina_List* list)
    {
        void* data = 0;
        EINA_LIST_FREE(list, data)
        ewk_object_unref(static_cast<Ewk_Object*>(data));
    }
};

TEST_F(EWK2BackForwardListTest, ewk_back_forward_list_current_item_get)
{
    const char* url = environment->defaultTestPageUrl();
    ASSERT_TRUE(loadUrlSync(url));
    Ewk_Back_Forward_List* backForwardList = ewk_view_back_forward_list_get(webView());
    ASSERT_TRUE(backForwardList);

    Ewk_Back_Forward_List_Item* currentItem = ewk_back_forward_list_current_item_get(backForwardList);
    checkItem(currentItem, ewk_view_title_get(webView()), url, url);

    Ewk_Back_Forward_List_Item* anotherCurrentItem = ewk_back_forward_list_current_item_get(backForwardList);
    ASSERT_EQ(currentItem, anotherCurrentItem);
}

TEST_F(EWK2BackForwardListTest, ewk_back_forward_list_previous_item_get)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallbackNavigation);

    WKEinaSharedString url1 = urlFromTitle(httpServer.get(), title1);
    ASSERT_TRUE(loadUrlSync(url1));
    ASSERT_STREQ(title1, ewk_view_title_get(webView()));

    ASSERT_TRUE(loadUrlSync(urlFromTitle(httpServer.get(), title2)));
    ASSERT_STREQ(title2, ewk_view_title_get(webView()));

    Ewk_Back_Forward_List* backForwardList = ewk_view_back_forward_list_get(webView());
    ASSERT_TRUE(backForwardList);

    Ewk_Back_Forward_List_Item* previousItem = ewk_back_forward_list_previous_item_get(backForwardList);
    checkItem(previousItem, title1, url1, url1);

    Ewk_Back_Forward_List_Item* anotherPreviousItem = ewk_back_forward_list_previous_item_get(backForwardList);
    ASSERT_EQ(previousItem, anotherPreviousItem);
}

TEST_F(EWK2BackForwardListTest, ewk_back_forward_list_next_item_get)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallbackNavigation);

    ASSERT_TRUE(loadUrlSync(urlFromTitle(httpServer.get(), title1)));
    ASSERT_STREQ(title1, ewk_view_title_get(webView()));

    WKEinaSharedString url2 = urlFromTitle(httpServer.get(), title2);
    ASSERT_TRUE(loadUrlSync(url2));
    ASSERT_STREQ(title2, ewk_view_title_get(webView()));

    // Go back to Page1.
    ewk_view_back(webView());
    ASSERT_TRUE(waitUntilTitleChangedTo(title1));

    Ewk_Back_Forward_List* backForwardList = ewk_view_back_forward_list_get(webView());
    ASSERT_TRUE(backForwardList);

    Ewk_Back_Forward_List_Item* nextItem = ewk_back_forward_list_next_item_get(backForwardList);
    checkItem(nextItem, title2, url2, url2);

    Ewk_Back_Forward_List_Item* anotherNextItem = ewk_back_forward_list_next_item_get(backForwardList);
    ASSERT_EQ(nextItem, anotherNextItem);
}

TEST_F(EWK2BackForwardListTest, ewk_back_forward_list_item_at_index_get)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallbackNavigation);

    WKEinaSharedString url1 = urlFromTitle(httpServer.get(), title1);
    ASSERT_TRUE(loadUrlSync(url1));
    ASSERT_STREQ(title1, ewk_view_title_get(webView()));

    ASSERT_TRUE(loadUrlSync(urlFromTitle(httpServer.get(), title2)));
    ASSERT_STREQ(title2, ewk_view_title_get(webView()));

    Ewk_Back_Forward_List* backForwardList = ewk_view_back_forward_list_get(webView());
    ASSERT_TRUE(backForwardList);

    Ewk_Back_Forward_List_Item* previousItem = ewk_back_forward_list_item_at_index_get(backForwardList, -1);
    checkItem(previousItem, title1, url1, url1);

    Ewk_Back_Forward_List_Item* anotherPreviousItem = ewk_back_forward_list_item_at_index_get(backForwardList, -1);
    ASSERT_EQ(previousItem, anotherPreviousItem);

    Ewk_Back_Forward_List_Item* nonExistingItem = ewk_back_forward_list_item_at_index_get(backForwardList, 10);    
    ASSERT_FALSE(nonExistingItem);
}

TEST_F(EWK2BackForwardListTest, ewk_back_forward_list_count)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallbackNavigation);

    ASSERT_TRUE(loadUrlSync(urlFromTitle(httpServer.get(), title1)));
    ASSERT_STREQ(title1, ewk_view_title_get(webView()));

    ASSERT_TRUE(loadUrlSync(urlFromTitle(httpServer.get(), title2)));
    ASSERT_STREQ(title2, ewk_view_title_get(webView()));

    Ewk_Back_Forward_List* backForwardList = ewk_view_back_forward_list_get(webView());
    ASSERT_TRUE(backForwardList);

    EXPECT_EQ(2, ewk_back_forward_list_count(backForwardList));
}

TEST_F(EWK2BackForwardListTest, ewk_back_forward_list_n_back_items_copy)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallbackNavigation);

    WKEinaSharedString url1 = urlFromTitle(httpServer.get(), title1);
    ASSERT_TRUE(loadUrlSync(url1));
    ASSERT_STREQ(title1, ewk_view_title_get(webView()));

    WKEinaSharedString url2 = urlFromTitle(httpServer.get(), title2);
    ASSERT_TRUE(loadUrlSync(url2));
    ASSERT_STREQ(title2, ewk_view_title_get(webView()));

    ASSERT_TRUE(loadUrlSync(urlFromTitle(httpServer.get(), title3)));
    ASSERT_STREQ(title3, ewk_view_title_get(webView()));

    Ewk_Back_Forward_List* backForwardList = ewk_view_back_forward_list_get(webView());
    ASSERT_TRUE(backForwardList);

    Eina_List* backList = ewk_back_forward_list_n_back_items_copy(backForwardList, 1);
    ASSERT_TRUE(backList);
    ASSERT_EQ(1, eina_list_count(backList));
    checkItem(static_cast<Ewk_Back_Forward_List_Item*>(eina_list_nth(backList, 0)), title2, url2, url2);
    freeEinaList(backList);

    // Check '0' limit.
    backList = ewk_back_forward_list_n_back_items_copy(backForwardList, 0);
    ASSERT_FALSE(backList);

    backList = ewk_back_forward_list_n_back_items_copy(backForwardList, -1);
    ASSERT_TRUE(backList);
    ASSERT_EQ(2, eina_list_count(backList));
    checkItem(static_cast<Ewk_Back_Forward_List_Item*>(eina_list_nth(backList, 0)), title1, url1, url1);
    checkItem(static_cast<Ewk_Back_Forward_List_Item*>(eina_list_nth(backList, 1)), title2, url2, url2);
    freeEinaList(backList);
}

TEST_F(EWK2BackForwardListTest, ewk_back_forward_list_n_forward_items_copy)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallbackNavigation);

    ASSERT_TRUE(loadUrlSync(urlFromTitle(httpServer.get(), title1)));
    ASSERT_STREQ(title1, ewk_view_title_get(webView()));

    WKEinaSharedString url2 = urlFromTitle(httpServer.get(), title2);
    ASSERT_TRUE(loadUrlSync(url2));
    ASSERT_STREQ(title2, ewk_view_title_get(webView()));

    WKEinaSharedString url3 = urlFromTitle(httpServer.get(), title3);
    ASSERT_TRUE(loadUrlSync(url3));
    ASSERT_STREQ(title3, ewk_view_title_get(webView()));

    // Go back to Page1.
    ewk_view_back(webView());
    ASSERT_TRUE(waitUntilTitleChangedTo(title2));
    ewk_view_back(webView());
    ASSERT_TRUE(waitUntilTitleChangedTo(title1));

    Ewk_Back_Forward_List* backForwardList = ewk_view_back_forward_list_get(webView());
    ASSERT_TRUE(backForwardList);

    Eina_List* forwardList = ewk_back_forward_list_n_forward_items_copy(backForwardList, 1);
    ASSERT_TRUE(forwardList);
    ASSERT_EQ(1, eina_list_count(forwardList));
    checkItem(static_cast<Ewk_Back_Forward_List_Item*>(eina_list_nth(forwardList, 0)), title2, url2, url2);
    freeEinaList(forwardList);

    // Check '0' limit.
    forwardList = ewk_back_forward_list_n_forward_items_copy(backForwardList, 0);
    ASSERT_FALSE(forwardList);

    forwardList = ewk_back_forward_list_n_forward_items_copy(backForwardList, -1);
    ASSERT_TRUE(forwardList);
    ASSERT_EQ(2, eina_list_count(forwardList));
    checkItem(static_cast<Ewk_Back_Forward_List_Item*>(eina_list_nth(forwardList, 0)), title2, url2, url2);
    checkItem(static_cast<Ewk_Back_Forward_List_Item*>(eina_list_nth(forwardList, 1)), title3, url3, url3);
    freeEinaList(forwardList);
}
