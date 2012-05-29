/*
 * Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "RenderSVGInline.h"

#include "RenderSVGResource.h"
#include "RenderSVGText.h"
#include "SVGInlineFlowBox.h"

namespace WebCore {
    
RenderSVGInline::RenderSVGInline(Node* n)
    : RenderInline(n)
{
    setAlwaysCreateLineBoxes();
}

InlineFlowBox* RenderSVGInline::createInlineFlowBox()
{
    InlineFlowBox* box = new (renderArena()) SVGInlineFlowBox(this);
    box->setHasVirtualLogicalHeight();
    return box;
}

FloatRect RenderSVGInline::objectBoundingBox() const
{
    if (const RenderObject* object = RenderSVGText::locateRenderSVGTextAncestor(this))
        return object->objectBoundingBox();

    return FloatRect();
}

FloatRect RenderSVGInline::strokeBoundingBox() const
{
    if (const RenderObject* object = RenderSVGText::locateRenderSVGTextAncestor(this))
        return object->strokeBoundingBox();

    return FloatRect();
}

FloatRect RenderSVGInline::repaintRectInLocalCoordinates() const
{
    if (const RenderObject* object = RenderSVGText::locateRenderSVGTextAncestor(this))
        return object->repaintRectInLocalCoordinates();

    return FloatRect();
}

IntRect RenderSVGInline::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer)
{
    return SVGRenderSupport::clippedOverflowRectForRepaint(this, repaintContainer);
}

void RenderSVGInline::computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& repaintRect, bool fixed)
{
    SVGRenderSupport::computeRectForRepaint(this, repaintContainer, repaintRect, fixed);
}

void RenderSVGInline::mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState& transformState) const
{
    SVGRenderSupport::mapLocalToContainer(this, repaintContainer, useTransforms, fixed, transformState);
}

void RenderSVGInline::absoluteQuads(Vector<FloatQuad>& quads)
{
    RenderObject* object = RenderSVGText::locateRenderSVGTextAncestor(this);
    if (!object)
        return;

    FloatRect textBoundingBox = object->strokeBoundingBox();
    for (InlineFlowBox* box = firstLineBox(); box; box = box->nextLineBox())
        quads.append(localToAbsoluteQuad(FloatRect(textBoundingBox.x() + box->x(), textBoundingBox.y() + box->y(), box->logicalWidth(), box->logicalHeight())));
}

void RenderSVGInline::destroy()
{
    if (RenderSVGText* textRenderer = RenderSVGText::locateRenderSVGTextAncestor(this))
        textRenderer->setNeedsPositioningValuesUpdate();

    SVGResourcesCache::clientDestroyed(this);
    RenderInline::destroy();
}

void RenderSVGInline::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (diff == StyleDifferenceLayout)
        setNeedsBoundariesUpdate();
    RenderInline::styleWillChange(diff, newStyle);
}

void RenderSVGInline::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderInline::styleDidChange(diff, oldStyle);
    SVGResourcesCache::clientStyleChanged(this, diff, style());
}

void RenderSVGInline::updateFromElement()
{
    RenderInline::updateFromElement();
    SVGResourcesCache::clientUpdatedFromElement(this, style());
}


}

#endif // ENABLE(SVG)
