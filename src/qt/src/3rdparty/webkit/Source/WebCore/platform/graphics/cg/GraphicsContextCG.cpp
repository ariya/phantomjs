/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#include "KURL.h"
#include "Path.h"
#include "Pattern.h"
#include "ShadowBlur.h"

#include <CoreGraphics/CoreGraphics.h>
#include <wtf/MathExtras.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/UnusedParam.h>

#if PLATFORM(MAC) || PLATFORM(CHROMIUM)
#include "WebCoreSystemInterface.h"
#endif

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

#if PLATFORM(MAC) || (PLATFORM(CHROMIUM) && OS(DARWIN))

#ifndef BUILDING_ON_LEOPARD
// Building on 10.6 or later: kCGInterpolationMedium is defined in the CGInterpolationQuality enum.
#define HAVE_CG_INTERPOLATION_MEDIUM 1
#endif

#ifndef TARGETING_LEOPARD
// Targeting 10.6 or later: use kCGInterpolationMedium.
#define WTF_USE_CG_INTERPOLATION_MEDIUM 1
#endif

#endif

// Undocumented CGContextSetCTM function, available at least since 10.4.
extern "C" {
    CG_EXTERN void CGContextSetCTM(CGContextRef, CGAffineTransform);
};

using namespace std;

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

CGColorSpaceRef linearRGBColorSpaceRef()
{
    // FIXME: Windows should be able to use kCGColorSpaceGenericRGBLinear, this is tracked by http://webkit.org/b/31363.
#if PLATFORM(WIN)
    return deviceRGBColorSpaceRef();
#else
    static CGColorSpaceRef linearRGBSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGBLinear);
    return linearRGBSpace;
#endif
}

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

// Draws a filled rectangle with a stroked border.
void GraphicsContext::drawRect(const IntRect& rect)
{
    // FIXME: this function does not handle patterns and gradients
    // like drawPath does, it probably should.
    if (paintingDisabled())
        return;

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

        const CGFloat dottedLine[2] = { patWidth, patWidth };
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


void GraphicsContext::strokeArc(const IntRect& rect, int startAngle, int angleSpan)
{
    if (paintingDisabled() || strokeStyle() == NoStroke || strokeThickness() <= 0.0f)
        return;

    CGContextRef context = platformContext();
    CGContextSaveGState(context);
    CGContextBeginPath(context);
    CGContextSetShouldAntialias(context, false);

    int x = rect.x();
    int y = rect.y();
    float w = (float)rect.width();
    float h = (float)rect.height();
    float scaleFactor = h / w;
    float reverseScaleFactor = w / h;

    if (w != h)
        scale(FloatSize(1, scaleFactor));

    float hRadius = w / 2;
    float vRadius = h / 2;
    float fa = startAngle;
    float falen =  fa + angleSpan;
    float start = -fa * piFloat / 180.0f;
    float end = -falen * piFloat / 180.0f;
    CGContextAddArc(context, x + hRadius, (y + vRadius) * reverseScaleFactor, hRadius, start, end, true);

    if (w != h)
        scale(FloatSize(1, reverseScaleFactor));

    float width = strokeThickness();
    int patWidth = 0;

    switch (strokeStyle()) {
    case DottedStroke:
        patWidth = (int)(width / 2);
        break;
    case DashedStroke:
        patWidth = 3 * (int)(width / 2);
        break;
    default:
        break;
    }

    if (patWidth) {
        // Example: 80 pixels with a width of 30 pixels.
        // Remainder is 20.  The maximum pixels of line we could paint
        // will be 50 pixels.
        int distance;
        if (hRadius == vRadius)
            distance = static_cast<int>((piFloat * hRadius) / 2.0f);
        else // We are elliptical and will have to estimate the distance
            distance = static_cast<int>((piFloat * sqrtf((hRadius * hRadius + vRadius * vRadius) / 2.0f)) / 2.0f);

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
                    patternOffset += remainder / 2.0f;
                } else
                    patternOffset = patWidth / 2.0f;
            } else {
                if (remainder)
                    patternOffset = (patWidth - remainder) / 2.0f;
            }
        }

        const CGFloat dottedLine[2] = { patWidth, patWidth };
        CGContextSetLineDash(context, patternOffset, dottedLine, 2);
    }

    CGContextStrokePath(context);

    CGContextRestoreGState(context);
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

    RetainPtr<CGPatternRef> platformPattern(AdoptCF, m_state.strokePattern->createPlatformPattern(getCTM()));
    if (!platformPattern)
        return;

    RetainPtr<CGColorSpaceRef> patternSpace(AdoptCF, CGColorSpaceCreatePattern(0));
    CGContextSetStrokeColorSpace(cgContext, patternSpace.get());

    const CGFloat patternAlpha = 1;
    CGContextSetStrokePattern(cgContext, platformPattern.get(), &patternAlpha);
}

