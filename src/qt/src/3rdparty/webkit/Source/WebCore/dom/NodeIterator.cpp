/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NodeIterator.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "NodeFilter.h"
#include "ScriptState.h"

namespace WebCore {

NodeIterator::NodePointer::NodePointer()
{
}

NodeIterator::NodePointer::NodePointer(PassRefPtr<Node> n, bool b)
    : node(n)
    , isPointerBeforeNode(b)
{
}

void NodeIterator::NodePointer::clear()
{
    node.clear();
}

bool NodeIterator::NodePointer::moveToNext(Node* root)
{
    if (!node)
        return false;
    if (isPointerBeforeNode) {
        isPointerBeforeNode = false;
        return true;
    }
    node = node->traverseNextNode(root);
    return node;
}

bool NodeIterator::NodePointer::moveToPrevious(Node* root)
{
    if (!node)
        return false;
    if (!isPointerBeforeNode) {
        isPointerBeforeNode = true;
        return true;
    }
    node = node->traversePreviousNode(root);
    return node;
}

NodeIterator::NodeIterator(PassRefPtr<Node> rootNode, unsigned whatToShow, PassRefPtr<NodeFilter> filter, bool expandEntityReferences)
    : Traversal(rootNode, whatToShow, filter, expandEntityReferences)
    , m_referenceNode(root(), true)
    , m_detached(false)
{
    // Document type nodes may have a null document. But since they can't have children, there is no need to listen for modifications to these.
    ASSERT(root()->document() || root()->nodeType() == Node::DOCUMENT_TYPE_NODE);
    if (Document* ownerDocument = root()->document())
        ownerDocument->attachNodeIterator(this);
}

NodeIterator::~NodeIterator()
{
    if (Document* ownerDocument = root()->document())
        ownerDocument->detachNodeIterator(this);
}

PassRefPtr<Node> NodeIterator::nextNode(ScriptState* state, ExceptionCode& ec)
{
    if (m_detached) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    RefPtr<Node> result;

    m_candidateNode = m_referenceNode;
    while (m_candidateNode.moveToNext(root())) {
        // NodeIterators treat the DOM tree as a flat list of nodes.
        // In other words, FILTER_REJECT does not pass over descendants
        // of the rejected node. Hence, FILTER_REJECT is the same as FILTER_SKIP.
        RefPtr<Node> provisionalResult = m_candidateNode.node;
        bool nodeWasAccepted = acceptNode(state, provisionalResult.get()) == NodeFilter::FILTER_ACCEPT;
        if (state && state->hadException())
            break;
        if (nodeWasAccepted) {
            m_referenceNode = m_candidateNode;
            result = provisionalResult.release();
            break;
        }
    }

    m_candidateNode.clear();
    return result.release();
}

PassRefPtr<Node> NodeIterator::previousNode(ScriptState* state, ExceptionCode& ec)
{
    if (m_detached) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    RefPtr<Node> result;

    m_candidateNode = m_referenceNode;
    while (m_candidateNode.moveToPrevious(root())) {
        // NodeIterators treat the DOM tree as a flat list of nodes.
        // In other words, FILTER_REJECT does not pass over descendants
        // of the rejected node. Hence, FILTER_REJECT is the same as FILTER_SKIP.
        RefPtr<Node> provisionalResult = m_candidateNode.node;
        bool nodeWasAccepted = acceptNode(state, provisionalResult.get()) == NodeFilter::FILTER_ACCEPT;
        if (state && state->hadException())
            break;
        if (nodeWasAccepted) {
            m_referenceNode = m_candidateNode;
            result = provisionalResult.release();
            break;
        }
    }

    m_candidateNode.clear();
    return result.release();
}

void NodeIterator::detach()
{
    if (Document* ownerDocument = root()->document())
        ownerDocument->detachNodeIterator(this);
    m_detached = true;
    m_referenceNode.node.clear();
}

void NodeIterator::nodeWillBeRemoved(Node* removedNode)
{
    updateForNodeRemoval(removedNode, m_candidateNode);
    updateForNodeRemoval(removedNode, m_referenceNode);
}

void NodeIterator::updateForNodeRemoval(Node* removedNode, NodePointer& referenceNode) const
{
    ASSERT(!m_detached);
    ASSERT(removedNode);
    ASSERT(root()->document());
    ASSERT(root()->document() == removedNode->document());

    // Iterator is not affected if the removed node is the reference node and is the root.
    // or if removed node is not the reference node, or the ancestor of the reference node.
    if (!removedNode->isDescendantOf(root()))
        return;
    bool willRemoveReferenceNode = removedNode == referenceNode.node;
    bool willRemoveReferenceNodeAncestor = referenceNode.node && referenceNode.node->isDescendantOf(removedNode);
    if (!willRemoveReferenceNode && !willRemoveReferenceNodeAncestor)
        return;

    if (referenceNode.isPointerBeforeNode) {
        Node* node = removedNode->traverseNextNode(root());
        if (node) {
            // Move out from under the node being removed if the reference node is
            // a descendant of the node being removed.
            if (willRemoveReferenceNodeAncestor) {
                while (node && node->isDescendantOf(removedNode))
                    node = node->traverseNextNode(root());
            }
            if (node)
                referenceNode.node = node;
        } else {
            node = removedNode->traversePreviousNode(root());
            if (node) {
                // Move out from under the node being removed if the reference node is
                // a descendant of the node being removed.
                if (willRemoveReferenceNodeAncestor) {
                    while (node && node->isDescendantOf(removedNode))
                        node = node->traversePreviousNode(root());
                }
                if (node) {
                    // Removing last node.
                    // Need to move the pointer after the node preceding the 
                    // new reference node.
                    referenceNode.node = node;
                    referenceNode.isPointerBeforeNode = false;
                }
            }
        }
    } else {
        Node* node = removedNode->traversePreviousNode(root());
        if (node) {
            // Move out from under the node being removed if the reference node is
            // a descendant of the node being removed.
            if (willRemoveReferenceNodeAncestor) {
                while (node && node->isDescendantOf(removedNode))
                    node = node->traversePreviousNode(root());
            }
            if (node)
                referenceNode.node = node;
        } else {
            node = removedNode->traverseNextNode(root());
            // Move out from under the node being removed if the reference node is
            // a descendant of the node being removed.
            if (willRemoveReferenceNodeAncestor) {
                while (node && node->isDescendantOf(removedNode))
                    node = node->traversePreviousNode(root());
            }
            if (node)
                referenceNode.node = node;
        }
    }
}


} // namespace WebCore
