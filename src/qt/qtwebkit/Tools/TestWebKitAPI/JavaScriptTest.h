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

#if PLATFORM(MAC)
#ifdef __OBJC__
@class WKView;
@class WebView;
#else
class WKView;
class WebView;
#endif
#endif

namespace TestWebKitAPI {

// These macros execute |script| in the page and wait until it has run, then compare its return
// value to the expected result.
#define EXPECT_JS_EQ(page, script, result) EXPECT_PRED_FORMAT3(runJSTest, page, script, result)
#define EXPECT_JS_FALSE(page, script) EXPECT_JS_EQ(page, script, "false")
#define EXPECT_JS_TRUE(page, script) EXPECT_JS_EQ(page, script, "true")

::testing::AssertionResult runJSTest(const char* pageExpr, const char* scriptExpr, const char* expectedResultExpr, WKPageRef, const char* script, const char* expectedResult);
::testing::AssertionResult compareJSResult(const char* script, const char* actualResult, const char* expectedResult);

#if PLATFORM(MAC)
::testing::AssertionResult runJSTest(const char* webViewExpr, const char* scriptExpr, const char* expectedResultExpr, WebView *, const char* script, const char* expectedResult);
::testing::AssertionResult runJSTest(const char* viewExpr, const char* scriptExpr, const char* expectedResultExpr, WKView *, const char* script, const char* expectedResult);
#endif

} // namespace TestWebKitAPI
