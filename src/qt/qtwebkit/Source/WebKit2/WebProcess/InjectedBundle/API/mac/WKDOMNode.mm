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

#import "WKDOMNode.h"
#import "WKDOMNodePrivate.h"

#import "InjectedBundleNodeHandle.h"
#import "WKBundleAPICast.h"
#import "WKDOMInternals.h"
#import <WebCore/Document.h>

@implementation WKDOMNode

- (id)_initWithImpl:(WebCore::Node*)impl
{
    self = [super init];
    if (!self)
        return nil;

    _impl = impl;
    WebKit::WKDOMNodeCache().add(impl, self);

    return self;
}

- (void)dealloc
{
    WebKit::WKDOMNodeCache().remove(_impl.get());
    [super dealloc];
}

- (void)insertNode:(WKDOMNode *)node before:(WKDOMNode *)refNode
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec;
    _impl->insertBefore(WebKit::toWebCoreNode(node), WebKit::toWebCoreNode(refNode), ec);
}

- (void)appendChild:(WKDOMNode *)node
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec;
    _impl->appendChild(WebKit::toWebCoreNode(node), ec);
}

- (void)removeChild:(WKDOMNode *)node
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec;
    _impl->removeChild(WebKit::toWebCoreNode(node), ec);
}

- (WKDOMDocument *)document
{
    return WebKit::toWKDOMDocument(_impl->document());
}

- (WKDOMNode *)parentNode
{
    return WebKit::toWKDOMNode(_impl->parentNode());
}

- (WKDOMNode *)firstChild
{
    return WebKit::toWKDOMNode(_impl->firstChild());
}

- (WKDOMNode *)lastChild
{
    return WebKit::toWKDOMNode(_impl->lastChild());
}

- (WKDOMNode *)previousSibling
{
    return WebKit::toWKDOMNode(_impl->previousSibling());
}

- (WKDOMNode *)nextSibling
{
    return WebKit::toWKDOMNode(_impl->nextSibling());
}

- (NSArray *)textRects
{
    _impl->document()->updateLayoutIgnorePendingStylesheets();
    if (!_impl->renderer())
        return nil;
    Vector<WebCore::IntRect> rects;
    _impl->textRects(rects);
    return WebKit::toNSArray(rects);
}

@end

@implementation WKDOMNode (WKPrivate)

- (WKBundleNodeHandleRef)_copyBundleNodeHandleRef
{
    RefPtr<WebKit::InjectedBundleNodeHandle> nodeHandle = WebKit::InjectedBundleNodeHandle::getOrCreate(_impl.get());
    return toAPI(nodeHandle.release().leakRef());
}

@end

#endif // defined(__LP64__) && defined(__clang__)
