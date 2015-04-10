/*
 * Copyright (C) 2006, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "DOMInternal.h" // import first to make the private/public trick work
#import "DOMXPath.h"

#import "WebScriptObjectPrivate.h"
#import "XPathNSResolver.h"
#import <wtf/text/WTFString.h>

//------------------------------------------------------------------------------------------
// DOMNativeXPathNSResolver

@implementation DOMNativeXPathNSResolver

#define IMPL reinterpret_cast<WebCore::XPathNSResolver*>(_internal)

- (void)dealloc
{
    if (_internal)
        IMPL->deref();
    [super dealloc];
}

- (void)finalize
{
    if (_internal)
        IMPL->deref();
    [super finalize];
}

- (NSString *)lookupNamespaceURI:(NSString *)prefix
{
    return IMPL->lookupNamespaceURI(prefix);
}

@end

WebCore::XPathNSResolver* core(DOMNativeXPathNSResolver *wrapper)
{
    return wrapper ? reinterpret_cast<WebCore::XPathNSResolver*>(wrapper->_internal) : 0;
}

DOMNativeXPathNSResolver *kit(WebCore::XPathNSResolver* impl)
{
    if (!impl)
        return nil;
    
    if (DOMNativeXPathNSResolver *wrapper = getDOMWrapper(impl))
        return [[wrapper retain] autorelease];
    
    DOMNativeXPathNSResolver *wrapper = [[DOMNativeXPathNSResolver alloc] _init];
    wrapper->_internal = reinterpret_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(wrapper, impl);
    return [wrapper autorelease];    
}
