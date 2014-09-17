/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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
#include "RenderSVGContainer.h"

#include "GraphicsContext.h"
#include "RenderSVGResource.h"
#include "RenderSVGResourceFilter.h"
#include "RenderView.h"
#include "SVGRenderSupport.h"
#include "SVGResources.h"
#include "SVGStyledElement.h"

namespace WebCore {

RenderSVGContainer::RenderSVGContainer(SVGStyledElement* node)
    : RenderSVGModelObject(node)
    , m_needsBoundariesUpdate(true)
{
}

RenderSVGContainer::~RenderSVGContainer()
{
}

void RenderSVGContainer::layout()
{
    ASSERT(needsLayout());

    // RenderSVGRoot disables layoutState for the SVG rendering tree.
    ASSERT(!view()->layoutStateEnabled());

    // Allow RenderSVGViewportContainer to update its viewport.
    calcViewport();

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout() || selfWillPaint());

    // Allow RenderSVGTransformableContainer to update its transform.
    bool updatedTransform = calculateLocalTransform();

    SVGRenderSupport::layoutChildren(this, selfNeedsLayout());

    // Invalidate all resources of this client if our layout changed.
    if (m_everHadLayout && selfNeedsLayout())
        SVGResourcesCache::clientLayoutChanged(this);

    // At this point LayoutRepainter already grabbed the old bounds,
    // recalculate them now so repaintAfterLayout() uses the new bounds.
    if (m_needsBoundariesUpdate || updatedTransform) {
        updateCachedBoundaries();
        m_needsBoundariesUpdate = false;
    
        // If our bounds changed, notify the parents.
        RenderSVGModelObject::setNeedsBoundariesUpdate();
    }

    repainter.repaintAfterLayout();
    setNeedsLayout(false);
}

bool RenderSVGContainer::selfWillPaint()
{
#if ENABLE(FILTERS)
    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
    return resources && resources->filter();
#else
    return false;
#endif
}

void RenderSVGContainer::paint(PaintInfo& paintInfo, int, int)
{
    if (paintInfo.context->paintingDisabled())
        return;

    // Spec: groups w/o children still may render filter content.
    if (!firstChild() && !selfWillPaint())
        return;

    FloatRect repaintRect = repaintRectInLocalCoordinates();
    if (!SVGRenderSupport::paintInfoIntersectsRepaintRect(repaintRect, localToParentTransform(), paintInfo))
        return;

    PaintInfo childPaintInfo(paintInfo);
    {
        GraphicsContextStateSaver stateSaver(*childPaintInfo.context);

        // Let the RenderSVGViewportContainer subclass clip if necessary
        applyViewportClip(childPaintInfo);

        childPaintInfo.applyTransform(localToParentTransform());

        bool continueRendering = true;
        if (childPaintInfo.phase == PaintPhaseForeground)
            continueRendering = SVGRenderSupport::prepareToRenderSVGContent(this, childPaintInfo);

        if (continueRendering) {
            childPaintInfo.updatePaintingRootForChildren(this);
            for (RenderObject* child = firstChild(); child; child = child->nextSibling())
                child->paint(childPaintInfo, 0, 0);
        }

        if (paintInfo.phase == PaintPhaseForeground)
            SVGRenderSupport::finishRenderSVGContent(this, childPaintInfo, paintInfo.context);
    }
    
    // FIXME: This really should be drawn from local coordinates, but currently we hack it
    // to avoid our clip killing our outline rect.  Thus we translate our
    // outline rect into parent coords before drawing.
    // FIXME: This means our focus ring won't share our rotation like it should.
    // We should instead disable our clip during PaintPhaseOutline
    if ((paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) && style()->outlineWidth() && style()->visibility() == VISIBLE) {
        IntRect paintRectInParent = enclosingIntRect(localToParentTransform().mapRect(repaintRect));
        paintOutline(paintInfo.context, paintRectInParent.x(), paintRectInParent.y(), paintRectInParent.width(), paintRectInParent.height());
    }
}

// addFocusRingRects is called from paintOutline and needs to be in the same coordinates as the paintOuline call
void RenderSVGContainer::addFocusRingRects(Vector<IntRect>& rects, int, int)
{
    IntRect paintRectInParent = enclosingIntRect(localToParentTransform().mapRect(repaintRectInLocalCoordinates()));
    if (!paintRectInParent.isEmpty())
        rects.append(paintRectInParent);
}

void RenderSVGContainer::updateCachedBoundaries()
{
    m_objectBoundingBox = FloatRect();
    m_strokeBoundingBox = FloatRect();
    m_repaintBoundingBox = FloatRect();

    SVGRenderSupport::computeContainerBoundingBoxes(this, m_objectBoundingBox, m_strokeBoundingBox, m_repaintBoundingBox);
    SVGRenderSupport::intersectRepaintRectWithResources(this, m_repaintBoundingBox);
}

bool RenderSVGContainer::nodeAtFloatPoint(const HitTestRequest& request, HitTestResult& result, const FloatPoint& pointInParent, HitTestAction hitTestAction)
{
    // Give RenderSVGViewportContainer a chance to apply its viewport clip
    if (!pointIsInsideViewportClip(pointInParent))
        return false;

    FloatPoint localPoint = localToParentTransform().inverse().mapPoint(pointInParent);

    if (!SVGRenderSupport::pointInClippingArea(this, localPoint))
        return false;
                
    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        if (child->nodeAtFloatPoint(request, result, localPoint, hitTestAction)) {
            updateHitTestResult(result, roundedIntPoint(localPoint));
            return true;
        }
    }

    // Spec: Only graphical elements can be targeted by the mouse, period.
    // 16.4: "If there are no graphics elements whose relevant graphics content is under the pointer (i.e., there is no target element), the event is not dispatched."
    return false;
}

}

#endif // ENABLE(SVG)
