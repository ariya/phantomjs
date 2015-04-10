/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef Attr_h
#define Attr_h

#include "ContainerNode.h"
#include "QualifiedName.h"

namespace WebCore {

class CSSStyleDeclaration;
class MutableStylePropertySet;

// Attr can have Text and EntityReference children
// therefore it has to be a fullblown Node. The plan
// is to dynamically allocate a textchild and store the
// resulting nodevalue in the attribute upon
// destruction. however, this is not yet implemented.

class Attr FINAL : public ContainerNode {
public:
    static PassRefPtr<Attr> create(Element*, const QualifiedName&);
    static PassRefPtr<Attr> create(Document*, const QualifiedName&, const AtomicString& value);
    virtual ~Attr();

    String name() const { return qualifiedName().toString(); }
    bool specified() const { return m_specified; }
    Element* ownerElement() const { return m_element; }

    const AtomicString& value() const;
    void setValue(const AtomicString&, ExceptionCode&);
    void setValue(const AtomicString&);

    const QualifiedName& qualifiedName() const { return m_name; }

    bool isId() const;

    CSSStyleDeclaration* style();

    void setSpecified(bool specified) { m_specified = specified; }

    void attachToElement(Element*);
    void detachFromElementWithValue(const AtomicString&);

private:
    Attr(Element*, const QualifiedName&);
    Attr(Document*, const QualifiedName&, const AtomicString& value);

    void createTextChild();

    virtual String nodeName() const OVERRIDE { return name(); }
    virtual NodeType nodeType() const OVERRIDE { return ATTRIBUTE_NODE; }

    virtual const AtomicString& localName() const OVERRIDE { return m_name.localName(); }
    virtual const AtomicString& namespaceURI() const OVERRIDE { return m_name.namespaceURI(); }
    virtual const AtomicString& prefix() const OVERRIDE { return m_name.prefix(); }

    virtual void setPrefix(const AtomicString&, ExceptionCode&);

    virtual String nodeValue() const OVERRIDE { return value(); }
    virtual void setNodeValue(const String&, ExceptionCode&);
    virtual PassRefPtr<Node> cloneNode(bool deep);

    virtual bool isAttributeNode() const { return true; }
    virtual bool childTypeAllowed(NodeType) const;

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    Attribute& elementAttribute();

    // Attr wraps either an element/name, or a name/value pair (when it's a standalone Node.)
    // Note that m_name is always set, but m_element/m_standaloneValue may be null.
    Element* m_element;
    QualifiedName m_name;
    AtomicString m_standaloneValue;

    RefPtr<MutableStylePropertySet> m_style;
    unsigned m_ignoreChildrenChanged : 31;
    bool m_specified : 1;
};

} // namespace WebCore

#endif // Attr_h
