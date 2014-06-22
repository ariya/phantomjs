/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
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

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"

#include "Attr.h"
#include "SVGElement.h"
#include "SVGNames.h"

namespace WebCore {

bool SVGExternalResourcesRequired::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == SVGNames::externalResourcesRequiredAttr) {
        setExternalResourcesRequiredBaseValue(value == "true");
        return true;
    }

    return false;
}

bool SVGExternalResourcesRequired::isKnownAttribute(const QualifiedName& attrName)
{
    return attrName == SVGNames::externalResourcesRequiredAttr;
}

void SVGExternalResourcesRequired::addSupportedAttributes(HashSet<QualifiedName>& supportedAttributes)
{
    supportedAttributes.add(SVGNames::externalResourcesRequiredAttr);
}

bool SVGExternalResourcesRequired::handleAttributeChange(SVGElement* targetElement, const QualifiedName& attrName)
{
    ASSERT(targetElement);
    if (!isKnownAttribute(attrName))
        return false;
    if (!targetElement->inDocument())
        return true;

    // Handle dynamic updates of the 'externalResourcesRequired' attribute. Only possible case: changing from 'true' to 'false'
    // causes an immediate dispatch of the SVGLoad event. If the attribute value was 'false' before inserting the script element
    // in the document, the SVGLoad event has already been dispatched.
    if (!externalResourcesRequiredBaseValue() && !haveFiredLoadEvent() && !isParserInserted()) {
        setHaveFiredLoadEvent(true);
        ASSERT(targetElement->haveLoadedRequiredResources());

        targetElement->sendSVGLoadEventIfPossible();
    }

    return true;
}

void SVGExternalResourcesRequired::dispatchLoadEvent(SVGElement* targetElement)
{
    bool externalResourcesRequired = externalResourcesRequiredBaseValue();

    if (isParserInserted())
        ASSERT(externalResourcesRequired != haveFiredLoadEvent());
    else if (haveFiredLoadEvent())
        return;

    // HTML and SVG differ completely in the 'onload' event handling of <script> elements.
    // HTML fires the 'load' event after it sucessfully loaded a remote resource, otherwise an error event.
    // SVG fires the SVGLoad event immediately after parsing the <script> element, if externalResourcesRequired
    // is set to 'false', otherwise it dispatches the 'SVGLoad' event just after loading the remote resource.
    if (!externalResourcesRequired)
        return;

    ASSERT(!haveFiredLoadEvent());

    // Dispatch SVGLoad event
    setHaveFiredLoadEvent(true);
    ASSERT(targetElement->haveLoadedRequiredResources());

    targetElement->sendSVGLoadEventIfPossible();
}

void SVGExternalResourcesRequired::insertedIntoDocument(SVGElement* targetElement)
{
    if (isParserInserted())
        return;

    // Eventually send SVGLoad event now for the dynamically inserted script element.
    if (externalResourcesRequiredBaseValue())
        return;
    setHaveFiredLoadEvent(true);
    targetElement->sendSVGLoadEventIfPossibleAsynchronously();
}

void SVGExternalResourcesRequired::finishParsingChildren()
{
    // A SVGLoad event has been fired by SVGElement::finishParsingChildren.
    if (!externalResourcesRequiredBaseValue())
        setHaveFiredLoadEvent(true);
}

bool SVGExternalResourcesRequired::haveLoadedRequiredResources() const
{
    return !externalResourcesRequiredBaseValue() || haveFiredLoadEvent();
}

}

#endif // ENABLE(SVG)
