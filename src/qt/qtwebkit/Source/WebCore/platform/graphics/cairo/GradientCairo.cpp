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

#include "GraphicsContext.h"
#include "PlatformContextCairo.h"
#include <cairo.h>

namespace WebCore {

void Gradient::platformDestroy()
{
    if (m_gradient) {
        cairo_pattern_destroy(m_gradient);
        m_gradient = 0;
    }
}

cairo_pattern_t* Gradient::platformGradient()
{
    return platformGradient(1);
}

cairo_pattern_t* Gradient::platformGradient(float globalAlpha)
{
    if (m_gradient && m_platformGradientAlpha == globalAlpha)
        return m_gradient;

    platformDestroy();
    m_platformGradientAlpha = globalAlpha;

    if (m_radial)
        m_gradient = cairo_pattern_create_radial(m_p0.x(), m_p0.y(), m_r0, m_p1.x(), m_p1.y(), m_r1);
    else
        m_gradient = cairo_pattern_create_linear(m_p0.x(), m_p0.y(), m_p1.x(), m_p1.y());

    Vector<ColorStop>::iterator stopIterator = m_stops.begin();
    while (stopIterator != m_stops.end()) {
        cairo_pattern_add_color_stop_rgba(m_gradient, stopIterator->stop,
                                          stopIterator->red, stopIterator->green, stopIterator->blue,
                                          stopIterator->alpha * globalAlpha);
        ++stopIterator;
    }

    switch (m_spreadMethod) {
    case SpreadMethodPad:
        cairo_pattern_set_extend(m_gradient, CAIRO_EXTEND_PAD);
        break;
    case SpreadMethodReflect:
        cairo_pattern_set_extend(m_gradient, CAIRO_EXTEND_REFLECT);
        break;
    case SpreadMethodRepeat:
        cairo_pattern_set_extend(m_gradient, CAIRO_EXTEND_REPEAT);
        break;
    }

    cairo_matrix_t matrix = m_gradientSpaceTransformation;
    cairo_matrix_invert(&matrix);
    cairo_pattern_set_matrix(m_gradient, &matrix);

    return m_gradient;
}

void Gradient::setPlatformGradientSpaceTransform(const AffineTransform& gradientSpaceTransformation)
{
    if (m_gradient) {
        cairo_matrix_t matrix = gradientSpaceTransformation;
        cairo_matrix_invert(&matrix);
        cairo_pattern_set_matrix(m_gradient, &matrix);
    }
}

void Gradient::fill(GraphicsContext* context, const FloatRect& rect)
{
    cairo_t* cr = context->platformContext()->cr();

    context->save();
    cairo_set_source(cr, platformGradient());
    cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
    cairo_fill(cr);
    context->restore();
}

} //namespace
