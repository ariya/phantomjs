/*
 * Copyright (C) 2012 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IdTargetObserverRegistry.h"

#include "IdTargetObserver.h"

namespace WebCore {

PassOwnPtr<IdTargetObserverRegistry> IdTargetObserverRegistry::create()
{
    return adoptPtr(new IdTargetObserverRegistry());
}

void IdTargetObserverRegistry::addObserver(const AtomicString& id, IdTargetObserver* observer)
{
    if (id.isEmpty())
        return;
    
    IdToObserverSetMap::AddResult result = m_registry.add(id.impl(), nullptr);
    if (result.isNewEntry)
        result.iterator->value = adoptPtr(new ObserverSet());

    result.iterator->value->add(observer);
}

void IdTargetObserverRegistry::removeObserver(const AtomicString& id, IdTargetObserver* observer)
{
    if (id.isEmpty() || m_registry.isEmpty())
        return;

    IdToObserverSetMap::iterator iter = m_registry.find(id.impl());

    ObserverSet* set = iter->value.get();
    set->remove(observer);
    if (set->isEmpty() && set != m_notifyingObserversInSet)
        m_registry.remove(iter);
}

void IdTargetObserverRegistry::notifyObserversInternal(const AtomicString& id)
{
    ASSERT(!id.isEmpty());
    ASSERT(!m_registry.isEmpty());

    m_notifyingObserversInSet = m_registry.get(id.impl());
    if (!m_notifyingObserversInSet)
        return;

    Vector<IdTargetObserver*> copy;
    copyToVector(*m_notifyingObserversInSet, copy);
    for (Vector<IdTargetObserver*>::const_iterator it = copy.begin(); it != copy.end(); ++it) {
        if (m_notifyingObserversInSet->contains(*it))
            (*it)->idTargetChanged();
    }

    if (m_notifyingObserversInSet->isEmpty())
        m_registry.remove(id.impl());

    m_notifyingObserversInSet = 0;
}

} // namespace WebCore
