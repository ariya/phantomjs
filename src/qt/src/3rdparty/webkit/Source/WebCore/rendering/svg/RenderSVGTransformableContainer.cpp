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

#include "SVGNames.h"
#include "SVGShadowTreeElements.h"
#include "SVGStyledTransformableElement.h"

namespace WebCore {
    
RenderSVGTransformableContainer::RenderSVGTransformableContainer(SVGStyledTransformableElement* node)
    : RenderSVGContainer(node)
    , m_needsTransformUpdate(true)
{
}

bool RenderSVGTransformableContainer::calculateLocalTransform()
{
    SVGStyledTransformableElement* element = static_cast<SVGStyledTransformableElement*>(node());

    bool needsUpdate = m_needsTransformUpdate;
    if (needsUpdate) {
        m_localTransform = element->animatedLocalTransform();
        m_needsTransformUpdate = false;
    }

    if (!element->hasTagName(SVGNames::gTag) || !static_cast<SVGGElement*>(element)->isShadowTreeContainerElement())
        return needsUpdate;

    FloatSize translation = static_cast<SVGShadowTreeContainerElement*>(element)->containerTranslation();
    if (!translation.width() && !translation.height())
        return needsUpdate;

    // FIXME: Could optimize this case for use to avoid refetching the animatedLocalTransform() here, if only the containerTranslation() changed.
    if (!needsUpdate)
        m_localTransform = element->animatedLocalTransform();

    m_localTransform.translate(translation.width(), translation.height());
    return true;
}

}

#endif // ENABLE(SVG)
