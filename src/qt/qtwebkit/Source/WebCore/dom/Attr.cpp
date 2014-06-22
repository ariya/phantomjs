/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2012 Apple Inc. All rights reserved.
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

#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "ScopedEventQueue.h"
#include "StylePropertySet.h"
#include "StyledElement.h"
#include "Text.h"
#include "XMLNSNames.h"
#include <wtf/text/AtomicString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

Attr::Attr(Element* element, const QualifiedName& name)
    : ContainerNode(element->document())
    , m_element(element)
    , m_name(name)
    , m_ignoreChildrenChanged(0)
    , m_specified(true)
{
}

Attr::Attr(Document* document, const QualifiedName& name, const AtomicString& standaloneValue)
    : ContainerNode(document)
    , m_element(0)
    , m_name(name)
    , m_standaloneValue(standaloneValue)
    , m_ignoreChildrenChanged(0)
    , m_specified(true)
{
}

PassRefPtr<Attr> Attr::create(Element* element, const QualifiedName& name)
{
    RefPtr<Attr> attr = adoptRef(new Attr(element, name));
    attr->createTextChild();
    return attr.release();
}

PassRefPtr<Attr> Attr::create(Document* document, const QualifiedName& name, const AtomicString& value)
{
    RefPtr<Attr> attr = adoptRef(new Attr(document, name, value));
    attr->createTextChild();
    return attr.release();
}

Attr::~Attr()
{
}

void Attr::createTextChild()
{
    ASSERT(refCount());
    if (!value().isEmpty()) {
        RefPtr<Text> textNode = document()->createTextNode(value().string());

        // This does everything appendChild() would do in this situation (assuming m_ignoreChildrenChanged was set),
        // but much more efficiently.
        textNode->setParentOrShadowHostNode(this);
        setFirstChild(textNode.get());
        setLastChild(textNode.get());
    }
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

    const AtomicString& newPrefix = prefix.isEmpty() ? nullAtom : prefix;

    if (m_element)
        elementAttribute().setPrefix(newPrefix);
    m_name.setPrefix(newPrefix);
}

void Attr::setValue(const AtomicString& value)
{
    EventQueueScope scope;
    m_ignoreChildrenChanged++;
    removeChildren();
    if (m_element)
        elementAttribute().setValue(value);
    else
        m_standaloneValue = value;
    createTextChild();
    m_ignoreChildrenChanged--;

    invalidateNodeListCachesInAncestors(&m_name, m_element);
}

void Attr::setValue(const AtomicString& value, ExceptionCode&)
{
    if (m_element)
        m_element->willModifyAttribute(qualifiedName(), this->value(), value);

    setValue(value);

    if (m_element)
        m_element->didModifyAttribute(qualifiedName(), value);
}

void Attr::setNodeValue(const String& v, ExceptionCode& ec)
{
    setValue(v, ec);
}

PassRefPtr<Node> Attr::cloneNode(bool /*deep*/)
{
    RefPtr<Attr> clone = adoptRef(new Attr(document(), qualifiedName(), value()));
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

void Attr::childrenChanged(bool, Node*, Node*, int)
{
    if (m_ignoreChildrenChanged > 0)
        return;

    invalidateNodeListCachesInAncestors(&qualifiedName(), m_element);

    // FIXME: We should include entity references in the value

    StringBuilder valueBuilder;
    for (Node *n = firstChild(); n; n = n->nextSibling()) {
        if (n->isTextNode())
            valueBuilder.append(toText(n)->data());
    }

    AtomicString newValue = valueBuilder.toAtomicString();
    if (m_element)
        m_element->willModifyAttribute(qualifiedName(), value(), newValue);

    if (m_element)
        elementAttribute().setValue(newValue);
    else
        m_standaloneValue = newValue;

    if (m_element)
        m_element->attributeChanged(qualifiedName(), newValue);
}

bool Attr::isId() const
{
    return qualifiedName().matches(document()->idAttributeName());
}

CSSStyleDeclaration* Attr::style()
{
    // This function only exists to support the Obj-C bindings.
    if (!m_element || !m_element->isStyledElement())
        return 0;
    m_style = MutableStylePropertySet::create();
    static_cast<StyledElement*>(m_element)->collectStyleForPresentationAttribute(qualifiedName(), value(), m_style.get());
    return m_style->ensureCSSStyleDeclaration();
}

const AtomicString& Attr::value() const
{
    if (m_element)
        return m_element->getAttribute(qualifiedName());
    return m_standaloneValue;
}

Attribute& Attr::elementAttribute()
{
    ASSERT(m_element);
    ASSERT(m_element->elementData());
    return *m_element->ensureUniqueElementData()->getAttributeItem(qualifiedName());
}

void Attr::detachFromElementWithValue(const AtomicString& value)
{
    ASSERT(m_element);
    ASSERT(m_standaloneValue.isNull());
    m_standaloneValue = value;
    m_element = 0;
}

void Attr::attachToElement(Element* element)
{
    ASSERT(!m_element);
    m_element = element;
    m_standaloneValue = nullAtom;
}

}
