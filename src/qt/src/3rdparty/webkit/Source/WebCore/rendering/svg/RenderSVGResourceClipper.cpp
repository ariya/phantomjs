/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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
#include "RenderSVGResourceClipper.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "ImageBuffer.h"
#include "IntRect.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "RenderStyle.h"
#include "SVGClipPathElement.h"
#include "SVGElement.h"
#include "SVGImageBufferTools.h"
#include "SVGNames.h"
#include "SVGRenderSupport.h"
#include "SVGResources.h"
#include "SVGStyledElement.h"
#include "SVGStyledTransformableElement.h"
#include "SVGUnitTypes.h"
#include "SVGUseElement.h"
#include <wtf/UnusedParam.h>

namespace WebCore {

RenderSVGResourceType RenderSVGResourceClipper::s_resourceType = ClipperResourceType;

RenderSVGResourceClipper::RenderSVGResourceClipper(SVGClipPathElement* node)
    : RenderSVGResourceContainer(node)
    , m_invalidationBlocked(false)
{
}

RenderSVGResourceClipper::~RenderSVGResourceClipper()
{
    if (m_clipper.isEmpty())
        return;

    deleteAllValues(m_clipper);
    m_clipper.clear();
}

void RenderSVGResourceClipper::removeAllClientsFromCache(bool markForInvalidation)
{
    if (m_invalidationBlocked)
        return;

    m_clipBoundaries = FloatRect();
    if (!m_clipper.isEmpty()) {
        deleteAllValues(m_clipper);
        m_clipper.clear();
    }

    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceClipper::removeClientFromCache(RenderObject* client, bool markForInvalidation)
{
    ASSERT(client);
    if (m_invalidationBlocked)
        return;

    if (m_clipper.contains(client))
        delete m_clipper.take(client);

    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

bool RenderSVGResourceClipper::applyResource(RenderObject* object, RenderStyle*, GraphicsContext*& context, unsigned short resourceMode)
{
    ASSERT(object);
    ASSERT(context);
#ifndef NDEBUG
    ASSERT(resourceMode == ApplyToDefaultMode);
#else
    UNUSED_PARAM(resourceMode);
#endif

    return applyClippingToContext(object, object->objectBoundingBox(), object->repaintRectInLocalCoordinates(), context);
}

bool RenderSVGResourceClipper::pathOnlyClipping(GraphicsContext* context, const FloatRect& objectBoundingBox)
{
    // If the current clip-path gets clipped itself, we have to fallback to masking.
    if (!style()->svgStyle()->clipperResource().isEmpty())
        return false;
    WindRule clipRule = RULE_NONZERO;
    Path clipPath = Path();

    // If clip-path only contains one visible shape or path, we can use path-based clipping. Invisible
    // shapes don't affect the clipping and can be ignored. If clip-path contains more than one
    // visible shape, the additive clipping may not work, caused by the clipRule. EvenOdd
    // as well as NonZero can cause self-clipping of the elements.
    // See also http://www.w3.org/TR/SVG/painting.html#FillRuleProperty
    for (Node* childNode = node()->firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!renderer)
            continue;
        // Only shapes or paths are supported for direct clipping. We need to fallback to masking for texts.
        if (renderer->isSVGText())
            return false;
        if (!childNode->isSVGElement() || !static_cast<SVGElement*>(childNode)->isStyledTransformable())
            continue;
        SVGStyledTransformableElement* styled = static_cast<SVGStyledTransformableElement*>(childNode);
        RenderStyle* style = renderer->style();
        if (!style || style->display() == NONE || style->visibility() != VISIBLE)
             continue;
        const SVGRenderStyle* svgStyle = style->svgStyle();
        // Current shape in clip-path gets clipped too. Fallback to masking.
        if (!svgStyle->clipperResource().isEmpty())
            return false;
        // Fallback to masking, if there is more than one clipping path.
        if (clipPath.isEmpty()) {
            styled->toClipPath(clipPath);
            clipRule = svgStyle->clipRule();
        } else
            return false;
    }
    // Only one visible shape/path was found. Directly continue clipping and transform the content to userspace if necessary.
    if (static_cast<SVGClipPathElement*>(node())->clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        AffineTransform transform;
        transform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        transform.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        clipPath.transform(transform);
    }
    // The SVG specification wants us to clip everything, if clip-path doesn't have a child.
    if (clipPath.isEmpty())
        clipPath.addRect(FloatRect());
    context->clipPath(clipPath, clipRule);
    return true;
}

bool RenderSVGResourceClipper::applyClippingToContext(RenderObject* object, const FloatRect& objectBoundingBox,
                                                      const FloatRect& repaintRect, GraphicsContext* context)
{
    if (!m_clipper.contains(object))
        m_clipper.set(object, new ClipperData);

    bool shouldCreateClipData = false;
    ClipperData* clipperData = m_clipper.get(object);
    if (!clipperData->clipMaskImage) {
        if (pathOnlyClipping(context, objectBoundingBox))
            return true;
        shouldCreateClipData = true;
    }

    AffineTransform absoluteTransform;
    SVGImageBufferTools::calculateTransformationToOutermostSVGCoordinateSystem(object, absoluteTransform);

    FloatRect absoluteTargetRect = absoluteTransform.mapRect(repaintRect);
    FloatRect clampedAbsoluteTargetRect = SVGImageBufferTools::clampedAbsoluteTargetRectForRenderer(object, absoluteTargetRect);

    if (shouldCreateClipData && !clampedAbsoluteTargetRect.isEmpty()) {
        if (!SVGImageBufferTools::createImageBuffer(absoluteTargetRect, clampedAbsoluteTargetRect, clipperData->clipMaskImage, ColorSpaceDeviceRGB))
            return false;

        GraphicsContext* maskContext = clipperData->clipMaskImage->context();
        ASSERT(maskContext);

        // The save/restore pair is needed for clipToImageBuffer - it doesn't work without it on non-Cg platforms.
        GraphicsContextStateSaver stateSaver(*maskContext);
        maskContext->translate(-clampedAbsoluteTargetRect.x(), -clampedAbsoluteTargetRect.y());
        maskContext->concatCTM(absoluteTransform);

        // clipPath can also be clipped by another clipPath.
        if (SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this)) {
            if (RenderSVGResourceClipper* clipper = resources->clipper()) {
                if (!clipper->applyClippingToContext(this, objectBoundingBox, repaintRect, maskContext))
                    return false;
            }
        }

        drawContentIntoMaskImage(clipperData, objectBoundingBox);
    }

    if (!clipperData->clipMaskImage)
        return false;

    SVGImageBufferTools::clipToImageBuffer(context, absoluteTransform, clampedAbsoluteTargetRect, clipperData->clipMaskImage);
    return true;
}

bool RenderSVGResourceClipper::drawContentIntoMaskImage(ClipperData* clipperData, const FloatRect& objectBoundingBox)
{
    ASSERT(clipperData);
    ASSERT(clipperData->clipMaskImage);

    GraphicsContext* maskContext = clipperData->clipMaskImage->context();
    ASSERT(maskContext);

    AffineTransform maskContentTransformation;
    SVGClipPathElement* clipPath = static_cast<SVGClipPathElement*>(node());
    if (clipPath->clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        maskContentTransformation.translate(objectBoundingBox.x(), objectBoundingBox.y());
        maskContentTransformation.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        maskContext->concatCTM(maskContentTransformation);
    }

    // Draw all clipPath children into a global mask.
    for (Node* childNode = node()->firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!childNode->isSVGElement() || !static_cast<SVGElement*>(childNode)->isStyled() || !renderer)
            continue;
        RenderStyle* style = renderer->style();
        if (!style || style->display() == NONE || style->visibility() != VISIBLE)
            continue;

        WindRule newClipRule = style->svgStyle()->clipRule();
        bool isUseElement = renderer->isSVGShadowTreeRootContainer();
        if (isUseElement) {
            SVGUseElement* useElement = static_cast<SVGUseElement*>(childNode);
            renderer = useElement->rendererClipChild();
            if (!renderer)
                continue;
            if (!useElement->hasAttribute(SVGNames::clip_ruleAttr))
                newClipRule = renderer->style()->svgStyle()->clipRule();
        }

        // Only shapes, paths and texts are allowed for clipping.
        if (!renderer->isSVGPath() && !renderer->isSVGText())
            continue;

        // Save the old RenderStyle of the current object for restoring after drawing
        // it to the MaskImage. The new intermediate RenderStyle needs to inherit from
        // the old one.
        RefPtr<RenderStyle> oldRenderStyle = renderer->style();
        RefPtr<RenderStyle> newRenderStyle = RenderStyle::clone(oldRenderStyle.get());
        SVGRenderStyle* svgStyle = newRenderStyle.get()->accessSVGStyle();
        svgStyle->setFillPaint(SVGRenderStyle::initialFillPaintType(), SVGRenderStyle::initialFillPaintColor(), SVGRenderStyle::initialFillPaintUri());
        svgStyle->setStrokePaint(SVGRenderStyle::initialStrokePaintType(), SVGRenderStyle::initialStrokePaintColor(), SVGRenderStyle::initialStrokePaintUri());
        svgStyle->setFillRule(newClipRule);
        newRenderStyle.get()->setOpacity(1.0f);
        svgStyle->setFillOpacity(1.0f);
        svgStyle->setStrokeOpacity(1.0f);
        svgStyle->setFilterResource(String());
        svgStyle->setMaskerResource(String());

        // The setStyle() call results in a styleDidChange() call, which in turn invalidations the resources.
        // As we're mutating the resource on purpose, block updates until we've resetted the style again.
        m_invalidationBlocked = true;
        renderer->setStyle(newRenderStyle.release());

        // In the case of a <use> element, we obtained its renderere above, to retrieve its clipRule.
        // We have to pass the <use> renderer itself to renderSubtreeToImageBuffer() to apply it's x/y/transform/etc. values when rendering.
        // So if isUseElement is true, refetch the childNode->renderer(), as renderer got overriden above.
        SVGImageBufferTools::renderSubtreeToImageBuffer(clipperData->clipMaskImage.get(), isUseElement ? childNode->renderer() : renderer, maskContentTransformation);

        renderer->setStyle(oldRenderStyle.release());
        m_invalidationBlocked = false;
    }

