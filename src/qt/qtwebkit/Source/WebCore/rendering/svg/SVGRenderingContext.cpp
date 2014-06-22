/*
 * Copyright (C) 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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
#include "SVGRenderingContext.h"

#include "BasicShapes.h"
#include "Frame.h"
#include "FrameView.h"
#include "RenderLayer.h"
#include "RenderSVGImage.h"
#include "RenderSVGResource.h"
#include "RenderSVGResourceClipper.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMasker.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"

static int kMaxImageBufferSize = 4096;

namespace WebCore {

static inline bool isRenderingMaskImage(RenderObject* object)
{
    if (object->frame() && object->frame()->view())
        return object->frame()->view()->paintBehavior() & PaintBehaviorRenderingSVGMask;
    return false;
}

SVGRenderingContext::~SVGRenderingContext()
{
    // Fast path if we don't need to restore anything.
    if (!(m_renderingFlags & ActionsNeeded))
        return;

    ASSERT(m_object && m_paintInfo);

#if ENABLE(FILTERS)
    if (m_renderingFlags & EndFilterLayer) {
        ASSERT(m_filter);
        m_filter->postApplyResource(m_object, m_paintInfo->context, ApplyToDefaultMode, 0, 0);
        m_paintInfo->context = m_savedContext;
        m_paintInfo->rect = m_savedPaintRect;
    }
#endif

    if (m_renderingFlags & EndOpacityLayer)
        m_paintInfo->context->endTransparencyLayer();

    if (m_renderingFlags & EndShadowLayer)
        m_paintInfo->context->endTransparencyLayer();

    if (m_renderingFlags & RestoreGraphicsContext)
        m_paintInfo->context->restore();
}

void SVGRenderingContext::prepareToRenderSVGContent(RenderObject* object, PaintInfo& paintInfo, NeedsGraphicsContextSave needsGraphicsContextSave)
{
    ASSERT(object);

#ifndef NDEBUG
    // This function must not be called twice!
    ASSERT(!(m_renderingFlags & PrepareToRenderSVGContentWasCalled));
    m_renderingFlags |= PrepareToRenderSVGContentWasCalled;
#endif

    m_object = object;
    m_paintInfo = &paintInfo;
#if ENABLE(FILTERS)
    m_filter = 0;
#endif

    // We need to save / restore the context even if the initialization failed.
    if (needsGraphicsContextSave == SaveGraphicsContext) {
        m_paintInfo->context->save();
        m_renderingFlags |= RestoreGraphicsContext;
    }

    RenderStyle* style = m_object->style();
    ASSERT(style);

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    // Setup transparency layers before setting up SVG resources!
    bool isRenderingMask = isRenderingMaskImage(m_object);
    float opacity = isRenderingMask ? 1 : style->opacity();
    const ShadowData* shadow = svgStyle->shadow();
    if (opacity < 1 || shadow) {
        FloatRect repaintRect = m_object->repaintRectInLocalCoordinates();

        if (opacity < 1) {
            m_paintInfo->context->clip(repaintRect);
            m_paintInfo->context->beginTransparencyLayer(opacity);
            m_renderingFlags |= EndOpacityLayer;
        }

        if (shadow) {
            m_paintInfo->context->clip(repaintRect);
            m_paintInfo->context->setShadow(IntSize(roundToInt(shadow->x()), roundToInt(shadow->y())), shadow->radius(), shadow->color(), style->colorSpace());
            m_paintInfo->context->beginTransparencyLayer(1);
            m_renderingFlags |= EndShadowLayer;
        }
    }

    ClipPathOperation* clipPathOperation = style->clipPath();
    if (clipPathOperation && clipPathOperation->getOperationType() == ClipPathOperation::SHAPE) {
        ShapeClipPathOperation* clipPath = static_cast<ShapeClipPathOperation*>(clipPathOperation);
        m_paintInfo->context->clipPath(clipPath->path(object->objectBoundingBox()), clipPath->windRule());
    }

    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(m_object);
    if (!resources) {
#if ENABLE(FILTERS)
        if (svgStyle->hasFilter())
            return;
#endif
        m_renderingFlags |= RenderingPrepared;
        return;
    }

    if (!isRenderingMask) {
        if (RenderSVGResourceMasker* masker = resources->masker()) {
            if (!masker->applyResource(m_object, style, m_paintInfo->context, ApplyToDefaultMode))
                return;
        }
    }

    RenderSVGResourceClipper* clipper = resources->clipper();
    if (!clipPathOperation && clipper) {
        if (!clipper->applyResource(m_object, style, m_paintInfo->context, ApplyToDefaultMode))
            return;
    }

#if ENABLE(FILTERS)
    if (!isRenderingMask) {
        m_filter = resources->filter();
        if (m_filter) {
            m_savedContext = m_paintInfo->context;
            m_savedPaintRect = m_paintInfo->rect;
            // Return with false here may mean that we don't need to draw the content
            // (because it was either drawn before or empty) but we still need to apply the filter.
            m_renderingFlags |= EndFilterLayer;
            if (!m_filter->applyResource(m_object, style, m_paintInfo->context, ApplyToDefaultMode))
                return;

            // Since we're caching the resulting bitmap and do not invalidate it on repaint rect
            // changes, we need to paint the whole filter region. Otherwise, elements not visible
            // at the time of the initial paint (due to scrolling, window size, etc.) will never
            // be drawn.
            m_paintInfo->rect = IntRect(m_filter->drawingRegion(m_object));
        }
    }
#endif

    m_renderingFlags |= RenderingPrepared;
}

static AffineTransform& currentContentTransformation()
{
    DEFINE_STATIC_LOCAL(AffineTransform, s_currentContentTransformation, ());
    return s_currentContentTransformation;
}

float SVGRenderingContext::calculateScreenFontSizeScalingFactor(const RenderObject* renderer)
{
    ASSERT(renderer);

    AffineTransform ctm;
    calculateTransformationToOutermostCoordinateSystem(renderer, ctm);
    return narrowPrecisionToFloat(sqrt((pow(ctm.xScale(), 2) + pow(ctm.yScale(), 2)) / 2));
}

void SVGRenderingContext::calculateTransformationToOutermostCoordinateSystem(const RenderObject* renderer, AffineTransform& absoluteTransform)
{
    ASSERT(renderer);
    absoluteTransform = currentContentTransformation();

    // Walk up the render tree, accumulating SVG transforms.
    while (renderer) {
        absoluteTransform = renderer->localToParentTransform() * absoluteTransform;
        if (renderer->isSVGRoot())
            break;
        renderer = renderer->parent();
    }

    // Continue walking up the layer tree, accumulating CSS transforms.
    RenderLayer* layer = renderer ? renderer->enclosingLayer() : 0;
    while (layer) {
        if (TransformationMatrix* layerTransform = layer->transform())
            absoluteTransform = layerTransform->toAffineTransform() * absoluteTransform;

        // We can stop at compositing layers, to match the backing resolution.
        if (layer->isComposited())
            break;

        layer = layer->parent();
    }
}

bool SVGRenderingContext::createImageBuffer(const FloatRect& targetRect, const AffineTransform& absoluteTransform, OwnPtr<ImageBuffer>& imageBuffer, ColorSpace colorSpace, RenderingMode renderingMode)
{
    IntRect paintRect = calculateImageBufferRect(targetRect, absoluteTransform);
    // Don't create empty ImageBuffers.
    if (paintRect.isEmpty())
        return false;

    IntSize clampedSize = clampedAbsoluteSize(paintRect.size());
    OwnPtr<ImageBuffer> image = ImageBuffer::create(clampedSize, 1, colorSpace, renderingMode);
    if (!image)
        return false;

    GraphicsContext* imageContext = image->context();
    ASSERT(imageContext);

    imageContext->scale(FloatSize(static_cast<float>(clampedSize.width()) / paintRect.width(),
                                  static_cast<float>(clampedSize.height()) / paintRect.height()));
    imageContext->translate(-paintRect.x(), -paintRect.y());
    imageContext->concatCTM(absoluteTransform);

    imageBuffer = image.release();
    return true;
}

bool SVGRenderingContext::createImageBufferForPattern(const FloatRect& absoluteTargetRect, const FloatRect& clampedAbsoluteTargetRect, OwnPtr<ImageBuffer>& imageBuffer, ColorSpace colorSpace, RenderingMode renderingMode)
{
    IntSize imageSize(roundedIntSize(clampedAbsoluteTargetRect.size()));
    IntSize unclampedImageSize(roundedIntSize(absoluteTargetRect.size()));

    // Don't create empty ImageBuffers.
    if (imageSize.isEmpty())
        return false;

    OwnPtr<ImageBuffer> image = ImageBuffer::create(imageSize, 1, colorSpace, renderingMode);
    if (!image)
        return false;

    GraphicsContext* imageContext = image->context();
    ASSERT(imageContext);

    // Compensate rounding effects, as the absolute target rect is using floating-point numbers and the image buffer size is integer.
    imageContext->scale(FloatSize(unclampedImageSize.width() / absoluteTargetRect.width(), unclampedImageSize.height() / absoluteTargetRect.height()));

    imageBuffer = image.release();
    return true;
}

void SVGRenderingContext::renderSubtreeToImageBuffer(ImageBuffer* image, RenderObject* item, const AffineTransform& subtreeContentTransformation)
{
    ASSERT(item);
    ASSERT(image);
    ASSERT(image->context());

    PaintInfo info(image->context(), PaintInfo::infiniteRect(), PaintPhaseForeground, PaintBehaviorNormal);

    AffineTransform& contentTransformation = currentContentTransformation();
    AffineTransform savedContentTransformation = contentTransformation;
    contentTransformation = subtreeContentTransformation * contentTransformation;

    ASSERT(!item->needsLayout());
    item->paint(info, IntPoint());

    contentTransformation = savedContentTransformation;
}

void SVGRenderingContext::clipToImageBuffer(GraphicsContext* context, const AffineTransform& absoluteTransform, const FloatRect& targetRect, OwnPtr<ImageBuffer>& imageBuffer, bool safeToClear)
{
    ASSERT(context);
    ASSERT(imageBuffer);

    FloatRect absoluteTargetRect = calculateImageBufferRect(targetRect, absoluteTransform);

    // The mask image has been created in the absolute coordinate space, as the image should not be scaled.
    // So the actual masking process has to be done in the absolute coordinate space as well.
    context->concatCTM(absoluteTransform.inverse());
    context->clipToImageBuffer(imageBuffer.get(), absoluteTargetRect);
    context->concatCTM(absoluteTransform);

    // When nesting resources, with objectBoundingBox as content unit types, there's no use in caching the
    // resulting image buffer as the parent resource already caches the result.
    if (safeToClear && !currentContentTransformation().isIdentity())
        imageBuffer.clear();
}

FloatRect SVGRenderingContext::clampedAbsoluteTargetRect(const FloatRect& absoluteTargetRect)
{
    const FloatSize maxImageBufferSize(kMaxImageBufferSize, kMaxImageBufferSize);
    return FloatRect(absoluteTargetRect.location(), absoluteTargetRect.size().shrunkTo(maxImageBufferSize));
}

IntSize SVGRenderingContext::clampedAbsoluteSize(const IntSize& absoluteSize)
{
    const IntSize maxImageBufferSize(kMaxImageBufferSize, kMaxImageBufferSize);
    return absoluteSize.shrunkTo(maxImageBufferSize);
}

void SVGRenderingContext::clear2DRotation(AffineTransform& transform)
{
    AffineTransform::DecomposedType decomposition;
    transform.decompose(decomposition);
    decomposition.angle = 0;
    transform.recompose(decomposition);
}

bool SVGRenderingContext::bufferForeground(OwnPtr<ImageBuffer>& imageBuffer)
{
    ASSERT(m_paintInfo);
    ASSERT(m_object->isSVGImage());
    FloatRect boundingBox = m_object->objectBoundingBox();

    // Invalidate an existing buffer if the scale is not correct.
    if (imageBuffer) {
        AffineTransform transform = m_paintInfo->context->getCTM(GraphicsContext::DefinitelyIncludeDeviceScale);
        IntSize expandedBoundingBox = expandedIntSize(boundingBox.size());
        IntSize bufferSize(static_cast<int>(ceil(expandedBoundingBox.width() * transform.xScale())), static_cast<int>(ceil(expandedBoundingBox.height() * transform.yScale())));
        if (bufferSize != imageBuffer->internalSize())
            imageBuffer.clear();
    }

    // Create a new buffer and paint the foreground into it.
    if (!imageBuffer) {
        if ((imageBuffer = m_paintInfo->context->createCompatibleBuffer(expandedIntSize(boundingBox.size()), true))) {
            GraphicsContext* bufferedRenderingContext = imageBuffer->context();
            bufferedRenderingContext->translate(-boundingBox.x(), -boundingBox.y());
            PaintInfo bufferedInfo(*m_paintInfo);
            bufferedInfo.context = bufferedRenderingContext;
            toRenderSVGImage(m_object)->paintForeground(bufferedInfo);
        } else
            return false;
    }

    m_paintInfo->context->drawImageBuffer(imageBuffer.get(), ColorSpaceDeviceRGB, boundingBox);
    return true;
}

}

#endif
