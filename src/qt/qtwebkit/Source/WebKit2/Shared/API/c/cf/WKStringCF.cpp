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

#include "config.h"
#include "WKStringCF.h"

#include "WKAPICast.h"
#include <wtf/text/WTFString.h>

using namespace WebCore;
using namespace WebKit;

WKStringRef WKStringCreateWithCFString(CFStringRef cfString)
{
    String string(cfString);
    return toCopiedAPI(string);
}

CFStringRef WKStringCopyCFString(CFAllocatorRef allocatorRef, WKStringRef stringRef)
{
    ASSERT(!toImpl(stringRef)->string().isNull());

    // NOTE: This does not use StringImpl::createCFString() since that function
    // expects to be called on the thread running WebCore.
    if (toImpl(stringRef)->string().is8Bit())
        return CFStringCreateWithBytes(allocatorRef, reinterpret_cast<const UInt8*>(toImpl(stringRef)->string().characters8()), toImpl(stringRef)->string().length(), kCFStringEncodingISOLatin1, true);
    return CFStringCreateWithCharacters(allocatorRef, reinterpret_cast<const UniChar*>(toImpl(stringRef)->string().characters16()), toImpl(stringRef)->string().length());
}
