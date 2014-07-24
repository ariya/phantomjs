/*
 * Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Motorola Mobility, Inc. nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MICRODATA)

#include "HTMLPropertiesCollection.h"

#include "DOMSettableTokenList.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "Node.h"
#include "NodeTraversal.h"
#include "PropertyNodeList.h"

namespace WebCore {

using namespace HTMLNames;

PassRefPtr<HTMLPropertiesCollection> HTMLPropertiesCollection::create(Node* itemNode, CollectionType)
{
    return adoptRef(new HTMLPropertiesCollection(itemNode));
}

HTMLPropertiesCollection::HTMLPropertiesCollection(Node* itemNode)
    : HTMLCollection(itemNode, ItemProperties, OverridesItemAfter)
{
}

HTMLPropertiesCollection::~HTMLPropertiesCollection()
{
}

void HTMLPropertiesCollection::updateRefElements() const
{
    if (isItemRefElementsCacheValid())
        return;

    m_itemRefElements.clear();
    setItemRefElementsCacheValid();
    toHTMLElement(ownerNode())->getItemRefElements(m_itemRefElements);
}

static Node* nextNodeWithProperty(Node* rootNode, Node* previous, Node* ownerNode)
{
    // An Microdata item may contain properties which in turn are items themselves. Properties can
    // also themselves be groups of name-value pairs, by putting the itemscope attribute on the element
    // that declares the property. If the property has an itemscope attribute specified then we need
    // to traverse the next sibling.
    return previous == ownerNode || (previous->isHTMLElement() && !toHTMLElement(previous)->fastHasAttribute(itemscopeAttr))
        ? NodeTraversal::next(previous, rootNode)
        : NodeTraversal::nextSkippingChildren(previous, rootNode);
}

Element* HTMLPropertiesCollection::virtualItemAfter(unsigned& offsetInArray, Element* previousItem) const
{
    while (offsetInArray < m_itemRefElements.size()) {
        if (Element* next = virtualItemAfter(m_itemRefElements[offsetInArray], previousItem))
            return next;
        offsetInArray++;
        previousItem = 0;
    }
    return 0;
}

HTMLElement* HTMLPropertiesCollection::virtualItemAfter(HTMLElement* rootNode, Element* previous) const
{
    Node* current;
    Node* ownerNode = this->ownerNode();
    current = previous ? nextNodeWithProperty(rootNode, previous, ownerNode) : rootNode;

    for (; current; current = nextNodeWithProperty(rootNode, current, ownerNode)) {
        if (current == ownerNode || !current->isHTMLElement())
            continue;
        HTMLElement* element = toHTMLElement(current);
        if (element->fastHasAttribute(itempropAttr) && element->itemProp()->length()) {
            return element;
        }
    }

    return 0;
}

void HTMLPropertiesCollection::updateNameCache() const
{
    if (hasNameCache())
        return;

    updateRefElements();

    for (unsigned i = 0; i < m_itemRefElements.size(); ++i) {
        HTMLElement* refElement = m_itemRefElements[i];
        for (HTMLElement* element = virtualItemAfter(refElement, 0); element; element = virtualItemAfter(refElement, element)) {
            DOMSettableTokenList* itemProperty = element->itemProp();
            for (unsigned propertyIndex = 0; propertyIndex < itemProperty->length(); ++propertyIndex)
                updatePropertyCache(itemProperty->item(propertyIndex));
        }
    }

    setHasNameCache();
}

PassRefPtr<DOMStringList> HTMLPropertiesCollection::names() const
{
    updateNameCache();
    if (!m_propertyNames)
        m_propertyNames = DOMStringList::create();
    return m_propertyNames;
}

PassRefPtr<PropertyNodeList> HTMLPropertiesCollection::propertyNodeList(const String& name) const
{
    return ownerNode()->propertyNodeList(name);
}

bool HTMLPropertiesCollection::hasNamedItem(const AtomicString& name) const
{
    updateNameCache();
    if (m_propertyNames)
        return m_propertyNames->contains(name);
    return false;
}

} // namespace WebCore

#endif // ENABLE(MICRODATA)
