/*
 * Copyright (C) 2009 Julien Chaffraix <jchaffraix@pleyo.com>
 * Copyright (C) 2010, 2012 Research In Motion Limited. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#ifndef CookieParser_h
#define CookieParser_h

#include "KURL.h"
#include <wtf/Vector.h>

namespace WTF {
class String;
}
namespace WebCore {

class ParsedCookie;

class CookieParser {
public:
    CookieParser(const KURL& defaultCookieURL);
    ~CookieParser();

    // Parses a sequence of "Cookie:" header and return the parsed cookies.
    Vector<RefPtr<ParsedCookie> > parse(const String& cookies);

    PassRefPtr<ParsedCookie> parseOneCookie(const String& cookie);

private:
    // FIXME: curTime, start, end parameters should be removed. And this method can be public.
    PassRefPtr<ParsedCookie> parseOneCookie(const String& cookie, unsigned start, unsigned end, double curTime);

    KURL m_defaultCookieURL;
    String m_defaultCookieHost;
    bool m_defaultDomainIsIPAddress;
};

} // namespace WebCore

#endif // CookieParser_h
