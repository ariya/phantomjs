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
#import "SecItemShimLibrary.h"

#import <Security/SecItem.h>
#import <WebCore/DynamicLinkerInterposing.h>
#import <wtf/Platform.h>

namespace WebKit {

extern "C" void WebKitSecItemShimInitialize(const SecItemShimCallbacks&);

static SecItemShimCallbacks secItemShimCallbacks;

static OSStatus shimSecItemCopyMatching(CFDictionaryRef query, CFTypeRef* result)
{
    return secItemShimCallbacks.secItemCopyMatching(query, result);
}

static OSStatus shimSecItemAdd(CFDictionaryRef query, CFTypeRef* result)
{
    return secItemShimCallbacks.secItemAdd(query, result);
}

static OSStatus shimSecItemUpdate(CFDictionaryRef query, CFDictionaryRef attributesToUpdate)
{
    return secItemShimCallbacks.secItemUpdate(query, attributesToUpdate);
}

static OSStatus shimSecItemDelete(CFDictionaryRef query)
{
    return secItemShimCallbacks.secItemDelete(query);
}

DYLD_INTERPOSE(shimSecItemCopyMatching, SecItemCopyMatching)
DYLD_INTERPOSE(shimSecItemAdd, SecItemAdd)
DYLD_INTERPOSE(shimSecItemUpdate, SecItemUpdate)
DYLD_INTERPOSE(shimSecItemDelete, SecItemDelete)

__attribute__((visibility("default")))
void WebKitSecItemShimInitialize(const SecItemShimCallbacks& callbacks)
{
    secItemShimCallbacks = callbacks;
}

} // namespace WebKit
