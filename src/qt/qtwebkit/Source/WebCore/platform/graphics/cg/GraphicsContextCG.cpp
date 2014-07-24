/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#define _USE_MATH_DEFINES 1
#include "config.h"
#include "GraphicsContextCG.h"

#include "AffineTransform.h"
#include "FloatConversion.h"
#include "GraphicsContextPlatformPrivateCG.h"
#include "ImageBuffer.h"
#include "ImageOrientation.h"
#include "KURL.h"
#include "Path.h"
#include "Pattern.h"
#include "ShadowBlur.h"
#include "SubimageCacheWithTimer.h"
#include "Timer.h"
#include <CoreGraphics/CoreGraphics.h>
#include <wtf/MathExtras.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/RetainPtr.h>

#if PLATFORM(MAC)
#include "WebCoreSystemInterface.h"
#endif

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

extern "C" {
    CG_EXTERN void CGContextSetCTM(CGContextRef, CGAffineTransform);
    CG_EXTERN CGAffineTransform CGContextGetBaseCTM(CGContextRef);
};

using namespace std;

// FIXME: The following using declaration should be in <wtf/HashFunctions.h>.
using WTF::pairIntHash;

// FIXME: The following using declaration should be in <wtf/HashTraits.h>.
using WTF::GenericHashTraits;

