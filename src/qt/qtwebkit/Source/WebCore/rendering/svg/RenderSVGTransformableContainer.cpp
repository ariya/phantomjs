/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.
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
#include "RenderSVGTransformableContainer.h"

#include "SVGGraphicsElement.h"
#include "SVGNames.h"
#include "SVGRenderSupport.h"
#include "SVGUseElement.h"

namespace WebCore {
    
RenderSVGTransformableContainer::RenderSVGTransformableContainer(SVGGraphicsElement* node)
    : RenderSVGContainer(node)
    , m_needsTransformUpdate(true)
    , m_didTransformToRootUpdate(false)
{
}

bool RenderSVGTransformableContainer::calculateLocalTransform()
{
    SVGGraphicsElement* element = toSVGGraphicsElement(node());

    // If we're either the renderer for a <use> element, or for any <g> element inside the shadow
    // tree, that was created during the use/symbol/svg expansion in SVGUseElement. These containers
    // need to respect the translations induced by their corresponding use elements x/y attributes.
    SVGUseElement* useElement = 0;
    if (element->hasTagName(SVGNames::useTag))
        useElement = toSVGUseElement(element);
    else if (element->isInShadowTree() && element->hasTagName(SVGNames::gTag)) {
        SVGElement* correspondingElement = element->correspondingElement();
        if (correspondingElement && correspondingElement->hasTagName(SVGNames::useTag))
            useElement = toSVGUseElement(correspondingElement);
    }

    if (useElement) {
        SVGLengthContext lengthContext(useElement);
        FloatSize translation(useElement->x().value(lengthContext), useElement->y().value(lengthContext));
        if (translation != m_lastTranslation)
            m_needsTransformUpdate = true;
        m_lastTranslation = translation;
    }

    m_didTransformToRootUpdate = m_needsTransformUpdate || SVGRenderSupport::transformToRootChanged(parent());
    if (!m_needsTransformUpdate)
        return false;

    m_localTransform = element->animatedLocalTransform();
    m_localTransform.translate(m_lastTranslation.width(), m_lastTranslation.height());
    m_needsTransformUpdate = false;
    return true;
}

}

#endif // ENABLE(SVG)
