/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007, 2008, 2009 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "RenderSVGRoot.h"

#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "RenderSVGContainer.h"
#include "RenderSVGResource.h"
#include "RenderView.h"
#include "SVGLength.h"
#include "SVGRenderSupport.h"
#include "SVGResources.h"
#include "SVGSVGElement.h"
#include "SVGStyledElement.h"
#include "TransformState.h"

#if ENABLE(FILTERS)
#include "RenderSVGResourceFilter.h"
#endif

using namespace std;

namespace WebCore {

RenderSVGRoot::RenderSVGRoot(SVGStyledElement* node)
    : RenderBox(node)
    , m_isLayoutSizeChanged(false)
    , m_needsBoundariesOrTransformUpdate(true)
{
    setReplaced(true);
}

RenderSVGRoot::~RenderSVGRoot()
{
}

void RenderSVGRoot::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    int borderAndPadding = borderAndPaddingWidth();
    int width = computeReplacedLogicalWidth(false) + borderAndPadding;

    if (style()->maxWidth().isFixed() && style()->maxWidth().value() != undefinedLength)
        width = min(width, style()->maxWidth().value() + (style()->boxSizing() == CONTENT_BOX ? borderAndPadding : 0));

    if (style()->width().isPercent() || (style()->width().isAuto() && style()->height().isPercent())) {
        m_minPreferredLogicalWidth = 0;
        m_maxPreferredLogicalWidth = width;
    } else
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = width;

    setPreferredLogicalWidthsDirty(false);
}

int RenderSVGRoot::computeReplacedLogicalWidth(bool includeMaxWidth) const
{
    int replacedWidth = RenderBox::computeReplacedLogicalWidth(includeMaxWidth);
    if (!style()->logicalWidth().isPercent())
        return replacedWidth;

    // FIXME: Investigate in size rounding issues
    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
    return static_cast<int>(roundf(replacedWidth * svg->currentScale()));
}

int RenderSVGRoot::computeReplacedLogicalHeight() const
{
    int replacedHeight = RenderBox::computeReplacedLogicalHeight();
    if (!style()->logicalHeight().isPercent())
        return replacedHeight;

    // FIXME: Investigate in size rounding issues
    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
    return static_cast<int>(roundf(replacedHeight * svg->currentScale()));
}

void RenderSVGRoot::layout()
{
    ASSERT(needsLayout());

    // Arbitrary affine transforms are incompatible with LayoutState.
    view()->disableLayoutState();

    bool needsLayout = selfNeedsLayout();
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout() && needsLayout);

    IntSize oldSize(width(), height());
    computeLogicalWidth();
    computeLogicalHeight();
    calcViewport();

    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
    m_isLayoutSizeChanged = svg->hasRelativeLengths() && oldSize != size();

    SVGRenderSupport::layoutChildren(this, needsLayout);
    m_isLayoutSizeChanged = false;

    // At this point LayoutRepainter already grabbed the old bounds,
    // recalculate them now so repaintAfterLayout() uses the new bounds.
    if (m_needsBoundariesOrTransformUpdate) {
        updateCachedBoundaries();
        m_needsBoundariesOrTransformUpdate = false;
    }

    repainter.repaintAfterLayout();

    view()->enableLayoutState();
    setNeedsLayout(false);
}

bool RenderSVGRoot::selfWillPaint()
{
#if ENABLE(FILTERS)
    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
    return resources && resources->filter();
#else
    return false;
#endif
}

