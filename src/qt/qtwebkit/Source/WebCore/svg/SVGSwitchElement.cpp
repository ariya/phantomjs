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
#include "SVGSwitchElement.h"

#include "NodeRenderingContext.h"
#include "RenderSVGTransformableContainer.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_BOOLEAN(SVGSwitchElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGSwitchElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGSwitchElement::SVGSwitchElement(const QualifiedName& tagName, Document* document)
    : SVGGraphicsElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::switchTag));
    registerAnimatedPropertiesForSVGSwitchElement();
}

PassRefPtr<SVGSwitchElement> SVGSwitchElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGSwitchElement(tagName, document));
}

bool SVGSwitchElement::childShouldCreateRenderer(const NodeRenderingContext& childContext) const
{
    // FIXME: This function does not do what the comment below implies it does.
    // It will create a renderer for any valid SVG element children, not just the first one.
    for (Node* node = firstChild(); node; node = node->nextSibling()) {
        if (!node->isSVGElement())
            continue;

        SVGElement* element = toSVGElement(node);
        if (!element || !element->isValid())
            continue;

        return node == childContext.node(); // Only allow this child if it's the first valid child
    }

    return false;
}

RenderObject* SVGSwitchElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGTransformableContainer(this);
}

}

#endif // ENABLE(SVG)
