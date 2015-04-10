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

#import "WKDOMInternals.h"

#import <WebCore/Document.h>
#import <WebCore/Element.h>
#import <WebCore/Node.h>
#import <WebCore/Range.h>
#import <WebCore/Text.h>

// Classes to instantiate.
#import "WKDOMElement.h"
#import "WKDOMDocument.h"
#import "WKDOMText.h"

namespace WebKit {

template<typename WebCoreType, typename WKDOMType>
static WKDOMType toWKDOMType(WebCoreType impl, DOMCache<WebCoreType, WKDOMType>& cache);

// -- Caches -- 

DOMCache<WebCore::Node*, WKDOMNode *>& WKDOMNodeCache()
{
    typedef DOMCache<WebCore::Node*, WKDOMNode *> Cache;
    DEFINE_STATIC_LOCAL(Cache, cache, ());
    return cache;
}

DOMCache<WebCore::Range*, WKDOMRange *>& WKDOMRangeCache()
{
    typedef DOMCache<WebCore::Range*, WKDOMRange *> Cache;
    DEFINE_STATIC_LOCAL(Cache, cache, ());
    return cache;
}

// -- Node and classes derived from Node. --

static Class WKDOMNodeClass(WebCore::Node* impl)
{
    switch (impl->nodeType()) {
    case WebCore::Node::ELEMENT_NODE:
        return [WKDOMElement class];
    case WebCore::Node::DOCUMENT_NODE:
        return [WKDOMDocument class];
    case WebCore::Node::TEXT_NODE:
        return [WKDOMText class];
    case WebCore::Node::ATTRIBUTE_NODE:
    case WebCore::Node::CDATA_SECTION_NODE:
    case WebCore::Node::ENTITY_REFERENCE_NODE:
    case WebCore::Node::ENTITY_NODE:
    case WebCore::Node::PROCESSING_INSTRUCTION_NODE:
    case WebCore::Node::COMMENT_NODE:
    case WebCore::Node::DOCUMENT_TYPE_NODE:
    case WebCore::Node::DOCUMENT_FRAGMENT_NODE:
    case WebCore::Node::NOTATION_NODE:
    case WebCore::Node::XPATH_NAMESPACE_NODE:
        return [WKDOMNode class];
    }
    ASSERT_NOT_REACHED();
    return nil;
}

static WKDOMNode *initWithImpl(WebCore::Node* impl)
{
    return [[WKDOMNodeClass(impl) alloc] _initWithImpl:impl];
}

WebCore::Node* toWebCoreNode(WKDOMNode *wrapper)
{
    return wrapper ? wrapper->_impl.get() : 0;
}

WKDOMNode *toWKDOMNode(WebCore::Node* impl)
{
    return toWKDOMType<WebCore::Node*, WKDOMNode *>(impl, WKDOMNodeCache());
}

WebCore::Element* toWebCoreElement(WKDOMElement *wrapper)
{
    return wrapper ? reinterpret_cast<WebCore::Element*>(wrapper->_impl.get()) : 0;
}

WKDOMElement *toWKDOMElement(WebCore::Element* impl)
{
    return static_cast<WKDOMElement*>(toWKDOMNode(static_cast<WebCore::Node*>(impl)));
}

WebCore::Document* toWebCoreDocument(WKDOMDocument *wrapper)
{
    return wrapper ? reinterpret_cast<WebCore::Document*>(wrapper->_impl.get()) : 0;
}

WKDOMDocument *toWKDOMDocument(WebCore::Document* impl)
{
    return static_cast<WKDOMDocument*>(toWKDOMNode(static_cast<WebCore::Node*>(impl)));
}

WebCore::Text* toWebCoreText(WKDOMText *wrapper)
{
    return wrapper ? reinterpret_cast<WebCore::Text*>(wrapper->_impl.get()) : 0;
}

WKDOMText *toWKDOMText(WebCore::Text* impl)
{
    return static_cast<WKDOMText*>(toWKDOMNode(static_cast<WebCore::Node*>(impl)));
}

// -- Range. --

static WKDOMRange *initWithImpl(WebCore::Range* impl)
{
    return [[WKDOMRange alloc] _initWithImpl:impl];
}

WebCore::Range* toWebCoreRange(WKDOMRange * wrapper)
{
    return wrapper ? wrapper->_impl.get() : 0;
}

WKDOMRange *toWKDOMRange(WebCore::Range* impl)
{
    return toWKDOMType<WebCore::Range*, WKDOMRange *>(impl, WKDOMRangeCache());
}

// -- Helpers --

template<typename WebCoreType, typename WKDOMType>
static WKDOMType toWKDOMType(WebCoreType impl, DOMCache<WebCoreType, WKDOMType>& cache)
{
    if (!impl)
        return nil;
    if (WKDOMType wrapper = cache.get(impl))
        return [[wrapper retain] autorelease];
    WKDOMType wrapper = initWithImpl(impl);
    if (!wrapper)
        return nil;
    return [wrapper autorelease];
}

NSArray *toNSArray(const Vector<WebCore::IntRect>& rects)
{
    size_t size = rects.size();
    NSMutableArray *array = [NSMutableArray arrayWithCapacity:size];
    for (size_t i = 0; i < size; ++i)
        [array addObject:[NSValue valueWithRect:rects[i]]];
    return array;
}

} // namespace WebKit

#endif // defined(__LP64__) && defined(__clang__)
