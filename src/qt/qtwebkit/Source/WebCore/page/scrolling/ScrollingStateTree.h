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

#ifndef ScrollingStateTree_h
#define ScrollingStateTree_h

#if ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)

#include "ScrollingStateScrollingNode.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>
 
namespace WebCore {
 
// The ScrollingStateTree is a tree that managed ScrollingStateNodes. The nodes keep track of the current
// state of scrolling related properties. Whenever any properties change, the scrolling coordinator
// will be informed and will schedule a timer that will clone the new state tree and send it over to
// the scrolling thread, avoiding locking. 

class ScrollingStateTree {
    friend class ScrollingStateNode;
public:
    
    static PassOwnPtr<ScrollingStateTree> create();
    ~ScrollingStateTree();

    ScrollingStateScrollingNode* rootStateNode() const { return m_rootStateNode.get(); }
    ScrollingStateNode* stateNodeForID(ScrollingNodeID);

    ScrollingNodeID attachNode(ScrollingNodeType, ScrollingNodeID, ScrollingNodeID parentID);
    void detachNode(ScrollingNodeID);
    void clear();
    
    const Vector<ScrollingNodeID>& removedNodes() const { return m_nodesRemovedSinceLastCommit; }

    // Copies the current tree state and clears the changed properties mask in the original.
    PassOwnPtr<ScrollingStateTree> commit();

    void setHasChangedProperties(bool changedProperties) { m_hasChangedProperties = changedProperties; }
    bool hasChangedProperties() const { return m_hasChangedProperties; }

    bool hasNewRootStateNode() const { return m_hasNewRootStateNode; }

private:
    ScrollingStateTree();

    void setRootStateNode(PassOwnPtr<ScrollingStateScrollingNode> rootStateNode) { m_rootStateNode = rootStateNode; }
    void removeNode(ScrollingStateNode*);
    void didRemoveNode(ScrollingNodeID);

    PassOwnPtr<ScrollingStateTree> clone();

    HashMap<ScrollingNodeID, ScrollingStateNode*> m_stateNodeMap;
    OwnPtr<ScrollingStateScrollingNode> m_rootStateNode;
    Vector<ScrollingNodeID> m_nodesRemovedSinceLastCommit;
    bool m_hasChangedProperties;
    bool m_hasNewRootStateNode;
};

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)

#endif // ScrollingStateTree_h
