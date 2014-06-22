/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 David Smith <catfish.man@gmail.com>
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

#ifndef NodeRareData_h
#define NodeRareData_h

#include "ChildNodeList.h"
#include "DOMSettableTokenList.h"
#include "HTMLNames.h"
#include "LiveNodeList.h"
#include "MutationObserver.h"
#include "MutationObserverRegistration.h"
#include "Page.h"
#include "QualifiedName.h"
#include "TagNodeList.h"
#if ENABLE(VIDEO_TRACK)
#include "TextTrack.h"
#endif
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/StringHash.h>

#if ENABLE(MICRODATA)
#include "HTMLPropertiesCollection.h"
#include "MicroDataAttributeTokenList.h"
#include "MicroDataItemList.h"
#endif

namespace WebCore {

class LabelsNodeList;
class RadioNodeList;
class TreeScope;

class NodeListsNodeData {
    WTF_MAKE_NONCOPYABLE(NodeListsNodeData); WTF_MAKE_FAST_ALLOCATED;
public:
    void clearChildNodeListCache()
    {
        if (m_childNodeList)
            m_childNodeList->invalidateCache();
    }

    PassRefPtr<ChildNodeList> ensureChildNodeList(Node* node)
    {
        if (m_childNodeList)
            return m_childNodeList;
        RefPtr<ChildNodeList> list = ChildNodeList::create(node);
        m_childNodeList = list.get();
        return list.release();
    }

    void removeChildNodeList(ChildNodeList* list)
    {
        ASSERT(m_childNodeList = list);
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_childNodeList = 0;
    }

    template <typename StringType>
    struct NodeListCacheMapEntryHash {
        static unsigned hash(const std::pair<unsigned char, StringType>& entry)
        {
            return DefaultHash<StringType>::Hash::hash(entry.second) + entry.first;
        }
        static bool equal(const std::pair<unsigned char, StringType>& a, const std::pair<unsigned char, StringType>& b) { return a == b; }
        static const bool safeToCompareToEmptyOrDeleted = DefaultHash<StringType>::Hash::safeToCompareToEmptyOrDeleted;
    };

    typedef HashMap<std::pair<unsigned char, AtomicString>, LiveNodeListBase*, NodeListCacheMapEntryHash<AtomicString> > NodeListAtomicNameCacheMap;
    typedef HashMap<std::pair<unsigned char, String>, LiveNodeListBase*, NodeListCacheMapEntryHash<String> > NodeListNameCacheMap;
    typedef HashMap<QualifiedName, TagNodeList*> TagNodeListCacheNS;

    template<typename T>
    PassRefPtr<T> addCacheWithAtomicName(Node* node, CollectionType collectionType, const AtomicString& name)
    {
        NodeListAtomicNameCacheMap::AddResult result = m_atomicNameCaches.add(namedNodeListKey(collectionType, name), 0);
        if (!result.isNewEntry)
            return static_cast<T*>(result.iterator->value);

        RefPtr<T> list = T::create(node, collectionType, name);
        result.iterator->value = list.get();
        return list.release();
    }

    // FIXME: This function should be renamed since it doesn't have an atomic name.
    template<typename T>
    PassRefPtr<T> addCacheWithAtomicName(Node* node, CollectionType collectionType)
    {
        NodeListAtomicNameCacheMap::AddResult result = m_atomicNameCaches.add(namedNodeListKey(collectionType, starAtom), 0);
        if (!result.isNewEntry)
            return static_cast<T*>(result.iterator->value);

        RefPtr<T> list = T::create(node, collectionType);
        result.iterator->value = list.get();
        return list.release();
    }

    template<typename T>
    T* cacheWithAtomicName(CollectionType collectionType)
    {
        return static_cast<T*>(m_atomicNameCaches.get(namedNodeListKey(collectionType, starAtom)));
    }

    template<typename T>
    PassRefPtr<T> addCacheWithName(Node* node, CollectionType collectionType, const String& name)
    {
        NodeListNameCacheMap::AddResult result = m_nameCaches.add(namedNodeListKey(collectionType, name), 0);
        if (!result.isNewEntry)
            return static_cast<T*>(result.iterator->value);

        RefPtr<T> list = T::create(node, name);
        result.iterator->value = list.get();
        return list.release();
    }

