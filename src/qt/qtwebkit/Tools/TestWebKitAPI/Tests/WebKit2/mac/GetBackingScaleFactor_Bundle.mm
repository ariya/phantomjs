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

#import "config.h"

#import "InjectedBundleTest.h"
#import "PlatformUtilities.h"
#import <WebKit2/WKBundlePage.h>
#import <assert.h>

namespace TestWebKitAPI {

class GetBackingScaleFactorTest : public InjectedBundleTest {
public:
    GetBackingScaleFactorTest(const std::string&);

private:
    virtual void didCreatePage(WKBundleRef, WKBundlePageRef);
    virtual void didReceiveMessage(WKBundleRef, WKStringRef messageName, WKTypeRef messageBody);

    WKBundlePageRef m_page;
};

static InjectedBundleTest::Register<GetBackingScaleFactorTest> registrar("GetBackingScaleFactorTest");

GetBackingScaleFactorTest::GetBackingScaleFactorTest(const std::string& identifier)
    : InjectedBundleTest(identifier)
    , m_page(0)
{
}

void GetBackingScaleFactorTest::didCreatePage(WKBundleRef, WKBundlePageRef page)
{
    assert(!m_page);
    m_page = page;
}

void GetBackingScaleFactorTest::didReceiveMessage(WKBundleRef bundle, WKStringRef messageName, WKTypeRef messageBody)
{
    if (!WKStringIsEqualToUTF8CString(messageName, "GetBackingScaleFactor"))
        return;

    WKRetainPtr<WKDoubleRef> backingScaleFactor = adoptWK(WKDoubleCreate(WKBundlePageGetBackingScaleFactor(m_page)));
    WKBundlePostMessage(bundle, Util::toWK("DidGetBackingScaleFactor").get(), backingScaleFactor.get());
}

} // namespace TestWebKitAPI
