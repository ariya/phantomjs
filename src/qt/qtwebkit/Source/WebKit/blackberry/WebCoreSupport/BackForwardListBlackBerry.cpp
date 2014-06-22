/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "BackForwardListBlackBerry.h"

#include "HistoryItem.h"
#include "WebPage_p.h"

using namespace BlackBerry::WebKit;

namespace WebCore {

BackForwardListBlackBerry::BackForwardListBlackBerry(WebPagePrivate* pagePrivate)
    : m_impl(BackForwardListImpl::create(pagePrivate->m_page))
    , m_webPagePrivate(pagePrivate)
{
}

BackForwardListBlackBerry::~BackForwardListBlackBerry()
{
}

int BackForwardListBlackBerry::current()
{
    return m_impl->backListCount();
}

void BackForwardListBlackBerry::notifyBackForwardListChanged()
{
    m_webPagePrivate->m_client->resetBackForwardList(m_impl->entries().size(), current());
}

void BackForwardListBlackBerry::addItem(PassRefPtr<HistoryItem> prpItem)
{
    if (!isActive())
        return;

    m_impl->addItem(prpItem);
    notifyBackForwardListChanged();
}

void BackForwardListBlackBerry::goToItem(HistoryItem* item)
{
    if (!m_impl->entries().size() || !item)
        return;

    int oldIndex = current();
    m_impl->goToItem(item);
    if (oldIndex != current())
        notifyBackForwardListChanged();
    else {
        // FIXME: this might not be needed anymore. See PR 310030.
        // Since the recent pages dialog is in another process, it's possible for
        // the user to choose an entry after it's been removed from the underlying
        // BackForwardList (for example, if the current page is in the middle of
        // the stack and it sets document.location with a timer, clearing the
        // forward list, while the user has the recent pages dialog open). In this
        // case the best thing to do is re-add the item at the end of the stack.
        addItem(item);
    }
}

HistoryItem* BackForwardListBlackBerry::itemAtIndex(int index)
{
    return m_impl->itemAtIndex(index);
}

int BackForwardListBlackBerry::backListCount()
{
    return m_impl->backListCount();
}

int BackForwardListBlackBerry::forwardListCount()
{
    return m_impl->forwardListCount();
}

bool BackForwardListBlackBerry::isActive()
{
    return m_impl->enabled() && m_impl->capacity();
}

void BackForwardListBlackBerry::close()
{
    return m_impl->close();
}

void BackForwardListBlackBerry::clear()
{
    int capacity = m_impl->capacity();
    m_impl->setCapacity(0);
    m_impl->setCapacity(capacity);
}

HistoryItemVector& BackForwardListBlackBerry::entries()
{
    return m_impl->entries();
}

void BackForwardListBlackBerry::backListWithLimit(int limit, HistoryItemVector& list)
{
    m_impl->backListWithLimit(limit, list);
}

void BackForwardListBlackBerry::forwardListWithLimit(int limit, HistoryItemVector& list)
{
    m_impl->forwardListWithLimit(limit, list);
}

int BackForwardListBlackBerry::capacity()
{
    return m_impl->capacity();
}

HistoryItem* BackForwardListBlackBerry::currentItem()
{
    return m_impl->currentItem();
}

}; // namespace WebCore
