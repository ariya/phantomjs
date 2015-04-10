/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
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
#include "ContentDistributor.h"

#include "ElementShadow.h"
#include "HTMLContentElement.h"
#include "NodeTraversal.h"
#include "ShadowRoot.h"


namespace WebCore {

ContentDistributor::ContentDistributor()
    : m_insertionPointListIsValid(true)
    , m_validity(Undetermined)
{
}

ContentDistributor::~ContentDistributor()
{
}

void ContentDistributor::invalidateInsertionPointList()
{
    m_insertionPointListIsValid = false;
    m_insertionPointList.clear();
}

const Vector<RefPtr<InsertionPoint> >& ContentDistributor::ensureInsertionPointList(ShadowRoot* shadowRoot)
{
    if (m_insertionPointListIsValid)
        return m_insertionPointList;

    m_insertionPointListIsValid = true;
    ASSERT(m_insertionPointList.isEmpty());

    for (Element* element = ElementTraversal::firstWithin(shadowRoot); element; element = ElementTraversal::next(element, shadowRoot)) {
        if (element->isInsertionPoint())
            m_insertionPointList.append(toInsertionPoint(element));
    }

    return m_insertionPointList;
}

InsertionPoint* ContentDistributor::findInsertionPointFor(const Node* key) const
{
    return m_nodeToInsertionPoint.get(key);
}

void ContentDistributor::distribute(Element* host)
{
    ASSERT(needsDistribution());
    ASSERT(m_nodeToInsertionPoint.isEmpty());
    ASSERT(!host->containingShadowRoot() || host->containingShadowRoot()->owner()->distributor().isValid());

    m_validity = Valid;

    if (ShadowRoot* root = host->shadowRoot()) {
        const Vector<RefPtr<InsertionPoint> >& insertionPoints = ensureInsertionPointList(root);
        for (size_t i = 0; i < insertionPoints.size(); ++i) {
            InsertionPoint* point = insertionPoints[i].get();
            if (!point->isActive())
                continue;

            distributeSelectionsTo(point, host);
        }
    }
}

bool ContentDistributor::invalidate(Element* host)
{
    ASSERT(needsInvalidation());
    bool needsReattach = (m_validity == Undetermined) || !m_nodeToInsertionPoint.isEmpty();

    if (ShadowRoot* root = host->shadowRoot()) {
        const Vector<RefPtr<InsertionPoint> >& insertionPoints = ensureInsertionPointList(root);
        for (size_t i = 0; i < insertionPoints.size(); ++i) {
            needsReattach = true;
            insertionPoints[i]->clearDistribution();
        }
    }

    m_validity = Invalidating;
    m_nodeToInsertionPoint.clear();
    return needsReattach;
}

void ContentDistributor::distributeSelectionsTo(InsertionPoint* insertionPoint, Element* host)
{
    for (Node* child = host->firstChild(); child; child = child->nextSibling()) {
        ASSERT(!child->isInsertionPoint());

        if (insertionPoint->matchTypeFor(child) != InsertionPoint::AlwaysMatches)
            continue;

        m_nodeToInsertionPoint.add(child, insertionPoint);
    }

    if (m_nodeToInsertionPoint.isEmpty())
        return;
    insertionPoint->setHasDistribution();
}

void ContentDistributor::ensureDistribution(ShadowRoot* shadowRoot)
{
    ASSERT(shadowRoot);

    Vector<ElementShadow*, 8> elementShadows;
    for (Element* current = shadowRoot->host(); current; current = current->shadowHost()) {
        ElementShadow* elementShadow = current->shadow();
        if (!elementShadow->distributor().needsDistribution())
            break;

        elementShadows.append(elementShadow);
    }

    for (size_t i = elementShadows.size(); i > 0; --i)
        elementShadows[i - 1]->distributor().distribute(elementShadows[i - 1]->host());
}


void ContentDistributor::invalidateDistribution(Element* host)
{
    bool didNeedInvalidation = needsInvalidation();
    bool needsReattach = didNeedInvalidation ? invalidate(host) : false;

    if (needsReattach && host->attached()) {
        for (Node* n = host->firstChild(); n; n = n->nextSibling())
            n->lazyReattach();
        host->setNeedsStyleRecalc();
    }

    if (didNeedInvalidation) {
        ASSERT(m_validity == Invalidating);
        m_validity = Invalidated;
    }
}

void ContentDistributor::didShadowBoundaryChange(Element* host)
{
    setValidity(Undetermined);
    invalidateDistribution(host);
}

}