namespace WebCore {

static void setCGFillColor(CGContextRef context, const Color& color, ColorSpace colorSpace)
{
    CGContextSetFillColorWithColor(context, cachedCGColor(color, colorSpace));
}

static void setCGStrokeColor(CGContextRef context, const Color& color, ColorSpace colorSpace)
{
    CGContextSetStrokeColorWithColor(context, cachedCGColor(color, colorSpace));
}

CGColorSpaceRef deviceRGBColorSpaceRef()
{
    static CGColorSpaceRef deviceSpace = CGColorSpaceCreateDeviceRGB();
    return deviceSpace;
}

CGColorSpaceRef sRGBColorSpaceRef()
{
    // FIXME: Windows should be able to use kCGColorSpaceSRGB, this is tracked by http://webkit.org/b/31363.
#if PLATFORM(WIN)
    return deviceRGBColorSpaceRef();
#else
    static CGColorSpaceRef sRGBSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    return sRGBSpace;
#endif
}

#if PLATFORM(WIN)
CGColorSpaceRef linearRGBColorSpaceRef()
{
    // FIXME: Windows should be able to use linear sRGB, this is tracked by http://webkit.org/b/80000.
    return deviceRGBColorSpaceRef();
}
#endif

void GraphicsContext::platformInit(CGContextRef cgContext)
{
    m_data = new GraphicsContextPlatformPrivate(cgContext);
    setPaintingDisabled(!cgContext);
    if (cgContext) {
        // Make sure the context starts in sync with our state.
        setPlatformFillColor(fillColor(), fillColorSpace());
        setPlatformStrokeColor(strokeColor(), strokeColorSpace());
    }
}

void GraphicsContext::platformDestroy()
{
    delete m_data;
}

CGContextRef GraphicsContext::platformContext() const
{
    ASSERT(!paintingDisabled());
    ASSERT(m_data->m_cgContext);
    return m_data->m_cgContext.get();
}

void GraphicsContext::savePlatformState()
{
    // Note: Do not use this function within this class implementation, since we want to avoid the extra
    // save of the secondary context (in GraphicsContextPlatformPrivateCG.h).
    CGContextSaveGState(platformContext());
    m_data->save();
}

void GraphicsContext::restorePlatformState()
{
    // Note: Do not use this function within this class implementation, since we want to avoid the extra
    // restore of the secondary context (in GraphicsContextPlatformPrivateCG.h).
    CGContextRestoreGState(platformContext());
    m_data->restore();
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
}

void GraphicsContext::drawNativeImage(PassNativeImagePtr imagePtr, const FloatSize& imageSize, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode, ImageOrientation orientation)
{
    RetainPtr<CGImageRef> image(imagePtr);

    float currHeight = orientation.usesWidthAsHeight() ? CGImageGetWidth(image.get()) : CGImageGetHeight(image.get());

    if (currHeight <= srcRect.y())
        return;

    CGContextRef context = platformContext();
    CGContextSaveGState(context);

    bool shouldUseSubimage = false;

    // If the source rect is a subportion of the image, then we compute an inflated destination rect that will hold the entire image
    // and then set a clip to the portion that we want to display.
    FloatRect adjustedDestRect = destRect;

    if (srcRect.size() != imageSize) {
        CGInterpolationQuality interpolationQuality = CGContextGetInterpolationQuality(context);
        // When the image is scaled using high-quality interpolation, we create a temporary CGImage
        // containing only the portion we want to display. We need to do this because high-quality
        // interpolation smoothes sharp edges, causing pixels from outside the source rect to bleed
        // into the destination rect. See <rdar://problem/6112909>.
        shouldUseSubimage = (interpolationQuality != kCGInterpolationNone) && (srcRect.size() != destRect.size() || !getCTM().isIdentityOrTranslationOrFlipped());
        float xScale = srcRect.width() / destRect.width();
        float yScale = srcRect.height() / destRect.height();
        if (shouldUseSubimage) {
            FloatRect subimageRect = srcRect;
            float leftPadding = srcRect.x() - floorf(srcRect.x());
            float topPadding = srcRect.y() - floorf(srcRect.y());

            subimageRect.move(-leftPadding, -topPadding);
            adjustedDestRect.move(-leftPadding / xScale, -topPadding / yScale);

            subimageRect.setWidth(ceilf(subimageRect.width() + leftPadding));
            adjustedDestRect.setWidth(subimageRect.width() / xScale);

            subimageRect.setHeight(ceilf(subimageRect.height() + topPadding));
            adjustedDestRect.setHeight(subimageRect.height() / yScale);

#if CACHE_SUBIMAGES
            image = subimageCache().getSubimage(image.get(), subimageRect);
#else
            image = adoptCF(CGImageCreateWithImageInRect(image, subimageRect));
#endif
            if (currHeight < srcRect.maxY()) {
                ASSERT(CGImageGetHeight(image.get()) == currHeight - CGRectIntegral(srcRect).origin.y);
                adjustedDestRect.setHeight(CGImageGetHeight(image.get()) / yScale);
            }
        } else {
            adjustedDestRect.setLocation(FloatPoint(destRect.x() - srcRect.x() / xScale, destRect.y() - srcRect.y() / yScale));
            adjustedDestRect.setSize(FloatSize(imageSize.width() / xScale, imageSize.height() / yScale));
        }

        if (!destRect.contains(adjustedDestRect))
            CGContextClipToRect(context, destRect);
    }

    // If the image is only partially loaded, then shrink the destination rect that we're drawing into accordingly.
    if (!shouldUseSubimage && currHeight < imageSize.height())
        adjustedDestRect.setHeight(adjustedDestRect.height() * currHeight / imageSize.height());

    setPlatformCompositeOperation(op, blendMode);

    // ImageOrientation expects the origin to be at (0, 0)
    CGContextTranslateCTM(context, adjustedDestRect.x(), adjustedDestRect.y());
    adjustedDestRect.setLocation(FloatPoint());

    if (orientation != DefaultImageOrientation) {
        CGContextConcatCTM(context, orientation.transformFromDefault(adjustedDestRect.size()));
        if (orientation.usesWidthAsHeight()) {
            // The destination rect will have it's width and height already reversed for the orientation of
            // the image, as it was needed for page layout, so we need to reverse it back here.
            adjustedDestRect = FloatRect(adjustedDestRect.x(), adjustedDestRect.y(), adjustedDestRect.height(), adjustedDestRect.width());
        }
    }
    
    // Flip the coords.
    CGContextTranslateCTM(context, 0, adjustedDestRect.height());
    CGContextScaleCTM(context, 1, -1);

    // Adjust the color space.
    image = Image::imageWithColorSpace(image.get(), styleColorSpace);

    // Draw the image.
    CGContextDrawImage(context, adjustedDestRect, image.get());

    CGContextRestoreGState(context);
}

// Draws a filled rectangle with a stroked border.
void GraphicsContext::drawRect(const IntRect& rect)
{
    // FIXME: this function does not handle patterns and gradients
    // like drawPath does, it probably should.
    if (paintingDisabled())
        return;

    ASSERT(!rect.isEmpty());

    CGContextRef context = platformContext();

    CGContextFillRect(context, rect);

    if (strokeStyle() != NoStroke) {
        // We do a fill of four rects to simulate the stroke of a border.
        Color oldFillColor = fillColor();
        if (oldFillColor != strokeColor())
            setCGFillColor(context, strokeColor(), strokeColorSpace());
        CGRect rects[4] = {
            FloatRect(rect.x(), rect.y(), rect.width(), 1),
            FloatRect(rect.x(), rect.maxY() - 1, rect.width(), 1),
            FloatRect(rect.x(), rect.y() + 1, 1, rect.height() - 2),
            FloatRect(rect.maxX() - 1, rect.y() + 1, 1, rect.height() - 2)
        };
        CGContextFillRects(context, rects, 4);
        if (oldFillColor != strokeColor())
            setCGFillColor(context, oldFillColor, fillColorSpace());
    }
}

// This is only used to draw borders.
void GraphicsContext::drawLine(const IntPoint& point1, const IntPoint& point2)
{
    if (paintingDisabled())
        return;

    if (strokeStyle() == NoStroke)
        return;

    float width = strokeThickness();

    FloatPoint p1 = point1;
    FloatPoint p2 = point2;
    bool isVerticalLine = (p1.x() == p2.x());
    
    // For odd widths, we add in 0.5 to the appropriate x/y so that the float arithmetic
    // works out.  For example, with a border width of 3, KHTML will pass us (y1+y2)/2, e.g.,
    // (50+53)/2 = 103/2 = 51 when we want 51.5.  It is always true that an even width gave
    // us a perfect position, but an odd width gave us a position that is off by exactly 0.5.
    if (strokeStyle() == DottedStroke || strokeStyle() == DashedStroke) {
        if (isVerticalLine) {
            p1.move(0, width);
            p2.move(0, -width);
        } else {
            p1.move(width, 0);
            p2.move(-width, 0);
        }
    }
    
    if (((int)width) % 2) {
        if (isVerticalLine) {
            // We're a vertical line.  Adjust our x.
            p1.move(0.5f, 0.0f);
            p2.move(0.5f, 0.0f);
        } else {
            // We're a horizontal line. Adjust our y.
            p1.move(0.0f, 0.5f);
            p2.move(0.0f, 0.5f);
        }
    }
    
    int patWidth = 0;
    switch (strokeStyle()) {
    case NoStroke:
    case SolidStroke:
#if ENABLE(CSS3_TEXT)
    case DoubleStroke:
    case WavyStroke: // FIXME: https://bugs.webkit.org/show_bug.cgi?id=94112 - Needs platform support.
#endif // CSS3_TEXT
        break;
    case DottedStroke:
        patWidth = (int)width;
        break;
    case DashedStroke:
        patWidth = 3 * (int)width;
        break;
    }

    CGContextRef context = platformContext();

    if (shouldAntialias())
        CGContextSetShouldAntialias(context, false);

    if (patWidth) {
        CGContextSaveGState(context);

        // Do a rect fill of our endpoints.  This ensures we always have the
        // appearance of being a border.  We then draw the actual dotted/dashed line.
        setCGFillColor(context, strokeColor(), strokeColorSpace());  // The save/restore make it safe to mutate the fill color here without setting it back to the old color.
        if (isVerticalLine) {
            CGContextFillRect(context, FloatRect(p1.x() - width / 2, p1.y() - width, width, width));
            CGContextFillRect(context, FloatRect(p2.x() - width / 2, p2.y(), width, width));
        } else {
            CGContextFillRect(context, FloatRect(p1.x() - width, p1.y() - width / 2, width, width));
            CGContextFillRect(context, FloatRect(p2.x(), p2.y() - width / 2, width, width));
        }

        // Example: 80 pixels with a width of 30 pixels.
        // Remainder is 20.  The maximum pixels of line we could paint
        // will be 50 pixels.
        int distance = (isVerticalLine ? (point2.y() - point1.y()) : (point2.x() - point1.x())) - 2*(int)width;
        int remainder = distance % patWidth;
        int coverage = distance - remainder;
        int numSegments = coverage / patWidth;

        float patternOffset = 0.0f;
        // Special case 1px dotted borders for speed.
        if (patWidth == 1)
            patternOffset = 1.0f;
        else {
            bool evenNumberOfSegments = !(numSegments % 2);
            if (remainder)
                evenNumberOfSegments = !evenNumberOfSegments;
            if (evenNumberOfSegments) {
                if (remainder) {
                    patternOffset += patWidth - remainder;
                    patternOffset += remainder / 2;
                } else
                    patternOffset = patWidth / 2;
            } else {
                if (remainder)
                    patternOffset = (patWidth - remainder)/2;
            }
        }

        const CGFloat dottedLine[2] = { static_cast<CGFloat>(patWidth), static_cast<CGFloat>(patWidth) };
        CGContextSetLineDash(context, patternOffset, dottedLine, 2);
    }

    CGContextBeginPath(context);
    CGContextMoveToPoint(context, p1.x(), p1.y());
    CGContextAddLineToPoint(context, p2.x(), p2.y());

    CGContextStrokePath(context);

    if (patWidth)
        CGContextRestoreGState(context);

    if (shouldAntialias())
        CGContextSetShouldAntialias(context, true);
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    Path path;
    path.addEllipse(rect);
    drawPath(path);
}

static void addConvexPolygonToPath(Path& path, size_t numberOfPoints, const FloatPoint* points)
{
    ASSERT(numberOfPoints > 0);

    path.moveTo(points[0]);
    for (size_t i = 1; i < numberOfPoints; ++i)
        path.addLineTo(points[i]);
    path.closeSubpath();
}

void GraphicsContext::drawConvexPolygon(size_t numberOfPoints, const FloatPoint* points, bool antialiased)
{
    if (paintingDisabled())
        return;

    if (numberOfPoints <= 1)
        return;

    CGContextRef context = platformContext();

    if (antialiased != shouldAntialias())
        CGContextSetShouldAntialias(context, antialiased);

    Path path;
    addConvexPolygonToPath(path, numberOfPoints, points);
    drawPath(path);

    if (antialiased != shouldAntialias())
        CGContextSetShouldAntialias(context, shouldAntialias());
}

void GraphicsContext::clipConvexPolygon(size_t numberOfPoints, const FloatPoint* points, bool antialias)
{
    if (paintingDisabled())
        return;

    if (numberOfPoints <= 1)
        return;

    CGContextRef context = platformContext();

    if (antialias != shouldAntialias())
        CGContextSetShouldAntialias(context, antialias);

    Path path;
    addConvexPolygonToPath(path, numberOfPoints, points);
    clipPath(path, RULE_NONZERO);

    if (antialias != shouldAntialias())
        CGContextSetShouldAntialias(context, shouldAntialias());
}

void GraphicsContext::applyStrokePattern()
{
    CGContextRef cgContext = platformContext();
    AffineTransform userToBaseCTM = AffineTransform(wkGetUserToBaseCTM(cgContext));

    RetainPtr<CGPatternRef> platformPattern = adoptCF(m_state.strokePattern->createPlatformPattern(userToBaseCTM));
    if (!platformPattern)
        return;

    RetainPtr<CGColorSpaceRef> patternSpace = adoptCF(CGColorSpaceCreatePattern(0));
    CGContextSetStrokeColorSpace(cgContext, patternSpace.get());

    const CGFloat patternAlpha = 1;
    CGContextSetStrokePattern(cgContext, platformPattern.get(), &patternAlpha);
}

void GraphicsContext::applyFillPattern()
{
    CGContextRef cgContext = platformContext();
    AffineTransform userToBaseCTM = AffineTransform(wkGetUserToBaseCTM(cgContext));

    RetainPtr<CGPatternRef> platformPattern = adoptCF(m_state.fillPattern->createPlatformPattern(userToBaseCTM));
    if (!platformPattern)
        return;

    RetainPtr<CGColorSpaceRef> patternSpace = adoptCF(CGColorSpaceCreatePattern(0));
    CGContextSetFillColorSpace(cgContext, patternSpace.get());

    const CGFloat patternAlpha = 1;
    CGContextSetFillPattern(cgContext, platformPattern.get(), &patternAlpha);
}

static inline bool calculateDrawingMode(const GraphicsContextState& state, CGPathDrawingMode& mode)
{
    bool shouldFill = state.fillPattern || state.fillColor.alpha();
    bool shouldStroke = state.strokePattern || (state.strokeStyle != NoStroke && state.strokeColor.alpha());
    bool useEOFill = state.fillRule == RULE_EVENODD;

    if (shouldFill) {
        if (shouldStroke) {
            if (useEOFill)
                mode = kCGPathEOFillStroke;
            else
                mode = kCGPathFillStroke;
        } else { // fill, no stroke
            if (useEOFill)
                mode = kCGPathEOFill;
            else
                mode = kCGPathFill;
        }
    } else {
        // Setting mode to kCGPathStroke even if shouldStroke is false. In that case, we return false and mode will not be used,
        // but the compiler will not complain about an uninitialized variable.
        mode = kCGPathStroke;
    }

    return shouldFill || shouldStroke;
}

void GraphicsContext::drawPath(const Path& path)
{
    if (paintingDisabled() || path.isEmpty())
        return;

    CGContextRef context = platformContext();
    const GraphicsContextState& state = m_state;

    if (state.fillGradient || state.strokeGradient) {
        // We don't have any optimized way to fill & stroke a path using gradients
        // FIXME: Be smarter about this.
        fillPath(path);
        strokePath(path);
        return;
    }

    CGContextBeginPath(context);
    CGContextAddPath(context, path.platformPath());

    if (state.fillPattern)
        applyFillPattern();
    if (state.strokePattern)
        applyStrokePattern();

    CGPathDrawingMode drawingMode;
    if (calculateDrawingMode(state, drawingMode))
        CGContextDrawPath(context, drawingMode);
}

static inline void fillPathWithFillRule(CGContextRef context, WindRule fillRule)
{
    if (fillRule == RULE_EVENODD)
        CGContextEOFillPath(context);
    else
        CGContextFillPath(context);
}

void GraphicsContext::fillPath(const Path& path)
{
    if (paintingDisabled() || path.isEmpty())
        return;

    CGContextRef context = platformContext();

    if (m_state.fillGradient) {
        if (hasShadow()) {
            FloatRect rect = path.fastBoundingRect();
            FloatSize layerSize = getCTM().mapSize(rect.size());

            CGLayerRef layer = CGLayerCreateWithContext(context, layerSize, 0);
            CGContextRef layerContext = CGLayerGetContext(layer);

            CGContextScaleCTM(layerContext, layerSize.width() / rect.width(), layerSize.height() / rect.height());
            CGContextTranslateCTM(layerContext, -rect.x(), -rect.y());
            CGContextBeginPath(layerContext);
            CGContextAddPath(layerContext, path.platformPath());
            CGContextConcatCTM(layerContext, m_state.fillGradient->gradientSpaceTransform());

            if (fillRule() == RULE_EVENODD)
                CGContextEOClip(layerContext);
            else
                CGContextClip(layerContext);

            m_state.fillGradient->paint(layerContext);
            CGContextDrawLayerInRect(context, rect, layer);
            CGLayerRelease(layer);
        } else {
            CGContextBeginPath(context);
            CGContextAddPath(context, path.platformPath());
            CGContextSaveGState(context);
            CGContextConcatCTM(context, m_state.fillGradient->gradientSpaceTransform());

            if (fillRule() == RULE_EVENODD)
                CGContextEOClip(context);
            else
                CGContextClip(context);

            m_state.fillGradient->paint(this);
            CGContextRestoreGState(context);
        }

        return;
    }

    CGContextBeginPath(context);
    CGContextAddPath(context, path.platformPath());

    if (m_state.fillPattern)
        applyFillPattern();
    fillPathWithFillRule(context, fillRule());
}

void GraphicsContext::strokePath(const Path& path)
{
    if (paintingDisabled() || path.isEmpty())
        return;

    CGContextRef context = platformContext();

    CGContextBeginPath(context);
    CGContextAddPath(context, path.platformPath());

    if (m_state.strokeGradient) {
        if (hasShadow()) {
            FloatRect rect = path.fastBoundingRect();
            float lineWidth = strokeThickness();
            float doubleLineWidth = lineWidth * 2;
            float adjustedWidth = ceilf(rect.width() + doubleLineWidth);
            float adjustedHeight = ceilf(rect.height() + doubleLineWidth);

            FloatSize layerSize = getCTM().mapSize(FloatSize(adjustedWidth, adjustedHeight));

            CGLayerRef layer = CGLayerCreateWithContext(context, layerSize, 0);
            CGContextRef layerContext = CGLayerGetContext(layer);
            CGContextSetLineWidth(layerContext, lineWidth);

            // Compensate for the line width, otherwise the layer's top-left corner would be
            // aligned with the rect's top-left corner. This would result in leaving pixels out of
            // the layer on the left and top sides.
            float translationX = lineWidth - rect.x();
            float translationY = lineWidth - rect.y();
            CGContextScaleCTM(layerContext, layerSize.width() / adjustedWidth, layerSize.height() / adjustedHeight);
            CGContextTranslateCTM(layerContext, translationX, translationY);

            CGContextAddPath(layerContext, path.platformPath());
            CGContextReplacePathWithStrokedPath(layerContext);
            CGContextClip(layerContext);
            CGContextConcatCTM(layerContext, m_state.strokeGradient->gradientSpaceTransform());
            m_state.strokeGradient->paint(layerContext);

            float destinationX = roundf(rect.x() - lineWidth);
            float destinationY = roundf(rect.y() - lineWidth);
            CGContextDrawLayerInRect(context, CGRectMake(destinationX, destinationY, adjustedWidth, adjustedHeight), layer);
            CGLayerRelease(layer);
        } else {
            CGContextSaveGState(context);
            CGContextReplacePathWithStrokedPath(context);
            CGContextClip(context);
            CGContextConcatCTM(context, m_state.strokeGradient->gradientSpaceTransform());
            m_state.strokeGradient->paint(this);
            CGContextRestoreGState(context);
        }
        return;
    }

    if (m_state.strokePattern)
        applyStrokePattern();
    CGContextStrokePath(context);
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();

    if (m_state.fillGradient) {
        CGContextSaveGState(context);
        if (hasShadow()) {
            FloatSize layerSize = getCTM().mapSize(rect.size());

            CGLayerRef layer = CGLayerCreateWithContext(context, layerSize, 0);
            CGContextRef layerContext = CGLayerGetContext(layer);

            CGContextScaleCTM(layerContext, layerSize.width() / rect.width(), layerSize.height() / rect.height());
            CGContextTranslateCTM(layerContext, -rect.x(), -rect.y());
            CGContextAddRect(layerContext, rect);
            CGContextClip(layerContext);

            CGContextConcatCTM(layerContext, m_state.fillGradient->gradientSpaceTransform());
            m_state.fillGradient->paint(layerContext);
            CGContextDrawLayerInRect(context, rect, layer);
            CGLayerRelease(layer);
        } else {
            CGContextClipToRect(context, rect);
            CGContextConcatCTM(context, m_state.fillGradient->gradientSpaceTransform());
            m_state.fillGradient->paint(this);
        }
        CGContextRestoreGState(context);
        return;
    }

    if (m_state.fillPattern)
        applyFillPattern();

    bool drawOwnShadow = !isAcceleratedContext() && hasBlurredShadow() && !m_state.shadowsIgnoreTransforms; // Don't use ShadowBlur for canvas yet.
    if (drawOwnShadow) {
        // Turn off CG shadows.
        CGContextSaveGState(context);
        CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);

        ShadowBlur contextShadow(m_state);
        contextShadow.drawRectShadow(this, rect, RoundedRect::Radii());
    }

