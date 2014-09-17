/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "CSSGradientValue.h"

#include "CSSValueKeywords.h"
#include "CSSStyleSelector.h"
#include "GeneratedImage.h"
#include "Gradient.h"
#include "Image.h"
#include "IntSize.h"
#include "IntSizeHash.h"
#include "NodeRenderStyle.h"
#include "PlatformString.h"
#include "RenderObject.h"

using namespace std;

namespace WebCore {

PassRefPtr<Image> CSSGradientValue::image(RenderObject* renderer, const IntSize& size)
{
    if (size.isEmpty())
        return 0;

    bool cacheable = isCacheable();
    if (cacheable) {
        if (!m_clients.contains(renderer))
            return 0;

        // Need to look up our size.  Create a string of width*height to use as a hash key.
        Image* result = getImage(renderer, size);
        if (result)
            return result;
    }
    
    // We need to create an image.
    RefPtr<Image> newImage = GeneratedImage::create(createGradient(renderer, size), size);
    if (cacheable)
        putImage(size, newImage);

    return newImage.release();
}

// Should only ever be called for deprecated gradients.
static inline bool compareStops(const CSSGradientColorStop& a, const CSSGradientColorStop& b)
{
    double aVal = a.m_position->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER);
    double bVal = b.m_position->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER);
    
    return aVal < bVal;
}

void CSSGradientValue::sortStopsIfNeeded()
{
    ASSERT(m_deprecatedType);
    if (!m_stopsSorted) {
        if (m_stops.size())
            std::stable_sort(m_stops.begin(), m_stops.end(), compareStops);
        m_stopsSorted = true;
    }
}

static inline int blend(int from, int to, float progress)
{  
    return int(from + (to - from) * progress);
}

static inline Color blend(const Color& from, const Color& to, float progress)
{
    // FIXME: when we interpolate gradients using premultiplied colors, this should also do premultiplication.
    return Color(blend(from.red(), to.red(), progress),
        blend(from.green(), to.green(), progress),
        blend(from.blue(), to.blue(), progress),
        blend(from.alpha(), to.alpha(), progress));
}

struct GradientStop {
    Color color;
    float offset;
    bool specified;
    
    GradientStop()
        : offset(0)
        , specified(false)
    { }
};

