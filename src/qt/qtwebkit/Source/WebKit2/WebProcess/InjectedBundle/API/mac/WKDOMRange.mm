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

#import "WKDOMRange.h"
#import "WKDOMRangePrivate.h"

#import "InjectedBundleRangeHandle.h"
#import "WKBundleAPICast.h"
#import "WKDOMInternals.h"
#import <WebCore/Document.h>

@implementation WKDOMRange

- (id)_initWithImpl:(WebCore::Range*)impl
{
    self = [super init];
    if (!self)
        return nil;

    _impl = impl;
    WebKit::WKDOMRangeCache().add(impl, self);

    return self;
}

- (id)initWithDocument:(WKDOMDocument *)document
{
    RefPtr<WebCore::Range> range = WebCore::Range::create(WebKit::toWebCoreDocument(document));
    self = [self _initWithImpl:range.get()];
    if (!self)
        return nil;

    return self;
}

- (void)dealloc
{
    WebKit::WKDOMRangeCache().remove(_impl.get());
    [super dealloc];
}

- (void)setStart:(WKDOMNode *)node offset:(int)offset
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    _impl->setStart(WebKit::toWebCoreNode(node), offset, ec);
}

- (void)setEnd:(WKDOMNode *)node offset:(int)offset
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    _impl->setEnd(WebKit::toWebCoreNode(node), offset, ec);
}

- (void)collapse:(BOOL)toStart
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    _impl->collapse(toStart, ec);
}

- (void)selectNode:(WKDOMNode *)node
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    _impl->selectNode(WebKit::toWebCoreNode(node), ec);
}

- (void)selectNodeContents:(WKDOMNode *)node
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    _impl->selectNodeContents(WebKit::toWebCoreNode(node), ec);
}

- (WKDOMNode *)startContainer
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    return WebKit::toWKDOMNode(_impl->startContainer(ec));
}

- (NSInteger)startOffset
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    return _impl->startOffset(ec);
}

- (WKDOMNode *)endContainer
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    return WebKit::toWKDOMNode(_impl->endContainer(ec));
}

- (NSInteger)endOffset
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    return _impl->endOffset(ec);
}

- (NSString *)text
{
    return _impl->text();
}

- (BOOL)isCollapsed
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    return _impl->collapsed(ec);
}

- (NSArray *)textRects
{
    _impl->ownerDocument()->updateLayoutIgnorePendingStylesheets();
    Vector<WebCore::IntRect> rects;
    _impl->textRects(rects);
    return WebKit::toNSArray(rects);
}

@end

@implementation WKDOMRange (WKPrivate)

- (WKBundleRangeHandleRef)_copyBundleRangeHandleRef
{
    RefPtr<WebKit::InjectedBundleRangeHandle> rangeHandle = WebKit::InjectedBundleRangeHandle::getOrCreate(_impl.get());
    return toAPI(rangeHandle.release().leakRef());
}

@end

#endif // defined(__LP64__) && defined(__clang__)
