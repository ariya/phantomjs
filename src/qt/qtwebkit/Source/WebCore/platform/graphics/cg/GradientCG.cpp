/*
 * Copyright (C) 2006, 2007, 2008 Apple Computer, Inc.  All rights reserved.
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

#include "GraphicsContextCG.h"
#include <ApplicationServices/ApplicationServices.h>
#include <wtf/RetainPtr.h>

namespace WebCore {

void Gradient::platformDestroy()
{
    CGGradientRelease(m_gradient);
    m_gradient = 0;
}

CGGradientRef Gradient::platformGradient()
{
    if (m_gradient)
        return m_gradient;

    sortStopsIfNecessary();
    
    const int cReservedStops = 3;
    Vector<CGFloat, 4 * cReservedStops> colorComponents;
    colorComponents.reserveInitialCapacity(m_stops.size() * 4); // RGBA components per stop

    Vector<CGFloat, cReservedStops> locations;
    locations.reserveInitialCapacity(m_stops.size());

    for (size_t i = 0; i < m_stops.size(); ++i) {
        colorComponents.uncheckedAppend(m_stops[i].red);
        colorComponents.uncheckedAppend(m_stops[i].green);
        colorComponents.uncheckedAppend(m_stops[i].blue);
        colorComponents.uncheckedAppend(m_stops[i].alpha);

        locations.uncheckedAppend(m_stops[i].stop);
    }
    
    m_gradient = CGGradientCreateWithColorComponents(deviceRGBColorSpaceRef(), colorComponents.data(), locations.data(), m_stops.size());

    return m_gradient;
}

void Gradient::fill(GraphicsContext* context, const FloatRect& rect)
{
    context->clip(rect);
    paint(context);
}

void Gradient::paint(GraphicsContext* context)
{
    CGContextRef ctx = context->platformContext();
    paint(ctx);
}

void Gradient::paint(CGContextRef context)
{
    CGGradientDrawingOptions extendOptions = kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation;
    if (!m_radial) {
        CGContextDrawLinearGradient(context, platformGradient(), m_p0, m_p1, extendOptions);
        return;
    }

    bool needScaling = aspectRatio() != 1;
    if (needScaling) {
        CGContextSaveGState(context);
        // Scale from the center of the gradient. We only ever scale non-deprecated gradients,
        // for which m_p0 == m_p1.
        ASSERT(m_p0 == m_p1);
        CGContextTranslateCTM(context, m_p0.x(), m_p0.y());
        CGContextScaleCTM(context, 1, 1 / aspectRatio());
        CGContextTranslateCTM(context, -m_p0.x(), -m_p0.y());
    }

    CGContextDrawRadialGradient(context, platformGradient(), m_p0, m_r0, m_p1, m_r1, extendOptions);

    if (needScaling)
        CGContextRestoreGState(context);
}

} //namespace
