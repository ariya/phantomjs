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

#include "config.h"
#include "WebBackForwardListItem.h"

#include "DataReference.h"
#include "WebCoreArgumentCoders.h"

namespace WebKit {

static uint64_t highestUsedItemID = 0;

WebBackForwardListItem::WebBackForwardListItem(const String& originalURL, const String& url, const String& title, const uint8_t* backForwardData, size_t backForwardDataSize, uint64_t itemID)
    : m_originalURL(originalURL)
    , m_url(url)
    , m_title(title)
    , m_itemID(itemID)
{
    if (m_itemID > highestUsedItemID)
        highestUsedItemID = m_itemID;

    setBackForwardData(backForwardData, backForwardDataSize);
}

WebBackForwardListItem::~WebBackForwardListItem()
{
}

uint64_t WebBackForwardListItem::highedUsedItemID()
{
    return highestUsedItemID;
}

void WebBackForwardListItem::setBackForwardData(const uint8_t* data, size_t size)
{
    m_backForwardData.clear();
    m_backForwardData.reserveCapacity(size);
    m_backForwardData.append(data, size);
}

void WebBackForwardListItem::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_originalURL;
    encoder << m_url;
    encoder << m_title;
    encoder << m_itemID;
    encoder << CoreIPC::DataReference(m_backForwardData);
}

PassRefPtr<WebBackForwardListItem> WebBackForwardListItem::decode(CoreIPC::ArgumentDecoder& decoder)
{
    String originalURL;
    if (!decoder.decode(originalURL))
        return 0;

    String url;
    if (!decoder.decode(url))
        return 0;

    String title;
    if (!decoder.decode(title))
        return 0;

    uint64_t itemID;
    if (!decoder.decode(itemID))
        return 0;
    
    CoreIPC::DataReference data;
    if (!decoder.decode(data))
        return 0;

    return create(originalURL, url, title, data.data(), data.size(), itemID);
}

} // namespace WebKit
