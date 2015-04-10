/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007, 2008, 2009 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#include "Chrome.h"
#include "ChromeClient.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "LayoutRepainter.h"
#include "Page.h"
#include "RenderPart.h"
#include "RenderSVGContainer.h"
#include "RenderSVGResource.h"
#include "RenderSVGResourceContainer.h"
#include "RenderView.h"
#include "SVGImage.h"
#include "SVGLength.h"
#include "SVGRenderingContext.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "SVGSVGElement.h"
#include "SVGStyledElement.h"
#include "SVGViewSpec.h"
#include "TransformState.h"
#include <wtf/StackStats.h>

#if ENABLE(FILTERS)
#include "RenderSVGResourceFilter.h"
#endif

using namespace std;

namespace WebCore {

RenderSVGRoot::RenderSVGRoot(SVGStyledElement* node)
    : RenderReplaced(node)
    , m_objectBoundingBoxValid(false)
    , m_isLayoutSizeChanged(false)
    , m_needsBoundariesOrTransformUpdate(true)
    , m_hasSVGShadow(false)
{
}

RenderSVGRoot::~RenderSVGRoot()
{
}

void RenderSVGRoot::computeIntrinsicRatioInformation(FloatSize& intrinsicSize, double& intrinsicRatio, bool& isPercentageIntrinsicSize) const
{
    // Spec: http://www.w3.org/TR/SVG/coords.html#IntrinsicSizing
    // SVG needs to specify how to calculate some intrinsic sizing properties to enable inclusion within other languages.
    // The intrinsic width and height of the viewport of SVG content must be determined from the ‘width’ and ‘height’ attributes.
    // If either of these are not specified, a value of '100%' must be assumed. Note: the ‘width’ and ‘height’ attributes are not
    // the same as the CSS width and height properties. Specifically, percentage values do not provide an intrinsic width or height,
    // and do not indicate a percentage of the containing block. Rather, once the viewport is established, they indicate the portion
    // of the viewport that is actually covered by image data.
    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);
    Length intrinsicWidthAttribute = svg->intrinsicWidth(SVGSVGElement::IgnoreCSSProperties);
    Length intrinsicHeightAttribute = svg->intrinsicHeight(SVGSVGElement::IgnoreCSSProperties);

    // The intrinsic aspect ratio of the viewport of SVG content is necessary for example, when including SVG from an ‘object’
    // element in HTML styled with CSS. It is possible (indeed, common) for an SVG graphic to have an intrinsic aspect ratio but
    // not to have an intrinsic width or height. The intrinsic aspect ratio must be calculated based upon the following rules:
    // - The aspect ratio is calculated by dividing a width by a height.
    // - If the ‘width’ and ‘height’ of the rootmost ‘svg’ element are both specified with unit identifiers (in, mm, cm, pt, pc,
    //   px, em, ex) or in user units, then the aspect ratio is calculated from the ‘width’ and ‘height’ attributes after
    //   resolving both values to user units.
    if (intrinsicWidthAttribute.isFixed() || intrinsicHeightAttribute.isFixed()) {
        if (intrinsicWidthAttribute.isFixed())
            intrinsicSize.setWidth(floatValueForLength(intrinsicWidthAttribute, 0));
        if (intrinsicHeightAttribute.isFixed())
            intrinsicSize.setHeight(floatValueForLength(intrinsicHeightAttribute, 0));
        if (!intrinsicSize.isEmpty())
            intrinsicRatio = intrinsicSize.width() / static_cast<double>(intrinsicSize.height());
        return;
    }

    // - If either/both of the ‘width’ and ‘height’ of the rootmost ‘svg’ element are in percentage units (or omitted), the
    //   aspect ratio is calculated from the width and height values of the ‘viewBox’ specified for the current SVG document
    //   fragment. If the ‘viewBox’ is not correctly specified, or set to 'none', the intrinsic aspect ratio cannot be
    //   calculated and is considered unspecified.
    intrinsicSize = svg->viewBox().size();
    if (!intrinsicSize.isEmpty()) {
        // The viewBox can only yield an intrinsic ratio, not an intrinsic size.
        intrinsicRatio = intrinsicSize.width() / static_cast<double>(intrinsicSize.height());
        intrinsicSize = FloatSize();
        return;
    }