    CGContextFillRect(context, rect);

    if (drawOwnShadow)
        CGContextRestoreGState(context);
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();
    Color oldFillColor = fillColor();
    ColorSpace oldColorSpace = fillColorSpace();

    if (oldFillColor != color || oldColorSpace != colorSpace)
        setCGFillColor(context, color, colorSpace);

    bool drawOwnShadow = !isAcceleratedContext() && hasBlurredShadow() && !m_state.shadowsIgnoreTransforms; // Don't use ShadowBlur for canvas yet.
    if (drawOwnShadow) {
        // Turn off CG shadows.
        CGContextSaveGState(context);
        CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);

        ShadowBlur contextShadow(m_state);
        contextShadow.drawRectShadow(this, rect, RoundedRect::Radii());
    }

    CGContextFillRect(context, rect);
    
    if (drawOwnShadow)
        CGContextRestoreGState(context);

    if (oldFillColor != color || oldColorSpace != colorSpace)
        setCGFillColor(context, oldFillColor, oldColorSpace);
}

void GraphicsContext::fillRoundedRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();
    Color oldFillColor = fillColor();
    ColorSpace oldColorSpace = fillColorSpace();

    if (oldFillColor != color || oldColorSpace != colorSpace)
        setCGFillColor(context, color, colorSpace);

    bool drawOwnShadow = !isAcceleratedContext() && hasBlurredShadow() && !m_state.shadowsIgnoreTransforms; // Don't use ShadowBlur for canvas yet.
    if (drawOwnShadow) {
        // Turn off CG shadows.
        CGContextSaveGState(context);
        CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);

        ShadowBlur contextShadow(m_state);
        contextShadow.drawRectShadow(this, rect, RoundedRect::Radii(topLeft, topRight, bottomLeft, bottomRight));
    }

    bool equalWidths = (topLeft.width() == topRight.width() && topRight.width() == bottomLeft.width() && bottomLeft.width() == bottomRight.width());
    bool equalHeights = (topLeft.height() == bottomLeft.height() && bottomLeft.height() == topRight.height() && topRight.height() == bottomRight.height());
    bool hasCustomFill = m_state.fillGradient || m_state.fillPattern;
    if (!hasCustomFill && equalWidths && equalHeights && topLeft.width() * 2 == rect.width() && topLeft.height() * 2 == rect.height())
        CGContextFillEllipseInRect(context, rect);
    else {
        Path path;
        path.addRoundedRect(rect, topLeft, topRight, bottomLeft, bottomRight);
        fillPath(path);
    }

    if (drawOwnShadow)
        CGContextRestoreGState(context);

    if (oldFillColor != color || oldColorSpace != colorSpace)
        setCGFillColor(context, oldFillColor, oldColorSpace);
}

