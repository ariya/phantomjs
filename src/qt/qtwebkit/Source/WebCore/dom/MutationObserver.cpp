/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "MutationObserver.h"

#include "Dictionary.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "MutationCallback.h"
#include "MutationObserverRegistration.h"
#include "MutationRecord.h"
#include "Node.h"
#include <algorithm>
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/Vector.h>

namespace WebCore {

static unsigned s_observerPriority = 0;

struct MutationObserver::ObserverLessThan {
    bool operator()(const RefPtr<MutationObserver>& lhs, const RefPtr<MutationObserver>& rhs)
    {
        return lhs->m_priority < rhs->m_priority;
    }
};

PassRefPtr<MutationObserver> MutationObserver::create(PassRefPtr<MutationCallback> callback)
{
    ASSERT(isMainThread());
    return adoptRef(new MutationObserver(callback));
}

MutationObserver::MutationObserver(PassRefPtr<MutationCallback> callback)
    : m_callback(callback)
    , m_priority(s_observerPriority++)
{
}

MutationObserver::~MutationObserver()
{
    ASSERT(m_registrations.isEmpty());
}

bool MutationObserver::validateOptions(MutationObserverOptions options)
{
    return (options & (Attributes | CharacterData | ChildList))
        && ((options & Attributes) || !(options & AttributeOldValue))
        && ((options & Attributes) || !(options & AttributeFilter))
        && ((options & CharacterData) || !(options & CharacterDataOldValue));
}

void MutationObserver::observe(Node* node, const Dictionary& optionsDictionary, ExceptionCode& ec)
{
    if (!node) {
        ec = NOT_FOUND_ERR;
        return;
    }

    static const struct {
        const char* name;
        MutationObserverOptions value;
    } booleanOptions[] = {
        { "childList", ChildList },
        { "attributes", Attributes },
        { "characterData", CharacterData },
        { "subtree", Subtree },
        { "attributeOldValue", AttributeOldValue },
        { "characterDataOldValue", CharacterDataOldValue }
    };
    MutationObserverOptions options = 0;
    for (unsigned i = 0; i < sizeof(booleanOptions) / sizeof(booleanOptions[0]); ++i) {
        bool value = false;
        if (optionsDictionary.get(booleanOptions[i].name, value) && value)
            options |= booleanOptions[i].value;
    }

    HashSet<AtomicString> attributeFilter;
    if (optionsDictionary.get("attributeFilter", attributeFilter))
        options |= AttributeFilter;

    if (!validateOptions(options)) {
        ec = SYNTAX_ERR;
        return;
    }

    node->registerMutationObserver(this, options, attributeFilter);
}

Vector<RefPtr<MutationRecord> > MutationObserver::takeRecords()
{
    Vector<RefPtr<MutationRecord> > records;
    records.swap(m_records);
    return records;
}

void MutationObserver::disconnect()
{
    m_records.clear();
    HashSet<MutationObserverRegistration*> registrations(m_registrations);
    for (HashSet<MutationObserverRegistration*>::iterator iter = registrations.begin(); iter != registrations.end(); ++iter)
        MutationObserverRegistration::unregisterAndDelete(*iter);
}

void MutationObserver::observationStarted(MutationObserverRegistration* registration)
{
    ASSERT(!m_registrations.contains(registration));
    m_registrations.add(registration);
}

void MutationObserver::observationEnded(MutationObserverRegistration* registration)
{
    ASSERT(m_registrations.contains(registration));
    m_registrations.remove(registration);
}

typedef HashSet<RefPtr<MutationObserver> > MutationObserverSet;

static MutationObserverSet& activeMutationObservers()
{
    DEFINE_STATIC_LOCAL(MutationObserverSet, activeObservers, ());
    return activeObservers;
}

static MutationObserverSet& suspendedMutationObservers()
{
    DEFINE_STATIC_LOCAL(MutationObserverSet, suspendedObservers, ());
    return suspendedObservers;
}

void MutationObserver::enqueueMutationRecord(PassRefPtr<MutationRecord> mutation)
{
    ASSERT(isMainThread());
    m_records.append(mutation);
    activeMutationObservers().add(this);
}

void MutationObserver::setHasTransientRegistration()
{
    ASSERT(isMainThread());
    activeMutationObservers().add(this);
}

HashSet<Node*> MutationObserver::getObservedNodes() const
{
    HashSet<Node*> observedNodes;
    for (HashSet<MutationObserverRegistration*>::const_iterator iter = m_registrations.begin(); iter != m_registrations.end(); ++iter)
        (*iter)->addRegistrationNodesToSet(observedNodes);
    return observedNodes;
}

bool MutationObserver::canDeliver()
{
    return !m_callback->scriptExecutionContext()->activeDOMObjectsAreSuspended();
}

void MutationObserver::deliver()
{
    ASSERT(canDeliver());

    // Calling clearTransientRegistrations() can modify m_registrations, so it's necessary
    // to make a copy of the transient registrations before operating on them.
    Vector<MutationObserverRegistration*, 1> transientRegistrations;
    for (HashSet<MutationObserverRegistration*>::iterator iter = m_registrations.begin(); iter != m_registrations.end(); ++iter) {
        if ((*iter)->hasTransientRegistrations())
            transientRegistrations.append(*iter);
    }
    for (size_t i = 0; i < transientRegistrations.size(); ++i)
        transientRegistrations[i]->clearTransientRegistrations();

    if (m_records.isEmpty())
        return;

    Vector<RefPtr<MutationRecord> > records;
    records.swap(m_records);

    m_callback->call(records, this);
}

void MutationObserver::deliverAllMutations()
{
    ASSERT(isMainThread());
    static bool deliveryInProgress = false;
    if (deliveryInProgress)
        return;
    deliveryInProgress = true;

    if (!suspendedMutationObservers().isEmpty()) {
        Vector<RefPtr<MutationObserver> > suspended;
        copyToVector(suspendedMutationObservers(), suspended);
        for (size_t i = 0; i < suspended.size(); ++i) {
            if (!suspended[i]->canDeliver())
                continue;

            suspendedMutationObservers().remove(suspended[i]);
            activeMutationObservers().add(suspended[i]);
        }
    }

    while (!activeMutationObservers().isEmpty()) {
        Vector<RefPtr<MutationObserver> > observers;
        copyToVector(activeMutationObservers(), observers);
        activeMutationObservers().clear();
        std::sort(observers.begin(), observers.end(), ObserverLessThan());
        for (size_t i = 0; i < observers.size(); ++i) {
            if (observers[i]->canDeliver())
                observers[i]->deliver();
            else
                suspendedMutationObservers().add(observers[i]);
        }
    }

    deliveryInProgress = false;
}

} // namespace WebCore
