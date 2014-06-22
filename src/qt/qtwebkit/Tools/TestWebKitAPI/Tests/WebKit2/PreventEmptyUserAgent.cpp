/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include <JavaScriptCore/JSContextRef.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKSerializedScriptValue.h>

namespace TestWebKitAPI {

static bool testDone;

static void didRunJavaScript(WKSerializedScriptValueRef resultSerializedScriptValue, WKErrorRef error, void* context)
{
    EXPECT_EQ(reinterpret_cast<void*>(0x1234578), context);
    EXPECT_NOT_NULL(resultSerializedScriptValue);

    JSGlobalContextRef scriptContext = JSGlobalContextCreate(0);
    JSValueRef scriptValue = WKSerializedScriptValueDeserialize(resultSerializedScriptValue, scriptContext, 0);
    EXPECT_TRUE(JSValueIsString(scriptContext, scriptValue));

    // Make sure that the result of navigator.userAgent isn't empty, even if we set the custom
    // user agent to the empty string.
    JSStringRef scriptString = JSValueToStringCopy(scriptContext, scriptValue, 0);
    EXPECT_GT(JSStringGetLength(scriptString), 0u);

    JSStringRelease(scriptString);
    JSGlobalContextRelease(scriptContext);

    testDone = true;
}

TEST(WebKit2, PreventEmptyUserAgent)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    PlatformWebView webView(context.get());

    WKPageSetCustomUserAgent(webView.page(), WKStringCreateWithUTF8CString(""));
    WKRetainPtr<WKStringRef> javaScriptString(AdoptWK, WKStringCreateWithUTF8CString("navigator.userAgent"));
    WKPageRunJavaScriptInMainFrame(webView.page(), javaScriptString.get(), reinterpret_cast<void*>(0x1234578), didRunJavaScript);

    Util::run(&testDone);
}

} // namespace TestWebKitAPI
