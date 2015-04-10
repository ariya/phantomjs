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
#include "WebBackForwardListProxy.h"

#include "DataReference.h"
#include "EncoderAdapter.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include "WebProcessProxyMessages.h"
#include <WebCore/HistoryItem.h>
#include <WebCore/PageCache.h>
#include <wtf/HashMap.h>

using namespace WebCore;

namespace WebKit {

static const unsigned DefaultCapacity = 100;
static const unsigned NoCurrentItemIndex = UINT_MAX;

// FIXME <rdar://problem/8819268>: This leaks all HistoryItems that go into these maps.
// We need to clear up the life time of these objects.

typedef HashMap<uint64_t, RefPtr<HistoryItem> > IDToHistoryItemMap;
typedef HashMap<RefPtr<HistoryItem>, uint64_t> HistoryItemToIDMap;

static IDToHistoryItemMap& idToHistoryItemMap()
{
    DEFINE_STATIC_LOCAL(IDToHistoryItemMap, map, ());
    return map;
} 

static HistoryItemToIDMap& historyItemToIDMap()
{
    DEFINE_STATIC_LOCAL(HistoryItemToIDMap, map, ());
    return map;
} 

static uint64_t uniqueHistoryItemID = 1;

static uint64_t generateHistoryItemID()
{
    // These IDs exist in the WebProcess for items created by the WebProcess.
    // The IDs generated here need to never collide with the IDs created in WebBackForwardList in the UIProcess.
    // We accomplish this by starting from 3, and only ever using odd ids.
    uniqueHistoryItemID += 2;
    return uniqueHistoryItemID;
}

void WebBackForwardListProxy::setHighestItemIDFromUIProcess(uint64_t itemID)
{
    if (itemID <= uniqueHistoryItemID)
        return;
    
     if (itemID % 2)
         uniqueHistoryItemID = itemID;
     else
         uniqueHistoryItemID = itemID + 1;
}

static void updateBackForwardItem(uint64_t itemID, HistoryItem* item)
{
    EncoderAdapter encoder;
    item->encodeBackForwardTree(encoder);

    WebProcess::shared().parentProcessConnection()->send(Messages::WebProcessProxy::AddBackForwardItem(itemID, item->originalURLString(), item->urlString(), item->title(), encoder.dataReference()), 0);
}

void WebBackForwardListProxy::addItemFromUIProcess(uint64_t itemID, PassRefPtr<WebCore::HistoryItem> prpItem)
{
    RefPtr<HistoryItem> item = prpItem;
    
    // This item/itemID pair should not already exist in our maps.
    ASSERT(!historyItemToIDMap().contains(item.get()));
    ASSERT(!idToHistoryItemMap().contains(itemID));
        
    historyItemToIDMap().set(item, itemID);
    idToHistoryItemMap().set(itemID, item);
}

static void WK2NotifyHistoryItemChanged(HistoryItem* item)
{
    uint64_t itemID = historyItemToIDMap().get(item);
    if (!itemID)
        return;

    updateBackForwardItem(itemID, item);
}

HistoryItem* WebBackForwardListProxy::itemForID(uint64_t itemID)
{
    return idToHistoryItemMap().get(itemID);
}

uint64_t WebBackForwardListProxy::idForItem(HistoryItem* item)
{
    ASSERT(item);
    return historyItemToIDMap().get(item);
}

void WebBackForwardListProxy::removeItem(uint64_t itemID)
{
    IDToHistoryItemMap::iterator it = idToHistoryItemMap().find(itemID);
    if (it == idToHistoryItemMap().end())
        return;
        
    WebCore::pageCache()->remove(it->value.get());

    historyItemToIDMap().remove(it->value);
    idToHistoryItemMap().remove(it);
}

WebBackForwardListProxy::WebBackForwardListProxy(WebPage* page)
    : m_page(page)
{
    WebCore::notifyHistoryItemChanged = WK2NotifyHistoryItemChanged;
}

void WebBackForwardListProxy::addItem(PassRefPtr<HistoryItem> prpItem)
{
    RefPtr<HistoryItem> item = prpItem;

    ASSERT(!historyItemToIDMap().contains(item));

    if (!m_page)
        return;

    uint64_t itemID = generateHistoryItemID();

    ASSERT(!idToHistoryItemMap().contains(itemID));

    m_associatedItemIDs.add(itemID);

    historyItemToIDMap().set(item, itemID);
    idToHistoryItemMap().set(itemID, item);

    updateBackForwardItem(itemID, item.get());
    m_page->send(Messages::WebPageProxy::BackForwardAddItem(itemID));
}

void WebBackForwardListProxy::goToItem(HistoryItem* item)
{
    if (!m_page)
        return;

    SandboxExtension::Handle sandboxExtensionHandle;
    m_page->sendSync(Messages::WebPageProxy::BackForwardGoToItem(historyItemToIDMap().get(item)), Messages::WebPageProxy::BackForwardGoToItem::Reply(sandboxExtensionHandle));
    m_page->sandboxExtensionTracker().beginLoad(m_page->mainWebFrame(), sandboxExtensionHandle);
}

HistoryItem* WebBackForwardListProxy::itemAtIndex(int itemIndex)
{
    if (!m_page)
        return 0;

    uint64_t itemID = 0;
    if (!WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebPageProxy::BackForwardItemAtIndex(itemIndex), Messages::WebPageProxy::BackForwardItemAtIndex::Reply(itemID), m_page->pageID()))
        return 0;

    if (!itemID)
        return 0;

    return idToHistoryItemMap().get(itemID);
}

int WebBackForwardListProxy::backListCount()
{
    if (!m_page)
        return 0;

    int backListCount = 0;
    if (!WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebPageProxy::BackForwardBackListCount(), Messages::WebPageProxy::BackForwardBackListCount::Reply(backListCount), m_page->pageID()))
        return 0;

    return backListCount;
}

int WebBackForwardListProxy::forwardListCount()
{
    if (!m_page)
        return 0;

    int forwardListCount = 0;
    if (!WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebPageProxy::BackForwardForwardListCount(), Messages::WebPageProxy::BackForwardForwardListCount::Reply(forwardListCount), m_page->pageID()))
        return 0;

    return forwardListCount;
}

void WebBackForwardListProxy::close()
{
    HashSet<uint64_t>::iterator end = m_associatedItemIDs.end();
    for (HashSet<uint64_t>::iterator i = m_associatedItemIDs.begin(); i != end; ++i)
        WebCore::pageCache()->remove(itemForID(*i));

    m_associatedItemIDs.clear();

    m_page = 0;
}

bool WebBackForwardListProxy::isActive()
{
    // FIXME: Should check the the list is enabled and has non-zero capacity.
    return true;
}

void WebBackForwardListProxy::clear()
{
    m_page->send(Messages::WebPageProxy::BackForwardClear());
}

} // namespace WebKit
