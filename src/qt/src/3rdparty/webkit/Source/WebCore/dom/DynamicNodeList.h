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

#ifndef DynamicNodeList_h
#define DynamicNodeList_h

#include "NodeList.h"
#include <wtf/RefCounted.h>
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class Element;
    class Node;

    class DynamicNodeList : public NodeList {
    public:
        struct Caches : RefCounted<Caches> {
            static PassRefPtr<Caches> create();
            void reset();

            unsigned cachedLength;
            Node* lastItem;
            unsigned lastItemOffset;
            bool isLengthCacheValid : 1;
            bool isItemCacheValid : 1;
        protected:
            Caches();
        };

        virtual ~DynamicNodeList();

        bool hasOwnCaches() const { return m_ownsCaches; }

        // DOM methods & attributes for NodeList
        virtual unsigned length() const;
        virtual Node* item(unsigned index) const;
        virtual Node* itemWithName(const AtomicString&) const;

        // Other methods (not part of DOM)
        void invalidateCache();
        Node* rootNode() const { return m_rootNode.get(); }

    protected:
        DynamicNodeList(PassRefPtr<Node> rootNode);
        DynamicNodeList(PassRefPtr<Node> rootNode, Caches*);

        virtual bool nodeMatches(Element*) const = 0;

        RefPtr<Node> m_rootNode;
        mutable RefPtr<Caches> m_caches;
        bool m_ownsCaches;

    private:
        virtual bool isDynamicNodeList() const;
        Node* itemForwardsFromCurrent(Node* start, unsigned offset, int remainingOffset) const;
        Node* itemBackwardsFromCurrent(Node* start, unsigned offset, int remainingOffset) const;
    };

} // namespace WebCore

#endif // DynamicNodeList_h