void GraphicsContext::fillRectWithRoundedHole(const IntRect& rect, const RoundedRect& roundedHoleRect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();

    Path path;
    path.addRect(rect);

    if (!roundedHoleRect.radii().isZero())
        path.addRoundedRect(roundedHoleRect);
    else
        path.addRect(roundedHoleRect.rect());

    WindRule oldFillRule = fillRule();
    Color oldFillColor = fillColor();
    ColorSpace oldFillColorSpace = fillColorSpace();
    
    setFillRule(RULE_EVENODD);
    setFillColor(color, colorSpace);

    // fillRectWithRoundedHole() assumes that the edges of rect are clipped out, so we only care about shadows cast around inside the hole.
    bool drawOwnShadow = !isAcceleratedContext() && hasBlurredShadow() && !m_state.shadowsIgnoreTransforms;
    if (drawOwnShadow) {
        // Turn off CG shadows.
        CGContextSaveGState(context);
        CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);

        ShadowBlur contextShadow(m_state);
        contextShadow.drawInsetShadow(this, rect, roundedHoleRect.rect(), roundedHoleRect.radii());
    }

    fillPath(path);

    if (drawOwnShadow)
        CGContextRestoreGState(context);
    
    setFillRule(oldFillRule);
    setFillColor(oldFillColor, oldFillColorSpace);
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;
    CGContextClipToRect(platformContext(), rect);
    m_data->clip(rect);
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    // FIXME: Using CGRectInfinite is much faster than getting the clip bounding box. However, due
    // to <rdar://problem/12584492>, CGRectInfinite can't be used with an accelerated context that
    // has certain transforms that aren't just a translation or a scale.
    const AffineTransform& ctm = getCTM();
    bool canUseCGRectInfinite = !isAcceleratedContext() || (!ctm.b() && !ctm.c());
    CGRect rects[2] = { canUseCGRectInfinite ? CGRectInfinite : CGContextGetClipBoundingBox(platformContext()), rect };
    CGContextBeginPath(platformContext());
    CGContextAddRects(platformContext(), rects, 2);
    CGContextEOClip(platformContext());
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    if (paintingDisabled())
        return;

    // Why does clipping to an empty path do nothing?
    // Why is this different from GraphicsContext::clip(const Path&).
    if (path.isEmpty())
        return;

    CGContextRef context = platformContext();

    CGContextBeginPath(platformContext());
    CGContextAddPath(platformContext(), path.platformPath());

    if (clipRule == RULE_EVENODD)
        CGContextEOClip(context);
    else
        CGContextClip(context);
}

