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

#ifndef InjectedBundleBackForwardListItem_h
#define InjectedBundleBackForwardListItem_h

#include "APIObject.h"
#include <WebCore/HistoryItem.h>

namespace WebKit {

class ImmutableArray;
class WebPageProxy;

class InjectedBundleBackForwardListItem : public TypedAPIObject<APIObject::TypeBundleBackForwardListItem> {
public:
    static PassRefPtr<InjectedBundleBackForwardListItem> create(PassRefPtr<WebCore::HistoryItem> item)
    {
        if (!item)
            return 0;
        return adoptRef(new InjectedBundleBackForwardListItem(item));
    }

    WebCore::HistoryItem* item() const { return m_item.get(); }

    const String& originalURL() const { return m_item->originalURLString(); }
    const String& url() const { return m_item->urlString(); }
    const String& title() const { return m_item->title(); }

    const String& target() const { return m_item->target(); }
    bool isTargetItem() const { return m_item->isTargetItem(); }
    bool isInPageCache() const { return m_item->isInPageCache(); }
    bool hasCachedPageExpired() const { return m_item->hasCachedPageExpired(); }

    PassRefPtr<ImmutableArray> children() const;

private:
    InjectedBundleBackForwardListItem(PassRefPtr<WebCore::HistoryItem> item) : m_item(item) { }

    RefPtr<WebCore::HistoryItem> m_item;
};

} // namespace WebKit

#endif // InjectedBundleBackForwardListItem_h
