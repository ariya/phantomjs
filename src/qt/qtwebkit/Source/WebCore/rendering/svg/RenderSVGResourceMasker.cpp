/*
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
#include "RenderSVGResourceMasker.h"

#include "AffineTransform.h"
#include "Element.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "ImageBuffer.h"
#include "IntRect.h"
#include "RenderSVGResource.h"
#include "SVGElement.h"
#include "SVGMaskElement.h"
#include "SVGRenderingContext.h"
#include "SVGStyledElement.h"
#include "SVGUnitTypes.h"
#include <wtf/Vector.h>

namespace WebCore {

RenderSVGResourceType RenderSVGResourceMasker::s_resourceType = MaskerResourceType;

RenderSVGResourceMasker::RenderSVGResourceMasker(SVGMaskElement* node)
    : RenderSVGResourceContainer(node)
{
}

RenderSVGResourceMasker::~RenderSVGResourceMasker()
{
    if (m_masker.isEmpty())
        return;

    deleteAllValues(m_masker);
    m_masker.clear();
}

void RenderSVGResourceMasker::removeAllClientsFromCache(bool markForInvalidation)
{
    m_maskContentBoundaries = FloatRect();
    if (!m_masker.isEmpty()) {
        deleteAllValues(m_masker);
        m_masker.clear();
    }

    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceMasker::removeClientFromCache(RenderObject* client, bool markForInvalidation)
{
    ASSERT(client);

    if (m_masker.contains(client))
        delete m_masker.take(client);

    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

bool RenderSVGResourceMasker::applyResource(RenderObject* object, RenderStyle*, GraphicsContext*& context, unsigned short resourceMode)
{
    ASSERT(object);
    ASSERT(context);
    ASSERT_UNUSED(resourceMode, resourceMode == ApplyToDefaultMode);

    bool missingMaskerData = !m_masker.contains(object);
    if (missingMaskerData)
        m_masker.set(object, new MaskerData);

    MaskerData* maskerData = m_masker.get(object);

    AffineTransform absoluteTransform;
    SVGRenderingContext::calculateTransformationToOutermostCoordinateSystem(object, absoluteTransform);

    FloatRect repaintRect = object->repaintRectInLocalCoordinates();

    if (!maskerData->maskImage && !repaintRect.isEmpty()) {
        SVGMaskElement* maskElement = static_cast<SVGMaskElement*>(node());
        if (!maskElement)
            return false;

        ASSERT(style());
        const SVGRenderStyle* svgStyle = style()->svgStyle();
        ASSERT(svgStyle);
        ColorSpace colorSpace = svgStyle->colorInterpolation() == CI_LINEARRGB ? ColorSpaceLinearRGB : ColorSpaceDeviceRGB;
        if (!SVGRenderingContext::createImageBuffer(repaintRect, absoluteTransform, maskerData->maskImage, colorSpace, Unaccelerated))
            return false;

        if (!drawContentIntoMaskImage(maskerData, colorSpace, maskElement, object)) {
            maskerData->maskImage.clear();
        }
    }

    if (!maskerData->maskImage)
        return false;

    SVGRenderingContext::clipToImageBuffer(context, absoluteTransform, repaintRect, maskerData->maskImage, missingMaskerData);
    return true;
}

bool RenderSVGResourceMasker::drawContentIntoMaskImage(MaskerData* maskerData, ColorSpace colorSpace, const SVGMaskElement* maskElement, RenderObject* object)
{
    GraphicsContext* maskImageContext = maskerData->maskImage->context();
    ASSERT(maskImageContext);

    // Eventually adjust the mask image context according to the target objectBoundingBox.
    AffineTransform maskContentTransformation;
    if (maskElement->maskContentUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        FloatRect objectBoundingBox = object->objectBoundingBox();
        maskContentTransformation.translate(objectBoundingBox.x(), objectBoundingBox.y());
        maskContentTransformation.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        maskImageContext->concatCTM(maskContentTransformation);
    }

    // Draw the content into the ImageBuffer.
    for (Node* node = maskElement->firstChild(); node; node = node->nextSibling()) {
        RenderObject* renderer = node->renderer();
        if (!node->isSVGElement() || !toSVGElement(node)->isSVGStyledElement() || !renderer)
            continue;
        if (renderer->needsLayout())
            return false;
        RenderStyle* style = renderer->style();
        if (!style || style->display() == NONE || style->visibility() != VISIBLE)
            continue;
        SVGRenderingContext::renderSubtreeToImageBuffer(maskerData->maskImage.get(), renderer, maskContentTransformation);
    }

#if !USE(CG)
    maskerData->maskImage->transformColorSpace(ColorSpaceDeviceRGB, colorSpace);
#else
    UNUSED_PARAM(colorSpace);
#endif

    ASSERT(style());
    ASSERT(style()->svgStyle());
    // Create the luminance mask.
    if (style()->svgStyle()->maskType() == MT_LUMINANCE)
        maskerData->maskImage->convertToLuminanceMask();

    return true;
}

void RenderSVGResourceMasker::calculateMaskContentRepaintRect()
{
    for (Node* childNode = node()->firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!childNode->isSVGElement() || !toSVGElement(childNode)->isSVGStyledElement() || !renderer)
            continue;
        RenderStyle* style = renderer->style();
        if (!style || style->display() == NONE || style->visibility() != VISIBLE)
             continue;
        m_maskContentBoundaries.unite(renderer->localToParentTransform().mapRect(renderer->repaintRectInLocalCoordinates()));
    }
}

FloatRect RenderSVGResourceMasker::resourceBoundingBox(RenderObject* object)
{
    SVGMaskElement* maskElement = static_cast<SVGMaskElement*>(node());
    ASSERT(maskElement);

    FloatRect objectBoundingBox = object->objectBoundingBox();
    FloatRect maskBoundaries = SVGLengthContext::resolveRectangle<SVGMaskElement>(maskElement, maskElement->maskUnits(), objectBoundingBox);

    // Resource was not layouted yet. Give back clipping rect of the mask.
    if (selfNeedsLayout())
        return maskBoundaries;

    if (m_maskContentBoundaries.isEmpty())
        calculateMaskContentRepaintRect();

    FloatRect maskRect = m_maskContentBoundaries;
    if (maskElement->maskContentUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        AffineTransform transform;
        transform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        transform.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        maskRect = transform.mapRect(maskRect);
    }

    maskRect.intersect(maskBoundaries);
    return maskRect;
}

}

#endif // ENABLE(SVG)
