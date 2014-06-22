/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef ContainerNode_h
#define ContainerNode_h

#include "ExceptionCodePlaceholder.h"
#include "Node.h"

#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class FloatPoint;
    
typedef void (*NodeCallback)(Node*, unsigned);

namespace Private { 
    template<class GenericNode, class GenericNodeContainer>
    void addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer*);
};

class NoEventDispatchAssertion {
public:
    NoEventDispatchAssertion()
    {
#ifndef NDEBUG
        if (!isMainThread())
            return;
        s_count++;
#endif
    }

    ~NoEventDispatchAssertion()
    {
#ifndef NDEBUG
        if (!isMainThread())
            return;
        ASSERT(s_count);
        s_count--;
#endif
    }

#ifndef NDEBUG
    static bool isEventDispatchForbidden()
    {
        if (!isMainThread())
            return false;
        return s_count;
    }
#endif

private:
#ifndef NDEBUG
    static unsigned s_count;
#endif
};

class ContainerNode : public Node {
    friend class PostAttachCallbackDisabler;
public:
    virtual ~ContainerNode();

    Node* firstChild() const { return m_firstChild; }
    Node* lastChild() const { return m_lastChild; }
    bool hasChildNodes() const { return m_firstChild; }

    unsigned childNodeCount() const;
    Node* childNode(unsigned index) const;

    bool insertBefore(PassRefPtr<Node> newChild, Node* refChild, ExceptionCode& = ASSERT_NO_EXCEPTION, AttachBehavior = AttachNow);
    bool replaceChild(PassRefPtr<Node> newChild, Node* oldChild, ExceptionCode& = ASSERT_NO_EXCEPTION, AttachBehavior = AttachNow);
    bool removeChild(Node* child, ExceptionCode& = ASSERT_NO_EXCEPTION);
    bool appendChild(PassRefPtr<Node> newChild, ExceptionCode& = ASSERT_NO_EXCEPTION, AttachBehavior = AttachNow);

    // These methods are only used during parsing.
    // They don't send DOM mutation events or handle reparenting.
    // However, arbitrary code may be run by beforeload handlers.
    void parserAppendChild(PassRefPtr<Node>);
    void parserRemoveChild(Node*);
    void parserInsertBefore(PassRefPtr<Node> newChild, Node* refChild);

    void removeChildren();
    void takeAllChildrenFrom(ContainerNode*);

    void cloneChildNodes(ContainerNode* clone);

    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;
    virtual void detach(const AttachContext& = AttachContext()) OVERRIDE;
    virtual LayoutRect boundingBox() const OVERRIDE;
    virtual void scheduleSetNeedsStyleRecalc(StyleChangeType = FullStyleChange) OVERRIDE FINAL;

    // -----------------------------------------------------------------------------
    // Notification of document structure changes (see Node.h for more notification methods)

    // Notifies the node that it's list of children have changed (either by adding or removing child nodes), or a child
    // node that is of the type CDATA_SECTION_NODE, TEXT_NODE or COMMENT_NODE has changed its value.
    virtual void childrenChanged(bool createdByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    void disconnectDescendantFrames();

    virtual bool childShouldCreateRenderer(const NodeRenderingContext&) const { return true; }

protected:
    ContainerNode(Document*, ConstructionType = CreateContainer);

    static void queuePostAttachCallback(NodeCallback, Node*, unsigned = 0);
    static bool postAttachCallbacksAreSuspended();

    template<class GenericNode, class GenericNodeContainer>
    friend void appendChildToContainer(GenericNode* child, GenericNodeContainer*);

    template<class GenericNode, class GenericNodeContainer>
    friend void Private::addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer*);

    void removeDetachedChildren();
    void setFirstChild(Node* child) { m_firstChild = child; }
    void setLastChild(Node* child) { m_lastChild = child; }

private:
    void removeBetween(Node* previousChild, Node* nextChild, Node* oldChild);
    void insertBeforeCommon(Node* nextChild, Node* oldChild);

    void attachChildren(const AttachContext& = AttachContext());
    void detachChildren(const AttachContext& = AttachContext());

    static void dispatchPostAttachCallbacks();
    void suspendPostAttachCallbacks();
    void resumePostAttachCallbacks();

    bool getUpperLeftCorner(FloatPoint&) const;
    bool getLowerRightCorner(FloatPoint&) const;

    Node* m_firstChild;
    Node* m_lastChild;
};

#ifndef NDEBUG
bool childAttachedAllowedWhenAttachingChildren(ContainerNode*);
#endif

inline ContainerNode* toContainerNode(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isContainerNode());
    return static_cast<ContainerNode*>(node);
}

inline const ContainerNode* toContainerNode(const Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isContainerNode());
    return static_cast<const ContainerNode*>(node);
}

// This will catch anyone doing an unnecessary cast.
void toContainerNode(const ContainerNode*);

