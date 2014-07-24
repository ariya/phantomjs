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
#include "WKString.h"
#include "WKStringPrivate.h"

#include "WKAPICast.h"

using namespace WebKit;

WKTypeID WKStringGetTypeID()
{
    return toAPI(WebString::APIType);
}

WKStringRef WKStringCreateWithUTF8CString(const char* string)
{
    RefPtr<WebString> webString = WebString::createFromUTF8String(string);
    return toAPI(webString.release().leakRef());
}

bool WKStringIsEmpty(WKStringRef stringRef)
{
    return toImpl(stringRef)->isEmpty();
}

size_t WKStringGetLength(WKStringRef stringRef)
{
    return toImpl(stringRef)->length();
}

size_t WKStringGetCharacters(WKStringRef stringRef, WKChar* buffer, size_t bufferLength)
{
    COMPILE_ASSERT(sizeof(WKChar) == sizeof(UChar), WKStringGetCharacters_sizeof_WKChar_matches_UChar);
    return (toImpl(stringRef)->getCharacters(static_cast<UChar*>(buffer), bufferLength));
}

size_t WKStringGetMaximumUTF8CStringSize(WKStringRef stringRef)
{
    return toImpl(stringRef)->maximumUTF8CStringSize();
}

size_t WKStringGetUTF8CString(WKStringRef stringRef, char* buffer, size_t bufferSize)
{
    return toImpl(stringRef)->getUTF8CString(buffer, bufferSize);
}

bool WKStringIsEqual(WKStringRef aRef, WKStringRef bRef)
{
    return toImpl(aRef)->equal(toImpl(bRef));
}

bool WKStringIsEqualToUTF8CString(WKStringRef aRef, const char* b)
{
    return toImpl(aRef)->equalToUTF8String(b);
}

bool WKStringIsEqualToUTF8CStringIgnoringCase(WKStringRef aRef, const char* b)
{
    return toImpl(aRef)->equalToUTF8StringIgnoringCase(b);
}

WKStringRef WKStringCreateWithJSString(JSStringRef jsStringRef)
{
    RefPtr<WebString> webString = WebString::create(jsStringRef);
    return toAPI(webString.release().leakRef());
}

JSStringRef WKStringCopyJSString(WKStringRef stringRef)
{
    return toImpl(stringRef)->createJSString();
}