void CSSGradientValue::addStops(Gradient* gradient, RenderObject* renderer, RenderStyle* rootStyle, float maxLengthForRepeat)
{
    RenderStyle* style = renderer->style();
    
    if (m_deprecatedType) {
        sortStopsIfNeeded();

        // We have to resolve colors.
        for (unsigned i = 0; i < m_stops.size(); i++) {
            const CSSGradientColorStop& stop = m_stops[i];
            Color color = renderer->document()->styleSelector()->getColorFromPrimitiveValue(stop.m_color.get());

            float offset;
            if (stop.m_position->primitiveType() == CSSPrimitiveValue::CSS_PERCENTAGE)
                offset = stop.m_position->getFloatValue(CSSPrimitiveValue::CSS_PERCENTAGE) / 100;
            else
                offset = stop.m_position->getFloatValue(CSSPrimitiveValue::CSS_NUMBER);
            
            gradient->addColorStop(offset, color);
        }

        // The back end already sorted the stops.
        gradient->setStopsSorted(true);
        return;
    }

    size_t numStops = m_stops.size();
    
    Vector<GradientStop> stops(numStops);
    
    float gradientLength = 0;
    bool computedGradientLength = false;
    
    FloatPoint gradientStart = gradient->p0();
    FloatPoint gradientEnd;
    if (isLinearGradient())
        gradientEnd = gradient->p1();
    else if (isRadialGradient())
        gradientEnd = gradientStart + FloatSize(gradient->endRadius(), 0);
        
    for (size_t i = 0; i < numStops; ++i) {
        const CSSGradientColorStop& stop = m_stops[i];

        stops[i].color = renderer->document()->styleSelector()->getColorFromPrimitiveValue(stop.m_color.get());

        if (stop.m_position) {
            int type = stop.m_position->primitiveType();
            if (type == CSSPrimitiveValue::CSS_PERCENTAGE)
                stops[i].offset = stop.m_position->getFloatValue(CSSPrimitiveValue::CSS_PERCENTAGE) / 100;
            else if (CSSPrimitiveValue::isUnitTypeLength(type)) {
                float length = stop.m_position->computeLengthFloat(style, rootStyle, style->effectiveZoom());
                if (!computedGradientLength) {
                    FloatSize gradientSize(gradientStart - gradientEnd);
                    gradientLength = gradientSize.diagonalLength();
                }
                stops[i].offset = (gradientLength > 0) ? length / gradientLength : 0;
            } else {
                ASSERT_NOT_REACHED();
                stops[i].offset = 0;
            }
            stops[i].specified = true;
        } else {
            // If the first color-stop does not have a position, its position defaults to 0%.
            // If the last color-stop does not have a position, its position defaults to 100%.
            if (!i) {
                stops[i].offset = 0;
                stops[i].specified = true;
            } else if (numStops > 1 && i == numStops - 1) {
                stops[i].offset = 1;
                stops[i].specified = true;
            }
        }

        // If a color-stop has a position that is less than the specified position of any
        // color-stop before it in the list, its position is changed to be equal to the
        // largest specified position of any color-stop before it.
        if (stops[i].specified && i > 0) {
            size_t prevSpecifiedIndex;
            for (prevSpecifiedIndex = i - 1; prevSpecifiedIndex; --prevSpecifiedIndex) {
                if (stops[prevSpecifiedIndex].specified)
                    break;
            }
            
            if (stops[i].offset < stops[prevSpecifiedIndex].offset)
                stops[i].offset = stops[prevSpecifiedIndex].offset;
        }
    }

    ASSERT(stops[0].specified && stops[numStops - 1].specified);
    
    // If any color-stop still does not have a position, then, for each run of adjacent
    // color-stops without positions, set their positions so that they are evenly spaced
    // between the preceding and following color-stops with positions.
    if (numStops > 2) {
        size_t unspecifiedRunStart = 0;
        bool inUnspecifiedRun = false;

        for (size_t i = 0; i < numStops; ++i) {
            if (!stops[i].specified && !inUnspecifiedRun) {
                unspecifiedRunStart = i;
                inUnspecifiedRun = true;
            } else if (stops[i].specified && inUnspecifiedRun) {
                size_t unspecifiedRunEnd = i;

                if (unspecifiedRunStart < unspecifiedRunEnd) {
                    float lastSpecifiedOffset = stops[unspecifiedRunStart - 1].offset;
                    float nextSpecifiedOffset = stops[unspecifiedRunEnd].offset;
                    float delta = (nextSpecifiedOffset - lastSpecifiedOffset) / (unspecifiedRunEnd - unspecifiedRunStart + 1);
                    
                    for (size_t j = unspecifiedRunStart; j < unspecifiedRunEnd; ++j)
                        stops[j].offset = lastSpecifiedOffset + (j - unspecifiedRunStart + 1) * delta;
                }

                inUnspecifiedRun = false;
            }
        }
    }

    // If the gradient is repeating, repeat the color stops.
    // We can't just push this logic down into the platform-specific Gradient code,
    // because we have to know the extent of the gradient, and possible move the end points.
    if (m_repeating && numStops > 1) {
        // If the difference in the positions of the first and last color-stops is 0,
        // the gradient defines a solid-color image with the color of the last color-stop in the rule.
        float gradientRange = stops[numStops - 1].offset - stops[0].offset;
        if (!gradientRange) {
            stops.first().offset = 0;
            stops.first().color = stops.last().color;
            stops.shrink(1);
            numStops = 1;
        } else {
            float maxExtent = 1;

            // Radial gradients may need to extend further than the endpoints, because they have
            // to repeat out to the corners of the box.
            if (isRadialGradient()) {
                if (!computedGradientLength) {
                    FloatSize gradientSize(gradientStart - gradientEnd);
                    gradientLength = gradientSize.diagonalLength();
                }
                
                if (maxLengthForRepeat > gradientLength)
                    maxExtent = maxLengthForRepeat / gradientLength;
            }

            size_t originalNumStops = numStops;
            size_t originalFirstStopIndex = 0;

            // Work backwards from the first, adding stops until we get one before 0.
            float firstOffset = stops[0].offset;
            if (firstOffset > 0) {
                float currOffset = firstOffset;
                size_t srcStopOrdinal = originalNumStops - 1;
                
                while (true) {
                    GradientStop newStop = stops[originalFirstStopIndex + srcStopOrdinal];
                    newStop.offset = currOffset;
                    stops.prepend(newStop);
                    ++originalFirstStopIndex;
                    if (currOffset < 0)
                        break;

                    if (srcStopOrdinal)
                        currOffset -= stops[originalFirstStopIndex + srcStopOrdinal].offset - stops[originalFirstStopIndex + srcStopOrdinal - 1].offset;
                    srcStopOrdinal = (srcStopOrdinal + originalNumStops - 1) % originalNumStops;
                }
            }
            
            // Work forwards from the end, adding stops until we get one after 1.
            float lastOffset = stops[stops.size() - 1].offset;
            if (lastOffset < maxExtent) {
                float currOffset = lastOffset;
                size_t srcStopOrdinal = originalFirstStopIndex;

                while (true) {
                    GradientStop newStop = stops[srcStopOrdinal];
                    newStop.offset = currOffset;
                    stops.append(newStop);
                    if (currOffset > maxExtent)
                        break;
                    if (srcStopOrdinal < originalNumStops - 1)
                        currOffset += stops[srcStopOrdinal + 1].offset - stops[srcStopOrdinal].offset;
                    srcStopOrdinal = (srcStopOrdinal + 1) % originalNumStops;
                }
            }
        }
    }
    
    numStops = stops.size();
    
    // If the gradient goes outside the 0-1 range, normalize it by moving the endpoints, and adjusting the stops.
    if (numStops > 1 && (stops[0].offset < 0 || stops[numStops - 1].offset > 1)) {
        if (isLinearGradient()) {
            float firstOffset = stops[0].offset;
            float lastOffset = stops[numStops - 1].offset;
            float scale = lastOffset - firstOffset;

            for (size_t i = 0; i < numStops; ++i)
                stops[i].offset = (stops[i].offset - firstOffset) / scale;

            FloatPoint p0 = gradient->p0();
            FloatPoint p1 = gradient->p1();
            gradient->setP0(FloatPoint(p0.x() + firstOffset * (p1.x() - p0.x()), p0.y() + firstOffset * (p1.y() - p0.y())));
            gradient->setP1(FloatPoint(p1.x() + (lastOffset - 1) * (p1.x() - p0.x()), p1.y() + (lastOffset - 1) * (p1.y() - p0.y())));
        } else if (isRadialGradient()) {
            // Rather than scaling the points < 0, we truncate them, so only scale according to the largest point.
            float firstOffset = 0;
            float lastOffset = stops[numStops - 1].offset;
            float scale = lastOffset - firstOffset;

            // Reset points below 0 to the first visible color.
            size_t firstZeroOrGreaterIndex = numStops;
            for (size_t i = 0; i < numStops; ++i) {
                if (stops[i].offset >= 0) {
                    firstZeroOrGreaterIndex = i;
                    break;
                }
            }
            
            if (firstZeroOrGreaterIndex > 0) {
                if (firstZeroOrGreaterIndex < numStops && stops[firstZeroOrGreaterIndex].offset > 0) {
                    float prevOffset = stops[firstZeroOrGreaterIndex - 1].offset;
                    float nextOffset = stops[firstZeroOrGreaterIndex].offset;
                    
                    float interStopProportion = -prevOffset / (nextOffset - prevOffset);
                    Color blendedColor = blend(stops[firstZeroOrGreaterIndex - 1].color, stops[firstZeroOrGreaterIndex].color, interStopProportion);

                    // Clamp the positions to 0 and set the color.
                    for (size_t i = 0; i < firstZeroOrGreaterIndex; ++i) {
                        stops[i].offset = 0;
                        stops[i].color = blendedColor;
                    }
                } else {
                    // All stops are below 0; just clamp them.
                    for (size_t i = 0; i < firstZeroOrGreaterIndex; ++i)
                        stops[i].offset = 0;
                }
            }
            
            for (size_t i = 0; i < numStops; ++i)
                stops[i].offset /= scale;
            
            gradient->setStartRadius(gradient->startRadius() * scale);
            gradient->setEndRadius(gradient->endRadius() * scale);
        }
    }
    
    for (unsigned i = 0; i < numStops; i++)
        gradient->addColorStop(stops[i].offset, stops[i].color);

    gradient->setStopsSorted(true);
}

