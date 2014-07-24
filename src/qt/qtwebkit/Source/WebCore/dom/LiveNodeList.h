/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
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

#ifndef LiveNodeList_h
#define LiveNodeList_h

#include "CollectionType.h"
#include "Document.h"
#include "HTMLNames.h"
#include "NodeList.h"
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Element;

enum NodeListRootType {
    NodeListIsRootedAtNode,
    NodeListIsRootedAtDocument,
    NodeListIsRootedAtDocumentIfOwnerHasItemrefAttr,
};

class LiveNodeListBase : public NodeList {
public:
    enum ItemAfterOverrideType {
        OverridesItemAfter,
        DoesNotOverrideItemAfter,
    };

    LiveNodeListBase(Node* ownerNode, NodeListRootType rootType, NodeListInvalidationType invalidationType,
        bool shouldOnlyIncludeDirectChildren, CollectionType collectionType, ItemAfterOverrideType itemAfterOverrideType)
        : m_ownerNode(ownerNode)
        , m_cachedItem(0)
        , m_isLengthCacheValid(false)
        , m_isItemCacheValid(false)
        , m_rootType(rootType)
        , m_invalidationType(invalidationType)
        , m_shouldOnlyIncludeDirectChildren(shouldOnlyIncludeDirectChildren)
        , m_isNameCacheValid(false)
        , m_collectionType(collectionType)
        , m_overridesItemAfter(itemAfterOverrideType == OverridesItemAfter)
        , m_isItemRefElementsCacheValid(false)
    {
        ASSERT(m_rootType == static_cast<unsigned>(rootType));
        ASSERT(m_invalidationType == static_cast<unsigned>(invalidationType));
        ASSERT(m_collectionType == static_cast<unsigned>(collectionType));
        ASSERT(!m_overridesItemAfter || !isNodeList(collectionType));

        if (collectionType != ChildNodeListType)
            document()->registerNodeList(this);
    }

    virtual ~LiveNodeListBase()
    {
        if (type() != ChildNodeListType)
            document()->unregisterNodeList(this);
    }

    // DOM API
    virtual unsigned length() const OVERRIDE;
    virtual Node* item(unsigned offset) const OVERRIDE;

    ALWAYS_INLINE bool hasIdNameCache() const { return !isNodeList(type()); }
    ALWAYS_INLINE bool isRootedAtDocument() const { return m_rootType == NodeListIsRootedAtDocument || m_rootType == NodeListIsRootedAtDocumentIfOwnerHasItemrefAttr; }
    ALWAYS_INLINE NodeListInvalidationType invalidationType() const { return static_cast<NodeListInvalidationType>(m_invalidationType); }
    ALWAYS_INLINE CollectionType type() const { return static_cast<CollectionType>(m_collectionType); }
    Node* ownerNode() const { return m_ownerNode.get(); }
    ALWAYS_INLINE void invalidateCache(const QualifiedName* attrName) const
    {
        if (!attrName || shouldInvalidateTypeOnAttributeChange(invalidationType(), *attrName))
            invalidateCache();
        else if (hasIdNameCache() && (*attrName == HTMLNames::idAttr || *attrName == HTMLNames::nameAttr))
            invalidateIdNameCacheMaps();
    }
    void invalidateCache() const;
    void invalidateIdNameCacheMaps() const;

    static bool shouldInvalidateTypeOnAttributeChange(NodeListInvalidationType, const QualifiedName&);

protected:
    Document* document() const { return m_ownerNode->document(); }
    Node* rootNode() const;
    ContainerNode* rootContainerNode() const;
    bool overridesItemAfter() const { return m_overridesItemAfter; }

    ALWAYS_INLINE bool isItemCacheValid() const { return m_isItemCacheValid; }
    ALWAYS_INLINE Node* cachedItem() const { return m_cachedItem; }
    ALWAYS_INLINE unsigned cachedItemOffset() const { return m_cachedItemOffset; }

    ALWAYS_INLINE bool isLengthCacheValid() const { return m_isLengthCacheValid; }
    ALWAYS_INLINE unsigned cachedLength() const { return m_cachedLength; }
    ALWAYS_INLINE void setLengthCache(unsigned length) const
    {
        m_cachedLength = length;
        m_isLengthCacheValid = true;
    }
    ALWAYS_INLINE void setItemCache(Node* item, unsigned offset) const
    {
        ASSERT(item);
        m_cachedItem = item;
        m_cachedItemOffset = offset;
        m_isItemCacheValid = true;
    }
    void setItemCache(Node* item, unsigned offset, unsigned elementsArrayOffset) const;

