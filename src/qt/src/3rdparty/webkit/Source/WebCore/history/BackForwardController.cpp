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

#include "config.h"
#include "BackForwardController.h"

#include "BackForwardListImpl.h"
#include "HistoryItem.h"
#include "Page.h"

namespace WebCore {

BackForwardController::BackForwardController(Page* page, PassRefPtr<BackForwardList> client)
    : m_page(page)
    , m_client(client)
{
    if (!m_client)
        m_client = BackForwardListImpl::create(page);
}

BackForwardController::~BackForwardController()
{
}

bool BackForwardController::canGoBackOrForward(int distance) const
{
    return m_page->canGoBackOrForward(distance);
}

void BackForwardController::goBackOrForward(int distance)
{
    m_page->goBackOrForward(distance);
}

bool BackForwardController::goBack()
{
    return m_page->goBack();
}

bool BackForwardController::goForward()
{
    return m_page->goForward();
}

void BackForwardController::addItem(PassRefPtr<HistoryItem> item)
{
    m_client->addItem(item);
}

void BackForwardController::setCurrentItem(HistoryItem* item)
{
    m_client->goToItem(item);
}

int BackForwardController::count() const
{
    return m_page->getHistoryLength();
}

int BackForwardController::backCount() const
{
    return m_client->backListCount();
}

int BackForwardController::forwardCount() const
{
    return m_client->forwardListCount();
}

HistoryItem* BackForwardController::itemAtIndex(int i)
{
    return m_client->itemAtIndex(i);
}

bool BackForwardController::isActive()
{
    return m_client->isActive();
}

void BackForwardController::close()
{
    m_client->close();
}

} // namespace WebCore