void GraphicsContext::applyFillPattern()
{
    CGContextRef cgContext = platformContext();

    RetainPtr<CGPatternRef> platformPattern(AdoptCF, m_state.fillPattern->createPlatformPattern(getCTM()));
    if (!platformPattern)
        return;

    RetainPtr<CGColorSpaceRef> patternSpace(AdoptCF, CGColorSpaceCreatePattern(0));
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
    if (paintingDisabled())
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
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();

    if (m_state.fillGradient) {
        if (hasShadow()) {
            FloatRect rect = path.boundingRect();
            CGLayerRef layer = CGLayerCreateWithContext(context, CGSizeMake(rect.width(), rect.height()), 0);
            CGContextRef layerContext = CGLayerGetContext(layer);

            CGContextTranslateCTM(layerContext, -rect.x(), -rect.y());
            CGContextBeginPath(layerContext);
            CGContextAddPath(layerContext, path.platformPath());
            CGContextConcatCTM(layerContext, m_state.fillGradient->gradientSpaceTransform());

            if (fillRule() == RULE_EVENODD)
                CGContextEOClip(layerContext);
            else
                CGContextClip(layerContext);

            m_state.fillGradient->paint(layerContext);
            CGContextDrawLayerAtPoint(context, CGPointMake(rect.x(), rect.y()), layer);
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
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();

    CGContextBeginPath(context);
    CGContextAddPath(context, path.platformPath());

    if (m_state.strokeGradient) {
        if (hasShadow()) {
            FloatRect rect = path.boundingRect();
            float lineWidth = strokeThickness();
            float doubleLineWidth = lineWidth * 2;
            float layerWidth = ceilf(rect.width() + doubleLineWidth);
            float layerHeight = ceilf(rect.height() + doubleLineWidth);

            CGLayerRef layer = CGLayerCreateWithContext(context, CGSizeMake(layerWidth, layerHeight), 0);
            CGContextRef layerContext = CGLayerGetContext(layer);
            CGContextSetLineWidth(layerContext, lineWidth);

            // Compensate for the line width, otherwise the layer's top-left corner would be
            // aligned with the rect's top-left corner. This would result in leaving pixels out of
            // the layer on the left and top sides.
            float translationX = lineWidth - rect.x();
            float translationY = lineWidth - rect.y();
            CGContextTranslateCTM(layerContext, translationX, translationY);

            CGContextAddPath(layerContext, path.platformPath());
            CGContextReplacePathWithStrokedPath(layerContext);
            CGContextClip(layerContext);
            CGContextConcatCTM(layerContext, m_state.strokeGradient->gradientSpaceTransform());
            m_state.strokeGradient->paint(layerContext);

            float destinationX = roundf(rect.x() - lineWidth);
            float destinationY = roundf(rect.y() - lineWidth);
            CGContextDrawLayerAtPoint(context, CGPointMake(destinationX, destinationY), layer);
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

static float radiusToLegacyRadius(float radius)
{
    return radius > 8 ? 8 + 4 * sqrt((radius - 8) / 2) : radius;
}

static bool hasBlurredShadow(const GraphicsContextState& state)
{
    return state.shadowColor.isValid() && state.shadowColor.alpha() && state.shadowBlur;
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    CGContextRef context = platformContext();

    if (m_state.fillGradient) {
        CGContextSaveGState(context);
        if (hasShadow()) {
            CGLayerRef layer = CGLayerCreateWithContext(context, CGSizeMake(rect.width(), rect.height()), 0);
            CGContextRef layerContext = CGLayerGetContext(layer);

            CGContextTranslateCTM(layerContext, -rect.x(), -rect.y());
            CGContextAddRect(layerContext, rect);
            CGContextClip(layerContext);

            CGContextConcatCTM(layerContext, m_state.fillGradient->gradientSpaceTransform());
            m_state.fillGradient->paint(layerContext);
            CGContextDrawLayerAtPoint(context, CGPointMake(rect.x(), rect.y()), layer);
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

    bool drawOwnShadow = !isAcceleratedContext() && hasBlurredShadow(m_state) && !m_state.shadowsIgnoreTransforms; // Don't use ShadowBlur for canvas yet.
    if (drawOwnShadow) {
        float shadowBlur = m_state.shadowsUseLegacyRadius ? radiusToLegacyRadius(m_state.shadowBlur) : m_state.shadowBlur;
        // Turn off CG shadows.
        CGContextSaveGState(context);
        CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);

        ShadowBlur contextShadow(FloatSize(shadowBlur, shadowBlur), m_state.shadowOffset, m_state.shadowColor, m_state.shadowColorSpace);
        contextShadow.drawRectShadow(this, rect, RoundedIntRect::Radii());
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

    bool drawOwnShadow = !isAcceleratedContext() && hasBlurredShadow(m_state) && !m_state.shadowsIgnoreTransforms; // Don't use ShadowBlur for canvas yet.
    if (drawOwnShadow) {
        float shadowBlur = m_state.shadowsUseLegacyRadius ? radiusToLegacyRadius(m_state.shadowBlur) : m_state.shadowBlur;
        // Turn off CG shadows.
        CGContextSaveGState(context);
        CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);

        ShadowBlur contextShadow(FloatSize(shadowBlur, shadowBlur), m_state.shadowOffset, m_state.shadowColor, m_state.shadowColorSpace);
        contextShadow.drawRectShadow(this, rect, RoundedIntRect::Radii());
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

    bool drawOwnShadow = !isAcceleratedContext() && hasBlurredShadow(m_state) && !m_state.shadowsIgnoreTransforms; // Don't use ShadowBlur for canvas yet.
    if (drawOwnShadow) {
        float shadowBlur = m_state.shadowsUseLegacyRadius ? radiusToLegacyRadius(m_state.shadowBlur) : m_state.shadowBlur;

        // Turn off CG shadows.
        CGContextSaveGState(context);
        CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);

        ShadowBlur contextShadow(FloatSize(shadowBlur, shadowBlur), m_state.shadowOffset, m_state.shadowColor, m_state.shadowColorSpace);
        contextShadow.drawRectShadow(this, rect, RoundedIntRect::Radii(topLeft, topRight, bottomLeft, bottomRight));
    }

    bool equalWidths = (topLeft.width() == topRight.width() && topRight.width() == bottomLeft.width() && bottomLeft.width() == bottomRight.width());
    bool equalHeights = (topLeft.height() == bottomLeft.height() && bottomLeft.height() == topRight.height() && topRight.height() == bottomRight.height());
    if (equalWidths && equalHeights && topLeft.width() * 2 == rect.width() && topLeft.height() * 2 == rect.height())
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

void GraphicsContext::fillRectWithRoundedHole(const IntRect& rect, const RoundedIntRect& roundedHoleRect, const Color& color, ColorSpace colorSpace)
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
    bool drawOwnShadow = !isAcceleratedContext() && hasBlurredShadow(m_state) && !m_state.shadowsIgnoreTransforms;
    if (drawOwnShadow) {
        float shadowBlur = m_state.shadowsUseLegacyRadius ? radiusToLegacyRadius(m_state.shadowBlur) : m_state.shadowBlur;

        // Turn off CG shadows.
        CGContextSaveGState(context);
        CGContextSetShadowWithColor(platformContext(), CGSizeZero, 0, 0);

        ShadowBlur contextShadow(FloatSize(shadowBlur, shadowBlur), m_state.shadowOffset, m_state.shadowColor, m_state.shadowColorSpace);
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

    CGRect rects[2] = { CGContextGetClipBoundingBox(platformContext()), rect };
    CGContextBeginPath(platformContext());
    CGContextAddRects(platformContext(), rects, 2);
    CGContextEOClip(platformContext());
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    if (paintingDisabled())
        return;

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

void GraphicsContext::addInnerRoundedRectClip(const IntRect& rect, int thickness)
{
    if (paintingDisabled())
        return;

    clip(rect);
    CGContextRef context = platformContext();

    // Add outer ellipse
    CGContextAddEllipseInRect(context, CGRectMake(rect.x(), rect.y(), rect.width(), rect.height()));
    // Add inner ellipse.
    CGContextAddEllipseInRect(context, CGRectMake(rect.x() + thickness, rect.y() + thickness,
        rect.width() - (thickness * 2), rect.height() - (thickness * 2)));

    CGContextEOClip(context);
}

void GraphicsContext::beginTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;
    CGContextRef context = platformContext();
    CGContextSaveGState(context);
    CGContextSetAlpha(context, opacity);
    CGContextBeginTransparencyLayer(context, 0);
    m_data->beginTransparencyLayer();
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
}

void GraphicsContext::endTransparencyLayer()
{
    if (paintingDisabled())
        return;
    CGContextRef context = platformContext();
    CGContextEndTransparencyLayer(context);
    CGContextRestoreGState(context);
    m_data->endTransparencyLayer();
    m_data->m_userToDeviceTransformKnownToBeIdentity = false;
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

    // Work around <rdar://problem/5539388> by ensuring that the offsets will get truncated
    // to the desired integer.
    static const CGFloat extraShadowOffset = narrowPrecisionToCGFloat(1.0 / 128);
    if (xOffset > 0)
        xOffset += extraShadowOffset;
    else if (xOffset < 0)
        xOffset -= extraShadowOffset;

    if (yOffset > 0)
        yOffset += extraShadowOffset;
    else if (yOffset < 0)
        yOffset -= extraShadowOffset;

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
            const float layerWidth = ceilf(rect.width() + doubleLineWidth);
            const float layerHeight = ceilf(rect.height() + doubleLineWidth);
            CGLayerRef layer = CGLayerCreateWithContext(context, CGSizeMake(layerWidth, layerHeight), 0);

            CGContextRef layerContext = CGLayerGetContext(layer);
            m_state.strokeThickness = lineWidth;
            CGContextSetLineWidth(layerContext, lineWidth);

            // Compensate for the line width, otherwise the layer's top-left corner would be
            // aligned with the rect's top-left corner. This would result in leaving pixels out of
            // the layer on the left and top sides.
            const float translationX = lineWidth - rect.x();
            const float translationY = lineWidth - rect.y();
            CGContextTranslateCTM(layerContext, translationX, translationY);

            CGContextAddRect(layerContext, rect);
            CGContextReplacePathWithStrokedPath(layerContext);
            CGContextClip(layerContext);
            CGContextConcatCTM(layerContext, m_state.strokeGradient->gradientSpaceTransform());
            m_state.strokeGradient->paint(layerContext);

            const float destinationX = roundf(rect.x() - lineWidth);
            const float destinationY = roundf(rect.y() - lineWidth);
            CGContextDrawLayerAtPoint(context, CGPointMake(destinationX, destinationY), layer);
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

void GraphicsContext::clip(const Path& path)
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
        CGContextClip(context);
    }
    m_data->clip(path);
}

void GraphicsContext::canvasClip(const Path& path)
{
    clip(path);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    CGContextBeginPath(platformContext());
    CGContextAddRect(platformContext(), CGContextGetClipBoundingBox(platformContext()));
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

AffineTransform GraphicsContext::getCTM() const
{
    CGAffineTransform t = CGContextGetCTM(platformContext());
    return AffineTransform(t.a, t.b, t.c, t.d, t.tx, t.ty);
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& rect, RoundingMode roundingMode)
{
#if PLATFORM(CHROMIUM)
    return rect;
#else
    // It is not enough just to round to pixels in device space. The rotation part of the
    // affine transform matrix to device space can mess with this conversion if we have a
    // rotating image like the hands of the world clock widget. We just need the scale, so
    // we get the affine transform matrix and extract the scale.

    if (m_data->m_userToDeviceTransformKnownToBeIdentity)
        return rect;

    CGAffineTransform deviceMatrix = CGContextGetUserSpaceToDeviceSpaceTransform(platformContext());
    if (CGAffineTransformIsIdentity(deviceMatrix)) {
        m_data->m_userToDeviceTransformKnownToBeIdentity = true;
        return rect;
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
#endif
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

    RetainPtr<CFURLRef> urlRef(AdoptCF, link.createCFURL());
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

    // Fall through to InterpolationHigh if kCGInterpolationMedium is not usable.
    case InterpolationMedium:
#if USE(CG_INTERPOLATION_MEDIUM)
        quality = kCGInterpolationMedium;
        break;
#endif
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
#if HAVE(CG_INTERPOLATION_MEDIUM)
    // kCGInterpolationMedium is known to be present in the CGInterpolationQuality enum.
    case kCGInterpolationMedium:
#if USE(CG_INTERPOLATION_MEDIUM)
        // Only map to InterpolationMedium if targeting a system that understands it.
        return InterpolationMedium;
#else
        return InterpolationDefault;
#endif  // USE(CG_INTERPOLATION_MEDIUM)
#endif  // HAVE(CG_INTERPOLATION_MEDIUM)
    case kCGInterpolationHigh:
        return InterpolationHigh;
    }
    return InterpolationDefault;
}

void GraphicsContext::setAllowsFontSmoothing(bool allowsFontSmoothing)
{
    UNUSED_PARAM(allowsFontSmoothing);
#if !defined(BUILDING_ON_LEOPARD)
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

    // Wow, wish CG had used bits here.
    CGContextRef context = platformContext();
    switch (mode) {
    case TextModeInvisible:
        CGContextSetTextDrawingMode(context, kCGTextInvisible);
        break;
    case TextModeFill:
        CGContextSetTextDrawingMode(context, kCGTextFill);
        break;
    case TextModeStroke:
        CGContextSetTextDrawingMode(context, kCGTextStroke);
        break;
    case TextModeFill | TextModeStroke:
        CGContextSetTextDrawingMode(context, kCGTextFillStroke);
        break;
    case TextModeClip:
        CGContextSetTextDrawingMode(context, kCGTextClip);
        break;
    case TextModeFill | TextModeClip:
        CGContextSetTextDrawingMode(context, kCGTextFillClip);
        break;
    case TextModeStroke | TextModeClip:
        CGContextSetTextDrawingMode(context, kCGTextStrokeClip);
        break;
    case TextModeFill | TextModeStroke | TextModeClip:
        CGContextSetTextDrawingMode(context, kCGTextFillStrokeClip);
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

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator mode)
{
    if (paintingDisabled())
        return;

    CGBlendMode target = kCGBlendModeNormal;
    switch (mode) {
    case CompositeClear:
        target = kCGBlendModeClear;
        break;
    case CompositeCopy:
        target = kCGBlendModeCopy;
        break;
    case CompositeSourceOver:
        //kCGBlendModeNormal
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
    case CompositeHighlight:
        // currently unsupported
        break;
    case CompositePlusLighter:
        target = kCGBlendModePlusLighter;
        break;
    }
    CGContextSetBlendMode(platformContext(), target);
}

}
