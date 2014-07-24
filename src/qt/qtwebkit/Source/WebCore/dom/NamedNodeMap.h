/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010, 2013 Apple Inc. All rights reserved.
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

#ifndef NamedNodeMap_h
#define NamedNodeMap_h

#include "ScriptWrappable.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class Node;
class Element;

typedef int ExceptionCode;

class NamedNodeMap : public ScriptWrappable {
    WTF_MAKE_FAST_ALLOCATED;
    friend class Element;
public:
    static PassOwnPtr<NamedNodeMap> create(Element* element)
    {
        return adoptPtr(new NamedNodeMap(element));
    }

    void ref();
    void deref();

    // Public DOM interface.

    PassRefPtr<Node> getNamedItem(const AtomicString&) const;
    PassRefPtr<Node> removeNamedItem(const AtomicString& name, ExceptionCode&);

    PassRefPtr<Node> getNamedItemNS(const AtomicString& namespaceURI, const AtomicString& localName) const;
    PassRefPtr<Node> removeNamedItemNS(const AtomicString& namespaceURI, const AtomicString& localName, ExceptionCode&);

    PassRefPtr<Node> setNamedItem(Node*, ExceptionCode&);
    PassRefPtr<Node> setNamedItemNS(Node*, ExceptionCode&);

    PassRefPtr<Node> item(unsigned index) const;
    unsigned length() const;

    Element* element() const { return m_element; }

private:
    explicit NamedNodeMap(Element* element)
        : m_element(element)
    {
        // Only supports NamedNodeMaps with Element associated, DocumentType.entities and DocumentType.notations are not supported yet.
        ASSERT(m_element);
    }

    Element* m_element;
};

} // namespace WebCore

#endif // NamedNodeMap_h