    // If our intrinsic size is in percentage units, return those to the caller through the intrinsicSize. Notify the caller
    // about the special situation, by setting isPercentageIntrinsicSize=true, so it knows how to interpret the return values.
    if (intrinsicWidthAttribute.isPercent() && intrinsicHeightAttribute.isPercent()) {
        isPercentageIntrinsicSize = true;
        intrinsicSize = FloatSize(intrinsicWidthAttribute.percent(), intrinsicHeightAttribute.percent());
    }
}

bool RenderSVGRoot::isEmbeddedThroughSVGImage() const
{
    if (!node())
        return false;
    return isInSVGImage(toSVGSVGElement(node()));
}

bool RenderSVGRoot::isEmbeddedThroughFrameContainingSVGDocument() const
{
    if (!node())
        return false;

    Frame* frame = node()->document()->frame();
    if (!frame)
        return false;

    // If our frame has an owner renderer, we're embedded through eg. object/embed/iframe,
    // but we only negotiate if we're in an SVG document.
    if (!frame->ownerRenderer())
        return false;
    return frame->document()->isSVGDocument();
}

static inline LayoutUnit resolveLengthAttributeForSVG(const Length& length, float scale, float maxSize, RenderView* renderView)
{
    return static_cast<LayoutUnit>(valueForLength(length, maxSize, renderView) * (length.isFixed() ? scale : 1));
}

LayoutUnit RenderSVGRoot::computeReplacedLogicalWidth(ShouldComputePreferred shouldComputePreferred) const
{
    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);

    // When we're embedded through SVGImage (border-image/background-image/<html:img>/...) we're forced to resize to a specific size.
    if (!m_containerSize.isEmpty())
        return m_containerSize.width();

    if (style()->logicalWidth().isSpecified() || style()->logicalMaxWidth().isSpecified())
        return RenderReplaced::computeReplacedLogicalWidth(shouldComputePreferred);

    if (svg->widthAttributeEstablishesViewport())
        return resolveLengthAttributeForSVG(svg->intrinsicWidth(SVGSVGElement::IgnoreCSSProperties), style()->effectiveZoom(), containingBlock()->availableLogicalWidth(), view());

    // SVG embedded through object/embed/iframe.
    if (isEmbeddedThroughFrameContainingSVGDocument())
        return document()->frame()->ownerRenderer()->availableLogicalWidth();

    // SVG embedded via SVGImage (background-image/border-image/etc) / Inline SVG.
    return RenderReplaced::computeReplacedLogicalWidth(shouldComputePreferred);
}

LayoutUnit RenderSVGRoot::computeReplacedLogicalHeight() const
{
    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);

    // When we're embedded through SVGImage (border-image/background-image/<html:img>/...) we're forced to resize to a specific size.
    if (!m_containerSize.isEmpty())
        return m_containerSize.height();

    if (style()->logicalHeight().isSpecified() || style()->logicalMaxHeight().isSpecified())
        return RenderReplaced::computeReplacedLogicalHeight();

    if (svg->heightAttributeEstablishesViewport()) {
        Length height = svg->intrinsicHeight(SVGSVGElement::IgnoreCSSProperties);
        if (height.isPercent()) {
            RenderBlock* cb = containingBlock();
            ASSERT(cb);
            while (cb->isAnonymous()) {
                cb = cb->containingBlock();
                cb->addPercentHeightDescendant(const_cast<RenderSVGRoot*>(this));
            }
        } else
            RenderBlock::removePercentHeightDescendant(const_cast<RenderSVGRoot*>(this));

        return resolveLengthAttributeForSVG(height, style()->effectiveZoom(), containingBlock()->availableLogicalHeight(IncludeMarginBorderPadding), view());
    }

    // SVG embedded through object/embed/iframe.
    if (isEmbeddedThroughFrameContainingSVGDocument())
        return document()->frame()->ownerRenderer()->availableLogicalHeight(IncludeMarginBorderPadding);

    // SVG embedded via SVGImage (background-image/border-image/etc) / Inline SVG.
    return RenderReplaced::computeReplacedLogicalHeight();
}