void RenderSVGRoot::paint(PaintInfo& paintInfo, int parentX, int parentY)
{
    if (paintInfo.context->paintingDisabled())
        return;

    bool isVisible = style()->visibility() == VISIBLE;
    IntPoint parentOriginInContainer(parentX, parentY);
    IntPoint borderBoxOriginInContainer = parentOriginInContainer + parentOriginToBorderBox();

    if (hasBoxDecorations() && (paintInfo.phase == PaintPhaseBlockBackground || paintInfo.phase == PaintPhaseChildBlockBackground) && isVisible)
        paintBoxDecorations(paintInfo, borderBoxOriginInContainer.x(), borderBoxOriginInContainer.y());

    if (paintInfo.phase == PaintPhaseBlockBackground)
        return;

    // An empty viewport disables rendering.  FIXME: Should we still render filters?
    if (m_viewportSize.isEmpty())
        return;

    // Don't paint if we don't have kids, except if we have filters we should paint those.
    if (!firstChild() && !selfWillPaint())
        return;

    // Make a copy of the PaintInfo because applyTransform will modify the damage rect.
    PaintInfo childPaintInfo(paintInfo);
    childPaintInfo.context->save();

    // Apply initial viewport clip - not affected by overflow handling
    childPaintInfo.context->clip(overflowClipRect(borderBoxOriginInContainer.x(), borderBoxOriginInContainer.y()));

    // Convert from container offsets (html renderers) to a relative transform (svg renderers).
    // Transform from our paint container's coordinate system to our local coords.
    childPaintInfo.applyTransform(localToRepaintContainerTransform(parentOriginInContainer));

    bool continueRendering = true;
    if (childPaintInfo.phase == PaintPhaseForeground)
        continueRendering = SVGRenderSupport::prepareToRenderSVGContent(this, childPaintInfo);

    if (continueRendering)
        RenderBox::paint(childPaintInfo, 0, 0);

    if (childPaintInfo.phase == PaintPhaseForeground)
        SVGRenderSupport::finishRenderSVGContent(this, childPaintInfo, paintInfo.context);

    childPaintInfo.context->restore();

    if ((paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) && style()->outlineWidth() && isVisible)
        paintOutline(paintInfo.context, borderBoxOriginInContainer.x(), borderBoxOriginInContainer.y(), width(), height());
}

void RenderSVGRoot::destroy()
{
    SVGResourcesCache::clientDestroyed(this);
    RenderBox::destroy();
}

void RenderSVGRoot::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (diff == StyleDifferenceLayout)
        setNeedsBoundariesUpdate();
    RenderBox::styleWillChange(diff, newStyle);
}

void RenderSVGRoot::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBox::styleDidChange(diff, oldStyle);
    SVGResourcesCache::clientStyleChanged(this, diff, style());
}

void RenderSVGRoot::updateFromElement()
{
    RenderBox::updateFromElement();
    SVGResourcesCache::clientUpdatedFromElement(this, style());
}

void RenderSVGRoot::calcViewport()
{
    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());

    if (!svg->hasSetContainerSize()) {
        // In the normal case of <svg> being stand-alone or in a CSSBoxModel object we use
        // RenderBox::width()/height() (which pulls data from RenderStyle)
        m_viewportSize = FloatSize(width(), height());
        return;
    }

    // In the SVGImage case grab the SVGLength values off of SVGSVGElement and use
    // the special relativeWidthValue accessors which respect the specified containerSize
    // FIXME: Check how SVGImage + zooming is supposed to be handled?
    SVGLength width = svg->width();
    SVGLength height = svg->height();
    m_viewportSize = FloatSize(width.unitType() == LengthTypePercentage ? svg->relativeWidthValue() : width.value(svg),
                               height.unitType() == LengthTypePercentage ? svg->relativeHeightValue() : height.value(svg));
}

// RenderBox methods will expect coordinates w/o any transforms in coordinates
// relative to our borderBox origin.  This method gives us exactly that.
AffineTransform RenderSVGRoot::localToBorderBoxTransform() const
{
    IntSize borderAndPadding = borderOriginToContentBox();
    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
    float scale = svg->currentScale();
    FloatPoint translate = svg->currentTranslate();
    AffineTransform ctm(scale, 0, 0, scale, borderAndPadding.width() + translate.x(), borderAndPadding.height() + translate.y());
    return ctm * svg->viewBoxToViewTransform(width() / scale, height() / scale);
}

IntSize RenderSVGRoot::parentOriginToBorderBox() const
{
    return IntSize(x(), y());
}

IntSize RenderSVGRoot::borderOriginToContentBox() const
{
    return IntSize(borderLeft() + paddingLeft(), borderTop() + paddingTop());
}

AffineTransform RenderSVGRoot::localToRepaintContainerTransform(const IntPoint& parentOriginInContainer) const
{
    return AffineTransform::translation(parentOriginInContainer.x(), parentOriginInContainer.y()) * localToParentTransform();
}

