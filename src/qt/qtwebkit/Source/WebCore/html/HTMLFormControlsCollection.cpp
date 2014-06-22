/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2010, 2011, 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "HTMLFormControlsCollection.h"

#include "HTMLFieldSetElement.h"
#include "HTMLFormControlElement.h"
#include "HTMLFormElement.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

// Since the collections are to be "live", we have to do the
// calculation every time if anything has changed.

HTMLFormControlsCollection::HTMLFormControlsCollection(Node* ownerNode)
    : HTMLCollection(ownerNode, FormControls, OverridesItemAfter)
{
    ASSERT(isHTMLFormElement(ownerNode) || ownerNode->hasTagName(fieldsetTag));
}

PassRefPtr<HTMLFormControlsCollection> HTMLFormControlsCollection::create(Node* ownerNode, CollectionType)
{
    return adoptRef(new HTMLFormControlsCollection(ownerNode));
}

HTMLFormControlsCollection::~HTMLFormControlsCollection()
{
}

const Vector<FormAssociatedElement*>& HTMLFormControlsCollection::formControlElements() const
{
    ASSERT(ownerNode());
    ASSERT(isHTMLFormElement(ownerNode()) || ownerNode()->hasTagName(fieldsetTag));
    if (isHTMLFormElement(ownerNode()))
        return toHTMLFormElement(ownerNode())->associatedElements();
    return static_cast<HTMLFieldSetElement*>(ownerNode())->associatedElements();
}

const Vector<HTMLImageElement*>& HTMLFormControlsCollection::formImageElements() const
{
    ASSERT(ownerNode());
    ASSERT(isHTMLFormElement(ownerNode()));
    return toHTMLFormElement(ownerNode())->imageElements();
}

Element* HTMLFormControlsCollection::virtualItemAfter(unsigned& offset, Element* previousItem) const
{
    const Vector<FormAssociatedElement*>& elementsArray = formControlElements();
    if (previousItem)
        offset++;
    while (offset < elementsArray.size()) {
        FormAssociatedElement* element = elementsArray[offset];
        if (element->isEnumeratable())
            return toHTMLElement(element);
        offset++;
    }
    return 0;
}

static HTMLElement* firstNamedItem(const Vector<FormAssociatedElement*>& elementsArray,
    const Vector<HTMLImageElement*>* imageElementsArray, const QualifiedName& attrName, const String& name)
{
    ASSERT(attrName == idAttr || attrName == nameAttr);

    for (unsigned i = 0; i < elementsArray.size(); ++i) {
        HTMLElement* element = toHTMLElement(elementsArray[i]);
        if (elementsArray[i]->isEnumeratable() && element->fastGetAttribute(attrName) == name)
            return element;
    }

    if (!imageElementsArray)
        return 0;

    for (unsigned i = 0; i < imageElementsArray->size(); ++i) {
        HTMLImageElement* element = (*imageElementsArray)[i];
        if (element->fastGetAttribute(attrName) == name)
            return element;
    }

    return 0;
}

Node* HTMLFormControlsCollection::namedItem(const AtomicString& name) const
{
    // http://msdn.microsoft.com/workshop/author/dhtml/reference/methods/nameditem.asp
    // This method first searches for an object with a matching id
    // attribute. If a match is not found, the method then searches for an
    // object with a matching name attribute, but only on those elements
    // that are allowed a name attribute.
    const Vector<HTMLImageElement*>* imagesElements = ownerNode()->hasTagName(fieldsetTag) ? 0 : &formImageElements();
    if (HTMLElement* item = firstNamedItem(formControlElements(), imagesElements, idAttr, name))
        return item;

    return firstNamedItem(formControlElements(), imagesElements, nameAttr, name);
}

void HTMLFormControlsCollection::updateNameCache() const
{
    if (hasNameCache())
        return;

    HashSet<AtomicStringImpl*> foundInputElements;

    const Vector<FormAssociatedElement*>& elementsArray = formControlElements();

    for (unsigned i = 0; i < elementsArray.size(); ++i) {
        FormAssociatedElement* associatedElement = elementsArray[i];
        if (associatedElement->isEnumeratable()) {
            HTMLElement* element = toHTMLElement(associatedElement);
            const AtomicString& idAttrVal = element->getIdAttribute();
            const AtomicString& nameAttrVal = element->getNameAttribute();
            if (!idAttrVal.isEmpty()) {
                appendIdCache(idAttrVal, element);
                foundInputElements.add(idAttrVal.impl());
            }
            if (!nameAttrVal.isEmpty() && idAttrVal != nameAttrVal) {
                appendNameCache(nameAttrVal, element);
                foundInputElements.add(nameAttrVal.impl());
            }
        }
    }

    if (isHTMLFormElement(ownerNode())) {
        const Vector<HTMLImageElement*>& imageElementsArray = formImageElements();
        for (unsigned i = 0; i < imageElementsArray.size(); ++i) {
            HTMLImageElement* element = imageElementsArray[i];
            const AtomicString& idAttrVal = element->getIdAttribute();
            const AtomicString& nameAttrVal = element->getNameAttribute();
            if (!idAttrVal.isEmpty() && !foundInputElements.contains(idAttrVal.impl()))
                appendIdCache(idAttrVal, element);
            if (!nameAttrVal.isEmpty() && idAttrVal != nameAttrVal && !foundInputElements.contains(nameAttrVal.impl()))
                appendNameCache(nameAttrVal, element);
        }
    }

    setHasNameCache();
}

}