void RenderSVGRoot::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());

    m_resourcesNeedingToInvalidateClients.clear();

    // Arbitrary affine transforms are incompatible with LayoutState.
    LayoutStateDisabler layoutStateDisabler(view());

    bool needsLayout = selfNeedsLayout();
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout() && needsLayout);

    LayoutSize oldSize = size();
    updateLogicalWidth();
    updateLogicalHeight();
    buildLocalToBorderBoxTransform();

    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);
    m_isLayoutSizeChanged = needsLayout || (svg->hasRelativeLengths() && oldSize != size());
    SVGRenderSupport::layoutChildren(this, needsLayout || SVGRenderSupport::filtersForceContainerLayout(this));

    if (!m_resourcesNeedingToInvalidateClients.isEmpty()) {
        // Invalidate resource clients, which may mark some nodes for layout.
        HashSet<RenderSVGResourceContainer*>::iterator end = m_resourcesNeedingToInvalidateClients.end();
        for (HashSet<RenderSVGResourceContainer*>::iterator it = m_resourcesNeedingToInvalidateClients.begin(); it != end; ++it)
            (*it)->removeAllClientsFromCache();

        m_isLayoutSizeChanged = false;
        SVGRenderSupport::layoutChildren(this, false);
    }

    // At this point LayoutRepainter already grabbed the old bounds,
    // recalculate them now so repaintAfterLayout() uses the new bounds.
    if (m_needsBoundariesOrTransformUpdate) {
        updateCachedBoundaries();
        m_needsBoundariesOrTransformUpdate = false;
    }

    updateLayerTransform();

    repainter.repaintAfterLayout();

    setNeedsLayout(false);
}

void RenderSVGRoot::paintReplaced(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    // An empty viewport disables rendering.
    if (pixelSnappedBorderBoxRect().isEmpty())
        return;

    // Don't paint, if the context explicitly disabled it.
    if (paintInfo.context->paintingDisabled())
        return;

    // An empty viewBox also disables rendering.
    // (http://www.w3.org/TR/SVG/coords.html#ViewBoxAttribute)
    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);
    if (svg->hasEmptyViewBox())
        return;

    Page* page = 0;
    if (Frame* frame = this->frame())
        page = frame->page();

    // Don't paint if we don't have kids, except if we have filters we should paint those.
    if (!firstChild()) {
        SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
        if (!resources || !resources->filter()) {
            if (page && paintInfo.phase == PaintPhaseForeground)
                page->addRelevantUnpaintedObject(this, visualOverflowRect());
            return;
        }
    }

    if (page && paintInfo.phase == PaintPhaseForeground)
        page->addRelevantRepaintedObject(this, visualOverflowRect());

    // Make a copy of the PaintInfo because applyTransform will modify the damage rect.
    PaintInfo childPaintInfo(paintInfo);
    childPaintInfo.context->save();

    // Apply initial viewport clip - not affected by overflow handling
    childPaintInfo.context->clip(pixelSnappedIntRect(overflowClipRect(paintOffset, paintInfo.renderRegion)));

    // Convert from container offsets (html renderers) to a relative transform (svg renderers).
    // Transform from our paint container's coordinate system to our local coords.
    IntPoint adjustedPaintOffset = roundedIntPoint(paintOffset);
    childPaintInfo.applyTransform(AffineTransform::translation(adjustedPaintOffset.x(), adjustedPaintOffset.y()) * localToBorderBoxTransform());

    // SVGRenderingContext must be destroyed before we restore the childPaintInfo.context, because a filter may have
    // changed the context and it is only reverted when the SVGRenderingContext destructor finishes applying the filter.
    {
        SVGRenderingContext renderingContext;
        bool continueRendering = true;
        if (childPaintInfo.phase == PaintPhaseForeground) {
            renderingContext.prepareToRenderSVGContent(this, childPaintInfo);
            continueRendering = renderingContext.isRenderingPrepared();
        }

        if (continueRendering)
            RenderBox::paint(childPaintInfo, LayoutPoint());
    }

    childPaintInfo.context->restore();
}

