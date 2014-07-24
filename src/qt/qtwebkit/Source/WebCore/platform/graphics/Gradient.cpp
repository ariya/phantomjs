/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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
#include "Gradient.h"

#include "Color.h"
#include "FloatRect.h"
#include <wtf/HashFunctions.h>
#include <wtf/StringHasher.h>

using WTF::pairIntHash;

namespace WebCore {

Gradient::Gradient(const FloatPoint& p0, const FloatPoint& p1)
    : m_radial(false)
    , m_p0(p0)
    , m_p1(p1)
    , m_r0(0)
    , m_r1(0)
    , m_aspectRatio(1)
    , m_stopsSorted(false)
    , m_spreadMethod(SpreadMethodPad)
    , m_cachedHash(0)
{
    platformInit();
}

Gradient::Gradient(const FloatPoint& p0, float r0, const FloatPoint& p1, float r1, float aspectRatio)
    : m_radial(true)
    , m_p0(p0)
    , m_p1(p1)
    , m_r0(r0)
    , m_r1(r1)
    , m_aspectRatio(aspectRatio)
    , m_stopsSorted(false)
    , m_spreadMethod(SpreadMethodPad)
    , m_cachedHash(0)
{
    platformInit();
}

Gradient::~Gradient()
{
    platformDestroy();
}

void Gradient::adjustParametersForTiledDrawing(IntSize& size, FloatRect& srcRect)
{
    if (m_radial)
        return;

    if (srcRect.isEmpty())
        return;

    if (m_p0.x() == m_p1.x()) {
        size.setWidth(1);
        srcRect.setWidth(1);
        srcRect.setX(0);
        return;
    }
    if (m_p0.y() != m_p1.y())
        return;

    size.setHeight(1);
    srcRect.setHeight(1);
    srcRect.setY(0);
}

void Gradient::addColorStop(float value, const Color& color)
{
    float r;
    float g;
    float b;
    float a;
    color.getRGBA(r, g, b, a);
    m_stops.append(ColorStop(value, r, g, b, a));

    m_stopsSorted = false;
    platformDestroy();

    invalidateHash();
}

void Gradient::addColorStop(const Gradient::ColorStop& stop)
{
    m_stops.append(stop);

    m_stopsSorted = false;
    platformDestroy();

    invalidateHash();
}

static inline bool compareStops(const Gradient::ColorStop& a, const Gradient::ColorStop& b)
{
    return a.stop < b.stop;
}

void Gradient::sortStopsIfNecessary()
{
    if (m_stopsSorted)
        return;

    m_stopsSorted = true;

    if (!m_stops.size())
        return;

    std::stable_sort(m_stops.begin(), m_stops.end(), compareStops);

    invalidateHash();
}

bool Gradient::hasAlpha() const
{
    for (size_t i = 0; i < m_stops.size(); i++) {
        if (m_stops[i].alpha < 1)
            return true;
    }

    return false;
}

void Gradient::setSpreadMethod(GradientSpreadMethod spreadMethod)
{
    // FIXME: Should it become necessary, allow calls to this method after m_gradient has been set.
    ASSERT(m_gradient == 0);

    if (m_spreadMethod == spreadMethod)
        return;

    m_spreadMethod = spreadMethod;

    invalidateHash();
}

void Gradient::setGradientSpaceTransform(const AffineTransform& gradientSpaceTransformation)
{
    if (m_gradientSpaceTransformation == gradientSpaceTransformation)
        return;

    m_gradientSpaceTransformation = gradientSpaceTransformation;
    setPlatformGradientSpaceTransform(gradientSpaceTransformation);

    invalidateHash();
}

#if !USE(CAIRO) && !PLATFORM(BLACKBERRY)
void Gradient::setPlatformGradientSpaceTransform(const AffineTransform&)
{
}
#endif

unsigned Gradient::hash() const
{
    if (m_cachedHash)
        return m_cachedHash;

    struct {
        AffineTransform gradientSpaceTransformation;
        FloatPoint p0;
        FloatPoint p1;
        float r0;
        float r1;
        float aspectRatio;
        GradientSpreadMethod spreadMethod;
        bool radial;
    } parameters;

    // StringHasher requires that the memory it hashes be a multiple of two in size.
    COMPILE_ASSERT(!(sizeof(parameters) % 2), Gradient_parameters_size_should_be_multiple_of_two);
    COMPILE_ASSERT(!(sizeof(ColorStop) % 2), Color_stop_size_should_be_multiple_of_two);
    
    // Ensure that any padding in the struct is zero-filled, so it will not affect the hash value.
    memset(&parameters, 0, sizeof(parameters));
    
    parameters.gradientSpaceTransformation = m_gradientSpaceTransformation;
    parameters.p0 = m_p0;
    parameters.p1 = m_p1;
    parameters.r0 = m_r0;
    parameters.r1 = m_r1;
    parameters.aspectRatio = m_aspectRatio;
    parameters.spreadMethod = m_spreadMethod;
    parameters.radial = m_radial;

    unsigned parametersHash = StringHasher::hashMemory(&parameters, sizeof(parameters));
    unsigned stopHash = StringHasher::hashMemory(m_stops.data(), m_stops.size() * sizeof(ColorStop));

    m_cachedHash = pairIntHash(parametersHash, stopHash);

    return m_cachedHash;
}

} //namespace
