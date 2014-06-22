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

#ifndef WebUserContentURLPattern_h
#define WebUserContentURLPattern_h

#include "APIObject.h"

#include <WebCore/KURL.h>
#include <WebCore/UserContentURLPattern.h>
#include <wtf/RefPtr.h>

namespace WebKit {

class WebUserContentURLPattern : public TypedAPIObject<APIObject::TypeUserContentURLPattern> {
public:
    static PassRefPtr<WebUserContentURLPattern> create(const String& pattern)
    {
        return adoptRef(new WebUserContentURLPattern(pattern));
    }

    const String& host() const { return m_pattern.host(); }
    const String& scheme() const { return m_pattern.scheme(); }
    bool isValid() const { return m_pattern.isValid(); };
    bool matchesURL(const String& url) const { return m_pattern.matches(WebCore::KURL(WebCore::ParsedURLString, url)); }
    bool matchesSubdomains() const { return m_pattern.matchSubdomains(); }

    const String& patternString() const { return m_patternString; }

private:
    explicit WebUserContentURLPattern(const String& pattern)
        : m_pattern(WebCore::UserContentURLPattern(pattern))
        , m_patternString(pattern)
    {
    }

    WebCore::UserContentURLPattern m_pattern;
    String m_patternString;
};

}

#endif // WebUserContentURLPattern_h
