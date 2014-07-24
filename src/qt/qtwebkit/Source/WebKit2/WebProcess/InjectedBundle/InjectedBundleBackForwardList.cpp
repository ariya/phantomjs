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
#include "InjectedBundleBackForwardList.h"

#include "InjectedBundleBackForwardListItem.h"
#include "WebBackForwardListProxy.h"
#include "WebPage.h"
#include <WebCore/BackForwardController.h>
#include <WebCore/Page.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<InjectedBundleBackForwardListItem> InjectedBundleBackForwardList::itemAtIndex(int index) const
{
    if (!m_page)
        return 0;
    Page* page = m_page->corePage();
    if (!page)
        return 0;
    return InjectedBundleBackForwardListItem::create(page->backForward()->itemAtIndex(index));
}

int InjectedBundleBackForwardList::backListCount() const
{
    if (!m_page)
        return 0;
    Page* page = m_page->corePage();
    if (!page)
        return 0;
    return page->backForward()->backCount();
}

int InjectedBundleBackForwardList::forwardListCount() const
{
    if (!m_page)
        return 0;
    Page* page = m_page->corePage();
    if (!page)
        return 0;
    return page->backForward()->forwardCount();
}

void InjectedBundleBackForwardList::clear()
{
    if (!m_page)
        return;
    Page* page = m_page->corePage();
    if (!page)
        return;
    static_cast<WebBackForwardListProxy*>(page->backForward()->client())->clear();
}

} // namespace WebKit
