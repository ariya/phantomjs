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
#include "SVGImageBufferTools.h"
#include "SVGMaskElement.h"
#include "SVGStyledElement.h"
#include "SVGUnitTypes.h"

#include <wtf/ByteArray.h>
#include <wtf/UnusedParam.h>
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
#ifndef NDEBUG
    ASSERT(resourceMode == ApplyToDefaultMode);
#else
    UNUSED_PARAM(resourceMode);
#endif

    if (!m_masker.contains(object))
        m_masker.set(object, new MaskerData);

    MaskerData* maskerData = m_masker.get(object);

    AffineTransform absoluteTransform;
    SVGImageBufferTools::calculateTransformationToOutermostSVGCoordinateSystem(object, absoluteTransform);

    FloatRect absoluteTargetRect = absoluteTransform.mapRect(object->repaintRectInLocalCoordinates());
    FloatRect clampedAbsoluteTargetRect = SVGImageBufferTools::clampedAbsoluteTargetRectForRenderer(object, absoluteTargetRect);

    if (!maskerData->maskImage && !clampedAbsoluteTargetRect.isEmpty()) {
        SVGMaskElement* maskElement = static_cast<SVGMaskElement*>(node());
        if (!maskElement)
            return false;

        if (!SVGImageBufferTools::createImageBuffer(absoluteTargetRect, clampedAbsoluteTargetRect, maskerData->maskImage, ColorSpaceLinearRGB))
            return false;

        GraphicsContext* maskImageContext = maskerData->maskImage->context();
        ASSERT(maskImageContext);

        // The save/restore pair is needed for clipToImageBuffer - it doesn't work without it on non-Cg platforms.
        maskImageContext->save();
        maskImageContext->translate(-clampedAbsoluteTargetRect.x(), -clampedAbsoluteTargetRect.y());
        maskImageContext->concatCTM(absoluteTransform);

        drawContentIntoMaskImage(maskerData, maskElement, object);
    }

    if (!maskerData->maskImage)
        return false;

    SVGImageBufferTools::clipToImageBuffer(context, absoluteTransform, clampedAbsoluteTargetRect, maskerData->maskImage);
    return true;
}

void RenderSVGResourceMasker::drawContentIntoMaskImage(MaskerData* maskerData, const SVGMaskElement* maskElement, RenderObject* object)
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
        if (!node->isSVGElement() || !static_cast<SVGElement*>(node)->isStyled() || !renderer)
            continue;
        RenderStyle* style = renderer->style();
        if (!style || style->display() == NONE || style->visibility() != VISIBLE)
            continue;
        SVGImageBufferTools::renderSubtreeToImageBuffer(maskerData->maskImage.get(), renderer, maskContentTransformation);
    }

    maskImageContext->restore();

#if !USE(CG)
    maskerData->maskImage->transformColorSpace(ColorSpaceDeviceRGB, ColorSpaceLinearRGB);
#endif

    // Create the luminance mask.
    IntRect maskImageRect(IntPoint(), maskerData->maskImage->size());
    RefPtr<ByteArray> srcPixelArray = maskerData->maskImage->getUnmultipliedImageData(maskImageRect);

    unsigned pixelArrayLength = srcPixelArray->length();
    for (unsigned pixelOffset = 0; pixelOffset < pixelArrayLength; pixelOffset += 4) {
        unsigned char a = srcPixelArray->get(pixelOffset + 3);
        if (!a)
            continue;
        unsigned char r = srcPixelArray->get(pixelOffset);
        unsigned char g = srcPixelArray->get(pixelOffset + 1);
        unsigned char b = srcPixelArray->get(pixelOffset + 2);

        double luma = (r * 0.2125 + g * 0.7154 + b * 0.0721) * ((double)a / 255.0);
        srcPixelArray->set(pixelOffset + 3, luma);
    }

    maskerData->maskImage->putUnmultipliedImageData(srcPixelArray.get(), maskImageRect.size(), maskImageRect, IntPoint());
}

void RenderSVGResourceMasker::calculateMaskContentRepaintRect()
{
    for (Node* childNode = node()->firstChild(); childNode; childNode = childNode->nextSibling()) {
        RenderObject* renderer = childNode->renderer();
        if (!childNode->isSVGElement() || !static_cast<SVGElement*>(childNode)->isStyled() || !renderer)
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
    FloatRect maskBoundaries = maskElement->maskBoundingBox(objectBoundingBox);

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