static float positionFromValue(CSSPrimitiveValue* value, RenderStyle* style, RenderStyle* rootStyle, const IntSize& size, bool isHorizontal)
{
    float zoomFactor = style->effectiveZoom();

    switch (value->primitiveType()) {
    case CSSPrimitiveValue::CSS_NUMBER:
        return value->getFloatValue() * zoomFactor;

    case CSSPrimitiveValue::CSS_PERCENTAGE:
        return value->getFloatValue() / 100.f * (isHorizontal ? size.width() : size.height());

    case CSSPrimitiveValue::CSS_IDENT:
        switch (value->getIdent()) {
            case CSSValueTop:
                ASSERT(!isHorizontal);
                return 0;
            case CSSValueLeft:
                ASSERT(isHorizontal);
                return 0;
            case CSSValueBottom:
                ASSERT(!isHorizontal);
                return size.height();
            case CSSValueRight:
                ASSERT(isHorizontal);
                return size.width();
        }

    default:
        return value->computeLengthFloat(style, rootStyle, zoomFactor);
    }
}

FloatPoint CSSGradientValue::computeEndPoint(CSSPrimitiveValue* first, CSSPrimitiveValue* second, RenderStyle* style, RenderStyle* rootStyle, const IntSize& size)
{
    FloatPoint result;

    if (first)
        result.setX(positionFromValue(first, style, rootStyle, size, true));

    if (second)
        result.setY(positionFromValue(second, style, rootStyle, size, false));
        
    return result;
}

