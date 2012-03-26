/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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
#include "HTMLMapElement.h"

#include "Attribute.h"
#include "Document.h"
#include "HTMLAreaElement.h"
#include "HTMLCollection.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "IntSize.h"
#include "RenderObject.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

HTMLMapElement::HTMLMapElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(mapTag));
}

PassRefPtr<HTMLMapElement> HTMLMapElement::create(Document* document)
{
    return adoptRef(new HTMLMapElement(mapTag, document));
}

PassRefPtr<HTMLMapElement> HTMLMapElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLMapElement(tagName, document));
}

HTMLMapElement::~HTMLMapElement()
{
}

bool HTMLMapElement::mapMouseEvent(int x, int y, const IntSize& size, HitTestResult& result)
{
    HTMLAreaElement* defaultArea = 0;
    Node *node = this;
    while ((node = node->traverseNextNode(this))) {
        if (node->hasTagName(areaTag)) {
            HTMLAreaElement* areaElt = static_cast<HTMLAreaElement*>(node);
            if (areaElt->isDefault()) {
                if (!defaultArea)
                    defaultArea = areaElt;
            } else if (areaElt->mapMouseEvent(x, y, size, result))
                return true;
        }
    }
    
    if (defaultArea) {
        result.setInnerNode(defaultArea);
        result.setURLElement(defaultArea);
    }
    return defaultArea;
}

HTMLImageElement* HTMLMapElement::imageElement() const
{
    RefPtr<HTMLCollection> coll = document()->images();
    for (Node* curr = coll->firstItem(); curr; curr = coll->nextItem()) {
        if (!curr->hasTagName(imgTag))
            continue;
        
        // The HTMLImageElement's useMap() value includes the '#' symbol at the beginning,
        // which has to be stripped off.
        HTMLImageElement* imageElement = static_cast<HTMLImageElement*>(curr);
        String useMapName = imageElement->getAttribute(usemapAttr).string().substring(1);
        if (equalIgnoringCase(useMapName, m_name))
            return imageElement;
    }
    
    return 0;    
}

void HTMLMapElement::parseMappedAttribute(Attribute* attribute)
{
    // FIXME: This logic seems wrong for XML documents.
    // Either the id or name will be used depending on the order the attributes are parsed.

    const QualifiedName& attrName = attribute->name();
    if (isIdAttributeName(attrName) || attrName == nameAttr) {
        if (isIdAttributeName(attrName)) {
            // Call base class so that hasID bit gets set.
            HTMLElement::parseMappedAttribute(attribute);
            if (document()->isHTMLDocument())
                return;
        }
        if (inDocument())
            treeScope()->removeImageMap(this);
        String mapName = attribute->value();
        if (mapName[0] == '#')
            mapName = mapName.substring(1);
        m_name = document()->isHTMLDocument() ? mapName.lower() : mapName;
        if (inDocument())
            treeScope()->addImageMap(this);
        return;
    }

    HTMLElement::parseMappedAttribute(attribute);
}

PassRefPtr<HTMLCollection> HTMLMapElement::areas()
{
    return HTMLCollection::create(this, MapAreas);
}

void HTMLMapElement::insertedIntoDocument()
{
    treeScope()->addImageMap(this);
    HTMLElement::insertedIntoDocument();
}

void HTMLMapElement::removedFromDocument()
{
    treeScope()->removeImageMap(this);
    HTMLElement::removedFromDocument();
}

}
