/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include "Test.h"

namespace TestWebKitAPI {

// State for part 1 - setting up the connection.
static bool connectionEstablished;
static WKConnectionRef connectionToBundle;

// State for part 2 - send/recieving messages.
static bool messageReceived;

// State for part 3 - tearing down the connection.
static bool connectionTornDown;


/* WKContextConnectionClient */
static void didCreateConnection(WKContextRef context, WKConnectionRef connection, const void* clientInfo)
{
    connectionEstablished = true;

    // Store off the conneciton to use.
    connectionToBundle = (WKConnectionRef)WKRetain(connection);
}

/* WKConnectionClient */
static void connectionDidReceiveMessage(WKConnectionRef connection, WKStringRef messageName, WKTypeRef messageBody, const void *clientInfo)
{
    // We only expect to get the "Pong" message.
    EXPECT_WK_STREQ(messageName, "PongMessageName");
    EXPECT_WK_STREQ((WKStringRef)messageBody, "PongMessageBody");

    messageReceived = true;
}

static void connectionDidClose(WKConnectionRef connection, const void* clientInfo)
{
    connectionTornDown = true;
}

TEST(WebKit2, WKConnectionTest)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, Util::createContextForInjectedBundleTest("WKConnectionTest"));

    // Set up the context's connection client so that we can access the connection when
    // it is created.
    WKContextConnectionClient contextConnectionClient;
    memset(&contextConnectionClient, 0, sizeof(contextConnectionClient));
    contextConnectionClient.version = kWKContextConnectionClientCurrentVersion;
    contextConnectionClient.clientInfo = 0;
    contextConnectionClient.didCreateConnection = didCreateConnection;
    WKContextSetConnectionClient(context.get(), &contextConnectionClient);
 
    // Load a simple page to start the WebProcess and establish a connection.
    PlatformWebView webView(context.get());
    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("simple", "html"));
    WKPageLoadURL(webView.page(), url.get());

    // Wait until the connection is established.
    Util::run(&connectionEstablished);
    ASSERT_NOT_NULL(connectionToBundle);

    // Setup a client on the connection so we can listen for messages and
    // tear down notifications.
    WKConnectionClient connectionClient;
    memset(&connectionClient, 0, sizeof(connectionClient));
    connectionClient.version = WKConnectionClientCurrentVersion;
    connectionClient.clientInfo = 0;
    connectionClient.didReceiveMessage = connectionDidReceiveMessage;
    connectionClient.didClose = connectionDidClose;
    WKConnectionSetConnectionClient(connectionToBundle, &connectionClient);
    
    // Post a simple message to the bundle via the connection.
    WKConnectionPostMessage(connectionToBundle, Util::toWK("PingMessageName").get(), Util::toWK("PingMessageBody").get());

    // Wait for the reply.
    Util::run(&messageReceived);

    // Terminate the page to force the connection closed.
    WKPageTerminate(webView.page());
    
    // Wait for the connection to close.
    Util::run(&connectionTornDown);

    // This release is to balance the retain in didCreateConnection.
    WKRelease(connectionToBundle);
}

} // namespace TestWebKitAPI
