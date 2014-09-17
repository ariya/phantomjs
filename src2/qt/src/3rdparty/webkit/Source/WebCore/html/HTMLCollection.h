/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef HTMLCollection_h
#define HTMLCollection_h

#include "CollectionType.h"
#include <wtf/RefCounted.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

class Element;
class Node;
class NodeList;

struct CollectionCache;

class HTMLCollection : public RefCounted<HTMLCollection> {
public:
    static PassRefPtr<HTMLCollection> create(PassRefPtr<Node> base, CollectionType);
    virtual ~HTMLCollection();
    
    unsigned length() const;
    
    virtual Node* item(unsigned index) const;
    virtual Node* nextItem() const;

    virtual Node* namedItem(const AtomicString& name) const;
    virtual Node* nextNamedItem(const AtomicString& name) const; // In case of multiple items named the same way

    Node* firstItem() const;

    void namedItems(const AtomicString& name, Vector<RefPtr<Node> >&) const;

    PassRefPtr<NodeList> tags(const String&);

    Node* base() const { return m_base.get(); }
    CollectionType type() const { return m_type; }

protected:
    HTMLCollection(PassRefPtr<Node> base, CollectionType, CollectionCache*);
    HTMLCollection(PassRefPtr<Node> base, CollectionType);

    CollectionCache* info() const { return m_info; }
    void resetCollectionInfo() const;

    mutable bool m_idsDone; // for nextNamedItem()

private:
    virtual Element* itemAfter(Element*) const;
    virtual unsigned calcLength() const;
    virtual void updateNameCache() const;

    bool checkForNameMatch(Element*, bool checkName, const AtomicString& name) const;

    RefPtr<Node> m_base;
    CollectionType m_type;

    mutable CollectionCache* m_info;
    mutable bool m_ownsInfo;
};

} // namespace

#endif