    ALWAYS_INLINE bool isItemRefElementsCacheValid() const { return m_isItemRefElementsCacheValid; }
    ALWAYS_INLINE void setItemRefElementsCacheValid() const { m_isItemRefElementsCacheValid = true; }

    ALWAYS_INLINE NodeListRootType rootType() const { return static_cast<NodeListRootType>(m_rootType); }

    bool hasNameCache() const { return m_isNameCacheValid; }
    void setHasNameCache() const { m_isNameCacheValid = true; }

    bool shouldOnlyIncludeDirectChildren() const { return m_shouldOnlyIncludeDirectChildren; }

private:
    Node* itemBeforeOrAfterCachedItem(unsigned offset, ContainerNode* root) const;
    Node* traverseChildNodeListForwardToOffset(unsigned offset, Node* currentNode, unsigned& currentOffset) const;
    Element* traverseLiveNodeListFirstElement(ContainerNode* root) const;
    Element* traverseLiveNodeListForwardToOffset(unsigned offset, Element* currentElement, unsigned& currentOffset, ContainerNode* root) const;
    bool isLastItemCloserThanLastOrCachedItem(unsigned offset) const;
    bool isFirstItemCloserThanCachedItem(unsigned offset) const;
    Node* iterateForPreviousNode(Node* current) const;
    Node* itemBefore(Node* previousItem) const;

    RefPtr<Node> m_ownerNode;
    mutable Node* m_cachedItem;
    mutable unsigned m_cachedLength;
    mutable unsigned m_cachedItemOffset;
    mutable unsigned m_isLengthCacheValid : 1;
    mutable unsigned m_isItemCacheValid : 1;
    const unsigned m_rootType : 2;
    const unsigned m_invalidationType : 4;
    const unsigned m_shouldOnlyIncludeDirectChildren : 1;

    // From HTMLCollection
    mutable unsigned m_isNameCacheValid : 1;
    const unsigned m_collectionType : 5;
    const unsigned m_overridesItemAfter : 1;
    mutable unsigned m_isItemRefElementsCacheValid : 1;
};

ALWAYS_INLINE bool LiveNodeListBase::shouldInvalidateTypeOnAttributeChange(NodeListInvalidationType type, const QualifiedName& attrName)
{
    switch (type) {
    case InvalidateOnClassAttrChange:
        return attrName == HTMLNames::classAttr;
    case InvalidateOnNameAttrChange:
        return attrName == HTMLNames::nameAttr;
    case InvalidateOnIdNameAttrChange:
        return attrName == HTMLNames::idAttr || attrName == HTMLNames::nameAttr;
    case InvalidateOnForAttrChange:
        return attrName == HTMLNames::forAttr;
    case InvalidateForFormControls:
        return attrName == HTMLNames::nameAttr || attrName == HTMLNames::idAttr || attrName == HTMLNames::forAttr
            || attrName == HTMLNames::formAttr || attrName == HTMLNames::typeAttr;
    case InvalidateOnHRefAttrChange:
        return attrName == HTMLNames::hrefAttr;
    case InvalidateOnItemAttrChange:
#if ENABLE(MICRODATA)
        return attrName == HTMLNames::itemscopeAttr || attrName == HTMLNames::itempropAttr
            || attrName == HTMLNames::itemtypeAttr || attrName == HTMLNames::itemrefAttr || attrName == HTMLNames::idAttr;
#endif // Intentionally fall through
    case DoNotInvalidateOnAttributeChanges:
        return false;
    case InvalidateOnAnyAttrChange:
        return true;
    }
    return false;
}

class LiveNodeList : public LiveNodeListBase {
public:
    LiveNodeList(PassRefPtr<Node> ownerNode, CollectionType collectionType, NodeListInvalidationType invalidationType, NodeListRootType rootType = NodeListIsRootedAtNode)
        : LiveNodeListBase(ownerNode.get(), rootType, invalidationType, collectionType == ChildNodeListType,
        collectionType, DoesNotOverrideItemAfter)
    { }

    virtual Node* namedItem(const AtomicString&) const OVERRIDE;
    virtual bool nodeMatches(Element*) const = 0;

private:
    virtual bool isLiveNodeList() const OVERRIDE { return true; }
};

} // namespace WebCore

#endif // LiveNodeList_h