bool CSSGradientValue::isCacheable() const
{
    for (size_t i = 0; i < m_stops.size(); ++i) {
        const CSSGradientColorStop& stop = m_stops[i];
        if (!stop.m_position)
            continue;

        unsigned short unitType = stop.m_position->primitiveType();
        if (unitType == CSSPrimitiveValue::CSS_EMS || unitType == CSSPrimitiveValue::CSS_EXS || unitType == CSSPrimitiveValue::CSS_REMS)
            return false;
    }
    
    return true;
}

String CSSLinearGradientValue::cssText() const
{
    String result;
    if (m_deprecatedType) {
        result = "-webkit-gradient(linear, ";
        result += m_firstX->cssText() + " ";
        result += m_firstY->cssText() + ", ";
        result += m_secondX->cssText() + " ";
        result += m_secondY->cssText();

        for (unsigned i = 0; i < m_stops.size(); i++) {
            const CSSGradientColorStop& stop = m_stops[i];
            result += ", ";
            if (stop.m_position->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER) == 0)
                result += "from(" + stop.m_color->cssText() + ")";
            else if (stop.m_position->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER) == 1)
                result += "to(" + stop.m_color->cssText() + ")";
            else
                result += "color-stop(" + String::number(stop.m_position->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER)) + ", " + stop.m_color->cssText() + ")";
        }
    } else {
        result = m_repeating ? "-webkit-repeating-linear-gradient(" : "-webkit-linear-gradient(";
        if (m_angle)
            result += m_angle->cssText();
        else {
            if (m_firstX && m_firstY)
                result += m_firstX->cssText() + " " + m_firstY->cssText();
            else if (m_firstX || m_firstY) {
                if (m_firstX)
                    result += m_firstX->cssText();

                if (m_firstY)
                    result += m_firstY->cssText();
            }
        }

        for (unsigned i = 0; i < m_stops.size(); i++) {
            const CSSGradientColorStop& stop = m_stops[i];
            result += ", ";
            result += stop.m_color->cssText();
            if (stop.m_position)
                result += " " + stop.m_position->cssText();
        }
    }

    result += ")";
    return result;
}

// Compute the endpoints so that a gradient of the given angle covers a box of the given size.
static void endPointsFromAngle(float angleDeg, const IntSize& size, FloatPoint& firstPoint, FloatPoint& secondPoint)
{
    angleDeg = fmodf(angleDeg, 360);
    if (angleDeg < 0)
        angleDeg += 360;
    
    if (!angleDeg) {
        firstPoint.set(0, 0);
        secondPoint.set(size.width(), 0);
        return;
    }
    
    if (angleDeg == 90) {
        firstPoint.set(0, size.height());
        secondPoint.set(0, 0);
        return;
    }

    if (angleDeg == 180) {
        firstPoint.set(size.width(), 0);
        secondPoint.set(0, 0);
        return;
    }

    float slope = tan(deg2rad(angleDeg));

    // We find the endpoint by computing the intersection of the line formed by the slope,
    // and a line perpendicular to it that intersects the corner.
    float perpendicularSlope = -1 / slope;
    
    // Compute start corner relative to center.
    float halfHeight = size.height() / 2;
    float halfWidth = size.width() / 2;
    FloatPoint endCorner;
    if (angleDeg < 90)
        endCorner.set(halfWidth, halfHeight);
    else if (angleDeg < 180)
        endCorner.set(-halfWidth, halfHeight);
    else if (angleDeg < 270)
        endCorner.set(-halfWidth, -halfHeight);
    else
        endCorner.set(halfWidth, -halfHeight);

    // Compute c (of y = mx + c) using the corner point.
    float c = endCorner.y() - perpendicularSlope * endCorner.x();
    float endX = c / (slope - perpendicularSlope);
    float endY = perpendicularSlope * endX + c;
    
    // We computed the end point, so set the second point, flipping the Y to account for angles going anticlockwise.
    secondPoint.set(halfWidth + endX, size.height() - (halfHeight + endY));
    // Reflect around the center for the start point.
    firstPoint.set(size.width() - secondPoint.x(), size.height() - secondPoint.y());
}

