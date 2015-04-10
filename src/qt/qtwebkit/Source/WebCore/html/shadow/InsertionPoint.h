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

#ifndef InsertionPoint_h
#define InsertionPoint_h

#include "ContentDistributor.h"
#include "ElementShadow.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "ShadowRoot.h"
#include <wtf/Forward.h>

namespace WebCore {

class InsertionPoint : public HTMLElement {
public:
    enum Type {
        InternalType,
        HTMLContentElementType
    };

    enum MatchType {
        AlwaysMatches,
        NeverMatches,
    };

    virtual ~InsertionPoint();

    bool hasDistribution() const { return m_hasDistribution; }
    void setHasDistribution() { m_hasDistribution = true; }
    void clearDistribution() { m_hasDistribution = false; }
    bool isShadowBoundary() const;
    bool isActive() const;

    virtual MatchType matchTypeFor(Node*) const { return AlwaysMatches; }
    virtual Type insertionPointType() const { return InternalType; }

    bool resetStyleInheritance() const;
    void setResetStyleInheritance(bool);

    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;
    virtual void detach(const AttachContext& = AttachContext()) OVERRIDE;

    bool shouldUseFallbackElements() const;

    Node* firstDistributed() const;
    Node* lastDistributed() const;
    Node* nextDistributedTo(const Node*) const;
    Node* previousDistributedTo(const Node*) const;

protected:
    InsertionPoint(const QualifiedName&, Document*);
    virtual bool rendererIsNeeded(const NodeRenderingContext&) OVERRIDE;
    virtual void childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta) OVERRIDE;
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool isInsertionPointNode() const OVERRIDE { return true; }

private:

    bool m_hasDistribution;
};

inline InsertionPoint* toInsertionPoint(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isInsertionPoint());
    return static_cast<InsertionPoint*>(node);
}

inline const InsertionPoint* toInsertionPoint(const Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isInsertionPoint());
    return static_cast<const InsertionPoint*>(node);
}

inline bool isActiveInsertionPoint(const Node* node)
{
    return node->isInsertionPoint() && toInsertionPoint(node)->isActive();
}

inline bool isLowerEncapsulationBoundary(Node* node)
{
    if (!node || !node->isInsertionPoint())
        return false;
    return toInsertionPoint(node)->isShadowBoundary();
}

inline Node* parentNodeForDistribution(const Node* node)
{
    ASSERT(node);

    if (Node* parent = node->parentNode()) {
        if (parent->isInsertionPoint() && toInsertionPoint(parent)->shouldUseFallbackElements())
            return parent->parentNode();
        return parent;
    }

    return 0;
}

inline Element* parentElementForDistribution(const Node* node)
{
    if (Node* parent = parentNodeForDistribution(node)) {
        if (parent->isElementNode())
            return toElement(parent);
    }

    return 0;
}

inline ElementShadow* shadowOfParentForDistribution(const Node* node)
{
    ASSERT(node);
    if (Element* parent = parentElementForDistribution(node))
        return parent->shadow();

    return 0;
}

InsertionPoint* resolveReprojection(const Node*);

} // namespace WebCore

#endif // InsertionPoint_h
