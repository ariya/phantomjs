/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "ColorDistance.h"

#include "Color.h"
#include <wtf/MathExtras.h>

namespace WebCore {

ColorDistance::ColorDistance()
    : m_redDiff(0)
    , m_greenDiff(0)
    , m_blueDiff(0)
{
}

ColorDistance::ColorDistance(const Color& fromColor, const Color& toColor)
    : m_redDiff(toColor.red() - fromColor.red())
    , m_greenDiff(toColor.green() - fromColor.green())
    , m_blueDiff(toColor.blue() - fromColor.blue())
{
}

ColorDistance::ColorDistance(int redDiff, int greenDiff, int blueDiff)
    : m_redDiff(redDiff)
    , m_greenDiff(greenDiff)
    , m_blueDiff(blueDiff)
{
}

static inline int clampColorValue(int v)
{
    if (v > 255)
        v = 255;
    else if (v < 0)
        v = 0;
    return v;
}

ColorDistance ColorDistance::scaledDistance(float scaleFactor) const
{
    return ColorDistance(static_cast<int>(scaleFactor * m_redDiff),
                         static_cast<int>(scaleFactor * m_greenDiff),
                         static_cast<int>(scaleFactor * m_blueDiff));
}

Color ColorDistance::addColorsAndClamp(const Color& first, const Color& second)
{
    return Color(clampColorValue(first.red() + second.red()),
                 clampColorValue(first.green() + second.green()),
                 clampColorValue(first.blue() + second.blue()));
}

Color ColorDistance::addToColorAndClamp(const Color& color) const
{
    return Color(clampColorValue(color.red() + m_redDiff),
                 clampColorValue(color.green() + m_greenDiff),
                 clampColorValue(color.blue() + m_blueDiff));
}

bool ColorDistance::isZero() const
{
    return !m_redDiff && !m_blueDiff && !m_greenDiff;
}

float ColorDistance::distance() const
{
    // This is just a simple distance calculation, not respecting color spaces
    return sqrtf(m_redDiff * m_redDiff + m_blueDiff * m_blueDiff + m_greenDiff * m_greenDiff);
}

}

#endif
