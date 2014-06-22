/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 */

#ifndef TagNodeList_h
#define TagNodeList_h

#include "Element.h"
#include "LiveNodeList.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

// NodeList that limits to a particular tag.
class TagNodeList : public LiveNodeList {
public:
    static PassRefPtr<TagNodeList> create(PassRefPtr<Node> rootNode, const AtomicString& namespaceURI, const AtomicString& localName)
    {
        ASSERT(namespaceURI != starAtom);
        return adoptRef(new TagNodeList(rootNode, TagNodeListType, namespaceURI, localName));
    }

    static PassRefPtr<TagNodeList> create(PassRefPtr<Node> rootNode, CollectionType type, const AtomicString& localName)
    {
        ASSERT_UNUSED(type, type == TagNodeListType);
        return adoptRef(new TagNodeList(rootNode, TagNodeListType, starAtom, localName));
    }

    virtual ~TagNodeList();

protected:
    TagNodeList(PassRefPtr<Node> rootNode, CollectionType, const AtomicString& namespaceURI, const AtomicString& localName);

    virtual bool nodeMatches(Element*) const;

    AtomicString m_namespaceURI;
    AtomicString m_localName;
};

class HTMLTagNodeList : public TagNodeList {
public:
    static PassRefPtr<HTMLTagNodeList> create(PassRefPtr<Node> rootNode, CollectionType type, const AtomicString& localName)
    {
        ASSERT_UNUSED(type, type == HTMLTagNodeListType);
        return adoptRef(new HTMLTagNodeList(rootNode, localName));
    }

    bool nodeMatchesInlined(Element*) const;

private:
    HTMLTagNodeList(PassRefPtr<Node> rootNode, const AtomicString& localName);

    virtual bool nodeMatches(Element*) const;

    AtomicString m_loweredLocalName;
};

inline bool HTMLTagNodeList::nodeMatchesInlined(Element* testNode) const
{
    // Implements http://dvcs.w3.org/hg/domcore/raw-file/tip/Overview.html#concept-getelementsbytagname
    if (m_localName != starAtom) {
        const AtomicString& localName = testNode->isHTMLElement() ? m_loweredLocalName : m_localName;
        if (localName != testNode->localName())
            return false;
    }
    ASSERT(m_namespaceURI == starAtom);
    return true;
}

} // namespace WebCore

#endif // TagNodeList_h
