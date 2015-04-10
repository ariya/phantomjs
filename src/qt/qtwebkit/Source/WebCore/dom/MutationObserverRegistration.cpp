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

#include "MutationObserverRegistration.h"

#include "Document.h"
#include "Node.h"
#include "QualifiedName.h"

namespace WebCore {

PassOwnPtr<MutationObserverRegistration> MutationObserverRegistration::create(PassRefPtr<MutationObserver> observer, Node* registrationNode, MutationObserverOptions options, const HashSet<AtomicString>& attributeFilter)
{
    return adoptPtr(new MutationObserverRegistration(observer, registrationNode, options, attributeFilter));
}

MutationObserverRegistration::MutationObserverRegistration(PassRefPtr<MutationObserver> observer, Node* registrationNode, MutationObserverOptions options, const HashSet<AtomicString>& attributeFilter)
    : m_observer(observer)
    , m_registrationNode(registrationNode)
    , m_options(options)
    , m_attributeFilter(attributeFilter)
{
    m_observer->observationStarted(this);
}

MutationObserverRegistration::~MutationObserverRegistration()
{
    clearTransientRegistrations();
    m_observer->observationEnded(this);
}

void MutationObserverRegistration::resetObservation(MutationObserverOptions options, const HashSet<AtomicString>& attributeFilter)
{
    clearTransientRegistrations();
    m_options = options;
    m_attributeFilter = attributeFilter;
}

void MutationObserverRegistration::observedSubtreeNodeWillDetach(Node* node)
{
    if (!isSubtree())
        return;

    node->registerTransientMutationObserver(this);
    m_observer->setHasTransientRegistration();

    if (!m_transientRegistrationNodes) {
        m_transientRegistrationNodes = adoptPtr(new NodeHashSet);

        ASSERT(!m_registrationNodeKeepAlive);
        m_registrationNodeKeepAlive = m_registrationNode; // Balanced in clearTransientRegistrations.
    }
    m_transientRegistrationNodes->add(node);
}

void MutationObserverRegistration::clearTransientRegistrations()
{
    if (!m_transientRegistrationNodes) {
        ASSERT(!m_registrationNodeKeepAlive);
        return;
    }

    for (NodeHashSet::iterator iter = m_transientRegistrationNodes->begin(); iter != m_transientRegistrationNodes->end(); ++iter)
        (*iter)->unregisterTransientMutationObserver(this);

    m_transientRegistrationNodes.clear();

    ASSERT(m_registrationNodeKeepAlive);
    m_registrationNodeKeepAlive = 0; // Balanced in observeSubtreeNodeWillDetach.
}

void MutationObserverRegistration::unregisterAndDelete(MutationObserverRegistration* registry)
{
    RefPtr<Node> registrationNode(registry->m_registrationNode);
    registrationNode->unregisterMutationObserver(registry);
    // The above line will cause registry to be deleted, so don't do any more in this function.
}

bool MutationObserverRegistration::shouldReceiveMutationFrom(Node* node, MutationObserver::MutationType type, const QualifiedName* attributeName) const
{
    ASSERT((type == MutationObserver::Attributes && attributeName) || !attributeName);
    if (!(m_options & type))
        return false;

    if (m_registrationNode != node && !isSubtree())
        return false;

    if (type != MutationObserver::Attributes || !(m_options & MutationObserver::AttributeFilter))
        return true;

    if (!attributeName->namespaceURI().isNull())
        return false;

    return m_attributeFilter.contains(attributeName->localName());
}

void MutationObserverRegistration::addRegistrationNodesToSet(HashSet<Node*>& nodes) const
{
    nodes.add(m_registrationNode);
    if (!m_transientRegistrationNodes)
        return;
    for (NodeHashSet::const_iterator iter = m_transientRegistrationNodes->begin(); iter != m_transientRegistrationNodes->end(); ++iter)
        nodes.add(iter->get());
}

} // namespace WebCore
