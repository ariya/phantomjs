/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
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
#include "config.h"
#include "Attr.h"

#include "Element.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "ScopedEventQueue.h"
#include "Text.h"
#include "XMLNSNames.h"

namespace WebCore {

using namespace HTMLNames;

inline Attr::Attr(Element* element, Document* document, PassRefPtr<Attribute> attribute)
    : ContainerNode(document)
    , m_element(element)
    , m_attribute(attribute)
    , m_ignoreChildrenChanged(0)
    , m_specified(true)
{
    ASSERT(!m_attribute->attr());
    m_attribute->bindAttr(this);
}

PassRefPtr<Attr> Attr::create(Element* element, Document* document, PassRefPtr<Attribute> attribute)
{
    RefPtr<Attr> attr = adoptRef(new Attr(element, document, attribute));
    attr->createTextChild();
    return attr.release();
}

Attr::~Attr()
{
    ASSERT(m_attribute->attr() == this);
    m_attribute->unbindAttr(this);
}

void Attr::createTextChild()
{
    ASSERT(refCount());
    if (!m_attribute->value().isEmpty()) {
        RefPtr<Text> textNode = document()->createTextNode(m_attribute->value().string());

        // This does everything appendChild() would do in this situation (assuming m_ignoreChildrenChanged was set),
        // but much more efficiently.
        textNode->setParent(this);
        setFirstChild(textNode.get());
        setLastChild(textNode.get());
    }
}

String Attr::nodeName() const
{
    return name();
}

Node::NodeType Attr::nodeType() const
{
    return ATTRIBUTE_NODE;
}

const AtomicString& Attr::localName() const
{
    return m_attribute->localName();
}

const AtomicString& Attr::namespaceURI() const
{
    return m_attribute->namespaceURI();
}

const AtomicString& Attr::prefix() const
{
    return m_attribute->prefix();
}

void Attr::setPrefix(const AtomicString& prefix, ExceptionCode& ec)
{
    ec = 0;
    checkSetPrefix(prefix, ec);
    if (ec)
        return;

    if ((prefix == xmlnsAtom && namespaceURI() != XMLNSNames::xmlnsNamespaceURI)
        || static_cast<Attr*>(this)->qualifiedName() == xmlnsAtom) {
        ec = NAMESPACE_ERR;
        return;
    }

    m_attribute->setPrefix(prefix.isEmpty() ? AtomicString() : prefix);
}

String Attr::nodeValue() const
{
    return value();
}

void Attr::setValue(const AtomicString& value)
{
    EventQueueScope scope;
    m_ignoreChildrenChanged++;
    removeChildren();
    m_attribute->setValue(value);
    createTextChild();
    m_ignoreChildrenChanged--;
}

void Attr::setValue(const AtomicString& value, ExceptionCode&)
{
    if (m_element && m_element->isIdAttributeName(m_attribute->name()))
        m_element->updateId(m_element->getIdAttribute(), value);

    setValue(value);

    if (m_element)
        m_element->attributeChanged(m_attribute.get());
}

void Attr::setNodeValue(const String& v, ExceptionCode& ec)
{
    setValue(v, ec);
}

PassRefPtr<Node> Attr::cloneNode(bool /*deep*/)
{
    RefPtr<Attr> clone = adoptRef(new Attr(0, document(), m_attribute->clone()));
    cloneChildNodes(clone.get());
    return clone.release();
}

// DOM Section 1.1.1
bool Attr::childTypeAllowed(NodeType type) const
{
    switch (type) {
        case TEXT_NODE:
        case ENTITY_REFERENCE_NODE:
            return true;
        default:
            return false;
    }
}

void Attr::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    if (m_ignoreChildrenChanged > 0)
        return;
 
    Node::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    // FIXME: We should include entity references in the value
    
    String val = "";
    for (Node *n = firstChild(); n; n = n->nextSibling()) {
        if (n->isTextNode())
            val += static_cast<Text *>(n)->data();
    }

    if (m_element && m_element->isIdAttributeName(m_attribute->name()))
        m_element->updateId(m_attribute->value(), val);

    m_attribute->setValue(val.impl());
    if (m_element)
        m_element->attributeChanged(m_attribute.get());
}

bool Attr::isId() const
{
    return qualifiedName().matches(document()->idAttributeName());
}

}
