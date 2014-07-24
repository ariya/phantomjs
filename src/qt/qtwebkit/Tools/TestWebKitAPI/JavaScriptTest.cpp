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
#include "JavaScriptTest.h"

#include "PlatformUtilities.h"
#include "Test.h"
#include <JavaScriptCore/JSContextRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKSerializedScriptValue.h>
#include <wtf/OwnArrayPtr.h>

namespace TestWebKitAPI {

struct JavaScriptCallbackContext {
    JavaScriptCallbackContext() : didFinish(false) { }

    bool didFinish;
    JSRetainPtr<JSStringRef> actualString;
};

static void javaScriptCallback(WKSerializedScriptValueRef resultSerializedScriptValue, WKErrorRef error, void* ctx)
{
    ASSERT_NOT_NULL(resultSerializedScriptValue);

    JavaScriptCallbackContext* context = static_cast<JavaScriptCallbackContext*>(ctx);

    JSGlobalContextRef scriptContext = JSGlobalContextCreate(0);
    ASSERT_NOT_NULL(scriptContext);

    JSValueRef scriptValue = WKSerializedScriptValueDeserialize(resultSerializedScriptValue, scriptContext, 0);
    ASSERT_NOT_NULL(scriptValue);

    context->actualString.adopt(JSValueToStringCopy(scriptContext, scriptValue, 0));
    ASSERT_NOT_NULL(context->actualString.get());

    context->didFinish = true;

    JSGlobalContextRelease(scriptContext);

    EXPECT_NULL(error);
}

::testing::AssertionResult runJSTest(const char*, const char*, const char*, WKPageRef page, const char* script, const char* expectedResult)
{
    JavaScriptCallbackContext context;
    WKPageRunJavaScriptInMainFrame(page, Util::toWK(script).get(), &context, javaScriptCallback);
    Util::run(&context.didFinish);

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(context.actualString.get());
    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[bufferSize]);
    JSStringGetUTF8CString(context.actualString.get(), buffer.get(), bufferSize);
    
    return compareJSResult(script, buffer.get(), expectedResult);
}
    
::testing::AssertionResult compareJSResult(const char* script, const char* actualResult, const char* expectedResult)
{
    if (!strcmp(actualResult, expectedResult))
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure()
        << "JS expression: " << script << "\n"
        << "       Actual: " << actualResult << "\n"
        << "     Expected: " << expectedResult;
}

} // namespace TestWebKitAPI