void RenderSVGRoot::willBeDestroyed()
{
    RenderBlock::removePercentHeightDescendant(const_cast<RenderSVGRoot*>(this));

    SVGResourcesCache::clientDestroyed(this);
    RenderReplaced::willBeDestroyed();
}

void RenderSVGRoot::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (diff == StyleDifferenceLayout)
        setNeedsBoundariesUpdate();
    RenderReplaced::styleWillChange(diff, newStyle);
}

void RenderSVGRoot::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderReplaced::styleDidChange(diff, oldStyle);
    SVGResourcesCache::clientStyleChanged(this, diff, style());
}

void RenderSVGRoot::addChild(RenderObject* child, RenderObject* beforeChild)
{
    RenderReplaced::addChild(child, beforeChild);
    SVGResourcesCache::clientWasAddedToTree(child, child->style());
}

void RenderSVGRoot::removeChild(RenderObject* child)
{
    SVGResourcesCache::clientWillBeRemovedFromTree(child);
    RenderReplaced::removeChild(child);
}

// RenderBox methods will expect coordinates w/o any transforms in coordinates
// relative to our borderBox origin.  This method gives us exactly that.
void RenderSVGRoot::buildLocalToBorderBoxTransform()
{
    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);
    float scale = style()->effectiveZoom();
    SVGPoint translate = svg->currentTranslate();
    LayoutSize borderAndPadding(borderLeft() + paddingLeft(), borderTop() + paddingTop());
    m_localToBorderBoxTransform = svg->viewBoxToViewTransform(contentWidth() / scale, contentHeight() / scale);
    if (borderAndPadding.isEmpty() && scale == 1 && translate == SVGPoint::zero())
        return;
    m_localToBorderBoxTransform = AffineTransform(scale, 0, 0, scale, borderAndPadding.width() + translate.x(), borderAndPadding.height() + translate.y()) * m_localToBorderBoxTransform;
}

const AffineTransform& RenderSVGRoot::localToParentTransform() const
{
    // Slightly optimized version of m_localToParentTransform = AffineTransform::translation(x(), y()) * m_localToBorderBoxTransform;
    m_localToParentTransform = m_localToBorderBoxTransform;
    if (x())
        m_localToParentTransform.setE(m_localToParentTransform.e() + roundToInt(x()));
    if (y())
        m_localToParentTransform.setF(m_localToParentTransform.f() + roundToInt(y()));
    return m_localToParentTransform;
}

LayoutRect RenderSVGRoot::clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const
{
    return SVGRenderSupport::clippedOverflowRectForRepaint(this, repaintContainer);
}

void RenderSVGRoot::computeFloatRectForRepaint(const RenderLayerModelObject* repaintContainer, FloatRect& repaintRect, bool fixed) const
{
    // Apply our local transforms (except for x/y translation), then our shadow, 
    // and then call RenderBox's method to handle all the normal CSS Box model bits
    repaintRect = m_localToBorderBoxTransform.mapRect(repaintRect);

    const SVGRenderStyle* svgStyle = style()->svgStyle();
    if (const ShadowData* shadow = svgStyle->shadow())
        shadow->adjustRectForShadow(repaintRect);

    // Apply initial viewport clip - not affected by overflow settings
    repaintRect.intersect(pixelSnappedBorderBoxRect());

    LayoutRect rect = enclosingIntRect(repaintRect);
    RenderReplaced::computeRectForRepaint(repaintContainer, rect, fixed);
    repaintRect = rect;
}

// This method expects local CSS box coordinates.
// Callers with local SVG viewport coordinates should first apply the localToBorderBoxTransform
// to convert from SVG viewport coordinates to local CSS box coordinates.
void RenderSVGRoot::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags mode, bool* wasFixed) const
{
    ASSERT(mode & ~IsFixed); // We should have no fixed content in the SVG rendering tree.
    ASSERT(mode & UseTransforms); // mapping a point through SVG w/o respecting trasnforms is useless.

    RenderReplaced::mapLocalToContainer(repaintContainer, transformState, mode | ApplyContainerFlip, wasFixed);
}

