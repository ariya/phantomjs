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

#include "config.h"
#include "HTMLCollection.h"

#include "HTMLDocument.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "HTMLOptionElement.h"
#include "NodeList.h"

#include <utility>

namespace WebCore {

using namespace HTMLNames;

HTMLCollection::HTMLCollection(PassRefPtr<Node> base, CollectionType type)
    : m_idsDone(false)
    , m_base(base)
    , m_type(type)
    , m_info(m_base->isDocumentNode() ? static_cast<Document*>(m_base.get())->collectionInfo(type) : 0)
    , m_ownsInfo(false)
{
}

HTMLCollection::HTMLCollection(PassRefPtr<Node> base, CollectionType type, CollectionCache* info)
    : m_idsDone(false)
    , m_base(base)
    , m_type(type)
    , m_info(info)
    , m_ownsInfo(false)
{
}

PassRefPtr<HTMLCollection> HTMLCollection::create(PassRefPtr<Node> base, CollectionType type)
{
    return adoptRef(new HTMLCollection(base, type));
}

HTMLCollection::~HTMLCollection()
{
    if (m_ownsInfo)
        delete m_info;
}

void HTMLCollection::resetCollectionInfo() const
{
    uint64_t docversion = static_cast<HTMLDocument*>(m_base->document())->domTreeVersion();

    if (!m_info) {
        m_info = new CollectionCache;
        m_ownsInfo = true;
        m_info->version = docversion;
        return;
    }

    if (m_info->version != docversion) {
        m_info->reset();
        m_info->version = docversion;
    }
}

static Node* nextNodeOrSibling(Node* base, Node* node, bool includeChildren)
{
    return includeChildren ? node->traverseNextNode(base) : node->traverseNextSibling(base);
}

Element* HTMLCollection::itemAfter(Element* previous) const
{
    bool deep = true;

    switch (m_type) {
        case DocAll:
        case DocAnchors:
        case DocApplets:
        case DocEmbeds:
        case DocForms:
        case DocImages:
        case DocLinks:
        case DocObjects:
        case DocScripts:
        case DocumentNamedItems:
        case MapAreas:
        case OtherCollection:
        case SelectOptions:
        case DataListOptions:
        case WindowNamedItems:
            break;
        case NodeChildren:
        case TRCells:
        case TSectionRows:
        case TableTBodies:
            deep = false;
            break;
    }

    Node* current;
    if (!previous)
        current = m_base->firstChild();
    else
        current = nextNodeOrSibling(m_base.get(), previous, deep);

    for (; current; current = nextNodeOrSibling(m_base.get(), current, deep)) {
        if (!current->isElementNode())
            continue;
        Element* e = static_cast<Element*>(current);
        switch (m_type) {
            case DocImages:
                if (e->hasLocalName(imgTag))
                    return e;
                break;
            case DocScripts:
                if (e->hasLocalName(scriptTag))
                    return e;
                break;
            case DocForms:
                if (e->hasLocalName(formTag))
                    return e;
                break;
            case TableTBodies:
                if (e->hasLocalName(tbodyTag))
                    return e;
                break;
            case TRCells:
                if (e->hasLocalName(tdTag) || e->hasLocalName(thTag))
                    return e;
                break;
            case TSectionRows:
                if (e->hasLocalName(trTag))
                    return e;
                break;
            case SelectOptions:
                if (e->hasLocalName(optionTag))
                    return e;
                break;
            case DataListOptions:
                if (e->hasLocalName(optionTag)) {
                    HTMLOptionElement* option = static_cast<HTMLOptionElement*>(e);
                    if (!option->disabled() && !option->value().isEmpty())
                        return e;
                }
                break;
            case MapAreas:
                if (e->hasLocalName(areaTag))
                    return e;
                break;
            case DocApplets: // all <applet> elements and <object> elements that contain Java Applets
                if (e->hasLocalName(appletTag))
                    return e;
                if (e->hasLocalName(objectTag) && static_cast<HTMLObjectElement*>(e)->containsJavaApplet())
                    return e;
                break;
            case DocEmbeds:
                if (e->hasLocalName(embedTag))
                    return e;
                break;
            case DocObjects:
                if (e->hasLocalName(objectTag))
                    return e;
                break;
            case DocLinks: // all <a> and <area> elements with a value for href
                if ((e->hasLocalName(aTag) || e->hasLocalName(areaTag)) && e->fastHasAttribute(hrefAttr))
                    return e;
                break;
            case DocAnchors: // all <a> elements with a value for name
                if (e->hasLocalName(aTag) && e->fastHasAttribute(nameAttr))
                    return e;
                break;
            case DocAll:
            case NodeChildren:
                return e;
            case DocumentNamedItems:
            case OtherCollection:
            case WindowNamedItems:
                ASSERT_NOT_REACHED();
                break;
        }
    }

    return 0;
}

unsigned HTMLCollection::calcLength() const
{
    unsigned len = 0;
    for (Element* current = itemAfter(0); current; current = itemAfter(current))
        ++len;
    return len;
}

// since the collections are to be "live", we have to do the
// calculation every time if anything has changed
unsigned HTMLCollection::length() const
{
    resetCollectionInfo();
    if (!m_info->hasLength) {
        m_info->length = calcLength();
        m_info->hasLength = true;
    }
    return m_info->length;
}

Node* HTMLCollection::item(unsigned index) const
{
     resetCollectionInfo();
     if (m_info->current && m_info->position == index)
         return m_info->current;
     if (m_info->hasLength && m_info->length <= index)
         return 0;
     if (!m_info->current || m_info->position > index) {
         m_info->current = itemAfter(0);
         m_info->position = 0;
         if (!m_info->current)
             return 0;
     }
     Element* e = m_info->current;
     for (unsigned pos = m_info->position; e && pos < index; pos++)
         e = itemAfter(e);
     m_info->current = e;
     m_info->position = index;
     return m_info->current;
}

Node* HTMLCollection::firstItem() const
{
     return item(0);
}

Node* HTMLCollection::nextItem() const
{
     resetCollectionInfo();
 
     // Look for the 'second' item. The first one is currentItem, already given back.
     Element* retval = itemAfter(m_info->current);
     m_info->current = retval;
     m_info->position++;
     return retval;
}

bool HTMLCollection::checkForNameMatch(Element* element, bool checkName, const AtomicString& name) const
{
    if (!element->isHTMLElement())
        return false;
    
    HTMLElement* e = toHTMLElement(element);
    if (!checkName)
        return e->getIdAttribute() == name;

    // document.all returns only images, forms, applets, objects and embeds
    // by name (though everything by id)
    if (m_type == DocAll && 
        !(e->hasLocalName(imgTag) || e->hasLocalName(formTag) ||
          e->hasLocalName(appletTag) || e->hasLocalName(objectTag) ||
          e->hasLocalName(embedTag) || e->hasLocalName(inputTag) ||
          e->hasLocalName(selectTag)))
        return false;

    return e->getAttribute(nameAttr) == name && e->getIdAttribute() != name;
}

Node* HTMLCollection::namedItem(const AtomicString& name) const
{
    // http://msdn.microsoft.com/workshop/author/dhtml/reference/methods/nameditem.asp
    // This method first searches for an object with a matching id
    // attribute. If a match is not found, the method then searches for an
    // object with a matching name attribute, but only on those elements
    // that are allowed a name attribute.
    resetCollectionInfo();
    m_idsDone = false;

    for (Element* e = itemAfter(0); e; e = itemAfter(e)) {
        if (checkForNameMatch(e, m_idsDone, name)) {
            m_info->current = e;
            return e;
        }
    }
        
    m_idsDone = true;

    for (Element* e = itemAfter(0); e; e = itemAfter(e)) {
        if (checkForNameMatch(e, m_idsDone, name)) {
            m_info->current = e;
            return e;
        }
    }

    m_info->current = 0;
    return 0;
}

void HTMLCollection::updateNameCache() const
{
    if (m_info->hasNameCache)
        return;
    
    for (Element* element = itemAfter(0); element; element = itemAfter(element)) {
        if (!element->isHTMLElement())
            continue;
        HTMLElement* e = toHTMLElement(element);
        const AtomicString& idAttrVal = e->getIdAttribute();
        const AtomicString& nameAttrVal = e->getAttribute(nameAttr);
        if (!idAttrVal.isEmpty()) {
            // add to id cache
            Vector<Element*>* idVector = m_info->idCache.get(idAttrVal.impl());
            if (!idVector) {
                idVector = new Vector<Element*>;
                m_info->idCache.add(idAttrVal.impl(), idVector);
            }
            idVector->append(e);
        }
        if (!nameAttrVal.isEmpty() && idAttrVal != nameAttrVal
            && (m_type != DocAll || 
                (e->hasLocalName(imgTag) || e->hasLocalName(formTag) ||
                 e->hasLocalName(appletTag) || e->hasLocalName(objectTag) ||
                 e->hasLocalName(embedTag) || e->hasLocalName(inputTag) ||
                 e->hasLocalName(selectTag)))) {
            // add to name cache
            Vector<Element*>* nameVector = m_info->nameCache.get(nameAttrVal.impl());
            if (!nameVector) {
                nameVector = new Vector<Element*>;
                m_info->nameCache.add(nameAttrVal.impl(), nameVector);
            }
            nameVector->append(e);
        }
    }

    m_info->hasNameCache = true;
}

void HTMLCollection::namedItems(const AtomicString& name, Vector<RefPtr<Node> >& result) const
{
    ASSERT(result.isEmpty());
    
    if (name.isEmpty())
        return;

    resetCollectionInfo();
    updateNameCache();
    m_info->checkConsistency();

    Vector<Element*>* idResults = m_info->idCache.get(name.impl());
    Vector<Element*>* nameResults = m_info->nameCache.get(name.impl());
    
    for (unsigned i = 0; idResults && i < idResults->size(); ++i)
        result.append(idResults->at(i));

    for (unsigned i = 0; nameResults && i < nameResults->size(); ++i)
        result.append(nameResults->at(i));
}


Node* HTMLCollection::nextNamedItem(const AtomicString& name) const
{
    resetCollectionInfo();
    m_info->checkConsistency();

    for (Element* e = itemAfter(m_info->current); e; e = itemAfter(e)) {
        if (checkForNameMatch(e, m_idsDone, name)) {
            m_info->current = e;
            return e;
        }
    }
    
    if (m_idsDone) {
        m_info->current = 0; 
        return 0;
    }
    m_idsDone = true;

    for (Element* e = itemAfter(m_info->current); e; e = itemAfter(e)) {
        if (checkForNameMatch(e, m_idsDone, name)) {
            m_info->current = e;
            return e;
        }
    }

    return 0;
}

PassRefPtr<NodeList> HTMLCollection::tags(const String& name)
{
    return m_base->getElementsByTagName(name);
}

} // namespace WebCore
