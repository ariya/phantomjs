/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "InsertionPoint.h"

#include "ElementShadow.h"
#include "HTMLNames.h"
#include "QualifiedName.h"
#include "ShadowRoot.h"
#include "StaticNodeList.h"

namespace WebCore {

using namespace HTMLNames;

InsertionPoint::InsertionPoint(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document, CreateInsertionPoint)
    , m_hasDistribution(false)
{
}

InsertionPoint::~InsertionPoint()
{
}

void InsertionPoint::attach(const AttachContext& context)
{
    if (ShadowRoot* shadowRoot = containingShadowRoot())
        ContentDistributor::ensureDistribution(shadowRoot);
    for (Node* current = firstDistributed(); current; current = nextDistributedTo(current)) {
        if (!current->attached())
            current->attach(context);
    }

    HTMLElement::attach(context);
}

void InsertionPoint::detach(const AttachContext& context)
{
    if (ShadowRoot* shadowRoot = containingShadowRoot())
        ContentDistributor::ensureDistribution(shadowRoot);

    for (Node* current = firstDistributed(); current; current = nextDistributedTo(current))
        current->detach(context);

    HTMLElement::detach(context);
}

bool InsertionPoint::shouldUseFallbackElements() const
{
    return isActive() && !hasDistribution();
}

bool InsertionPoint::isShadowBoundary() const
{
    return treeScope()->rootNode()->isShadowRoot() && isActive();
}

bool InsertionPoint::isActive() const
{
    if (!containingShadowRoot())
        return false;
    const Node* node = parentNode();
    while (node) {
        if (node->isInsertionPoint())
            return false;

        node = node->parentNode();
    }
    return true;
}

bool InsertionPoint::rendererIsNeeded(const NodeRenderingContext& context)
{
    return !isShadowBoundary() && HTMLElement::rendererIsNeeded(context);
}

void InsertionPoint::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    if (ShadowRoot* root = containingShadowRoot())
        if (ElementShadow* rootOwner = root->owner())
            rootOwner->invalidateDistribution();
}

Node::InsertionNotificationRequest InsertionPoint::insertedInto(ContainerNode* insertionPoint)
{
    HTMLElement::insertedInto(insertionPoint);

    if (ShadowRoot* root = containingShadowRoot()) {
        if (ElementShadow* rootOwner = root->owner()) {
            rootOwner->distributor().didShadowBoundaryChange(root->host());
            rootOwner->distributor().invalidateInsertionPointList();
        }
    }

    return InsertionDone;
}

void InsertionPoint::removedFrom(ContainerNode* insertionPoint)
{
    ShadowRoot* root = containingShadowRoot();
    if (!root)
        root = insertionPoint->containingShadowRoot();

    // host can be null when removedFrom() is called from ElementShadow destructor.
    ElementShadow* rootOwner = root ? root->owner() : 0;
    if (rootOwner) {
        rootOwner->invalidateDistribution();
        rootOwner->distributor().invalidateInsertionPointList();
    }

    // Since this insertion point is no longer visible from the shadow subtree, it need to clean itself up.
    clearDistribution();

    HTMLElement::removedFrom(insertionPoint);
}

void InsertionPoint::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == reset_style_inheritanceAttr) {
        if (!inDocument() || !attached() || !isActive())
            return;
        containingShadowRoot()->host()->setNeedsStyleRecalc();
    } else
        HTMLElement::parseAttribute(name, value);
}

bool InsertionPoint::resetStyleInheritance() const
{
    return fastHasAttribute(reset_style_inheritanceAttr);
}

void InsertionPoint::setResetStyleInheritance(bool value)
{
    setBooleanAttribute(reset_style_inheritanceAttr, value);
}
    
Node* InsertionPoint::firstDistributed() const
{
    if (!m_hasDistribution)
        return 0;
    for (Node* current = shadowHost()->firstChild(); current; current = current->nextSibling()) {
        if (matchTypeFor(current) == InsertionPoint::AlwaysMatches)
            return current;
    }
    return 0;
}

Node* InsertionPoint::lastDistributed() const
{
    if (!m_hasDistribution)
        return 0;
    for (Node* current = shadowHost()->lastChild(); current; current = current->previousSibling()) {
        if (matchTypeFor(current) == InsertionPoint::AlwaysMatches)
            return current;
    }
    return 0;
}

Node* InsertionPoint::nextDistributedTo(const Node* node) const
{
    for (Node* current = node->nextSibling(); current; current = current->nextSibling()) {
        if (matchTypeFor(current) == InsertionPoint::AlwaysMatches)
            return current;
    }
    return 0;
}

Node* InsertionPoint::previousDistributedTo(const Node* node) const
{
    for (Node* current = node->previousSibling(); current; current = current->previousSibling()) {
        if (matchTypeFor(current) == InsertionPoint::AlwaysMatches)
            return current;
    }
    return 0;
}

InsertionPoint* resolveReprojection(const Node* projectedNode)
{
    if (ElementShadow* shadow = shadowOfParentForDistribution(projectedNode)) {
        if (ShadowRoot* root = projectedNode->containingShadowRoot())
            ContentDistributor::ensureDistribution(root);
        return shadow->distributor().findInsertionPointFor(projectedNode);
    }
    return 0;
}

} // namespace WebCore