    PassRefPtr<TagNodeList> addCacheWithQualifiedName(Node* node, const AtomicString& namespaceURI, const AtomicString& localName)
    {
        QualifiedName name(nullAtom, localName, namespaceURI);
        TagNodeListCacheNS::AddResult result = m_tagNodeListCacheNS.add(name, 0);
        if (!result.isNewEntry)
            return result.iterator->value;

        RefPtr<TagNodeList> list = TagNodeList::create(node, namespaceURI, localName);
        result.iterator->value = list.get();
        return list.release();
    }

    void removeCacheWithAtomicName(LiveNodeListBase* list, CollectionType collectionType, const AtomicString& name = starAtom)
    {
        ASSERT(list == m_atomicNameCaches.get(namedNodeListKey(collectionType, name)));
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_atomicNameCaches.remove(namedNodeListKey(collectionType, name));
    }

    void removeCacheWithName(LiveNodeListBase* list, CollectionType collectionType, const String& name)
    {
        ASSERT(list == m_nameCaches.get(namedNodeListKey(collectionType, name)));
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_nameCaches.remove(namedNodeListKey(collectionType, name));
    }

    void removeCacheWithQualifiedName(LiveNodeList* list, const AtomicString& namespaceURI, const AtomicString& localName)
    {
        QualifiedName name(nullAtom, localName, namespaceURI);
        ASSERT(list == m_tagNodeListCacheNS.get(name));
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_tagNodeListCacheNS.remove(name);
    }

    static PassOwnPtr<NodeListsNodeData> create()
    {
        return adoptPtr(new NodeListsNodeData);
    }

    void invalidateCaches(const QualifiedName* attrName = 0);
    bool isEmpty() const
    {
        return m_atomicNameCaches.isEmpty() && m_nameCaches.isEmpty() && m_tagNodeListCacheNS.isEmpty();
    }

    void adoptTreeScope()
    {
        invalidateCaches();
    }

    void adoptDocument(Document* oldDocument, Document* newDocument)
    {
        invalidateCaches();

        if (oldDocument != newDocument) {
            NodeListAtomicNameCacheMap::const_iterator atomicNameCacheEnd = m_atomicNameCaches.end();
            for (NodeListAtomicNameCacheMap::const_iterator it = m_atomicNameCaches.begin(); it != atomicNameCacheEnd; ++it) {
                LiveNodeListBase* list = it->value;
                oldDocument->unregisterNodeList(list);
                newDocument->registerNodeList(list);
            }

            NodeListNameCacheMap::const_iterator nameCacheEnd = m_nameCaches.end();
            for (NodeListNameCacheMap::const_iterator it = m_nameCaches.begin(); it != nameCacheEnd; ++it) {
                LiveNodeListBase* list = it->value;
                oldDocument->unregisterNodeList(list);
                newDocument->registerNodeList(list);
            }

            TagNodeListCacheNS::const_iterator tagEnd = m_tagNodeListCacheNS.end();
            for (TagNodeListCacheNS::const_iterator it = m_tagNodeListCacheNS.begin(); it != tagEnd; ++it) {
                LiveNodeListBase* list = it->value;
                ASSERT(!list->isRootedAtDocument());
                oldDocument->unregisterNodeList(list);
                newDocument->registerNodeList(list);
            }
        }
    }

private:
    NodeListsNodeData()
        : m_childNodeList(0)
    { }

    std::pair<unsigned char, AtomicString> namedNodeListKey(CollectionType type, const AtomicString& name)
    {
        return std::pair<unsigned char, AtomicString>(type, name);
    }

    std::pair<unsigned char, String> namedNodeListKey(CollectionType type, const String& name)
    {
        return std::pair<unsigned char, String>(type, name);
    }

    bool deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(Node*);

    // FIXME: m_childNodeList should be merged into m_atomicNameCaches or at least be shared with HTMLCollection returned by Element::children
    // but it's tricky because invalidateCaches shouldn't invalidate this cache and adoptTreeScope shouldn't call registerNodeList or unregisterNodeList.
    ChildNodeList* m_childNodeList;
    NodeListAtomicNameCacheMap m_atomicNameCaches;
    NodeListNameCacheMap m_nameCaches;
    TagNodeListCacheNS m_tagNodeListCacheNS;
};

class NodeMutationObserverData {
    WTF_MAKE_NONCOPYABLE(NodeMutationObserverData); WTF_MAKE_FAST_ALLOCATED;
public:
    Vector<OwnPtr<MutationObserverRegistration> > registry;
    HashSet<MutationObserverRegistration*> transientRegistry;