    return true;
}

void RenderSVGResourceClipper::calculateClipContentRepaintRect()
{
    // This is a rough heuristic to appraise the clip size and doesn't consider clip on clip.
    for (Node* childNode = node()->firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!childNode->isSVGElement() || !static_cast<SVGElement*>(childNode)->isStyled() || !renderer)
            continue;
        if (!renderer->isSVGPath() && !renderer->isSVGText() && !renderer->isSVGShadowTreeRootContainer())
            continue;
        RenderStyle* style = renderer->style();
        if (!style || style->display() == NONE || style->visibility() != VISIBLE)
             continue;
        m_clipBoundaries.unite(renderer->localToParentTransform().mapRect(renderer->repaintRectInLocalCoordinates()));
    }
}

bool RenderSVGResourceClipper::hitTestClipContent(const FloatRect& objectBoundingBox, const FloatPoint& nodeAtPoint)
{
    FloatPoint point = nodeAtPoint;
    if (!SVGRenderSupport::pointInClippingArea(this, point))
        return false;

    if (static_cast<SVGClipPathElement*>(node())->clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        AffineTransform transform;
        transform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        transform.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        point = transform.inverse().mapPoint(point);
    }

    for (Node* childNode = node()->firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!childNode->isSVGElement() || !static_cast<SVGElement*>(childNode)->isStyled() || !renderer)
            continue;
        if (!renderer->isSVGPath() && !renderer->isSVGText() && !renderer->isSVGShadowTreeRootContainer())
            continue;
        IntPoint hitPoint;
        HitTestResult result(hitPoint);
        if (renderer->nodeAtFloatPoint(HitTestRequest(HitTestRequest::SVGClipContent), result, point, HitTestForeground))
            return true;
    }

    return false;
}

FloatRect RenderSVGResourceClipper::resourceBoundingBox(RenderObject* object)
{
    // Resource was not layouted yet. Give back the boundingBox of the object.
    if (selfNeedsLayout())
        return object->objectBoundingBox();
    
    if (m_clipBoundaries.isEmpty())
        calculateClipContentRepaintRect();

    if (static_cast<SVGClipPathElement*>(node())->clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        FloatRect objectBoundingBox = object->objectBoundingBox();
        AffineTransform transform;
        transform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        transform.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        return transform.mapRect(m_clipBoundaries);
    }

    return m_clipBoundaries;
}

}

#endif // ENABLE(SVG)
