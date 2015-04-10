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

#ifndef CookiesStrategy_h
#define CookiesStrategy_h

#include <wtf/HashSet.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class KURL;
class NetworkStorageSession;
struct Cookie;

class CookiesStrategy {
public:
    virtual String cookiesForDOM(const NetworkStorageSession&, const KURL& firstParty, const KURL&) = 0;
    virtual void setCookiesFromDOM(const NetworkStorageSession&, const KURL& firstParty, const KURL&, const String& cookieString) = 0;
    virtual bool cookiesEnabled(const NetworkStorageSession&, const KURL& firstParty, const KURL&) = 0;
    virtual String cookieRequestHeaderFieldValue(const NetworkStorageSession&, const KURL& firstParty, const KURL&) = 0;
    virtual bool getRawCookies(const NetworkStorageSession&, const KURL& firstParty, const KURL&, Vector<Cookie>&) = 0;
    virtual void deleteCookie(const NetworkStorageSession&, const KURL&, const String& cookieName) = 0;

protected:
    virtual ~CookiesStrategy() { }
};

} // namespace WebCore

#endif // CookiesStrategy_h