PassRefPtr<Gradient> CSSLinearGradientValue::createGradient(RenderObject* renderer, const IntSize& size)
{
    ASSERT(!size.isEmpty());
    
    RenderStyle* rootStyle = renderer->document()->documentElement()->renderStyle();

    FloatPoint firstPoint;
    FloatPoint secondPoint;
    if (m_angle) {
        float angle = m_angle->getFloatValue(CSSPrimitiveValue::CSS_DEG);
        endPointsFromAngle(angle, size, firstPoint, secondPoint);
    } else {
        firstPoint = computeEndPoint(m_firstX.get(), m_firstY.get(), renderer->style(), rootStyle, size);
        
        if (m_secondX || m_secondY)
            secondPoint = computeEndPoint(m_secondX.get(), m_secondY.get(), renderer->style(), rootStyle, size);
        else {
            if (m_firstX)
                secondPoint.setX(size.width() - firstPoint.x());
            if (m_firstY)
                secondPoint.setY(size.height() - firstPoint.y());
        }
    }

    RefPtr<Gradient> gradient = Gradient::create(firstPoint, secondPoint);

    // Now add the stops.
    addStops(gradient.get(), renderer, rootStyle, 1);

    return gradient.release();
}

String CSSRadialGradientValue::cssText() const
{
    String result;

    if (m_deprecatedType) {
        result = "-webkit-gradient(radial, ";

        result += m_firstX->cssText() + " ";
        result += m_firstY->cssText() + ", ";
        result += m_firstRadius->cssText() + ", ";
        result += m_secondX->cssText() + " ";
        result += m_secondY->cssText();
        result += ", ";
        result += m_secondRadius->cssText();

        // FIXME: share?
        for (unsigned i = 0; i < m_stops.size(); i++) {
            const CSSGradientColorStop& stop = m_stops[i];
            result += ", ";
            if (stop.m_position->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER) == 0)
                result += "from(" + stop.m_color->cssText() + ")";
            else if (stop.m_position->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER) == 1)
                result += "to(" + stop.m_color->cssText() + ")";
            else
                result += "color-stop(" + String::number(stop.m_position->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER)) + ", " + stop.m_color->cssText() + ")";
        }
    } else {

        result = m_repeating ? "-webkit-repeating-radial-gradient(" : "-webkit-radial-gradient(";
        if (m_firstX && m_firstY) {
            result += m_firstX->cssText() + " " + m_firstY->cssText();
        } else if (m_firstX)
            result += m_firstX->cssText();
         else if (m_firstY)
            result += m_firstY->cssText();
        else
            result += "center";


        if (m_shape || m_sizingBehavior) {
            result += ", ";
            if (m_shape)
                result += m_shape->cssText() + " ";
            else
                result += "ellipse ";

            if (m_sizingBehavior)
                result += m_sizingBehavior->cssText();
            else
                result += "cover";

        } else if (m_endHorizontalSize && m_endVerticalSize) {
            result += ", ";
            result += m_endHorizontalSize->cssText() + " " + m_endVerticalSize->cssText();
        }

        for (unsigned i = 0; i < m_stops.size(); i++) {
            const CSSGradientColorStop& stop = m_stops[i];
            result += ", ";
            result += stop.m_color->cssText();
            if (stop.m_position)
                result += " " + stop.m_position->cssText();
        }
    }

    result += ")";
    return result;
}

