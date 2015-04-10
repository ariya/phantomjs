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

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static const char defaultTitle[] = "Default Testing Web Page";

static const char toBeRedirectedPath[] = "/some_page_to_be_redirected";
static const char redirectionTargetPath[] = "/redirection_target";

static EWK2UnitTestServer* httpServer()
{
    static EWK2UnitTestServer* server = 0;

    if (!server)
        server = new EWK2UnitTestServer;

    return server;
}

#define DECLARE_INVOKE_FLAG(functionName) \
    static bool functionName##Invoked = false;

#define WAS_INVOKED(functionName) \
    if (functionName##Invoked)    \
        return;                   \
    functionName##Invoked = true

#define CHECK_WAS_INVOKED(functionName) \
    ASSERT_TRUE(functionName##Invoked)

DECLARE_INVOKE_FLAG(navigateWithNavigationData)
DECLARE_INVOKE_FLAG(performClientRedirect)
DECLARE_INVOKE_FLAG(performServerRedirect)
DECLARE_INVOKE_FLAG(updateHistoryTitle)
DECLARE_INVOKE_FLAG(populateVisitedLinks)

static void navigateWithNavigationData(const Evas_Object* view, Ewk_Navigation_Data* navigationData, void* userData)
{
    WAS_INVOKED(navigateWithNavigationData);

    EWK2UnitTestBase* unitTest = static_cast<EWK2UnitTestBase*>(userData);
    ASSERT_TRUE(unitTest);
    ASSERT_EQ(unitTest->webView(), view);
    // FIXME: WebFrameLoaderClient sends empty title.
    // ASSERT_STREQ(defaultTitle, ewk_navigation_data_title_get(navigationData));
    ASSERT_STREQ(environment->defaultTestPageUrl(), ewk_navigation_data_url_get(navigationData));

    Ewk_Url_Request* request = ewk_navigation_data_original_request_get(navigationData);
    ASSERT_STREQ("GET", ewk_url_request_http_method_get(request));
    ASSERT_STREQ(environment->defaultTestPageUrl(), ewk_url_request_url_get(request));
    ASSERT_EQ(0, ewk_request_cookies_first_party_get(request));
}

static void performClientRedirect(const Evas_Object* view, const char* sourceUrl, const char* destinationUrl, void* userData)
{
    WAS_INVOKED(performClientRedirect);

    EWK2UnitTestBase* unitTest = static_cast<EWK2UnitTestBase*>(userData);
    ASSERT_TRUE(unitTest);
    ASSERT_EQ(unitTest->webView(), view);
    ASSERT_STREQ(environment->urlForResource("redirect_url_to_default.html").data(), sourceUrl);
    ASSERT_STREQ(environment->defaultTestPageUrl(), destinationUrl);
}

static void performServerRedirect(const Evas_Object* view, const char* sourceUrl, const char* destinationUrl, void* userData)
{
    WAS_INVOKED(performServerRedirect);

    EWK2UnitTestBase* unitTest = static_cast<EWK2UnitTestBase*>(userData);
    ASSERT_TRUE(unitTest);
    ASSERT_EQ(unitTest->webView(), view);
    ASSERT_STREQ(httpServer()->getURLForPath(toBeRedirectedPath).data(), sourceUrl);
    ASSERT_STREQ(httpServer()->getURLForPath(redirectionTargetPath).data(), destinationUrl);
}

static void updateHistoryTitle(const Evas_Object* view, const char* title, const char* url, void* userData)
{
    WAS_INVOKED(updateHistoryTitle);

    EWK2UnitTestBase* unitTest = static_cast<EWK2UnitTestBase*>(userData);
    ASSERT_TRUE(unitTest);
    ASSERT_EQ(unitTest->webView(), view);
    ASSERT_STREQ(defaultTitle, title);
    ASSERT_STREQ(environment->defaultTestPageUrl(), url);
}

static void populateVisitedLinks(void* userData)
{
    WAS_INVOKED(populateVisitedLinks);

    EWK2UnitTestBase* unitTest = static_cast<EWK2UnitTestBase*>(userData);
    ASSERT_TRUE(unitTest);
}

static void onLoadFinishedForRedirection(void* userData, Evas_Object*, void*)
{
    int* countLoadFinished = static_cast<int*>(userData);
    --(*countLoadFinished);
}

static void serverCallbackRedirection(SoupServer*, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (strcmp(path, redirectionTargetPath)) { // Redirect if 'path' is not equal to 'redirectionTargetPath'.
        soup_message_set_status(message, SOUP_STATUS_TEMPORARY_REDIRECT);
        soup_message_headers_append(message->response_headers, "Location", httpServer()->getURLForPath(redirectionTargetPath).data());
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);
    Eina_Strbuf* body = eina_strbuf_new();
    eina_strbuf_append_printf(body, "<html><title>Redirection Target</title></html>");
    const size_t bodyLength = eina_strbuf_length_get(body);
    soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, eina_strbuf_string_steal(body), bodyLength);
    eina_strbuf_free(body);

    soup_message_body_complete(message->response_body);
}

TEST_F(EWK2UnitTestBase, ewk_context_history_callbacks_set)
{
    ewk_context_history_callbacks_set(ewk_view_context_get(webView()), navigateWithNavigationData, performClientRedirect, performServerRedirect, updateHistoryTitle, populateVisitedLinks, this);

    // Test navigation.
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    CHECK_WAS_INVOKED(navigateWithNavigationData);
    CHECK_WAS_INVOKED(updateHistoryTitle);
    CHECK_WAS_INVOKED(populateVisitedLinks);

    // Test client redirect.
    int countLoadFinished = 2;
    evas_object_smart_callback_add(webView(), "load,finished", onLoadFinishedForRedirection, &countLoadFinished);
    ewk_view_url_set(webView(), environment->urlForResource("redirect_url_to_default.html").data());
    while (countLoadFinished)
        ecore_main_loop_iterate();
    evas_object_smart_callback_del(webView(), "load,finished", onLoadFinishedForRedirection);
    CHECK_WAS_INVOKED(performClientRedirect);

    // Test server redirect.
    httpServer()->run(serverCallbackRedirection);

    ASSERT_TRUE(loadUrlSync(httpServer()->getURLForPath(toBeRedirectedPath).data()));
    CHECK_WAS_INVOKED(performServerRedirect);
}