IntRect GraphicsContext::clipBounds() const
{
    return enclosingIntRect(CGContextGetClipBoundingBox(platformContext()));
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    save();

    CGContextRef context = platformContext();
    CGContextSetAlpha(context, opacity);
    CGContextBeginTransparencyLayer(context, 0);
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;
    CGContextRef context = platformContext();
    CGContextEndTransparencyLayer(context);

    restore();
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

static void applyShadowOffsetWorkaroundIfNeeded(const GraphicsContext& context, CGFloat& xOffset, CGFloat& yOffset)
{
#if !PLATFORM(IOS)
    if (context.isAcceleratedContext())
        return;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    if (wkCGContextDrawsWithCorrectShadowOffsets(context.platformContext()))
        return;
#endif

    // Work around <rdar://problem/5539388> by ensuring that the offsets will get truncated
    // to the desired integer. Also see: <rdar://problem/10056277>
    static const CGFloat extraShadowOffset = narrowPrecisionToCGFloat(1.0 / 128);
    if (xOffset > 0)
        xOffset += extraShadowOffset;
    else if (xOffset < 0)
        xOffset -= extraShadowOffset;

    if (yOffset > 0)
        yOffset += extraShadowOffset;
    else if (yOffset < 0)
        yOffset -= extraShadowOffset;
#endif
}

void GraphicsContext::setPlatformShadow(const FloatSize& offset, float blur, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;
    
    // FIXME: we could avoid the shadow setup cost when we know we'll render the shadow ourselves.

    CGFloat xOffset = offset.width();
    CGFloat yOffset = offset.height();
    CGFloat blurRadius = blur;
    CGContextRef context = platformContext();

    if (!m_state.shadowsIgnoreTransforms) {
        CGAffineTransform userToBaseCTM = wkGetUserToBaseCTM(context);

        CGFloat A = userToBaseCTM.a * userToBaseCTM.a + userToBaseCTM.b * userToBaseCTM.b;
        CGFloat B = userToBaseCTM.a * userToBaseCTM.c + userToBaseCTM.b * userToBaseCTM.d;
        CGFloat C = B;
        CGFloat D = userToBaseCTM.c * userToBaseCTM.c + userToBaseCTM.d * userToBaseCTM.d;

        CGFloat smallEigenvalue = narrowPrecisionToCGFloat(sqrt(0.5 * ((A + D) - sqrt(4 * B * C + (A - D) * (A - D)))));

        blurRadius = blur * smallEigenvalue;

        CGSize offsetInBaseSpace = CGSizeApplyAffineTransform(offset, userToBaseCTM);

        xOffset = offsetInBaseSpace.width;
        yOffset = offsetInBaseSpace.height;
    }

    // Extreme "blur" values can make text drawing crash or take crazy long times, so clamp
    blurRadius = min(blurRadius, narrowPrecisionToCGFloat(1000.0));

    applyShadowOffsetWorkaroundIfNeeded(*this, xOffset, yOffset);

    // Check for an invalid color, as this means that the color was not set for the shadow
    // and we should therefore just use the default shadow color.
    if (!color.isValid())
        CGContextSetShadow(context, CGSizeMake(xOffset, yOffset), blurRadius);
    else
        CGContextSetShadowWithColor(context, CGSizeMake(xOffset, yOffset), blurRadius, cachedCGColor(color, colorSpace));
}

void GraphicsContext::clearPlatformShadow()
{
    if (paintingDisabled())
        return;
    CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);
}

