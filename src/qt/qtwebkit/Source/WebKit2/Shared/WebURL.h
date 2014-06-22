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

#ifndef WebURL_h
#define WebURL_h

#include "APIObject.h"
#include <WebCore/KURL.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

// WebURL - A URL type suitable for vending to an API.

class WebURL : public TypedAPIObject<APIObject::TypeURL> {
public:
    static PassRefPtr<WebURL> create(const String& string)
    {
        return adoptRef(new WebURL(string));
    }

    static PassRefPtr<WebURL> create(const WebURL* baseURL, const String& relativeURL)
    {
        using WebCore::KURL;

        ASSERT(baseURL);
        baseURL->parseURLIfNecessary();
        KURL* absoluteURL = new KURL(*baseURL->m_parsedURL.get(), relativeURL);

        return adoptRef(new WebURL(adoptPtr(absoluteURL), absoluteURL->string()));
    }

    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return m_string.isEmpty(); }

    const String& string() const { return m_string; }

    String host() const
    {
        parseURLIfNecessary();
        return m_parsedURL->isValid() ? m_parsedURL->host() : String();
    }

    String protocol() const
    {
        parseURLIfNecessary();
        return m_parsedURL->isValid() ? m_parsedURL->protocol() : String();
    }

    String path() const
    {
        parseURLIfNecessary();
        return m_parsedURL->isValid() ? m_parsedURL->path() : String();
    }

    String lastPathComponent() const
    {
        parseURLIfNecessary();
        return m_parsedURL->isValid() ? m_parsedURL->lastPathComponent() : String();
    }

private:
    WebURL(const String& string)
        : m_string(string)
    {
    }

    WebURL(PassOwnPtr<WebCore::KURL> parsedURL, const String& string)
        : m_string(string)
        , m_parsedURL(parsedURL)
    {
    }

    void parseURLIfNecessary() const
    {
        if (m_parsedURL)
            return;
        m_parsedURL = WTF::adoptPtr(new WebCore::KURL(WebCore::KURL(), m_string));
    }

    String m_string;
    mutable OwnPtr<WebCore::KURL> m_parsedURL;
};

} // namespace WebKit

#endif // WebURL_h
