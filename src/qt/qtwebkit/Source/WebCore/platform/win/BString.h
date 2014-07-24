/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef BString_h
#define BString_h

#include <wtf/Forward.h>

#if USE(CF)
typedef const struct __CFString * CFStringRef;
#endif

typedef wchar_t* BSTR;

namespace WebCore {

    class KURL;

    class BString {
    public:
        BString();
        BString(const wchar_t*);
        BString(const wchar_t*, size_t length);
        BString(const String&);
        BString(const AtomicString&);
        BString(const KURL&);
#if USE(CF)
        BString(CFStringRef);
#endif
        ~BString();

        void adoptBSTR(BSTR);
        void clear();

        BString(const BString&);
        BString& operator=(const BString&);
        BString& operator=(const BSTR&);

        BSTR* operator&() { ASSERT(!m_bstr); return &m_bstr; }
        operator BSTR() const { return m_bstr; }

        BSTR release() { BSTR result = m_bstr; m_bstr = 0; return result; }

    private:
        BSTR m_bstr;
    };

    bool operator ==(const BString&, const BString&);
    bool operator !=(const BString&, const BString&);
    bool operator ==(const BString&, BSTR);
    bool operator !=(const BString&, BSTR);
    bool operator ==(BSTR, const BString&);
    bool operator !=(BSTR, const BString&);

}

#endif