void GraphicsContext::setMiterLimit(float limit)
{
    if (paintingDisabled())
        return;
    CGContextSetMiterLimit(platformContext(), limit);
}

void GraphicsContext::setAlpha(float alpha)
{
    if (paintingDisabled())
        return;
    CGContextSetAlpha(platformContext(), alpha);
}

void GraphicsContext::clearRect(const FloatRect& r)
{
    if (paintingDisabled())
        return;
    CGContextClearRect(platformContext(), r);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();

    if (m_state.strokeGradient) {
        if (hasShadow()) {
            const float doubleLineWidth = lineWidth * 2;
            float adjustedWidth = ceilf(rect.width() + doubleLineWidth);
            float adjustedHeight = ceilf(rect.height() + doubleLineWidth);
            FloatSize layerSize = getCTM().mapSize(FloatSize(adjustedWidth, adjustedHeight));

            CGLayerRef layer = CGLayerCreateWithContext(context, layerSize, 0);

            CGContextRef layerContext = CGLayerGetContext(layer);
            m_state.strokeThickness = lineWidth;
            CGContextSetLineWidth(layerContext, lineWidth);

            // Compensate for the line width, otherwise the layer's top-left corner would be
            // aligned with the rect's top-left corner. This would result in leaving pixels out of
            // the layer on the left and top sides.
            const float translationX = lineWidth - rect.x();
            const float translationY = lineWidth - rect.y();
            CGContextScaleCTM(layerContext, layerSize.width() / adjustedWidth, layerSize.height() / adjustedHeight);
            CGContextTranslateCTM(layerContext, translationX, translationY);

            CGContextAddRect(layerContext, rect);
            CGContextReplacePathWithStrokedPath(layerContext);
            CGContextClip(layerContext);
            CGContextConcatCTM(layerContext, m_state.strokeGradient->gradientSpaceTransform());
            m_state.strokeGradient->paint(layerContext);

            const float destinationX = roundf(rect.x() - lineWidth);
            const float destinationY = roundf(rect.y() - lineWidth);
            CGContextDrawLayerInRect(context, CGRectMake(destinationX, destinationY, adjustedWidth, adjustedHeight), layer);
            CGLayerRelease(layer);
        } else {
            CGContextSaveGState(context);
            setStrokeThickness(lineWidth);
            CGContextAddRect(context, rect);
            CGContextReplacePathWithStrokedPath(context);
            CGContextClip(context);
            CGContextConcatCTM(context, m_state.strokeGradient->gradientSpaceTransform());
            m_state.strokeGradient->paint(this);
            CGContextRestoreGState(context);
        }
        return;
    }

    if (m_state.strokePattern)
        applyStrokePattern();
    CGContextStrokeRectWithWidth(context, rect, lineWidth);
}

void GraphicsContext::setLineCap(LineCap cap)
{
    if (paintingDisabled())
        return;
    switch (cap) {
    case ButtCap:
        CGContextSetLineCap(platformContext(), kCGLineCapButt);
        break;
    case RoundCap:
        CGContextSetLineCap(platformContext(), kCGLineCapRound);
        break;
    case SquareCap:
        CGContextSetLineCap(platformContext(), kCGLineCapSquare);
        break;
    }
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    CGContextSetLineDash(platformContext(), dashOffset, dashes.data(), dashes.size());
}

void GraphicsContext::setLineJoin(LineJoin join)
{
    if (paintingDisabled())
        return;
    switch (join) {
    case MiterJoin:
        CGContextSetLineJoin(platformContext(), kCGLineJoinMiter);
        break;
    case RoundJoin:
        CGContextSetLineJoin(platformContext(), kCGLineJoinRound);
        break;
    case BevelJoin:
        CGContextSetLineJoin(platformContext(), kCGLineJoinBevel);
        break;
    }
}

void GraphicsContext::clip(const Path& path, WindRule fillRule)
{
    if (paintingDisabled())
        return;
    CGContextRef context = platformContext();

    // CGContextClip does nothing if the path is empty, so in this case, we
    // instead clip against a zero rect to reduce the clipping region to
    // nothing - which is the intended behavior of clip() if the path is empty.    
    if (path.isEmpty())
        CGContextClipToRect(context, CGRectZero);
    else {
        CGContextBeginPath(context);
        CGContextAddPath(context, path.platformPath());
        if (fillRule == RULE_NONZERO)
            CGContextClip(context);
        else
            CGContextEOClip(context);
    }
    m_data->clip(path);
}

void GraphicsContext::canvasClip(const Path& path, WindRule fillRule)
{
    clip(path, fillRule);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    CGContextBeginPath(platformContext());
    CGContextAddRect(platformContext(), CGContextGetClipBoundingBox(platformContext()));
    if (!path.isEmpty())
        CGContextAddPath(platformContext(), path.platformPath());
    CGContextEOClip(platformContext());
}

void GraphicsContext::scale(const FloatSize& size)
{
    if (paintingDisabled())
        return;
    CGContextScaleCTM(platformContext(), size.width(), size.height());
    m_data->scale(size);
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
}

void GraphicsContext::rotate(float angle)
{
    if (paintingDisabled())
        return;
    CGContextRotateCTM(platformContext(), angle);
    m_data->rotate(angle);
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
}

void GraphicsContext::translate(float x, float y)
{
    if (paintingDisabled())
        return;
    CGContextTranslateCTM(platformContext(), x, y);
    m_data->translate(x, y);
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;
    CGContextConcatCTM(platformContext(), transform);
    m_data->concatCTM(transform);
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
}

