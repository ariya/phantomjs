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

#ifndef WebString_h
#define WebString_h

#include "APIObject.h"
#include <JavaScriptCore/InitializeThreading.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>
#include <wtf/unicode/UTF8.h>

namespace WebKit {

// WebString - A string type suitable for vending to an API.

class WebString : public TypedAPIObject<APIObject::TypeString> {
public:
    static PassRefPtr<WebString> createNull()
    {
        return adoptRef(new WebString());
    }

    static PassRefPtr<WebString> create(const String& string)
    {
        return adoptRef(new WebString(string));
    }

    static PassRefPtr<WebString> create(JSStringRef jsStringRef)
    {
        return adoptRef(new WebString(String(jsStringRef->string())));
    }

    static PassRefPtr<WebString> createFromUTF8String(const char* string)
    {
        return adoptRef(new WebString(String::fromUTF8(string)));
    }

    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return m_string.isEmpty(); }
    
    size_t length() const { return m_string.length(); }
    size_t getCharacters(UChar* buffer, size_t bufferLength) const
    {
        if (!bufferLength)
            return 0;
        bufferLength = std::min(bufferLength, static_cast<size_t>(m_string.length()));
        memcpy(buffer, m_string.characters(), bufferLength * sizeof(UChar));
        return bufferLength;
    }

    size_t maximumUTF8CStringSize() const { return m_string.length() * 3 + 1; }
    size_t getUTF8CString(char* buffer, size_t bufferSize)
    {
        if (!bufferSize)
            return 0;
        char* p = buffer;
        const UChar* d = m_string.characters();
        WTF::Unicode::ConversionResult result = WTF::Unicode::convertUTF16ToUTF8(&d, d + m_string.length(), &p, p + bufferSize - 1, /* strict */ true);
        *p++ = '\0';
        if (result != WTF::Unicode::conversionOK && result != WTF::Unicode::targetExhausted)
            return 0;
        return p - buffer;
    }

    bool equal(WebString* other) { return m_string == other->m_string; }
    bool equalToUTF8String(const char* other) { return m_string == String::fromUTF8(other); }
    bool equalToUTF8StringIgnoringCase(const char* other) { return equalIgnoringCase(m_string, other); }

    const String& string() const { return m_string; }

    JSStringRef createJSString() const
    {
        JSC::initializeThreading();
        return OpaqueJSString::create(m_string).leakRef();
    }

private:
    WebString()
        : m_string()
    {
    }

    WebString(const String& string)
        : m_string(!string.impl() ? String(StringImpl::empty()) : string)
    {
    }

    String m_string;
};

} // namespace WebKit

#endif // WebString_h
