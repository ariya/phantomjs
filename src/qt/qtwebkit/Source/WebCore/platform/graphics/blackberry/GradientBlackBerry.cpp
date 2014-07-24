/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "config.h"
#include "Gradient.h"

#include "AffineTransform.h"
#include "GraphicsContext.h"
#include "GraphicsContext3D.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "TransformationMatrix.h"

#include <BlackBerryPlatformGraphicsContext.h>
#include <stdio.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/Vector.h>

namespace WebCore {

// Determine the total number of stops needed, including pseudo-stops at the
// ends as necessary.
static size_t totalStopsNeeded(const Gradient::ColorStop* stopData, size_t count)
{
    // N.B.: The tests in this function should kept in sync with the ones in
    // fillStops(), or badness happens.
    const Gradient::ColorStop* stop = stopData;
    size_t countUsed = count;
    if (count < 1 || stop->stop > 0.0)
        countUsed++;
    stop += count - 1;
    if (count < 1 || stop->stop < 1.0)
        countUsed++;
    return countUsed;
}

// Collect sorted stop position and color information into the pos and colors
// buffers, ensuring stops at both 0.0 and 1.0. The buffers must be large
// enough to hold information for all stops, including the new endpoints if
// stops at 0.0 and 1.0 aren't already included.
static void fillStops(const Gradient::ColorStop* stopData, size_t count, float* pos, unsigned* colors)
{
    const Gradient::ColorStop* stop = stopData;
    size_t start = 0;
    if (count < 1) {
        // A gradient with no stops must be transparent black.
        pos[0] = 0.0;
        colors[0] = 0;
        start = 1;
    } else if (stop->stop > 0.0) {
        // Copy the first stop to 0.0. The first stop position may have a slight
        // rounding error, but we don't care in this float comparison, since
        // 0.0 comes through cleanly and people aren't likely to want a gradient
        // with a stop at (0 + epsilon).
        pos[0] = 0.0;
        colors[0] = Color(stop->red, stop->green, stop->blue, stop->alpha).rgb();
        start = 1;
    }

    for (size_t i = start; i < start + count; i++) {
        pos[i] = stop->stop;
        colors[i] = Color(stop->red, stop->green, stop->blue, stop->alpha).rgb();
        ++stop;
    }

    // Copy the last stop to 1.0 if needed. See comment above about this float
    // comparison.
    if (count < 1 || (--stop)->stop < 1.0) {
        pos[start + count] = 1.0;
        colors[start + count] = colors[start + count - 1];
    }
}


BlackBerry::Platform::Graphics::Gradient* Gradient::platformGradient()
{
    if (m_gradient)
        return m_gradient;

    if (m_radial)
        m_gradient = BlackBerry::Platform::Graphics::Gradient::createRadialGradient(m_p0, m_p1, m_r0, m_r1, m_aspectRatio, (BlackBerry::Platform::Graphics::Gradient::SpreadMethod)m_spreadMethod);
    else
        m_gradient = BlackBerry::Platform::Graphics::Gradient::createLinearGradient(m_p0, m_p1, (BlackBerry::Platform::Graphics::Gradient::SpreadMethod)m_spreadMethod);

    sortStopsIfNecessary();
    ASSERT(m_stopsSorted);

    size_t countUsed = totalStopsNeeded(m_stops.data(), m_stops.size());
    ASSERT(countUsed >= 2);
    ASSERT(countUsed >= m_stops.size());

    Vector<float> pos(countUsed);
    Vector<unsigned> colors(countUsed);
    fillStops(m_stops.data(), m_stops.size(), pos.data(), colors.data());
    m_gradient->setColorStops(pos.data(), colors.data(), countUsed);

    if (!m_gradientSpaceTransformation.isIdentity())
        m_gradient->setLocalMatrix(reinterpret_cast<const double*>(&m_gradientSpaceTransformation));

    return m_gradient;
}

void Gradient::platformDestroy()
{
    delete m_gradient;
    m_gradient = 0;
}

void Gradient::fill(GraphicsContext* context, const FloatRect& rect)
{
    platformGradient()->fill(context->platformContext(), rect);
}

void Gradient::setPlatformGradientSpaceTransform(const AffineTransform& gradientSpaceTransformation)
{
    platformGradient()->setLocalMatrix(reinterpret_cast<const double*>(&gradientSpaceTransformation));
}

}
