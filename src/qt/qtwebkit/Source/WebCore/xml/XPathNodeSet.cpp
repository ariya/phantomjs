/*
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XPathNodeSet.h"

#include "Attr.h"
#include "Element.h"
#include "Node.h"
#include "NodeTraversal.h"

namespace WebCore {
namespace XPath {

// When a node set is large, sorting it by traversing the whole document is better (we can
// assume that we aren't dealing with documents that we cannot even traverse in reasonable time).
const unsigned traversalSortCutoff = 10000;

static inline Node* parentWithDepth(unsigned depth, const Vector<Node*>& parents)
{
    ASSERT(parents.size() >= depth + 1);
    return parents[parents.size() - 1 - depth];
}

static void sortBlock(unsigned from, unsigned to, Vector<Vector<Node*> >& parentMatrix, bool mayContainAttributeNodes)
{
    ASSERT(from + 1 < to); // Should not call this function with less that two nodes to sort.
    unsigned minDepth = UINT_MAX;
    for (unsigned i = from; i < to; ++i) {
        unsigned depth = parentMatrix[i].size() - 1;
        if (minDepth > depth)
            minDepth = depth;
    }
    
    // Find the common ancestor.
    unsigned commonAncestorDepth = minDepth;
    Node* commonAncestor;
    while (true) {
        commonAncestor = parentWithDepth(commonAncestorDepth, parentMatrix[from]);
        if (commonAncestorDepth == 0)
            break;

        bool allEqual = true;
        for (unsigned i = from + 1; i < to; ++i) {
            if (commonAncestor != parentWithDepth(commonAncestorDepth, parentMatrix[i])) {
                allEqual = false;
                break;
            }
        }
        if (allEqual)
            break;
        
        --commonAncestorDepth;
    }

    if (commonAncestorDepth == minDepth) {
        // One of the nodes is the common ancestor => it is the first in document order.
        // Find it and move it to the beginning.
        for (unsigned i = from; i < to; ++i)
            if (commonAncestor == parentMatrix[i][0]) {
                parentMatrix[i].swap(parentMatrix[from]);
                if (from + 2 < to)
                    sortBlock(from + 1, to, parentMatrix, mayContainAttributeNodes);
                return;
            }
    }
    
    if (mayContainAttributeNodes && commonAncestor->isElementNode()) {
        // The attribute nodes and namespace nodes of an element occur before the children of the element.
        // The namespace nodes are defined to occur before the attribute nodes.
        // The relative order of namespace nodes is implementation-dependent.
        // The relative order of attribute nodes is implementation-dependent.
        unsigned sortedEnd = from;
        // FIXME: namespace nodes are not implemented.
        for (unsigned i = sortedEnd; i < to; ++i) {
            Node* n = parentMatrix[i][0];
            if (n->isAttributeNode() && static_cast<Attr*>(n)->ownerElement() == commonAncestor)
                parentMatrix[i].swap(parentMatrix[sortedEnd++]);
        }
        if (sortedEnd != from) {
            if (to - sortedEnd > 1)
                sortBlock(sortedEnd, to, parentMatrix, mayContainAttributeNodes);
            return;
        }
    }

    // Children nodes of the common ancestor induce a subdivision of our node-set.
    // Sort it according to this subdivision, and recursively sort each group.
    HashSet<Node*> parentNodes;
    for (unsigned i = from; i < to; ++i)
        parentNodes.add(parentWithDepth(commonAncestorDepth + 1, parentMatrix[i]));

    unsigned previousGroupEnd = from;
    unsigned groupEnd = from;
    for (Node* n = commonAncestor->firstChild(); n; n = n->nextSibling()) {
        // If parentNodes contains the node, perform a linear search to move its children in the node-set to the beginning.
        if (parentNodes.contains(n)) {
            for (unsigned i = groupEnd; i < to; ++i)
                if (parentWithDepth(commonAncestorDepth + 1, parentMatrix[i]) == n)
                    parentMatrix[i].swap(parentMatrix[groupEnd++]);

            if (groupEnd - previousGroupEnd > 1)
                sortBlock(previousGroupEnd, groupEnd, parentMatrix, mayContainAttributeNodes);

            ASSERT(previousGroupEnd != groupEnd);
            previousGroupEnd = groupEnd;
#ifndef NDEBUG
            parentNodes.remove(n);
#endif
        }
    }

    ASSERT(parentNodes.isEmpty());
}

void NodeSet::sort() const
{
    if (m_isSorted)
        return;

    unsigned nodeCount = m_nodes.size();
    if (nodeCount < 2) {
        const_cast<bool&>(m_isSorted) = true;
        return;
    }

    if (nodeCount > traversalSortCutoff) {
        traversalSort();
        return;
    }

    bool containsAttributeNodes = false;
    
    Vector<Vector<Node*> > parentMatrix(nodeCount);
    for (unsigned i = 0; i < nodeCount; ++i) {
        Vector<Node*>& parentsVector = parentMatrix[i];
        Node* n = m_nodes[i].get();
        parentsVector.append(n);
        if (n->isAttributeNode()) {
            n = static_cast<Attr*>(n)->ownerElement();
            parentsVector.append(n);
            containsAttributeNodes = true;
        }
        while ((n = n->parentNode()))
            parentsVector.append(n);
    }
    sortBlock(0, nodeCount, parentMatrix, containsAttributeNodes);
    
    // It is not possible to just assign the result to m_nodes, because some nodes may get dereferenced and destroyed.
    Vector<RefPtr<Node> > sortedNodes;
    sortedNodes.reserveInitialCapacity(nodeCount);
    for (unsigned i = 0; i < nodeCount; ++i)
        sortedNodes.append(parentMatrix[i][0]);
    
    const_cast<Vector<RefPtr<Node> >&>(m_nodes).swap(sortedNodes);
}

static Node* findRootNode(Node* node)
{
    if (node->isAttributeNode())
        node = static_cast<Attr*>(node)->ownerElement();
    if (node->inDocument())
        node = node->document();
    else {
        while (Node* parent = node->parentNode())
            node = parent;
    }
    return node;
}

void NodeSet::traversalSort() const
{
    HashSet<Node*> nodes;
    bool containsAttributeNodes = false;

    unsigned nodeCount = m_nodes.size();
    ASSERT(nodeCount > 1);
    for (unsigned i = 0; i < nodeCount; ++i) {
        Node* node = m_nodes[i].get();
        nodes.add(node);
        if (node->isAttributeNode())
            containsAttributeNodes = true;
    }

    Vector<RefPtr<Node> > sortedNodes;
    sortedNodes.reserveInitialCapacity(nodeCount);

    for (Node* n = findRootNode(m_nodes.first().get()); n; n = NodeTraversal::next(n)) {
        if (nodes.contains(n))
            sortedNodes.append(n);

        if (!containsAttributeNodes || !n->isElementNode())
            continue;

        Element* element = toElement(n);
        if (!element->hasAttributes())
            continue;

        unsigned attributeCount = element->attributeCount();
        for (unsigned i = 0; i < attributeCount; ++i) {
            RefPtr<Attr> attr = element->attrIfExists(element->attributeItem(i)->name());
            if (attr && nodes.contains(attr.get()))
                sortedNodes.append(attr);
        }
    }

    ASSERT(sortedNodes.size() == nodeCount);
    const_cast<Vector<RefPtr<Node> >&>(m_nodes).swap(sortedNodes);
}

void NodeSet::reverse()
{
    if (m_nodes.isEmpty())
        return;

    unsigned from = 0;
    unsigned to = m_nodes.size() - 1;
    while (from < to) {
        m_nodes[from].swap(m_nodes[to]);
        ++from;
        --to;
    }
}

Node* NodeSet::firstNode() const
{
    if (isEmpty())
        return 0;

    sort(); // FIXME: fully sorting the node-set just to find its first node is wasteful.
    return m_nodes.at(0).get();
}

Node* NodeSet::anyNode() const
{
    if (isEmpty())
        return 0;

    return m_nodes.at(0).get();
}

}
}
