/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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
#include "HTMLFormCollection.h"

#include "CollectionCache.h"
#include "HTMLFormControlElement.h"
#include "HTMLFormElement.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

// Since the collections are to be "live", we have to do the
// calculation every time if anything has changed.

inline CollectionCache* HTMLFormCollection::formCollectionInfo(HTMLFormElement* form)
{
    if (!form->m_collectionCache)
        form->m_collectionCache = adoptPtr(new CollectionCache);
    return form->m_collectionCache.get();
}

HTMLFormCollection::HTMLFormCollection(PassRefPtr<HTMLFormElement> form)
    : HTMLCollection(form.get(), OtherCollection, formCollectionInfo(form.get()))
{
}

PassRefPtr<HTMLFormCollection> HTMLFormCollection::create(PassRefPtr<HTMLFormElement> form)
{
    return adoptRef(new HTMLFormCollection(form));
}

HTMLFormCollection::~HTMLFormCollection()
{
}

unsigned HTMLFormCollection::calcLength() const
{
    return static_cast<HTMLFormElement*>(base())->length();
}

Node* HTMLFormCollection::item(unsigned index) const
{
    resetCollectionInfo();

    if (info()->current && info()->position == index)
        return info()->current;

    if (info()->hasLength && info()->length <= index)
        return 0;

    if (!info()->current || info()->position > index) {
        info()->current = 0;
        info()->position = 0;
        info()->elementsArrayPosition = 0;
    }

    Vector<FormAssociatedElement*>& elementsArray = static_cast<HTMLFormElement*>(base())->m_associatedElements;
    unsigned currentIndex = info()->position;
    
    for (unsigned i = info()->elementsArrayPosition; i < elementsArray.size(); i++) {
        if (elementsArray[i]->isEnumeratable()) {
            HTMLElement* element = toHTMLElement(elementsArray[i]);
            if (index == currentIndex) {
                info()->position = index;
                info()->current = element;
                info()->elementsArrayPosition = i;
                return element;
            }

            currentIndex++;
        }
    }

    return 0;
}

Element* HTMLFormCollection::getNamedItem(const QualifiedName& attrName, const AtomicString& name) const
{
    info()->position = 0;
    return getNamedFormItem(attrName, name, 0);
}

Element* HTMLFormCollection::getNamedFormItem(const QualifiedName& attrName, const String& name, int duplicateNumber) const
{
    HTMLFormElement* form = static_cast<HTMLFormElement*>(base());

    bool foundInputElements = false;
    for (unsigned i = 0; i < form->m_associatedElements.size(); ++i) {
        FormAssociatedElement* associatedElement = form->m_associatedElements[i];
        HTMLElement* element = toHTMLElement(associatedElement);
        if (associatedElement->isEnumeratable() && element->getAttribute(attrName) == name) {
            foundInputElements = true;
            if (!duplicateNumber)
                return element;
            --duplicateNumber;
        }
    }

    if (!foundInputElements) {
        for (unsigned i = 0; i < form->m_imageElements.size(); ++i) {
            HTMLImageElement* element = form->m_imageElements[i];
            if (element->getAttribute(attrName) == name) {
                if (!duplicateNumber)
                    return element;
                --duplicateNumber;
            }
        }
    }

    return 0;
}

Node* HTMLFormCollection::nextItem() const
{
    return item(info()->position + 1);
}

Element* HTMLFormCollection::nextNamedItemInternal(const String &name) const
{
    Element* retval = getNamedFormItem(m_idsDone ? nameAttr : idAttr, name, ++info()->position);
    if (retval)
        return retval;
    if (m_idsDone) // we're done
        return 0;
    // After doing id, do name
    m_idsDone = true;
    return getNamedItem(nameAttr, name);
}

Node* HTMLFormCollection::namedItem(const AtomicString& name) const
{
    // http://msdn.microsoft.com/workshop/author/dhtml/reference/methods/nameditem.asp
    // This method first searches for an object with a matching id
    // attribute. If a match is not found, the method then searches for an
    // object with a matching name attribute, but only on those elements
    // that are allowed a name attribute.
    resetCollectionInfo();
    m_idsDone = false;
    info()->current = getNamedItem(idAttr, name);
    if (info()->current)
        return info()->current;
    m_idsDone = true;
    info()->current = getNamedItem(nameAttr, name);
    return info()->current;
}

Node* HTMLFormCollection::nextNamedItem(const AtomicString& name) const
{
    // The nextNamedItemInternal function can return the same item twice if it has
    // both an id and name that are equal to the name parameter. So this function
    // checks if we are on the nameAttr half of the iteration and skips over any
    // that also have the same idAttributeName.
    Element* impl = nextNamedItemInternal(name);
    if (m_idsDone) {
        while (impl && impl->getIdAttribute() == name)
            impl = nextNamedItemInternal(name);
    }
    return impl;
}

void HTMLFormCollection::updateNameCache() const
{
    if (info()->hasNameCache)
        return;

    HashSet<AtomicStringImpl*> foundInputElements;

    HTMLFormElement* f = static_cast<HTMLFormElement*>(base());

    for (unsigned i = 0; i < f->m_associatedElements.size(); ++i) {
        FormAssociatedElement* associatedElement = f->m_associatedElements[i];
        if (associatedElement->isEnumeratable()) {
            HTMLElement* element = toHTMLElement(associatedElement);
            const AtomicString& idAttrVal = element->getIdAttribute();
            const AtomicString& nameAttrVal = element->getAttribute(nameAttr);
            if (!idAttrVal.isEmpty()) {
                // add to id cache
                Vector<Element*>* idVector = info()->idCache.get(idAttrVal.impl());
                if (!idVector) {
                    idVector = new Vector<Element*>;
                    info()->idCache.add(idAttrVal.impl(), idVector);
                }
                idVector->append(element);
                foundInputElements.add(idAttrVal.impl());
            }
            if (!nameAttrVal.isEmpty() && idAttrVal != nameAttrVal) {
                // add to name cache
                Vector<Element*>* nameVector = info()->nameCache.get(nameAttrVal.impl());
                if (!nameVector) {
                    nameVector = new Vector<Element*>;
                    info()->nameCache.add(nameAttrVal.impl(), nameVector);
                }
                nameVector->append(element);
                foundInputElements.add(nameAttrVal.impl());
            }
        }
    }

    for (unsigned i = 0; i < f->m_imageElements.size(); ++i) {
        HTMLImageElement* element = f->m_imageElements[i];
        const AtomicString& idAttrVal = element->getIdAttribute();
        const AtomicString& nameAttrVal = element->getAttribute(nameAttr);
        if (!idAttrVal.isEmpty() && !foundInputElements.contains(idAttrVal.impl())) {
            // add to id cache
            Vector<Element*>* idVector = info()->idCache.get(idAttrVal.impl());
            if (!idVector) {
                idVector = new Vector<Element*>;
                info()->idCache.add(idAttrVal.impl(), idVector);
            }
            idVector->append(element);
        }
        if (!nameAttrVal.isEmpty() && idAttrVal != nameAttrVal && !foundInputElements.contains(nameAttrVal.impl())) {
            // add to name cache
            Vector<Element*>* nameVector = info()->nameCache.get(nameAttrVal.impl());
            if (!nameVector) {
                nameVector = new Vector<Element*>;
                info()->nameCache.add(nameAttrVal.impl(), nameVector);
            }
            nameVector->append(element);
        }
    }

    info()->hasNameCache = true;
}

}
