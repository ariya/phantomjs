/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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
#include "HistoryItem.h"

#include <wtf/text/StringHash.h>

namespace WebCore {

id HistoryItem::viewState() const
{
    return m_viewState.get();
}

void HistoryItem::setViewState(id statePList)
{
    id newState = [statePList copy];
    m_viewState = newState;
    [newState release];
}

id HistoryItem::getTransientProperty(const String& key) const
{
    if (!m_transientProperties)
        return nil;
    return m_transientProperties->get(key).get();
}

void HistoryItem::setTransientProperty(const String& key, id value)
{
    if (!value) {
        if (m_transientProperties) {
            m_transientProperties->remove(key);
            if (m_transientProperties->isEmpty())
                m_transientProperties.clear();
        }
    } else {
        if (!m_transientProperties)
            m_transientProperties = adoptPtr(new HashMap<String, RetainPtr<id> >);
        m_transientProperties->set(key, value);
    }
}

} // namespace WebCore
