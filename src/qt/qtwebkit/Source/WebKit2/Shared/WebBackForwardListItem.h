/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef WebBackForwardListItem_h
#define WebBackForwardListItem_h

#include "APIObject.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

class WebBackForwardListItem : public TypedAPIObject<APIObject::TypeBackForwardListItem> {
public:
    static PassRefPtr<WebBackForwardListItem> create(const String& originalURL, const String& url, const String& title, const uint8_t* backForwardData, size_t backForwardDataSize, uint64_t itemID)
    {
        return adoptRef(new WebBackForwardListItem(originalURL, url, title, backForwardData, backForwardDataSize, itemID));
    }

    virtual ~WebBackForwardListItem();

    uint64_t itemID() const { return m_itemID; }

    void setOriginalURL(const String& originalURL) { m_originalURL = originalURL; }
    const String& originalURL() const { return m_originalURL; }

    void setURL(const String& url) { m_url = url; }
    const String& url() const { return m_url; }

    void setTitle(const String& title) { m_title = title; }
    const String& title() const { return m_title; }
    
    void setBackForwardData(const uint8_t* buffer, size_t size);
    const Vector<uint8_t>& backForwardData() const { return m_backForwardData; }

    void encode(CoreIPC::ArgumentEncoder&) const;
    static PassRefPtr<WebBackForwardListItem> decode(CoreIPC::ArgumentDecoder&);

    static uint64_t highedUsedItemID();

private:
    WebBackForwardListItem(const String& originalURL, const String& url, const String& title, const uint8_t* backForwardData, size_t backForwardDataSize, uint64_t itemID);

    String m_originalURL;
    String m_url;
    String m_title;
    uint64_t m_itemID;
    Vector<uint8_t> m_backForwardData;
};

} // namespace WebKit

#endif // WebBackForwardListItem_h
