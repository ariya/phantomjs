/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "config.h"
#include "ScrollingStateTree.h"

#if ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)

#include "ScrollingStateFixedNode.h"
#include "ScrollingStateScrollingNode.h"
#include "ScrollingStateStickyNode.h"

namespace WebCore {

PassOwnPtr<ScrollingStateTree> ScrollingStateTree::create()
{
    return adoptPtr(new ScrollingStateTree);
}

ScrollingStateTree::ScrollingStateTree()
    : m_hasChangedProperties(false)
    , m_hasNewRootStateNode(false)
{
}

ScrollingStateTree::~ScrollingStateTree()
{
}

ScrollingNodeID ScrollingStateTree::attachNode(ScrollingNodeType nodeType, ScrollingNodeID newNodeID, ScrollingNodeID parentID)
{
    ASSERT(newNodeID);

    if (ScrollingStateNode* node = stateNodeForID(newNodeID)) {
        ScrollingStateNode* parent = stateNodeForID(parentID);
        if (!parent)
            return newNodeID;
        if (node->parent() == parent)
            return newNodeID;

        // The node is being re-parented. To do that, we'll remove it, and then re-create a new node.
        removeNode(node);
    }

    ScrollingStateNode* newNode = 0;
    if (!parentID) {
        // If we're resetting the root node, we should clear the HashMap and destroy the current children.
        clear();

        setRootStateNode(ScrollingStateScrollingNode::create(this, newNodeID));
        newNode = rootStateNode();
        m_hasNewRootStateNode = true;
    } else {
        ScrollingStateNode* parent = stateNodeForID(parentID);
        if (!parent)
            return 0;

        switch (nodeType) {
        case FixedNode: {
            OwnPtr<ScrollingStateFixedNode> fixedNode = ScrollingStateFixedNode::create(this, newNodeID);
            newNode = fixedNode.get();
            parent->appendChild(fixedNode.release());
            break;
        }
        case StickyNode: {
            OwnPtr<ScrollingStateStickyNode> stickyNode = ScrollingStateStickyNode::create(this, newNodeID);
            newNode = stickyNode.get();
            parent->appendChild(stickyNode.release());
            break;
        }
        case ScrollingNode: {
            // FIXME: We currently only support child nodes that are fixed.
            ASSERT_NOT_REACHED();
            OwnPtr<ScrollingStateScrollingNode> scrollingNode = ScrollingStateScrollingNode::create(this, newNodeID);
            newNode = scrollingNode.get();
            parent->appendChild(scrollingNode.release());
            break;
        }
        }
    }

    m_stateNodeMap.set(newNodeID, newNode);
    return newNodeID;
}

void ScrollingStateTree::detachNode(ScrollingNodeID nodeID)
{
    if (!nodeID)
        return;

    // The node may not be found if clearStateTree() was recently called.
    ScrollingStateNode* node = m_stateNodeMap.take(nodeID);
    if (!node)
        return;

    removeNode(node);
}

void ScrollingStateTree::clear()
{
    removeNode(rootStateNode());
    m_stateNodeMap.clear();
}

PassOwnPtr<ScrollingStateTree> ScrollingStateTree::commit()
{
    // This function clones and resets the current state tree, but leaves the tree structure intact.
    OwnPtr<ScrollingStateTree> treeStateClone = ScrollingStateTree::create();
    if (m_rootStateNode)
        treeStateClone->setRootStateNode(static_pointer_cast<ScrollingStateScrollingNode>(m_rootStateNode->cloneAndReset()));

    // Copy the IDs of the nodes that have been removed since the last commit into the clone.
    treeStateClone->m_nodesRemovedSinceLastCommit.swap(m_nodesRemovedSinceLastCommit);

    // Now the clone tree has changed properties, and the original tree does not.
    treeStateClone->m_hasChangedProperties = true;
    m_hasChangedProperties = false;

    treeStateClone->m_hasNewRootStateNode = m_hasNewRootStateNode;
    m_hasNewRootStateNode = false;

    return treeStateClone.release();
}

void ScrollingStateTree::removeNode(ScrollingStateNode* node)
{
    if (!node)
        return;

    if (node == m_rootStateNode) {
        didRemoveNode(node->scrollingNodeID());
        m_rootStateNode = nullptr;
        return;
    }

    ASSERT(m_rootStateNode);
    m_rootStateNode->removeChild(node);

    // ScrollingStateTree::removeNode() will destroy children, so we have to make sure we remove those children
    // from the HashMap.
    size_t size = m_nodesRemovedSinceLastCommit.size();
    for (size_t i = 0; i < size; ++i)
        m_stateNodeMap.remove(m_nodesRemovedSinceLastCommit[i]);
}

void ScrollingStateTree::didRemoveNode(ScrollingNodeID nodeID)
{
    m_nodesRemovedSinceLastCommit.append(nodeID);
    m_hasChangedProperties = true;
}

ScrollingStateNode* ScrollingStateTree::stateNodeForID(ScrollingNodeID scrollLayerID)
{
    if (!scrollLayerID)
        return 0;

    HashMap<ScrollingNodeID, ScrollingStateNode*>::const_iterator it = m_stateNodeMap.find(scrollLayerID);
    if (it == m_stateNodeMap.end())
        return 0;

    ASSERT(it->value->scrollingNodeID() == scrollLayerID);
    return it->value;
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)
