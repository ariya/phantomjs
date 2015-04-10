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
#include <wtf/RetainPtr.h>

#import <WebKit/WebView.h>
#import <WebKit/WebPreferences.h>

@interface WebView (WebViewOtherInternal)
+ (WebCacheModel)_cacheModel;
@end

namespace TestWebKitAPI {

TEST(WebKit1, SetAndUpdateCacheModelInitialModel)
{
    EXPECT_EQ((int)WebCacheModelDocumentViewer, (int)[WebView _cacheModel]);

    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);

    EXPECT_EQ((int)WebCacheModelDocumentBrowser, (int)[WebView _cacheModel]);
}

TEST(WebKit1, SetAndUpdateCacheModelStandardPreferenceChange)
{
    EXPECT_EQ((int)WebCacheModelDocumentViewer, (int)[WebView _cacheModel]);

    WebPreferences *standardPreferences = [WebPreferences standardPreferences];
    EXPECT_EQ((int)WebCacheModelDocumentBrowser, (int)[WebView _cacheModel]);

    [standardPreferences setCacheModel:WebCacheModelPrimaryWebBrowser];
    EXPECT_EQ((int)WebCacheModelPrimaryWebBrowser, (int)[WebView _cacheModel]);

    [standardPreferences setCacheModel:WebCacheModelDocumentViewer];
    EXPECT_EQ((int)WebCacheModelDocumentViewer, (int)[WebView _cacheModel]);
}

TEST(WebKit1, SetAndUpdateCacheModelPreferencesChangeMix)
{
    // On change, the cache model always take the highest value of any preference bound to a WebView.
    EXPECT_EQ((int)WebCacheModelDocumentViewer, (int)[WebView _cacheModel]);

    WebPreferences *standardPreferences = [WebPreferences standardPreferences];
    RetainPtr<WebPreferences> customPreferences = adoptNS([[WebPreferences alloc] initWithIdentifier:@"SetAndUpdateCacheModelPreferencesChangeMix"]);

    // 1) The customPreferences is not set on a view.
    EXPECT_EQ((int)WebCacheModelDocumentBrowser, (int)[WebView _cacheModel]);

    [standardPreferences setCacheModel:WebCacheModelPrimaryWebBrowser];
    EXPECT_EQ((int)WebCacheModelPrimaryWebBrowser, (int)[WebView _cacheModel]);

    [standardPreferences setCacheModel:WebCacheModelDocumentViewer];
    EXPECT_EQ((int)WebCacheModelDocumentViewer, (int)[WebView _cacheModel]);
    [customPreferences.get() setCacheModel:WebCacheModelPrimaryWebBrowser];
    EXPECT_EQ((int)WebCacheModelPrimaryWebBrowser, (int)[WebView _cacheModel]);


    // 2) The cache model should follow the highest value of cache model between the two preferences.
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    [webView.get() setPreferences:customPreferences.get()];
    EXPECT_EQ((int)WebCacheModelPrimaryWebBrowser, (int)[WebView _cacheModel]);

    [customPreferences.get() setCacheModel:WebCacheModelDocumentBrowser];
    EXPECT_EQ((int)WebCacheModelDocumentBrowser, (int)[WebView _cacheModel]);

    [standardPreferences setCacheModel:WebCacheModelPrimaryWebBrowser];
    EXPECT_EQ((int)WebCacheModelPrimaryWebBrowser, (int)[WebView _cacheModel]);
    [customPreferences.get() setCacheModel:WebCacheModelDocumentViewer];
    EXPECT_EQ((int)WebCacheModelPrimaryWebBrowser, (int)[WebView _cacheModel]);

    // 3) Resetting the view should fall back to standardPreferences.
    [standardPreferences setCacheModel:WebCacheModelDocumentViewer];
    [customPreferences.get() setCacheModel:WebCacheModelPrimaryWebBrowser];
    EXPECT_EQ((int)WebCacheModelPrimaryWebBrowser, (int)[WebView _cacheModel]);

    webView.clear();
    EXPECT_EQ((int)WebCacheModelDocumentViewer, (int)[WebView _cacheModel]);
}

} // namespace TestWebKitAPI