void GraphicsContext::setCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;
    CGContextSetCTM(platformContext(), transform);
    m_data->setCTM(transform);
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale includeScale) const
{
    if (paintingDisabled())
        return AffineTransform();

    // The CTM usually includes the deviceScaleFactor except in WebKit 1 when the
    // content is non-composited, since the scale factor is integrated at a lower
    // level. To guarantee the deviceScale is included, we can use this CG API.
    if (includeScale == DefinitelyIncludeDeviceScale)
        return CGContextGetUserSpaceToDeviceSpaceTransform(platformContext());

    return CGContextGetCTM(platformContext());
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& rect, RoundingMode roundingMode)
{
    // It is not enough just to round to pixels in device space. The rotation part of the
    // affine transform matrix to device space can mess with this conversion if we have a
    // rotating image like the hands of the world clock widget. We just need the scale, so
    // we get the affine transform matrix and extract the scale.

    if (m_data->m_userToDeviceTransformKnownToBeIdentity)
        return roundedIntRect(rect);

    CGAffineTransform deviceMatrix = CGContextGetUserSpaceToDeviceSpaceTransform(platformContext());
    if (CGAffineTransformIsIdentity(deviceMatrix)) {
        m_data->m_userToDeviceTransformKnownToBeIdentity = true;
        return roundedIntRect(rect);
    }

    float deviceScaleX = sqrtf(deviceMatrix.a * deviceMatrix.a + deviceMatrix.b * deviceMatrix.b);
    float deviceScaleY = sqrtf(deviceMatrix.c * deviceMatrix.c + deviceMatrix.d * deviceMatrix.d);

    CGPoint deviceOrigin = CGPointMake(rect.x() * deviceScaleX, rect.y() * deviceScaleY);
    CGPoint deviceLowerRight = CGPointMake((rect.x() + rect.width()) * deviceScaleX,
        (rect.y() + rect.height()) * deviceScaleY);

    deviceOrigin.x = roundf(deviceOrigin.x);
    deviceOrigin.y = roundf(deviceOrigin.y);
    if (roundingMode == RoundAllSides) {
        deviceLowerRight.x = roundf(deviceLowerRight.x);
        deviceLowerRight.y = roundf(deviceLowerRight.y);
    } else {
        deviceLowerRight.x = deviceOrigin.x + roundf(rect.width() * deviceScaleX);
        deviceLowerRight.y = deviceOrigin.y + roundf(rect.height() * deviceScaleY);
    }

    // Don't let the height or width round to 0 unless either was originally 0
    if (deviceOrigin.y == deviceLowerRight.y && rect.height())
        deviceLowerRight.y += 1;
    if (deviceOrigin.x == deviceLowerRight.x && rect.width())
        deviceLowerRight.x += 1;

    FloatPoint roundedOrigin = FloatPoint(deviceOrigin.x / deviceScaleX, deviceOrigin.y / deviceScaleY);
    FloatPoint roundedLowerRight = FloatPoint(deviceLowerRight.x / deviceScaleX, deviceLowerRight.y / deviceScaleY);
    return FloatRect(roundedOrigin, roundedLowerRight - roundedOrigin);
}

void GraphicsContext::drawLineForText(const FloatPoint& point, float width, bool printing)
{
    if (paintingDisabled())
        return;

    if (width <= 0)
        return;

    float x = point.x();
    float y = point.y();
    float lineLength = width;

    // Use a minimum thickness of 0.5 in user space.
    // See http://bugs.webkit.org/show_bug.cgi?id=4255 for details of why 0.5 is the right minimum thickness to use.
    float thickness = max(strokeThickness(), 0.5f);

    bool restoreAntialiasMode = false;

    if (!printing) {
        // On screen, use a minimum thickness of 1.0 in user space (later rounded to an integral number in device space).
        float adjustedThickness = max(thickness, 1.0f);

        // FIXME: This should be done a better way.
        // We try to round all parameters to integer boundaries in device space. If rounding pixels in device space
        // makes our thickness more than double, then there must be a shrinking-scale factor and rounding to pixels
        // in device space will make the underlines too thick.
        CGRect lineRect = roundToDevicePixels(FloatRect(x, y, lineLength, adjustedThickness), RoundOriginAndDimensions);
        if (lineRect.size.height < thickness * 2.0) {
            x = lineRect.origin.x;
            y = lineRect.origin.y;
            lineLength = lineRect.size.width;
            thickness = lineRect.size.height;
            if (shouldAntialias()) {
                CGContextSetShouldAntialias(platformContext(), false);
                restoreAntialiasMode = true;
            }
        }
    }

    if (fillColor() != strokeColor())
        setCGFillColor(platformContext(), strokeColor(), strokeColorSpace());
    CGContextFillRect(platformContext(), CGRectMake(x, y, lineLength, thickness));
    if (fillColor() != strokeColor())
        setCGFillColor(platformContext(), fillColor(), fillColorSpace());

    if (restoreAntialiasMode)
        CGContextSetShouldAntialias(platformContext(), true);
}

void GraphicsContext::setURLForRect(const KURL& link, const IntRect& destRect)
{
    if (paintingDisabled())
        return;

    RetainPtr<CFURLRef> urlRef = link.createCFURL();
    if (!urlRef)
        return;

    CGContextRef context = platformContext();

    // Get the bounding box to handle clipping.
    CGRect box = CGContextGetClipBoundingBox(context);

    IntRect intBox((int)box.origin.x, (int)box.origin.y, (int)box.size.width, (int)box.size.height);
    IntRect rect = destRect;
    rect.intersect(intBox);

    CGPDFContextSetURLForRect(context, urlRef.get(),
        CGRectApplyAffineTransform(rect, CGContextGetCTM(context)));
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality mode)
{
    if (paintingDisabled())
        return;

    CGInterpolationQuality quality = kCGInterpolationDefault;
    switch (mode) {
    case InterpolationDefault:
        quality = kCGInterpolationDefault;
        break;
    case InterpolationNone:
        quality = kCGInterpolationNone;
        break;
    case InterpolationLow:
        quality = kCGInterpolationLow;
        break;
    case InterpolationMedium:
        quality = kCGInterpolationMedium;
        break;
    case InterpolationHigh:
        quality = kCGInterpolationHigh;
        break;
    }
    CGContextSetInterpolationQuality(platformContext(), quality);
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    if (paintingDisabled())
        return InterpolationDefault;

    CGInterpolationQuality quality = CGContextGetInterpolationQuality(platformContext());
    switch (quality) {
    case kCGInterpolationDefault:
        return InterpolationDefault;
    case kCGInterpolationNone:
        return InterpolationNone;
    case kCGInterpolationLow:
        return InterpolationLow;
    case kCGInterpolationMedium:
        return InterpolationMedium;
    case kCGInterpolationHigh:
        return InterpolationHigh;
    }
    return InterpolationDefault;
}

