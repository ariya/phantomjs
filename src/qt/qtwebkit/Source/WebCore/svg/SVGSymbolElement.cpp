/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "SVGSymbolElement.h"

#include "RenderSVGHiddenContainer.h"
#include "SVGElementInstance.h"
#include "SVGFitToViewBox.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_BOOLEAN(SVGSymbolElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGSymbolElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio) 
DEFINE_ANIMATED_RECT(SVGSymbolElement, SVGNames::viewBoxAttr, ViewBox, viewBox)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGSymbolElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_LOCAL_ANIMATED_PROPERTY(viewBox)
    REGISTER_LOCAL_ANIMATED_PROPERTY(preserveAspectRatio) 
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGStyledElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGSymbolElement::SVGSymbolElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::symbolTag));
    registerAnimatedPropertiesForSVGSymbolElement();
}

PassRefPtr<SVGSymbolElement> SVGSymbolElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGSymbolElement(tagName, document));
}

bool SVGSymbolElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        SVGFitToViewBox::addSupportedAttributes(supportedAttributes);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGSymbolElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGStyledElement::parseAttribute(name, value);
        return;
    }

    if (SVGLangSpace::parseAttribute(name, value))
        return;
    if (SVGExternalResourcesRequired::parseAttribute(name, value))
        return;
    if (SVGFitToViewBox::parseAttribute(this, name, value))
        return;

    ASSERT_NOT_REACHED();
}

void SVGSymbolElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGStyledElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    // Every other property change is ignored.
    if (attrName == SVGNames::viewBoxAttr)
        updateRelativeLengthsInformation();
}

bool SVGSymbolElement::selfHasRelativeLengths() const
{
    return hasAttribute(SVGNames::viewBoxAttr);
}

RenderObject* SVGSymbolElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGHiddenContainer(this);
}

}

#endif // ENABLE(SVG)
