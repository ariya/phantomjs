/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "RenderSVGGradientStop.h"

#include "RenderSVGResourceContainer.h"
#include "SVGGradientElement.h"
#include "SVGNames.h"
#include "SVGResourcesCache.h"
#include "SVGStopElement.h"
#include <wtf/StackStats.h>

namespace WebCore {
    
using namespace SVGNames;

RenderSVGGradientStop::RenderSVGGradientStop(SVGStopElement* element)
    : RenderObject(element)
{
}

RenderSVGGradientStop::~RenderSVGGradientStop()
{
}

void RenderSVGGradientStop::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderObject::styleDidChange(diff, oldStyle);
    if (diff == StyleDifferenceEqual)
        return;

    // <stop> elements should only be allowed to make renderers under gradient elements
    // but I can imagine a few cases we might not be catching, so let's not crash if our parent isn't a gradient.
    SVGGradientElement* gradient = gradientElement();
    if (!gradient)
        return;

    RenderObject* renderer = gradient->renderer();
    if (!renderer)
        return;

    ASSERT(renderer->isSVGResourceContainer());
    RenderSVGResourceContainer* container = renderer->toRenderSVGResourceContainer();
    container->removeAllClientsFromCache();
}

void RenderSVGGradientStop::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    setNeedsLayout(false);
}

SVGGradientElement* RenderSVGGradientStop::gradientElement() const
{
    ContainerNode* parentNode = node()->parentNode();
    if (parentNode->hasTagName(linearGradientTag) || parentNode->hasTagName(radialGradientTag))
        return toSVGGradientElement(parentNode);
    return 0;
}

}

#endif // ENABLE(SVG)