const AffineTransform& RenderSVGRoot::localToParentTransform() const
{
    IntSize parentToBorderBoxOffset = parentOriginToBorderBox();

    m_localToParentTransform = AffineTransform::translation(parentToBorderBoxOffset.width(), parentToBorderBoxOffset.height()) * localToBorderBoxTransform();

    return m_localToParentTransform;
}

IntRect RenderSVGRoot::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer)
{
    return SVGRenderSupport::clippedOverflowRectForRepaint(this, repaintContainer);
}

void RenderSVGRoot::computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& repaintRect, bool fixed)
{
    // Apply our local transforms (except for x/y translation), then our shadow, 
    // and then call RenderBox's method to handle all the normal CSS Box model bits
    repaintRect = localToBorderBoxTransform().mapRect(repaintRect);

    // Apply initial viewport clip - not affected by overflow settings    
    repaintRect.intersect(enclosingIntRect(FloatRect(FloatPoint(), m_viewportSize)));

    const SVGRenderStyle* svgStyle = style()->svgStyle();
    if (const ShadowData* shadow = svgStyle->shadow())
        shadow->adjustRectForShadow(repaintRect);

    RenderBox::computeRectForRepaint(repaintContainer, repaintRect, fixed);
}

void RenderSVGRoot::mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool fixed , bool useTransforms, TransformState& transformState) const
{
    ASSERT(!fixed); // We should have no fixed content in the SVG rendering tree.
    ASSERT(useTransforms); // mapping a point through SVG w/o respecting trasnforms is useless.

    // Transform to our border box and let RenderBox transform the rest of the way.
    transformState.applyTransform(localToBorderBoxTransform());
    RenderBox::mapLocalToContainer(repaintContainer, fixed, useTransforms, transformState);
}

void RenderSVGRoot::updateCachedBoundaries()
{
    m_objectBoundingBox = FloatRect();
    m_strokeBoundingBox = FloatRect();
    m_repaintBoundingBox = FloatRect();

    SVGRenderSupport::computeContainerBoundingBoxes(this, m_objectBoundingBox, m_strokeBoundingBox, m_repaintBoundingBox);
    SVGRenderSupport::intersectRepaintRectWithResources(this, m_repaintBoundingBox);
    m_repaintBoundingBox.inflate(borderAndPaddingWidth());
}

bool RenderSVGRoot::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, HitTestAction hitTestAction)
{
    IntPoint pointInContainer(x, y);
    IntSize containerToParentOffset(tx, ty);

    IntPoint pointInParent = pointInContainer - containerToParentOffset;
    IntPoint pointInBorderBox = pointInParent - parentOriginToBorderBox();

    // Note: For now, we're ignoring hits to border and padding for <svg>
    IntPoint pointInContentBox = pointInBorderBox - borderOriginToContentBox();
    if (!contentBoxRect().contains(pointInContentBox))
        return false;

    IntPoint localPoint = localToParentTransform().inverse().mapPoint(pointInParent);

    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        if (child->nodeAtFloatPoint(request, result, localPoint, hitTestAction)) {
            // FIXME: CSS/HTML assumes the local point is relative to the border box, right?
            updateHitTestResult(result, pointInBorderBox);
            // FIXME: nodeAtFloatPoint() doesn't handle rect-based hit tests yet.
            result.addNodeToRectBasedTestResult(child->node(), x, y);
            return true;
        }
    }

    // If we didn't early exit above, we've just hit the container <svg> element. Unlike SVG 1.1, 2nd Edition allows container elements to be hit.
    if (hitTestAction == HitTestBlockBackground && style()->pointerEvents() != PE_NONE) {
        // Only return true here, if the last hit testing phase 'BlockBackground' is executed. If we'd return true in the 'Foreground' phase,
        // hit testing would stop immediately. For SVG only trees this doesn't matter. Though when we have a <foreignObject> subtree we need
        // to be able to detect hits on the background of a <div> element. If we'd return true here in the 'Foreground' phase, we are not able 
        // to detect these hits anymore.
        updateHitTestResult(result, roundedIntPoint(localPoint));
        return true;
    }

    return false;
}

}

#endif // ENABLE(SVG)
