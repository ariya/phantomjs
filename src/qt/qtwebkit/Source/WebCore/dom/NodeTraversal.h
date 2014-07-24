/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef NodeTraversal_h
#define NodeTraversal_h

#include "Element.h"

namespace WebCore {

namespace ElementTraversal {

// First element child of the node.
Element* firstWithin(const Node*);
Element* firstWithin(const ContainerNode*);

// Pre-order traversal skipping non-element nodes.
Element* next(const Node*);
Element* next(const Node*, const Node* stayWithin);
Element* next(const ContainerNode*);
Element* next(const ContainerNode*, const Node* stayWithin);

// Like next, but skips children.
Element* nextSkippingChildren(const Node*);
Element* nextSkippingChildren(const Node*, const Node* stayWithin);
Element* nextSkippingChildren(const ContainerNode*);
Element* nextSkippingChildren(const ContainerNode*, const Node* stayWithin);

// Pre-order traversal including the pseudo-elements.
Element* previousIncludingPseudo(const Node*, const Node* = 0);
Element* nextIncludingPseudo(const Node*, const Node* = 0);
Element* nextIncludingPseudoSkippingChildren(const Node*, const Node* = 0);

// Utility function to traverse only the element and pseudo-element siblings of a node.
Element* pseudoAwarePreviousSibling(const Node*);

}

namespace NodeTraversal {

// Does a pre-order traversal of the tree to find the next node after this one.
// This uses the same order that tags appear in the source file. If the stayWithin
// argument is non-null, the traversal will stop once the specified node is reached.
// This can be used to restrict traversal to a particular sub-tree.
Node* next(const Node*);
Node* next(const Node*, const Node* stayWithin);
Node* next(const ContainerNode*);
Node* next(const ContainerNode*, const Node* stayWithin);

// Like next, but skips children and starts with the next sibling.
Node* nextSkippingChildren(const Node*);
Node* nextSkippingChildren(const Node*, const Node* stayWithin);
Node* nextSkippingChildren(const ContainerNode*);
Node* nextSkippingChildren(const ContainerNode*, const Node* stayWithin);

// Does a reverse pre-order traversal to find the node that comes before the current one in document order
Node* previous(const Node*, const Node* stayWithin = 0);

// Like previous, but skips children and starts with the next sibling.
Node* previousSkippingChildren(const Node*, const Node* stayWithin = 0);

// Like next, but visits parents after their children.
Node* nextPostOrder(const Node*, const Node* stayWithin = 0);

// Like previous/previousSkippingChildren, but visits parents before their children.
Node* previousPostOrder(const Node*, const Node* stayWithin = 0);
Node* previousSkippingChildrenPostOrder(const Node*, const Node* stayWithin = 0);

// Pre-order traversal including the pseudo-elements.
Node* previousIncludingPseudo(const Node*, const Node* = 0);
Node* nextIncludingPseudo(const Node*, const Node* = 0);
Node* nextIncludingPseudoSkippingChildren(const Node*, const Node* = 0);

}

namespace ElementTraversal {
template <class NodeType>
inline Element* firstElementWithinTemplate(NodeType* current)
{
    // Except for the root containers, only elements can have element children.
    Node* node = current->firstChild();
    while (node && !node->isElementNode())
        node = node->nextSibling();
    return toElement(node);
}
inline Element* firstWithin(const ContainerNode* current) { return firstElementWithinTemplate(current); }
inline Element* firstWithin(const Node* current) { return firstElementWithinTemplate(current); }

template <class NodeType>
inline Element* traverseNextElementTemplate(NodeType* current)
{
    Node* node = NodeTraversal::next(current);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextSkippingChildren(node);
    return toElement(node);
}
inline Element* next(const ContainerNode* current) { return traverseNextElementTemplate(current); }
inline Element* next(const Node* current) { return traverseNextElementTemplate(current); }

template <class NodeType>
inline Element* traverseNextElementTemplate(NodeType* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::next(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextSkippingChildren(node, stayWithin);
    return toElement(node);
}
inline Element* next(const ContainerNode* current, const Node* stayWithin) { return traverseNextElementTemplate(current, stayWithin); }
inline Element* next(const Node* current, const Node* stayWithin) { return traverseNextElementTemplate(current, stayWithin); }

template <class NodeType>
inline Element* traverseNextElementSkippingChildrenTemplate(NodeType* current)
{
    Node* node = NodeTraversal::nextSkippingChildren(current);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextSkippingChildren(node);
    return toElement(node);
}
inline Element* nextSkippingChildren(const ContainerNode* current) { return traverseNextElementSkippingChildrenTemplate(current); }
inline Element* nextSkippingChildren(const Node* current) { return traverseNextElementSkippingChildrenTemplate(current); }

template <class NodeType>
inline Element* traverseNextElementSkippingChildrenTemplate(NodeType* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::nextSkippingChildren(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextSkippingChildren(node, stayWithin);
    return toElement(node);
}
inline Element* nextSkippingChildren(const ContainerNode* current, const Node* stayWithin) { return traverseNextElementSkippingChildrenTemplate(current, stayWithin); }
inline Element* nextSkippingChildren(const Node* current, const Node* stayWithin) { return traverseNextElementSkippingChildrenTemplate(current, stayWithin); }

inline Element* previousIncludingPseudo(const Node* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::previousIncludingPseudo(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::previousIncludingPseudo(node, stayWithin);
    return toElement(node);
}

inline Element* nextIncludingPseudo(const Node* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::nextIncludingPseudo(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextIncludingPseudo(node, stayWithin);
    return toElement(node);
}

inline Element* nextIncludingPseudoSkippingChildren(const Node* current, const Node* stayWithin)
{
    Node* node = NodeTraversal::nextIncludingPseudoSkippingChildren(current, stayWithin);
    while (node && !node->isElementNode())
        node = NodeTraversal::nextIncludingPseudoSkippingChildren(node, stayWithin);
    return toElement(node);
}

inline Element* pseudoAwarePreviousSibling(const Node* current)
{
    Node* node = current->pseudoAwarePreviousSibling();
    while (node && !node->isElementNode())
        node = node->pseudoAwarePreviousSibling();
    return toElement(node);
}

}

namespace NodeTraversal {

Node* nextAncestorSibling(const Node*);
Node* nextAncestorSibling(const Node*, const Node* stayWithin);

template <class NodeType>
inline Node* traverseNextTemplate(NodeType* current)
{
    if (current->firstChild())
        return current->firstChild();
    if (current->nextSibling())
        return current->nextSibling();
    return nextAncestorSibling(current);
}
inline Node* next(const Node* current) { return traverseNextTemplate(current); }
inline Node* next(const ContainerNode* current) { return traverseNextTemplate(current); }
    
template <class NodeType>
inline Node* traverseNextTemplate(NodeType* current, const Node* stayWithin)
{
    if (current->firstChild())
        return current->firstChild();
    if (current == stayWithin)
        return 0;
    if (current->nextSibling())
        return current->nextSibling();
    return nextAncestorSibling(current, stayWithin);
}
inline Node* next(const Node* current, const Node* stayWithin) { return traverseNextTemplate(current, stayWithin); }
inline Node* next(const ContainerNode* current, const Node* stayWithin) { return traverseNextTemplate(current, stayWithin); }

template <class NodeType>
inline Node* traverseNextSkippingChildrenTemplate(NodeType* current)
{
    if (current->nextSibling())
        return current->nextSibling();
    return nextAncestorSibling(current);
}
inline Node* nextSkippingChildren(const Node* current) { return traverseNextSkippingChildrenTemplate(current); }
inline Node* nextSkippingChildren(const ContainerNode* current) { return traverseNextSkippingChildrenTemplate(current); }

template <class NodeType>
inline Node* traverseNextSkippingChildrenTemplate(NodeType* current, const Node* stayWithin)
{
    if (current == stayWithin)
        return 0;
    if (current->nextSibling())
        return current->nextSibling();
    return nextAncestorSibling(current, stayWithin);
}
inline Node* nextSkippingChildren(const Node* current, const Node* stayWithin) { return traverseNextSkippingChildrenTemplate(current, stayWithin); }
inline Node* nextSkippingChildren(const ContainerNode* current, const Node* stayWithin) { return traverseNextSkippingChildrenTemplate(current, stayWithin); }

}

}

#endif
