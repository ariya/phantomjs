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

#ifndef PlatformUtilities_h
#define PlatformUtilities_h

#include <WebKit2/WKRetainPtr.h>
#include <string>

#if PLATFORM(MAC)
#if __OBJC__
@class NSString;
#else
class NSString;
#endif
#endif

namespace TestWebKitAPI {
namespace Util {

// Runs a platform runloop until the 'done' is true. 
void run(bool* done);

#if PLATFORM(WIN)
bool shouldTranslateMessage(const MSG&);
#endif

void sleep(double seconds);

WKContextRef createContextWithInjectedBundle();
WKContextRef createContextForInjectedBundleTest(const std::string&, WKTypeRef userData = 0);
WKDictionaryRef createInitializationDictionaryForInjectedBundleTest(const std::string&, WKTypeRef userData);

WKStringRef createInjectedBundlePath();
WKURLRef createURLForResource(const char* resource, const char* extension);
WKURLRef URLForNonExistentResource();
WKRetainPtr<WKStringRef> MIMETypeForWKURLResponse(WKURLResponseRef);

bool isKeyDown(WKNativeEventPtr);

std::string toSTD(WKStringRef);
std::string toSTD(WKRetainPtr<WKStringRef>);
std::string toSTD(const char*);
#if PLATFORM(MAC)
std::string toSTD(NSString *);
#endif

WKRetainPtr<WKStringRef> toWK(const char* utf8String);

template<typename T, typename U>
static inline ::testing::AssertionResult assertWKStringEqual(const char* expected_expression, const char* actual_expression, T expected, U actual)
{
    return ::testing::internal::CmpHelperSTREQ(expected_expression, actual_expression, Util::toSTD(expected).c_str(), Util::toSTD(actual).c_str());
}

#define EXPECT_WK_STREQ(expected, actual) \
    EXPECT_PRED_FORMAT2(TestWebKitAPI::Util::assertWKStringEqual, expected, actual)

} // namespace Util
} // namespace TestWebKitAPI

#endif // PlatformUtilities_h
