/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

static bool didReceiveAllMessages = false;
static bool receivedMessageForAddingForm = false;
static const uint64_t expectedNumberOfElements = 1;

static void nullJavaScriptCallback(WKSerializedScriptValueRef, WKErrorRef, void*)
{
}

static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void*)
{
    EXPECT_WK_STREQ("DidReceiveDidAssociateFormControls", messageName);
    ASSERT_NOT_NULL(messageBody);
    EXPECT_EQ(WKDictionaryGetTypeID(), WKGetTypeID(messageBody));

    WKDictionaryRef dictionary = static_cast<WKDictionaryRef>(messageBody);
    uint64_t numberOfElements = WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(dictionary, Util::toWK("NumberOfControls").get())));

    EXPECT_EQ(expectedNumberOfElements, numberOfElements);

    if (!receivedMessageForAddingForm) {
        receivedMessageForAddingForm = true;

        WKPageRef page = static_cast<WKPageRef>(WKDictionaryGetItemForKey(dictionary, Util::toWK("Page").get()));
        WKPageRunJavaScriptInMainFrame(page, Util::toWK("addPasswordFieldToForm()").get(), 0, nullJavaScriptCallback);

        return;
    }

    didReceiveAllMessages = true;
}

static void setInjectedBundleClient(WKContextRef context)
{
    WKContextInjectedBundleClient injectedBundleClient;
    memset(&injectedBundleClient, 0, sizeof(injectedBundleClient));
    injectedBundleClient.didReceiveMessageFromInjectedBundle = didReceiveMessageFromInjectedBundle;

    WKContextSetInjectedBundleClient(context, &injectedBundleClient);
}

TEST(WebKit2, DidAssociateFormControls)
{
    WKRetainPtr<WKContextRef> context = adoptWK(Util::createContextForInjectedBundleTest("DidAssociateFormControlsTest"));
    setInjectedBundleClient(context.get());

    PlatformWebView webView(context.get());
    WKPageLoadURL(webView.page(), adoptWK(Util::createURLForResource("associate-form-controls", "html")).get());
    Util::run(&didReceiveAllMessages);
}

} // namespace TestWebKitAPI
