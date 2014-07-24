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

#import "config.h"

#if defined(__LP64__) && defined(__clang__)

#import "WKWebProcessPlugInBrowserContextController.h"
#import "WKWebProcessPlugInBrowserContextControllerInternal.h"
#import "WKWebProcessPlugInBrowserContextControllerPrivate.h"

#import "WKBundleAPICast.h"
#import "WKBundlePage.h"
#import "WKBundlePagePrivate.h"
#import "WKDOMInternals.h"
#import "WKRetainPtr.h"
#import "WebPage.h"
#import <WebCore/Document.h>
#import <WebCore/Frame.h>

@interface WKWebProcessPlugInBrowserContextController () {
    // Underlying WKBundlePageRef.
    WKRetainPtr<WKBundlePageRef> _bundlePageRef;
}
@end

@implementation WKWebProcessPlugInBrowserContextController (Internal)

- (id)_initWithBundlePageRef:(WKBundlePageRef)bundlePageRef
{
    self = [super init];
    if (!self)
        return nil;

    _bundlePageRef = bundlePageRef;

    return self;
}

@end

@implementation WKWebProcessPlugInBrowserContextController

- (WKDOMDocument *)mainFrameDocument
{
    WebCore::Frame* webCoreMainFrame = WebKit::toImpl(self._bundlePageRef)->mainFrame();
    if (!webCoreMainFrame)
        return nil;

    return WebKit::toWKDOMDocument(webCoreMainFrame->document());
}

- (WKDOMRange *)selectedRange
{
    RefPtr<WebCore::Range> range = WebKit::toImpl(self._bundlePageRef)->currentSelectionAsRange();
    if (!range)
        return nil;

    return WebKit::toWKDOMRange(range.get());
}

@end

@implementation WKWebProcessPlugInBrowserContextController (Private)

- (WKBundlePageRef)_bundlePageRef
{
    return _bundlePageRef.get();
}

@end

#endif // defined(__LP64__) && defined(__clang__)
