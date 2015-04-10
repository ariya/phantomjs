/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef WKEinaSharedString_h
#define WKEinaSharedString_h

#include <Eina.h>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>

namespace WebKit {

class WK_EXPORT WKEinaSharedString {
public:
    ALWAYS_INLINE WKEinaSharedString() : m_string(0) { }
    WKEinaSharedString(const WKEinaSharedString& other);
    WKEinaSharedString(const char* str);

    WKEinaSharedString(WKAdoptTag, WKStringRef);
    WKEinaSharedString(WKStringRef);

    WKEinaSharedString(WKAdoptTag, WKURLRef);
    WKEinaSharedString(WKURLRef);

    ~WKEinaSharedString();

    Eina_Stringshare* leakString();

    WKEinaSharedString& operator=(const WKEinaSharedString& other);
    WKEinaSharedString& operator=(const char* str);

    ALWAYS_INLINE bool operator==(const WKEinaSharedString& other) const { return this->m_string == other.m_string; }
    ALWAYS_INLINE bool operator!=(const WKEinaSharedString& other) const { return !(*this == other); }

    bool operator==(const char* str) const;
    ALWAYS_INLINE bool operator!=(const char* str) const { return !(*this == str); }

    ALWAYS_INLINE operator const char* () const { return m_string; }

    ALWAYS_INLINE bool isNull() const { return !m_string; }

    ALWAYS_INLINE size_t length() const { return m_string ? static_cast<size_t>(eina_stringshare_strlen(m_string)) : 0; }

    static WKEinaSharedString adopt(Eina_Stringshare*);

private:
    const char* m_string;
};

} // namespace WebKit

using WebKit::WKEinaSharedString;

#endif // WKEinaSharedString_h
