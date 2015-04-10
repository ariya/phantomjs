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
#include <wtf/PassOwnPtr.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static const char testUsername[] = "username";
static const char testPassword[] = "password";
static const char expectedSuccessTitle[] = "EFLWebKit2 Authentication test";
static const char expectedAuthorization[] = "Basic dXNlcm5hbWU6cGFzc3dvcmQ="; // Base64 encoding of "username:password".
static const char indexHTMLString[] =
    "<html>"
    "<head><title>EFLWebKit2 Authentication test</title></head>"
    "<body></body></html>";

class EWK2AuthRequestTest : public EWK2UnitTestBase {
public:
    static void serverCallback(SoupServer*, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, void*)
    {
        if (message->method != SOUP_METHOD_GET) {
            soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
            return;
        }

        if (!strcmp(path, "/index.html")) {
            const char* authorization = soup_message_headers_get_one(message->request_headers, "Authorization");
            // Require authentication
            if (authorization && !strcmp(authorization, expectedAuthorization)) {
                // Successful authentication.
                soup_message_set_status(message, SOUP_STATUS_OK);
                soup_message_body_append(message->response_body, SOUP_MEMORY_COPY, indexHTMLString, strlen(indexHTMLString));
            } else {
                // No (valid) authorization header provided by the client, request authentication.
                soup_message_set_status(message, SOUP_STATUS_UNAUTHORIZED);
                soup_message_headers_append(message->response_headers, "WWW-Authenticate", "Basic realm=\"my realm\"");
            }
        } else
            soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);

        soup_message_body_complete(message->response_body);
    }

    static void onAuthenticationRequest(void* userData, Evas_Object*, void* eventInfo)
    {
        Ewk_Auth_Request** returnRequest = static_cast<Ewk_Auth_Request**>(userData);
        ASSERT_TRUE(returnRequest);

        Ewk_Auth_Request* request = static_cast<Ewk_Auth_Request*>(eventInfo);
        ASSERT_TRUE(request);

        *returnRequest = ewk_object_ref(request);
    }

    static void onLoadFinished(void* userData, Evas_Object*, void*)
    {
        bool* isFinished = static_cast<bool*>(userData);
        ASSERT_TRUE(isFinished);

        *isFinished = true;
    }
};

TEST_F(EWK2AuthRequestTest, ewk_auth_request_success)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallback);

    Ewk_Auth_Request* authenticationRequest = 0;
    evas_object_smart_callback_add(webView(), "authentication,request", onAuthenticationRequest, &authenticationRequest);

    ewk_view_url_set(webView(), httpServer->getURLForPath("/index.html").data());

    while (!authenticationRequest)
        ecore_main_loop_iterate();

    ASSERT_TRUE(authenticationRequest);
    evas_object_smart_callback_del(webView(), "authentication,request", onAuthenticationRequest);

    EXPECT_STREQ("my realm", ewk_auth_request_realm_get(authenticationRequest));
    EXPECT_STREQ("127.0.0.1", ewk_auth_request_host_get(authenticationRequest));
    EXPECT_FALSE(ewk_auth_request_retrying_get(authenticationRequest));

    ASSERT_TRUE(ewk_auth_request_authenticate(authenticationRequest, testUsername, testPassword));

    ewk_object_unref(authenticationRequest);

    ASSERT_TRUE(waitUntilTitleChangedTo(expectedSuccessTitle));
}

TEST_F(EWK2AuthRequestTest, ewk_auth_request_failure_then_success)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallback);

    Ewk_Auth_Request* authenticationRequest = 0;
    evas_object_smart_callback_add(webView(), "authentication,request", onAuthenticationRequest, &authenticationRequest);

    ewk_view_url_set(webView(), httpServer->getURLForPath("/index.html").data());

    while (!authenticationRequest)
        ecore_main_loop_iterate();

    ASSERT_TRUE(authenticationRequest);

    EXPECT_STREQ("my realm", ewk_auth_request_realm_get(authenticationRequest));
    EXPECT_STREQ("127.0.0.1", ewk_auth_request_host_get(authenticationRequest));
    EXPECT_FALSE(ewk_auth_request_retrying_get(authenticationRequest));

    ASSERT_TRUE(ewk_auth_request_authenticate(authenticationRequest, testUsername, "wrongpassword"));

    ewk_object_unref(authenticationRequest);
    authenticationRequest = 0;

    // We expect a second authentication request since the first one failed.
    while (!authenticationRequest)
        ecore_main_loop_iterate();
    evas_object_smart_callback_del(webView(), "authentication,request", onAuthenticationRequest);

    EXPECT_STREQ("my realm", ewk_auth_request_realm_get(authenticationRequest));
    EXPECT_STREQ("127.0.0.1", ewk_auth_request_host_get(authenticationRequest));
    EXPECT_TRUE(ewk_auth_request_retrying_get(authenticationRequest));

    // Now provide the right password.
    ASSERT_TRUE(ewk_auth_request_authenticate(authenticationRequest, testUsername, testPassword));

    ewk_object_unref(authenticationRequest);

    ASSERT_TRUE(waitUntilTitleChangedTo(expectedSuccessTitle));
}

TEST_F(EWK2AuthRequestTest, ewk_auth_request_cancel)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallback);

    Ewk_Auth_Request* authenticationRequest = 0;
    evas_object_smart_callback_add(webView(), "authentication,request", onAuthenticationRequest, &authenticationRequest);

    ewk_view_url_set(webView(), httpServer->getURLForPath("/index.html").data());

    while (!authenticationRequest)
        ecore_main_loop_iterate();

    ASSERT_TRUE(authenticationRequest);
    evas_object_smart_callback_del(webView(), "authentication,request", onAuthenticationRequest);

    EXPECT_STREQ("my realm", ewk_auth_request_realm_get(authenticationRequest));
    EXPECT_STREQ("127.0.0.1", ewk_auth_request_host_get(authenticationRequest));
    EXPECT_FALSE(ewk_auth_request_retrying_get(authenticationRequest));

    bool isFinished = false;
    evas_object_smart_callback_add(webView(), "load,finished", onLoadFinished, &isFinished);

    // Will attempt to continue without authentication by default.
    ewk_object_unref(authenticationRequest);

    while (!isFinished)
        ecore_main_loop_iterate();

    ASSERT_STRNE(expectedSuccessTitle, ewk_view_title_get(webView()));

    evas_object_smart_callback_del(webView(), "load,finished", onLoadFinished);
}