float CSSRadialGradientValue::resolveRadius(CSSPrimitiveValue* radius, RenderStyle* style, RenderStyle* rootStyle, float* widthOrHeight)
{
    float zoomFactor = style->effectiveZoom();

    float result = 0;
    if (radius->primitiveType() == CSSPrimitiveValue::CSS_NUMBER)  // Can the radius be a percentage?
        result = radius->getFloatValue() * zoomFactor;
    else if (widthOrHeight && radius->primitiveType() == CSSPrimitiveValue::CSS_PERCENTAGE)
        result = *widthOrHeight * radius->getFloatValue() / 100;
    else
        result = radius->computeLengthFloat(style, rootStyle, zoomFactor);
 
    return result;
}

static float distanceToClosestCorner(const FloatPoint& p, const FloatSize& size, FloatPoint& corner)
{
    FloatPoint topLeft;
    float topLeftDistance = FloatSize(p - topLeft).diagonalLength();

    FloatPoint topRight(size.width(), 0);
    float topRightDistance = FloatSize(p - topRight).diagonalLength();

    FloatPoint bottomLeft(0, size.height());
    float bottomLeftDistance = FloatSize(p - bottomLeft).diagonalLength();

    FloatPoint bottomRight(size.width(), size.height());
    float bottomRightDistance = FloatSize(p - bottomRight).diagonalLength();

    corner = topLeft;
    float minDistance = topLeftDistance;
    if (topRightDistance < minDistance) {
        minDistance = topRightDistance;
        corner = topRight;
    }

    if (bottomLeftDistance < minDistance) {
        minDistance = bottomLeftDistance;
        corner = bottomLeft;
    }

    if (bottomRightDistance < minDistance) {
        minDistance = bottomRightDistance;
        corner = bottomRight;
    }
    return minDistance;
}

static float distanceToFarthestCorner(const FloatPoint& p, const FloatSize& size, FloatPoint& corner)
{
    FloatPoint topLeft;
    float topLeftDistance = FloatSize(p - topLeft).diagonalLength();

    FloatPoint topRight(size.width(), 0);
    float topRightDistance = FloatSize(p - topRight).diagonalLength();

    FloatPoint bottomLeft(0, size.height());
    float bottomLeftDistance = FloatSize(p - bottomLeft).diagonalLength();

    FloatPoint bottomRight(size.width(), size.height());
    float bottomRightDistance = FloatSize(p - bottomRight).diagonalLength();

    corner = topLeft;
    float maxDistance = topLeftDistance;
    if (topRightDistance > maxDistance) {
        maxDistance = topRightDistance;
        corner = topRight;
    }

    if (bottomLeftDistance > maxDistance) {
        maxDistance = bottomLeftDistance;
        corner = bottomLeft;
    }

    if (bottomRightDistance > maxDistance) {
        maxDistance = bottomRightDistance;
        corner = bottomRight;
    }
    return maxDistance;
}

// Compute horizontal radius of ellipse with center at 0,0 which passes through p, and has
// width/height given by aspectRatio.
static inline float horizontalEllipseRadius(const FloatSize& p, float aspectRatio)
{
    // x^2/a^2 + y^2/b^2 = 1
    // a/b = aspectRatio, b = a/aspectRatio
    // a = sqrt(x^2 + y^2/(1/r^2))
    return sqrtf(p.width() * p.width() + (p.height() * p.height()) / (1 / (aspectRatio * aspectRatio)));
}

