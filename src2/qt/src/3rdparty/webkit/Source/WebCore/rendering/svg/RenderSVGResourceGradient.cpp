/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
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
#include "RenderSVGResourceGradient.h"

#include "GradientAttributes.h"
#include "GraphicsContext.h"
#include "RenderSVGText.h"
#include "SVGImageBufferTools.h"
#include "SVGRenderSupport.h"
#include <wtf/UnusedParam.h>

namespace WebCore {

RenderSVGResourceGradient::RenderSVGResourceGradient(SVGGradientElement* node)
    : RenderSVGResourceContainer(node)
    , m_shouldCollectGradientAttributes(true)
#if USE(CG)
    , m_savedContext(0)
#endif
{
}

RenderSVGResourceGradient::~RenderSVGResourceGradient()
{
    if (m_gradient.isEmpty())
        return;

    deleteAllValues(m_gradient);
    m_gradient.clear();
}

void RenderSVGResourceGradient::removeAllClientsFromCache(bool markForInvalidation)
{
    if (!m_gradient.isEmpty()) {
        deleteAllValues(m_gradient);
        m_gradient.clear();
    }

    m_shouldCollectGradientAttributes = true;
    markAllClientsForInvalidation(markForInvalidation ? RepaintInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceGradient::removeClientFromCache(RenderObject* client, bool markForInvalidation)
{
    ASSERT(client);

    if (m_gradient.contains(client))
        delete m_gradient.take(client);

    markClientForInvalidation(client, markForInvalidation ? RepaintInvalidation : ParentOnlyInvalidation);
}

#if USE(CG)
static inline bool createMaskAndSwapContextForTextGradient(GraphicsContext*& context,
                                                           GraphicsContext*& savedContext,
                                                           OwnPtr<ImageBuffer>& imageBuffer,
                                                           RenderObject* object)
{
    RenderObject* textRootBlock = RenderSVGText::locateRenderSVGTextAncestor(object);
    ASSERT(textRootBlock);

    AffineTransform absoluteTransform;
    SVGImageBufferTools::calculateTransformationToOutermostSVGCoordinateSystem(textRootBlock, absoluteTransform);

    FloatRect absoluteTargetRect = absoluteTransform.mapRect(textRootBlock->repaintRectInLocalCoordinates());
    FloatRect clampedAbsoluteTargetRect = SVGImageBufferTools::clampedAbsoluteTargetRectForRenderer(textRootBlock, absoluteTargetRect);
    if (clampedAbsoluteTargetRect.isEmpty())
        return false;

    OwnPtr<ImageBuffer> maskImage;
    if (!SVGImageBufferTools::createImageBuffer(absoluteTargetRect, clampedAbsoluteTargetRect, maskImage, ColorSpaceDeviceRGB))
        return false;

    GraphicsContext* maskImageContext = maskImage->context();
    ASSERT(maskImageContext);

    maskImageContext->translate(-clampedAbsoluteTargetRect.x(), -clampedAbsoluteTargetRect.y());
    maskImageContext->concatCTM(absoluteTransform);

    ASSERT(maskImage);
    savedContext = context;
    context = maskImageContext;
    imageBuffer = maskImage.release();
    return true;
}

static inline AffineTransform clipToTextMask(GraphicsContext* context,
                                             OwnPtr<ImageBuffer>& imageBuffer,
                                             FloatRect& targetRect,
                                             RenderObject* object,
                                             bool boundingBoxMode,
                                             const AffineTransform& gradientTransform)
{
    RenderObject* textRootBlock = RenderSVGText::locateRenderSVGTextAncestor(object);
    ASSERT(textRootBlock);

    targetRect = textRootBlock->repaintRectInLocalCoordinates();

    AffineTransform absoluteTransform;
    SVGImageBufferTools::calculateTransformationToOutermostSVGCoordinateSystem(textRootBlock, absoluteTransform);

    FloatRect absoluteTargetRect = absoluteTransform.mapRect(targetRect);
    FloatRect clampedAbsoluteTargetRect = SVGImageBufferTools::clampedAbsoluteTargetRectForRenderer(textRootBlock, absoluteTargetRect);

    SVGImageBufferTools::clipToImageBuffer(context, absoluteTransform, clampedAbsoluteTargetRect, imageBuffer);

    AffineTransform matrix;
    if (boundingBoxMode) {
        FloatRect maskBoundingBox = textRootBlock->objectBoundingBox();
        matrix.translate(maskBoundingBox.x(), maskBoundingBox.y());
        matrix.scaleNonUniform(maskBoundingBox.width(), maskBoundingBox.height());
    }
    matrix *= gradientTransform;
    return matrix;
}
#endif

bool RenderSVGResourceGradient::applyResource(RenderObject* object, RenderStyle* style, GraphicsContext*& context, unsigned short resourceMode)
{
    ASSERT(object);
    ASSERT(style);
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    // Be sure to synchronize all SVG properties on the gradientElement _before_ processing any further.
    // Otherwhise the call to collectGradientAttributes() in createTileImage(), may cause the SVG DOM property
    // synchronization to kick in, which causes removeAllClientsFromCache() to be called, which in turn deletes our
    // GradientData object! Leaving out the line below will cause svg/dynamic-updates/SVG*GradientElement-svgdom* to crash.
    SVGGradientElement* gradientElement = static_cast<SVGGradientElement*>(node());
    if (!gradientElement)
        return false;

    if (m_shouldCollectGradientAttributes) {
        gradientElement->updateAnimatedSVGAttribute(anyQName());
        collectGradientAttributes(gradientElement);
        m_shouldCollectGradientAttributes = false;
    }

    // Spec: When the geometry of the applicable element has no width or height and objectBoundingBox is specified,
    // then the given effect (e.g. a gradient or a filter) will be ignored.
    FloatRect objectBoundingBox = object->objectBoundingBox();
    if (boundingBoxMode() && objectBoundingBox.isEmpty())
        return false;

    if (!m_gradient.contains(object))
        m_gradient.set(object, new GradientData);

    GradientData* gradientData = m_gradient.get(object);
    bool isPaintingText = resourceMode & ApplyToTextMode;

    // Create gradient object
    if (!gradientData->gradient) {
        buildGradient(gradientData, gradientElement);

        // CG platforms will handle the gradient space transform for text after applying the
        // resource, so don't apply it here. For non-CG platforms, we want the text bounding
        // box applied to the gradient space transform now, so the gradient shader can use it.
#if USE(CG)
        if (boundingBoxMode() && !objectBoundingBox.isEmpty() && !isPaintingText) {
#else
        if (boundingBoxMode() && !objectBoundingBox.isEmpty()) {
#endif
            gradientData->userspaceTransform.translate(objectBoundingBox.x(), objectBoundingBox.y());
            gradientData->userspaceTransform.scaleNonUniform(objectBoundingBox.width(), objectBoundingBox.height());
        }

        AffineTransform gradientTransform;
        calculateGradientTransform(gradientTransform);

        gradientData->userspaceTransform *= gradientTransform;
        gradientData->gradient->setGradientSpaceTransform(gradientData->userspaceTransform);
    }

    if (!gradientData->gradient)
        return false;

    // Draw gradient
    context->save();

    if (isPaintingText) {
#if USE(CG)
        if (!createMaskAndSwapContextForTextGradient(context, m_savedContext, m_imageBuffer, object)) {
            context->restore();
            return false;
        }
#endif

        context->setTextDrawingMode(resourceMode & ApplyToFillMode ? TextModeFill : TextModeStroke);
    }

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    if (resourceMode & ApplyToFillMode) {
        context->setAlpha(svgStyle->fillOpacity());
        context->setFillGradient(gradientData->gradient);
        context->setFillRule(svgStyle->fillRule());
    } else if (resourceMode & ApplyToStrokeMode) {
        if (svgStyle->vectorEffect() == VE_NON_SCALING_STROKE)
            gradientData->gradient->setGradientSpaceTransform(transformOnNonScalingStroke(object, gradientData->userspaceTransform));
        context->setAlpha(svgStyle->strokeOpacity());
        context->setStrokeGradient(gradientData->gradient);
        SVGRenderSupport::applyStrokeStyleToContext(context, style, object);
    }

    return true;
}

void RenderSVGResourceGradient::postApplyResource(RenderObject* object, GraphicsContext*& context, unsigned short resourceMode, const Path* path)
{
    ASSERT(context);
    ASSERT(resourceMode != ApplyToDefaultMode);

    if (resourceMode & ApplyToTextMode) {
#if USE(CG)
        // CG requires special handling for gradient on text
        if (m_savedContext && m_gradient.contains(object)) {
            GradientData* gradientData = m_gradient.get(object);

            // Restore on-screen drawing context
            context = m_savedContext;
            m_savedContext = 0;

            AffineTransform gradientTransform;
            calculateGradientTransform(gradientTransform);

            FloatRect targetRect;
            gradientData->gradient->setGradientSpaceTransform(clipToTextMask(context, m_imageBuffer, targetRect, object, boundingBoxMode(), gradientTransform));
            context->setFillGradient(gradientData->gradient);

            context->fillRect(targetRect);
            m_imageBuffer.clear();
        }
#else
        UNUSED_PARAM(object);
#endif
    } else if (path) {
        if (resourceMode & ApplyToFillMode)
            context->fillPath(*path);
        else if (resourceMode & ApplyToStrokeMode)
            context->strokePath(*path);
    }

    context->restore();
}

void RenderSVGResourceGradient::addStops(GradientData* gradientData, const Vector<Gradient::ColorStop>& stops) const
{
    ASSERT(gradientData->gradient);

    const Vector<Gradient::ColorStop>::const_iterator end = stops.end();
    for (Vector<Gradient::ColorStop>::const_iterator it = stops.begin(); it != end; ++it)
        gradientData->gradient->addColorStop(*it);
}

}

#endif