void GraphicsContext::setAllowsFontSmoothing(bool allowsFontSmoothing)
{
    UNUSED_PARAM(allowsFontSmoothing);
#if PLATFORM(MAC)
    CGContextRef context = platformContext();
    CGContextSetAllowsFontSmoothing(context, allowsFontSmoothing);
#endif
}

void GraphicsContext::setIsCALayerContext(bool isLayerContext)
{
    if (isLayerContext)
        m_data->m_contextFlags |= IsLayerCGContext;
    else
        m_data->m_contextFlags &= ~IsLayerCGContext;
}

bool GraphicsContext::isCALayerContext() const
{
    return m_data->m_contextFlags & IsLayerCGContext;
}

void GraphicsContext::setIsAcceleratedContext(bool isAccelerated)
{
    if (isAccelerated)
        m_data->m_contextFlags |= IsAcceleratedCGContext;
    else
        m_data->m_contextFlags &= ~IsAcceleratedCGContext;
}

bool GraphicsContext::isAcceleratedContext() const
{
    return m_data->m_contextFlags & IsAcceleratedCGContext;
}

void GraphicsContext::setPlatformTextDrawingMode(TextDrawingModeFlags mode)
{
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();
    switch (mode) {
    case TextModeFill:
        CGContextSetTextDrawingMode(context, kCGTextFill);
        break;
    case TextModeStroke:
        CGContextSetTextDrawingMode(context, kCGTextStroke);
        break;
    case TextModeFill | TextModeStroke:
        CGContextSetTextDrawingMode(context, kCGTextFillStroke);
        break;
    default:
        break;
    }
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;
    setCGStrokeColor(platformContext(), color, colorSpace);
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;
    CGContextSetLineWidth(platformContext(), thickness);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;
    setCGFillColor(platformContext(), color, colorSpace);
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;
    CGContextSetShouldAntialias(platformContext(), enable);
}

void GraphicsContext::setPlatformShouldSmoothFonts(bool enable)
{
    if (paintingDisabled())
        return;
    CGContextSetShouldSmoothFonts(platformContext(), enable);
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator mode, BlendMode blendMode)
{
    if (paintingDisabled())
        return;

    CGBlendMode target = kCGBlendModeNormal;
    if (blendMode != BlendModeNormal) {
        switch (blendMode) {
        case BlendModeMultiply:
            target = kCGBlendModeMultiply;
            break;
        case BlendModeScreen:
            target = kCGBlendModeScreen;
            break;
        case BlendModeOverlay:
            target = kCGBlendModeOverlay;
            break;
        case BlendModeDarken:
            target = kCGBlendModeDarken;
            break;
        case BlendModeLighten:
            target = kCGBlendModeLighten;
            break;
        case BlendModeColorDodge:
            target = kCGBlendModeColorDodge;
            break;
        case BlendModeColorBurn:
            target = kCGBlendModeColorBurn;
            break;
        case BlendModeHardLight:
            target = kCGBlendModeHardLight;
            break;
        case BlendModeSoftLight:
            target = kCGBlendModeSoftLight;
            break;
        case BlendModeDifference:
            target = kCGBlendModeDifference;
            break;
        case BlendModeExclusion:
            target = kCGBlendModeExclusion;
            break;
        case BlendModeHue:
            target = kCGBlendModeHue;
            break;
        case BlendModeSaturation:
            target = kCGBlendModeSaturation;
            break;
        case BlendModeColor:
            target = kCGBlendModeColor;
            break;
        case BlendModeLuminosity:
            target = kCGBlendModeLuminosity;
        default:
            break;
        }
    } else {
        switch (mode) {
        case CompositeClear:
            target = kCGBlendModeClear;
            break;
        case CompositeCopy:
            target = kCGBlendModeCopy;
            break;
        case CompositeSourceOver:
            // kCGBlendModeNormal
            break;
        case CompositeSourceIn:
            target = kCGBlendModeSourceIn;
            break;
        case CompositeSourceOut:
            target = kCGBlendModeSourceOut;
            break;
        case CompositeSourceAtop:
            target = kCGBlendModeSourceAtop;
            break;
        case CompositeDestinationOver:
            target = kCGBlendModeDestinationOver;
            break;
        case CompositeDestinationIn:
            target = kCGBlendModeDestinationIn;
            break;
        case CompositeDestinationOut:
            target = kCGBlendModeDestinationOut;
            break;
        case CompositeDestinationAtop:
            target = kCGBlendModeDestinationAtop;
            break;
        case CompositeXOR:
            target = kCGBlendModeXOR;
            break;
        case CompositePlusDarker:
            target = kCGBlendModePlusDarker;
            break;
        case CompositePlusLighter:
            target = kCGBlendModePlusLighter;
            break;
        case CompositeDifference:
            target = kCGBlendModeDifference;
            break;
        }
    }
    CGContextSetBlendMode(platformContext(), target);
}

void GraphicsContext::platformApplyDeviceScaleFactor(float deviceScaleFactor)
{
    // CoreGraphics expects the base CTM of a HiDPI context to have the scale factor applied to it.
    // Failing to change the base level CTM will cause certain CG features, such as focus rings,
    // to draw with a scale factor of 1 rather than the actual scale factor.
    wkSetBaseCTM(platformContext(), CGAffineTransformScale(CGContextGetBaseCTM(platformContext()), deviceScaleFactor, deviceScaleFactor));
}

void GraphicsContext::platformFillEllipse(const FloatRect& ellipse)
{
    if (paintingDisabled())
        return;

    // CGContextFillEllipseInRect only supports solid colors.
    if (m_state.fillGradient || m_state.fillPattern) {
        fillEllipseAsPath(ellipse);
        return;
    }

    CGContextRef context = platformContext();
    CGContextFillEllipseInRect(context, ellipse);
}

void GraphicsContext::platformStrokeEllipse(const FloatRect& ellipse)
{
    if (paintingDisabled())
        return;

    // CGContextStrokeEllipseInRect only supports solid colors.
    if (m_state.strokeGradient || m_state.strokePattern) {
        strokeEllipseAsPath(ellipse);
        return;
    }

    CGContextRef context = platformContext();
    CGContextStrokeEllipseInRect(context, ellipse);
}

}