    static PassOwnPtr<NodeMutationObserverData> create() { return adoptPtr(new NodeMutationObserverData); }

private:
    NodeMutationObserverData() { }
};

#if ENABLE(MICRODATA)
class NodeMicroDataTokenLists {
    WTF_MAKE_NONCOPYABLE(NodeMicroDataTokenLists); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<NodeMicroDataTokenLists> create() { return adoptPtr(new NodeMicroDataTokenLists); }

    MicroDataAttributeTokenList* itemProp(Node* node) const
    {
        if (!m_itemProp)
            m_itemProp = MicroDataAttributeTokenList::create(toElement(node), HTMLNames::itempropAttr);
        return m_itemProp.get();
    }

    MicroDataAttributeTokenList* itemRef(Node* node) const
    {
        if (!m_itemRef)
            m_itemRef = MicroDataAttributeTokenList::create(toElement(node), HTMLNames::itemrefAttr);
        return m_itemRef.get();
    }

    MicroDataAttributeTokenList* itemType(Node* node) const
    {
        if (!m_itemType)
            m_itemType = MicroDataAttributeTokenList::create(toElement(node), HTMLNames::itemtypeAttr);
        return m_itemType.get();
    }

private:
    NodeMicroDataTokenLists() { }

    mutable RefPtr<MicroDataAttributeTokenList> m_itemProp;
    mutable RefPtr<MicroDataAttributeTokenList> m_itemRef;
    mutable RefPtr<MicroDataAttributeTokenList> m_itemType;
};
#endif

class NodeRareData : public NodeRareDataBase {
    WTF_MAKE_NONCOPYABLE(NodeRareData); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<NodeRareData> create(RenderObject* renderer) { return adoptPtr(new NodeRareData(renderer)); }

    void clearNodeLists() { m_nodeLists.clear(); }
    NodeListsNodeData* nodeLists() const { return m_nodeLists.get(); }
    NodeListsNodeData* ensureNodeLists()
    {
        if (!m_nodeLists)
            m_nodeLists = NodeListsNodeData::create();
        return m_nodeLists.get();
    }

    NodeMutationObserverData* mutationObserverData() { return m_mutationObserverData.get(); }
    NodeMutationObserverData* ensureMutationObserverData()
    {
        if (!m_mutationObserverData)
            m_mutationObserverData = NodeMutationObserverData::create();
        return m_mutationObserverData.get();
    }

#if ENABLE(MICRODATA)
    NodeMicroDataTokenLists* ensureMicroDataTokenLists() const
    {
        if (!m_microDataTokenLists)
            m_microDataTokenLists = NodeMicroDataTokenLists::create();
        return m_microDataTokenLists.get();
    }
#endif

    unsigned connectedSubframeCount() const { return m_connectedFrameCount; }
    void incrementConnectedSubframeCount(unsigned amount)
    {
        m_connectedFrameCount += amount;
    }
    void decrementConnectedSubframeCount(unsigned amount)
    {
        ASSERT(m_connectedFrameCount);
        ASSERT(amount <= m_connectedFrameCount);
        m_connectedFrameCount -= amount;
    }

protected:
    NodeRareData(RenderObject* renderer)
        : NodeRareDataBase(renderer)
        , m_connectedFrameCount(0)
    { }

private:
    unsigned m_connectedFrameCount : 10; // Must fit Page::maxNumberOfFrames.

    OwnPtr<NodeListsNodeData> m_nodeLists;
    OwnPtr<NodeMutationObserverData> m_mutationObserverData;

#if ENABLE(MICRODATA)
    mutable OwnPtr<NodeMicroDataTokenLists> m_microDataTokenLists;
#endif
};

inline bool NodeListsNodeData::deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(Node* ownerNode)
{
    ASSERT(ownerNode);
    ASSERT(ownerNode->nodeLists() == this);
    if ((m_childNodeList ? 1 : 0) + m_atomicNameCaches.size() + m_nameCaches.size() + m_tagNodeListCacheNS.size() != 1)
        return false;
    ownerNode->clearNodeLists();
    return true;
}

// Ensure the 10 bits reserved for the m_connectedFrameCount cannot overflow
COMPILE_ASSERT(Page::maxNumberOfFrames < 1024, Frame_limit_should_fit_in_rare_data_count);

} // namespace WebCore

#endif // NodeRareData_h