inline ContainerNode::ContainerNode(Document* document, ConstructionType type)
    : Node(document, type)
    , m_firstChild(0)
    , m_lastChild(0)
{
}

inline void ContainerNode::attachChildren(const AttachContext& context)
{
    AttachContext childrenContext(context);
    childrenContext.resolvedStyle = 0;

    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        ASSERT(!child->attached() || childAttachedAllowedWhenAttachingChildren(this));
        if (!child->attached())
            child->attach(childrenContext);
    }
}

inline void ContainerNode::detachChildren(const AttachContext& context)
{
    AttachContext childrenContext(context);
    childrenContext.resolvedStyle = 0;

    for (Node* child = firstChild(); child; child = child->nextSibling())
        child->detach(childrenContext);
}

inline unsigned Node::childNodeCount() const
{
    if (!isContainerNode())
        return 0;
    return toContainerNode(this)->childNodeCount();
}

inline Node* Node::childNode(unsigned index) const
{
    if (!isContainerNode())
        return 0;
    return toContainerNode(this)->childNode(index);
}

inline Node* Node::firstChild() const
{
    if (!isContainerNode())
        return 0;
    return toContainerNode(this)->firstChild();
}

inline Node* Node::lastChild() const
{
    if (!isContainerNode())
        return 0;
    return toContainerNode(this)->lastChild();
}

inline Node* Node::highestAncestor() const
{
    Node* node = const_cast<Node*>(this);
    Node* highest = node;
    for (; node; node = node->parentNode())
        highest = node;
    return highest;
}

inline bool Node::needsShadowTreeWalker() const
{
    if (getFlag(NeedsShadowTreeWalkerFlag))
        return true;
    ContainerNode* parent = parentOrShadowHostNode();
    return parent && parent->getFlag(NeedsShadowTreeWalkerFlag);
}

inline bool Node::isTreeScope() const
{
    return treeScope()->rootNode() == this;
}

// This constant controls how much buffer is initially allocated
// for a Node Vector that is used to store child Nodes of a given Node.
// FIXME: Optimize the value.
const int initialNodeVectorSize = 11;
typedef Vector<RefPtr<Node>, initialNodeVectorSize> NodeVector;

inline void getChildNodes(Node* node, NodeVector& nodes)
{
    ASSERT(!nodes.size());
    for (Node* child = node->firstChild(); child; child = child->nextSibling())
        nodes.append(child);
}

class ChildNodesLazySnapshot {
    WTF_MAKE_NONCOPYABLE(ChildNodesLazySnapshot);
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit ChildNodesLazySnapshot(Node* parentNode)
        : m_currentNode(parentNode->firstChild())
        , m_currentIndex(0)
    {
        m_nextSnapshot = latestSnapshot;
        latestSnapshot = this;
    }

    ~ChildNodesLazySnapshot()
    {
        latestSnapshot = m_nextSnapshot;
    }

    // Returns 0 if there is no next Node.
    PassRefPtr<Node> nextNode()
    {
        if (LIKELY(!hasSnapshot())) {
            RefPtr<Node> node = m_currentNode;
            if (node)
                m_currentNode = node->nextSibling();
            return node.release();
        }
        Vector<RefPtr<Node> >& nodeVector = *m_childNodes;
        if (m_currentIndex >= nodeVector.size())
            return 0;
        return nodeVector[m_currentIndex++];
    }

    void takeSnapshot()
    {
        if (hasSnapshot())
            return;
        m_childNodes = adoptPtr(new Vector<RefPtr<Node> >());
        Node* node = m_currentNode.get();
        while (node) {
            m_childNodes->append(node);
            node = node->nextSibling();
        }
    }

    ChildNodesLazySnapshot* nextSnapshot() { return m_nextSnapshot; }
    bool hasSnapshot() { return !!m_childNodes.get(); }

    static void takeChildNodesLazySnapshot()
    {
        ChildNodesLazySnapshot* snapshot = latestSnapshot;
        while (snapshot && !snapshot->hasSnapshot()) {
            snapshot->takeSnapshot();
            snapshot = snapshot->nextSnapshot();
        }
    }

private:
    static ChildNodesLazySnapshot* latestSnapshot;

    RefPtr<Node> m_currentNode;
    unsigned m_currentIndex;
    OwnPtr<Vector<RefPtr<Node> > > m_childNodes; // Lazily instantiated.
    ChildNodesLazySnapshot* m_nextSnapshot;
};

class PostAttachCallbackDisabler {
public:
    PostAttachCallbackDisabler(ContainerNode* node)
        : m_node(node)
    {
        ASSERT(m_node);
        m_node->suspendPostAttachCallbacks();
    }

    ~PostAttachCallbackDisabler()
    {
        m_node->resumePostAttachCallbacks();
    }

private:
    ContainerNode* m_node;
};

} // namespace WebCore

#endif // ContainerNode_h
