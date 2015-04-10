/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "InjectedBundleTest.h"

#include "PlatformUtilities.h"
#include <WebKit2/WKArray.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundleBackForwardListItem.h>
#include <WebKit2/WKWebArchive.h>

namespace TestWebKitAPI {

class PasteboardNotificationsTest : public InjectedBundleTest {
public:
    PasteboardNotificationsTest(const std::string& identifier);

    virtual void didCreatePage(WKBundleRef, WKBundlePageRef);
};

static InjectedBundleTest::Register<PasteboardNotificationsTest> registrar("PasteboardNotificationsTest");

static void willWriteToPasteboard(WKBundlePageRef page, WKBundleRangeHandleRef range,  const void*)
{
    if (!range)
        WKBundlePostMessage(InjectedBundleController::shared().bundle(), Util::toWK("PasteboardNotificationTestDoneMessageName").get(), Util::toWK("willWritetoPasteboardFail").get());
}

static void getPasteboardDataForRange(WKBundlePageRef, WKBundleRangeHandleRef range, WKArrayRef* pasteboardTypes, WKArrayRef* pasteboardData, const void*)
{
    WKTypeRef typeName = WKStringCreateWithUTF8CString("AnotherArchivePasteboardType");
    *pasteboardTypes = WKArrayCreateAdoptingValues(&typeName, 1);
    WKTypeRef typeData = WKWebArchiveCopyData(WKWebArchiveCreateFromRange(range));
    *pasteboardData = WKArrayCreateAdoptingValues(&typeData, 1);
}

static void didWriteToPasteboard(WKBundlePageRef, const void*)
{
    WKBundlePostMessage(InjectedBundleController::shared().bundle(), Util::toWK("PasteboardNotificationTestDoneMessageName").get(), Util::toWK("didWriteToPasteboard").get());
}

PasteboardNotificationsTest::PasteboardNotificationsTest(const std::string& identifier)
    : InjectedBundleTest(identifier)
{
}

void PasteboardNotificationsTest::didCreatePage(WKBundleRef bundle, WKBundlePageRef page)
{    
    WKBundlePageEditorClient pageEditorClient;

    memset(&pageEditorClient, 0, sizeof(pageEditorClient));

    pageEditorClient.version = 1;
    pageEditorClient.clientInfo = this;
    pageEditorClient.willWriteToPasteboard = willWriteToPasteboard;
    pageEditorClient.getPasteboardDataForRange = getPasteboardDataForRange;
    pageEditorClient.didWriteToPasteboard = didWriteToPasteboard;

    WKBundlePageSetEditorClient(page, &pageEditorClient);
}

} // namespace TestWebKitAPI
