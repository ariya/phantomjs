/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2009 Google, Inc.
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

#if ENABLE(SVG) && ENABLE(SVG_FOREIGN_OBJECT)
#include "RenderSVGForeignObject.h"

#include "GraphicsContext.h"
#include "RenderSVGResource.h"
#include "RenderView.h"
#include "SVGForeignObjectElement.h"
#include "SVGRenderSupport.h"
#include "SVGSVGElement.h"
#include "TransformState.h"

namespace WebCore {

RenderSVGForeignObject::RenderSVGForeignObject(SVGForeignObjectElement* node) 
    : RenderSVGBlock(node)
    , m_needsTransformUpdate(true)
{
}

RenderSVGForeignObject::~RenderSVGForeignObject()
{
}

void RenderSVGForeignObject::paint(PaintInfo& paintInfo, int, int)
{
    if (paintInfo.context->paintingDisabled())
        return;

    PaintInfo childPaintInfo(paintInfo);
    GraphicsContextStateSaver stateSaver(*childPaintInfo.context);
    childPaintInfo.applyTransform(localTransform());

    if (SVGRenderSupport::isOverflowHidden(this))
        childPaintInfo.context->clip(m_viewport);

    float opacity = style()->opacity();
    if (opacity < 1.0f)
        childPaintInfo.context->beginTransparencyLayer(opacity);

    RenderBlock::paint(childPaintInfo, 0, 0);

    if (opacity < 1.0f)
        childPaintInfo.context->endTransparencyLayer();
}

IntRect RenderSVGForeignObject::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer)
{
    return SVGRenderSupport::clippedOverflowRectForRepaint(this, repaintContainer);
}

void RenderSVGForeignObject::computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& repaintRect, bool fixed)
{
    SVGRenderSupport::computeRectForRepaint(this, repaintContainer, repaintRect, fixed);
}

const AffineTransform& RenderSVGForeignObject::localToParentTransform() const
{
    m_localToParentTransform = localTransform();
    m_localToParentTransform.translate(m_viewport.x(), m_viewport.y());
    return m_localToParentTransform;
}

void RenderSVGForeignObject::computeLogicalWidth()
{
    // FIXME: Investigate in size rounding issues
    setWidth(static_cast<int>(roundf(m_viewport.width())));
}

void RenderSVGForeignObject::computeLogicalHeight()
{
    // FIXME: Investigate in size rounding issues
    setHeight(static_cast<int>(roundf(m_viewport.height())));
}

void RenderSVGForeignObject::layout()
{
    ASSERT(needsLayout());
    ASSERT(!view()->layoutStateEnabled()); // RenderSVGRoot disables layoutState for the SVG rendering tree.

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    SVGForeignObjectElement* foreign = static_cast<SVGForeignObjectElement*>(node());

    bool updateCachedBoundariesInParents = false;
    if (m_needsTransformUpdate) {
        m_localTransform = foreign->animatedLocalTransform();
        m_needsTransformUpdate = false;
        updateCachedBoundariesInParents = true;
    }

    FloatRect oldViewport = m_viewport;

    // Cache viewport boundaries
    FloatPoint viewportLocation(foreign->x().value(foreign), foreign->y().value(foreign));
    m_viewport = FloatRect(viewportLocation, FloatSize(foreign->width().value(foreign), foreign->height().value(foreign)));
    if (!updateCachedBoundariesInParents)
        updateCachedBoundariesInParents = oldViewport != m_viewport;

    // Set box origin to the foreignObject x/y translation, so positioned objects in XHTML content get correct
    // positions. A regular RenderBoxModelObject would pull this information from RenderStyle - in SVG those
    // properties are ignored for non <svg> elements, so we mimic what happens when specifying them through CSS.

    // FIXME: Investigate in location rounding issues - only affects RenderSVGForeignObject & RenderSVGText
    setLocation(roundedIntPoint(viewportLocation));

    bool layoutChanged = m_everHadLayout && selfNeedsLayout();
    RenderBlock::layout();
    ASSERT(!needsLayout());

    // If our bounds changed, notify the parents.
    if (updateCachedBoundariesInParents)
        RenderSVGBlock::setNeedsBoundariesUpdate();

    // Invalidate all resources of this client if our layout changed.
    if (layoutChanged)
        SVGResourcesCache::clientLayoutChanged(this);

    repainter.repaintAfterLayout();
}

bool RenderSVGForeignObject::nodeAtFloatPoint(const HitTestRequest& request, HitTestResult& result, const FloatPoint& pointInParent, HitTestAction hitTestAction)
{
    FloatPoint localPoint = localTransform().inverse().mapPoint(pointInParent);

    // Early exit if local point is not contained in clipped viewport area
    if (SVGRenderSupport::isOverflowHidden(this) && !m_viewport.contains(localPoint))
        return false;

    IntPoint roundedLocalPoint = roundedIntPoint(localPoint);
    return RenderBlock::nodeAtPoint(request, result, roundedLocalPoint.x(), roundedLocalPoint.y(), 0, 0, hitTestAction);
}

bool RenderSVGForeignObject::nodeAtPoint(const HitTestRequest&, HitTestResult&, int, int, int, int, HitTestAction)
{
    ASSERT_NOT_REACHED();
    return false;
}

void RenderSVGForeignObject::mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool fixed, bool useTransforms, TransformState& transformState) const
{
    // When crawling up the hierachy starting from foreignObject child content, useTransforms may not be set to true.
    if (!useTransforms)
        useTransforms = true;
    SVGRenderSupport::mapLocalToContainer(this, repaintContainer, fixed, useTransforms, transformState);
}

}

#endif