const RenderObject* RenderSVGRoot::pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap& geometryMap) const
{
    return RenderReplaced::pushMappingToContainer(ancestorToStopAt, geometryMap);
}

void RenderSVGRoot::updateCachedBoundaries()
{
    SVGRenderSupport::computeContainerBoundingBoxes(this, m_objectBoundingBox, m_objectBoundingBoxValid, m_strokeBoundingBox, m_repaintBoundingBoxExcludingShadow);
    SVGRenderSupport::intersectRepaintRectWithResources(this, m_repaintBoundingBoxExcludingShadow);
    m_repaintBoundingBoxExcludingShadow.inflate(borderAndPaddingWidth());

    m_repaintBoundingBox = m_repaintBoundingBoxExcludingShadow;
    SVGRenderSupport::intersectRepaintRectWithShadows(this, m_repaintBoundingBox);
}

bool RenderSVGRoot::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    LayoutPoint pointInParent = locationInContainer.point() - toLayoutSize(accumulatedOffset);
    LayoutPoint pointInBorderBox = pointInParent - toLayoutSize(location());

    // Only test SVG content if the point is in our content box.
    // FIXME: This should be an intersection when rect-based hit tests are supported by nodeAtFloatPoint.
    if (contentBoxRect().contains(pointInBorderBox)) {
        FloatPoint localPoint = localToParentTransform().inverse().mapPoint(FloatPoint(pointInParent));

        for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
            // FIXME: nodeAtFloatPoint() doesn't handle rect-based hit tests yet.
            if (child->nodeAtFloatPoint(request, result, localPoint, hitTestAction)) {
                updateHitTestResult(result, pointInBorderBox);
                if (!result.addNodeToRectBasedTestResult(child->node(), request, locationInContainer))
                    return true;
            }
        }
    }

    // If we didn't early exit above, we've just hit the container <svg> element. Unlike SVG 1.1, 2nd Edition allows container elements to be hit.
    if (hitTestAction == HitTestBlockBackground && visibleToHitTesting()) {
        // Only return true here, if the last hit testing phase 'BlockBackground' is executed. If we'd return true in the 'Foreground' phase,
        // hit testing would stop immediately. For SVG only trees this doesn't matter. Though when we have a <foreignObject> subtree we need
        // to be able to detect hits on the background of a <div> element. If we'd return true here in the 'Foreground' phase, we are not able 
        // to detect these hits anymore.
        LayoutRect boundsRect(accumulatedOffset + location(), size());
        if (locationInContainer.intersects(boundsRect)) {
            updateHitTestResult(result, pointInBorderBox);
            if (!result.addNodeToRectBasedTestResult(node(), request, locationInContainer, boundsRect))
                return true;
        }
    }

    return false;
}

bool RenderSVGRoot::hasRelativeDimensions() const
{
    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);

    return svg->intrinsicHeight(SVGSVGElement::IgnoreCSSProperties).isPercent() || svg->intrinsicWidth(SVGSVGElement::IgnoreCSSProperties).isPercent();
}

bool RenderSVGRoot::hasRelativeIntrinsicLogicalWidth() const
{
    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);
    return svg->intrinsicWidth(SVGSVGElement::IgnoreCSSProperties).isPercent();
}

bool RenderSVGRoot::hasRelativeLogicalHeight() const
{
    SVGSVGElement* svg = toSVGSVGElement(node());
    ASSERT(svg);

    return svg->intrinsicHeight(SVGSVGElement::IgnoreCSSProperties).isPercent();
}

void RenderSVGRoot::addResourceForClientInvalidation(RenderSVGResourceContainer* resource)
{
    RenderObject* svgRoot = resource->parent();
    while (svgRoot && !svgRoot->isSVGRoot())
        svgRoot = svgRoot->parent();
    if (!svgRoot)
        return;
    toRenderSVGRoot(svgRoot)->m_resourcesNeedingToInvalidateClients.add(resource);
}

}

#endif // ENABLE(SVG)
