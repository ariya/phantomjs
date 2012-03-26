/*
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
#include "SVGImageBufferTools.h"

#include "FrameView.h"
#include "GraphicsContext.h"
#include "RenderObject.h"
#include "RenderSVGContainer.h"
#include "RenderSVGRoot.h"

namespace WebCore {

static AffineTransform& currentContentTransformation()
{
    DEFINE_STATIC_LOCAL(AffineTransform, s_currentContentTransformation, ());
    return s_currentContentTransformation;
}

void SVGImageBufferTools::calculateTransformationToOutermostSVGCoordinateSystem(const RenderObject* renderer, AffineTransform& absoluteTransform)
{
    const RenderObject* current = renderer;
    ASSERT(current);

    absoluteTransform = currentContentTransformation();
    while (current) {
        absoluteTransform = current->localToParentTransform() * absoluteTransform;
        if (current->isSVGRoot())
            break;
        current = current->parent();
    }
}

bool SVGImageBufferTools::createImageBuffer(const FloatRect& absoluteTargetRect, const FloatRect& clampedAbsoluteTargetRect, OwnPtr<ImageBuffer>& imageBuffer, ColorSpace colorSpace)
{
    IntSize imageSize(roundedImageBufferSize(clampedAbsoluteTargetRect.size()));
    IntSize unclampedImageSize(SVGImageBufferTools::roundedImageBufferSize(absoluteTargetRect.size()));

    // Don't create empty ImageBuffers.
    if (imageSize.isEmpty())
        return false;

    OwnPtr<ImageBuffer> image = ImageBuffer::create(imageSize, colorSpace);
    if (!image)
        return false;

    GraphicsContext* imageContext = image->context();
    ASSERT(imageContext);

    // Compensate rounding effects, as the absolute target rect is using floating-point numbers and the image buffer size is integer.
    imageContext->scale(FloatSize(unclampedImageSize.width() / absoluteTargetRect.width(), unclampedImageSize.height() / absoluteTargetRect.height()));

    imageBuffer = image.release();
    return true;
}

void SVGImageBufferTools::renderSubtreeToImageBuffer(ImageBuffer* image, RenderObject* item, const AffineTransform& subtreeContentTransformation)
{
    ASSERT(item);
    ASSERT(image);
    ASSERT(image->context());

    PaintInfo info(image->context(), PaintInfo::infiniteRect(), PaintPhaseForeground, 0, 0, 0);

    AffineTransform& contentTransformation = currentContentTransformation();
    AffineTransform savedContentTransformation = contentTransformation;
    contentTransformation = subtreeContentTransformation * contentTransformation;

    item->layoutIfNeeded();
    item->paint(info, 0, 0);

    contentTransformation = savedContentTransformation;
}

void SVGImageBufferTools::clipToImageBuffer(GraphicsContext* context, const AffineTransform& absoluteTransform, const FloatRect& clampedAbsoluteTargetRect, OwnPtr<ImageBuffer>& imageBuffer)
{
    ASSERT(context);
    ASSERT(imageBuffer);

    // The mask image has been created in the absolute coordinate space, as the image should not be scaled.
    // So the actual masking process has to be done in the absolute coordinate space as well.
    context->concatCTM(absoluteTransform.inverse());
    context->clipToImageBuffer(imageBuffer.get(), clampedAbsoluteTargetRect);
    context->concatCTM(absoluteTransform);

    // When nesting resources, with objectBoundingBox as content unit types, there's no use in caching the
    // resulting image buffer as the parent resource already caches the result.
    if (!currentContentTransformation().isIdentity())
        imageBuffer.clear();
}

IntSize SVGImageBufferTools::roundedImageBufferSize(const FloatSize& size)
{
    return IntSize(static_cast<int>(lroundf(size.width())), static_cast<int>(lroundf(size.height())));
}

FloatRect SVGImageBufferTools::clampedAbsoluteTargetRectForRenderer(const RenderObject* renderer, const FloatRect& absoluteTargetRect)
{
    ASSERT(renderer);

    const RenderSVGRoot* svgRoot = SVGRenderSupport::findTreeRootObject(renderer);
    FloatRect clampedAbsoluteTargetRect = absoluteTargetRect;
    clampedAbsoluteTargetRect.intersect(svgRoot->contentBoxRect());
    return clampedAbsoluteTargetRect;
}

}

#endif // ENABLE(SVG)
