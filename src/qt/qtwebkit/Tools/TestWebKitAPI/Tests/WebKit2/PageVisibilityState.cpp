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
#include <JavaScriptCore/JSContextRef.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKSerializedScriptValue.h>

namespace TestWebKitAPI {

static bool testDone;

static void didRunStep1StateChangeVisibleToHidden(WKSerializedScriptValueRef, WKErrorRef, void*);
static void didRunStep2StateChangeHiddenToPrerender(WKSerializedScriptValueRef, WKErrorRef, void*);
static void didRunStep3StateChangePrerenderToUnloaded(WKSerializedScriptValueRef, WKErrorRef, void*);
static void didRunStep4InStateUnloaded(WKSerializedScriptValueRef, WKErrorRef, void*);

static void setPageVisibilityStateWithEvalContinuation(PlatformWebView* webView, WKPageVisibilityState visibilityState, WKPageRunJavaScriptFunction callback)
{
    WKPageSetVisibilityState(webView->page(), visibilityState, false);
    WKRetainPtr<WKStringRef> javaScriptString(AdoptWK, WKStringCreateWithUTF8CString("document.visibilityState"));
    WKPageRunJavaScriptInMainFrame(webView->page(), javaScriptString.get(), static_cast<void*>(webView), callback);
}

static void assertSerializedScriptValueIsStringValue(WKSerializedScriptValueRef serializedValue, WKErrorRef error, const char *expected)
{
    EXPECT_NULL(error);
    EXPECT_NOT_NULL(serializedValue);
    if (error || !serializedValue)
        return;

    JSGlobalContextRef scriptContext = JSGlobalContextCreate(0);
    JSValueRef scriptValue = WKSerializedScriptValueDeserialize(serializedValue, scriptContext, 0);
    EXPECT_TRUE(JSValueIsString(scriptContext, scriptValue));
    if (!JSValueIsString(scriptContext, scriptValue)) {
        JSGlobalContextRelease(scriptContext);
        return;
    }

    JSStringRef expectedString = JSStringCreateWithUTF8CString(expected);
    JSStringRef scriptString = JSValueToStringCopy(scriptContext, scriptValue, 0);
    EXPECT_TRUE(JSStringIsEqual(scriptString, expectedString));

    JSStringRelease(scriptString);
    JSStringRelease(expectedString);
    JSGlobalContextRelease(scriptContext);
}

static void didRunStep1StateChangeVisibleToHidden(WKSerializedScriptValueRef resultSerializedScriptValue, WKErrorRef error, void* context)
{
    assertSerializedScriptValueIsStringValue(resultSerializedScriptValue, error, "visible");
    setPageVisibilityStateWithEvalContinuation(static_cast<PlatformWebView*>(context), kWKPageVisibilityStateHidden, didRunStep2StateChangeHiddenToPrerender);
}

static void didRunStep2StateChangeHiddenToPrerender(WKSerializedScriptValueRef resultSerializedScriptValue, WKErrorRef error, void* context)
{
    assertSerializedScriptValueIsStringValue(resultSerializedScriptValue, error, "hidden");
    setPageVisibilityStateWithEvalContinuation(static_cast<PlatformWebView*>(context), kWKPageVisibilityStatePrerender, didRunStep3StateChangePrerenderToUnloaded);
}

static void didRunStep3StateChangePrerenderToUnloaded(WKSerializedScriptValueRef resultSerializedScriptValue, WKErrorRef error, void* context)
{
    assertSerializedScriptValueIsStringValue(resultSerializedScriptValue, error, "prerender");
    setPageVisibilityStateWithEvalContinuation(static_cast<PlatformWebView*>(context), kWKPageVisibilityStateUnloaded, didRunStep4InStateUnloaded);
}

static void didRunStep4InStateUnloaded(WKSerializedScriptValueRef resultSerializedScriptValue, WKErrorRef error, void* context)
{
    assertSerializedScriptValueIsStringValue(resultSerializedScriptValue, error, "unloaded");
    testDone = true;
}

TEST(WebKit2, PageVisibilityState)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());

    // Pass the PlatformWebView webView on as the context of the evals, so we can continue to eval on it.
    PlatformWebView webView(context.get());
    setPageVisibilityStateWithEvalContinuation(&webView, kWKPageVisibilityStateVisible, didRunStep1StateChangeVisibleToHidden);

    Util::run(&testDone);
}

} // namespace TestWebKitAPI