// FIXME: share code with the linear version
PassRefPtr<Gradient> CSSRadialGradientValue::createGradient(RenderObject* renderer, const IntSize& size)
{
    ASSERT(!size.isEmpty());
    
    RenderStyle* rootStyle = renderer->document()->documentElement()->renderStyle();

    FloatPoint firstPoint = computeEndPoint(m_firstX.get(), m_firstY.get(), renderer->style(), rootStyle, size);
    if (!m_firstX)
        firstPoint.setX(size.width() / 2);
    if (!m_firstY)
        firstPoint.setY(size.height() / 2);

    FloatPoint secondPoint = computeEndPoint(m_secondX.get(), m_secondY.get(), renderer->style(), rootStyle, size);
    if (!m_secondX)
        secondPoint.setX(size.width() / 2);
    if (!m_secondY)
        secondPoint.setY(size.height() / 2);
    
    float firstRadius = 0;
    if (m_firstRadius)
        firstRadius = resolveRadius(m_firstRadius.get(), renderer->style(), rootStyle);

    float secondRadius = 0;
    float aspectRatio = 1; // width / height.
    if (m_secondRadius)
        secondRadius = resolveRadius(m_secondRadius.get(), renderer->style(), rootStyle);
    else if (m_endHorizontalSize || m_endVerticalSize) {
        float width = size.width();
        float height = size.height();
        secondRadius = resolveRadius(m_endHorizontalSize.get(), renderer->style(), rootStyle, &width);
        aspectRatio = secondRadius / resolveRadius(m_endVerticalSize.get(), renderer->style(), rootStyle, &height);
    } else {
        enum GradientShape { Circle, Ellipse };
        GradientShape shape = Ellipse;
        if (m_shape && m_shape->primitiveType() == CSSPrimitiveValue::CSS_IDENT && m_shape->getIdent() == CSSValueCircle)
            shape = Circle;
        
        enum GradientFill { ClosestSide, ClosestCorner, FarthestSide, FarthestCorner };
        GradientFill fill = FarthestCorner;
        
        if (m_sizingBehavior && m_sizingBehavior->primitiveType() == CSSPrimitiveValue::CSS_IDENT) {
            switch (m_sizingBehavior->getIdent()) {
            case CSSValueContain:
            case CSSValueClosestSide:
                fill = ClosestSide;
                break;
            case CSSValueClosestCorner:
                fill = ClosestCorner;
                break;
            case CSSValueFarthestSide:
                fill = FarthestSide;
                break;
            case CSSValueCover:
            case CSSValueFarthestCorner:
                fill = FarthestCorner;
                break;
            }
        }
        
        // Now compute the end radii based on the second point, shape and fill.
        
        // Horizontal
        switch (fill) {
        case ClosestSide: {
            float xDist = min(secondPoint.x(), size.width() - secondPoint.x());
            float yDist = min(secondPoint.y(), size.height() - secondPoint.y());
            if (shape == Circle) {
                float smaller = min(xDist, yDist);
                xDist = smaller;
                yDist = smaller;
            }
            secondRadius = xDist;
            aspectRatio = xDist / yDist;
            break;
        }
        case FarthestSide: {
            float xDist = max(secondPoint.x(), size.width() - secondPoint.x());
            float yDist = max(secondPoint.y(), size.height() - secondPoint.y());
            if (shape == Circle) {
                float larger = max(xDist, yDist);
                xDist = larger;
                yDist = larger;
            }
            secondRadius = xDist;
            aspectRatio = xDist / yDist;
            break;
        }
        case ClosestCorner: {
            FloatPoint corner;
            float distance = distanceToClosestCorner(secondPoint, size, corner);
            if (shape == Circle)
                secondRadius = distance;
            else {
                // If <shape> is ellipse, the gradient-shape has the same ratio of width to height
                // that it would if closest-side or farthest-side were specified, as appropriate.
                float xDist = min(secondPoint.x(), size.width() - secondPoint.x());
                float yDist = min(secondPoint.y(), size.height() - secondPoint.y());
                
                secondRadius = horizontalEllipseRadius(corner - secondPoint, xDist / yDist);
                aspectRatio = xDist / yDist;
            }
            break;
        }

        case FarthestCorner: {
            FloatPoint corner;
            float distance = distanceToFarthestCorner(secondPoint, size, corner);
            if (shape == Circle)
                secondRadius = distance;
            else {
                // If <shape> is ellipse, the gradient-shape has the same ratio of width to height
                // that it would if closest-side or farthest-side were specified, as appropriate.
                float xDist = max(secondPoint.x(), size.width() - secondPoint.x());
                float yDist = max(secondPoint.y(), size.height() - secondPoint.y());
                
                secondRadius = horizontalEllipseRadius(corner - secondPoint, xDist / yDist);
                aspectRatio = xDist / yDist;
            }
            break;
        }
        }
    }

    RefPtr<Gradient> gradient = Gradient::create(firstPoint, firstRadius, secondPoint, secondRadius, aspectRatio);

    // addStops() only uses maxExtent for repeating gradients.
    float maxExtent = 0;
    if (m_repeating) {
        FloatPoint corner;
        maxExtent = distanceToFarthestCorner(secondPoint, size, corner);
    }

    // Now add the stops.
    addStops(gradient.get(), renderer, rootStyle, maxExtent);

    return gradient.release();
}

} // namespace WebCore
