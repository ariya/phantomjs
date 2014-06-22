/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef NodeRenderingTraversal_h
#define NodeRenderingTraversal_h

#include "Element.h"

namespace WebCore {

class InsertionPoint;

namespace NodeRenderingTraversal {

class ParentDetails {
public:
    ParentDetails()
        : m_insertionPoint(0)
        , m_resetStyleInheritance(false)
        , m_outOfComposition(false)
    { }

    InsertionPoint* insertionPoint() const { return m_insertionPoint; }
    bool resetStyleInheritance() const { return m_resetStyleInheritance; }
    bool outOfComposition() const { return m_outOfComposition; }

    void didTraverseInsertionPoint(InsertionPoint*);
    void didTraverseShadowRoot(const ShadowRoot*);
    void childWasOutOfComposition() { m_outOfComposition = true; }

    bool operator==(const ParentDetails& other)
    {
        return m_insertionPoint == other.m_insertionPoint
            && m_resetStyleInheritance == other.m_resetStyleInheritance
            && m_outOfComposition == other.m_outOfComposition;
    }

private:
    InsertionPoint* m_insertionPoint;
    bool m_resetStyleInheritance;
    bool m_outOfComposition;
};

ContainerNode* parent(const Node*, ParentDetails*);
ContainerNode* parentSlow(const Node*, ParentDetails*);
Node* nextSibling(const Node*);
Node* nextSiblingSlow(const Node*);
Node* previousSibling(const Node*);
Node* previousSiblingSlow(const Node*);

Node* nextInScope(const Node*);
Node* previousInScope(const Node*);
Node* parentInScope(const Node*);
Node* lastChildInScope(const Node*);

inline ContainerNode* parent(const Node* node, ParentDetails* details)
{
    if (!node->needsShadowTreeWalker()) {
#ifndef NDEBUG
        ParentDetails slowDetails;
        ASSERT(node->parentNode() == parentSlow(node, &slowDetails));
        ASSERT(slowDetails == *details);
#endif
        return node->parentNodeGuaranteedHostFree();
    }

    return parentSlow(node, details);
}

inline Node* nextSibling(const Node* node)
{
    if (!node->needsShadowTreeWalker()) {
        ASSERT(nextSiblingSlow(node) == node->nextSibling());
        return node->nextSibling();
    }

    return nextSiblingSlow(node);
}

inline Node* previousSibling(const Node* node)
{
    if (!node->needsShadowTreeWalker()) {
        ASSERT(previousSiblingSlow(node) == node->previousSibling());
        return node->previousSibling();
    }

    return previousSiblingSlow(node);
}

}

} // namespace WebCore

#endif
