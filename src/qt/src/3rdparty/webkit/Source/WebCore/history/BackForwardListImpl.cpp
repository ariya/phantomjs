/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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

#include "config.h"
#include "BackForwardListImpl.h"

#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HistoryItem.h"
#include "Logging.h"
#include "Page.h"
#include "PageCache.h"
#include "SerializedScriptValue.h"

using namespace std;

namespace WebCore {

static const unsigned DefaultCapacity = 100;
static const unsigned NoCurrentItemIndex = UINT_MAX;

BackForwardListImpl::BackForwardListImpl(Page* page)
    : m_page(page)
    , m_current(NoCurrentItemIndex)
    , m_capacity(DefaultCapacity)
    , m_closed(true)
    , m_enabled(true)
{
}

BackForwardListImpl::~BackForwardListImpl()
{
    ASSERT(m_closed);
}

void BackForwardListImpl::addItem(PassRefPtr<HistoryItem> prpItem)
{
    ASSERT(prpItem);
    if (m_capacity == 0 || !m_enabled)
        return;
    
    // Toss anything in the forward list    
    if (m_current != NoCurrentItemIndex) {
        unsigned targetSize = m_current + 1;
        while (m_entries.size() > targetSize) {
            RefPtr<HistoryItem> item = m_entries.last();
            m_entries.removeLast();
            m_entryHash.remove(item);
            pageCache()->remove(item.get());
        }
    }

    // Toss the first item if the list is getting too big, as long as we're not using it
    // (or even if we are, if we only want 1 entry).
    if (m_entries.size() == m_capacity && (m_current != 0 || m_capacity == 1)) {
        RefPtr<HistoryItem> item = m_entries[0];
        m_entries.remove(0);
        m_entryHash.remove(item);
        pageCache()->remove(item.get());
        m_current--;
        if (m_page)
            m_page->mainFrame()->loader()->client()->dispatchDidRemoveBackForwardItem(item.get());
    }

    m_entryHash.add(prpItem.get());
    m_entries.insert(m_current + 1, prpItem);
    m_current++;
    if (m_page)
        m_page->mainFrame()->loader()->client()->dispatchDidAddBackForwardItem(currentItem());
}

void BackForwardListImpl::goBack()
{
    ASSERT(m_current > 0);
    if (m_current > 0) {
        m_current--;
        if (m_page)
            m_page->mainFrame()->loader()->client()->dispatchDidChangeBackForwardIndex();
    }
}

void BackForwardListImpl::goForward()
{
    ASSERT(m_current < m_entries.size() - 1);
    if (m_current < m_entries.size() - 1) {
        m_current++;
        if (m_page)
            m_page->mainFrame()->loader()->client()->dispatchDidChangeBackForwardIndex();
    }
}

void BackForwardListImpl::goToItem(HistoryItem* item)
{
    if (!m_entries.size() || !item)
        return;
        
    unsigned int index = 0;
    for (; index < m_entries.size(); ++index)
        if (m_entries[index] == item)
            break;
    if (index < m_entries.size()) {
        m_current = index;
        if (m_page)
            m_page->mainFrame()->loader()->client()->dispatchDidChangeBackForwardIndex();
    }
}

HistoryItem* BackForwardListImpl::backItem()
{
    if (m_current && m_current != NoCurrentItemIndex)
        return m_entries[m_current - 1].get();
    return 0;
}

HistoryItem* BackForwardListImpl::currentItem()
{
    if (m_current != NoCurrentItemIndex)
        return m_entries[m_current].get();
    return 0;
}

HistoryItem* BackForwardListImpl::forwardItem()
{
    if (m_entries.size() && m_current < m_entries.size() - 1)
        return m_entries[m_current + 1].get();
    return 0;
}

void BackForwardListImpl::backListWithLimit(int limit, HistoryItemVector& list)
{
    list.clear();
    if (m_current != NoCurrentItemIndex) {
        unsigned first = max((int)m_current - limit, 0);
        for (; first < m_current; ++first)
            list.append(m_entries[first]);
    }
}

void BackForwardListImpl::forwardListWithLimit(int limit, HistoryItemVector& list)
{
    ASSERT(limit > -1);
    list.clear();
    if (!m_entries.size())
        return;
        
    unsigned lastEntry = m_entries.size() - 1;
    if (m_current < lastEntry) {
        int last = min(m_current + limit, lastEntry);
        limit = m_current + 1;
        for (; limit <= last; ++limit)
            list.append(m_entries[limit]);
    }
}

int BackForwardListImpl::capacity()
{
    return m_capacity;
}

void BackForwardListImpl::setCapacity(int size)
{    
    while (size < (int)m_entries.size()) {
        RefPtr<HistoryItem> item = m_entries.last();
        m_entries.removeLast();
        m_entryHash.remove(item);
        pageCache()->remove(item.get());
    }

    if (!size)
        m_current = NoCurrentItemIndex;
    else if (m_current > m_entries.size() - 1) {
        m_current = m_entries.size() - 1;
        if (m_page)
            m_page->mainFrame()->loader()->client()->dispatchDidChangeBackForwardIndex();
    }
    m_capacity = size;
}

bool BackForwardListImpl::enabled()
{
    return m_enabled;
}

void BackForwardListImpl::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled) {
        int capacity = m_capacity;
        setCapacity(0);
        setCapacity(capacity);
    }
}

int BackForwardListImpl::backListCount()
{
    return m_current == NoCurrentItemIndex ? 0 : m_current;
}

int BackForwardListImpl::forwardListCount()
{
    return m_current == NoCurrentItemIndex ? 0 : (int)m_entries.size() - (m_current + 1);
}

HistoryItem* BackForwardListImpl::itemAtIndex(int index)
{
    // Do range checks without doing math on index to avoid overflow.
    if (index < -(int)m_current)
        return 0;
    
    if (index > forwardListCount())
        return 0;
        
    return m_entries[index + m_current].get();
}

HistoryItemVector& BackForwardListImpl::entries()
{
    return m_entries;
}

void BackForwardListImpl::close()
{
    int size = m_entries.size();
    for (int i = 0; i < size; ++i)
        pageCache()->remove(m_entries[i].get());
    m_entries.clear();
    m_entryHash.clear();
    m_page = 0;
    m_closed = true;
}

bool BackForwardListImpl::closed()
{
    return m_closed;
}

void BackForwardListImpl::removeItem(HistoryItem* item)
{
    if (!item)
        return;
    
    for (unsigned i = 0; i < m_entries.size(); ++i)
        if (m_entries[i] == item) {
            m_entries.remove(i);
            m_entryHash.remove(item);
            if (m_current == NoCurrentItemIndex || m_current < i)
                break;
            if (m_current > i)
                m_current--;
            else {
                size_t count = m_entries.size();
                if (m_current >= count)
                    m_current = count ? count - 1 : NoCurrentItemIndex;
            }
            break;
        }
}

bool BackForwardListImpl::containsItem(HistoryItem* entry)
{
    return m_entryHash.contains(entry);
}

}; // namespace WebCore
