/*
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (c) 2008, Google Inc. All rights reserved.
 * Copyright (c) 2012, Intel Corporation
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
#include "PlatformContextCairo.h"

#include "CairoUtilities.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "OwnPtrCairo.h"
#include "Pattern.h"
#include <cairo.h>

namespace WebCore {

// In Cairo image masking is immediate, so to emulate image clipping we must save masking
// details as part of the context state and apply them during platform restore.
class ImageMaskInformation {
public:
    void update(cairo_surface_t* maskSurface, const FloatRect& maskRect)
    {
        m_maskSurface = maskSurface;
        m_maskRect = maskRect;
    }

    bool isValid() const { return m_maskSurface; }
    cairo_surface_t* maskSurface() const { return m_maskSurface.get(); }
    const FloatRect& maskRect() const { return m_maskRect; }

private:
    RefPtr<cairo_surface_t> m_maskSurface;
    FloatRect m_maskRect;
};


// Encapsulates the additional painting state information we store for each
// pushed graphics state.
class PlatformContextCairo::State {
public:
    State()
        : m_globalAlpha(1)
        , m_imageInterpolationQuality(InterpolationDefault)
    {
    }

    State(const State& state)
        : m_globalAlpha(state.m_globalAlpha)
        , m_imageInterpolationQuality(state.m_imageInterpolationQuality)
    {
        // We do not copy m_imageMaskInformation because otherwise it would be applied
        // more than once during subsequent calls to restore().
    }

    ImageMaskInformation m_imageMaskInformation;
    float m_globalAlpha;
    InterpolationQuality m_imageInterpolationQuality;
};

PlatformContextCairo::PlatformContextCairo(cairo_t* cr)
    : m_cr(cr)
{
    m_stateStack.append(State());
    m_state = &m_stateStack.last();
}

void PlatformContextCairo::restore()
{
    const ImageMaskInformation& maskInformation = m_state->m_imageMaskInformation;
    if (maskInformation.isValid()) {
        const FloatRect& maskRect = maskInformation.maskRect();
        cairo_pop_group_to_source(m_cr.get());
        cairo_mask_surface(m_cr.get(), maskInformation.maskSurface(), maskRect.x(), maskRect.y());
    }

    m_stateStack.removeLast();
    ASSERT(!m_stateStack.isEmpty());
    m_state = &m_stateStack.last();

    cairo_restore(m_cr.get());
}

PlatformContextCairo::~PlatformContextCairo()
{
}

void PlatformContextCairo::save()
{
    m_stateStack.append(State(*m_state));
    m_state = &m_stateStack.last();

    cairo_save(m_cr.get());
}

void PlatformContextCairo::pushImageMask(cairo_surface_t* surface, const FloatRect& rect)
{
    // We must call savePlatformState at least once before we can use image masking,
    // since we actually apply the mask in restorePlatformState.
    ASSERT(!m_stateStack.isEmpty());
    m_state->m_imageMaskInformation.update(surface, rect);

    // Cairo doesn't support the notion of an image clip, so we push a group here
    // and then paint it to the surface with an image mask (which is an immediate
    // operation) during restorePlatformState.

    // We want to allow the clipped elements to composite with the surface as it
    // is now, but they are isolated in another group. To make this work, we're
    // going to blit the current surface contents onto the new group once we push it.
    cairo_surface_t* currentTarget = cairo_get_target(m_cr.get());
    cairo_surface_flush(currentTarget);

    // Pushing a new group ensures that only things painted after this point are clipped.
    cairo_push_group(m_cr.get());
    cairo_set_operator(m_cr.get(), CAIRO_OPERATOR_SOURCE);

    cairo_set_source_surface(m_cr.get(), currentTarget, 0, 0);
    cairo_rectangle(m_cr.get(), rect.x(), rect.y(), rect.width(), rect.height());
    cairo_fill(m_cr.get());
}

static void drawPatternToCairoContext(cairo_t* cr, cairo_pattern_t* pattern, const FloatRect& destRect, float alpha)
{
    cairo_translate(cr, destRect.x(), destRect.y());
    cairo_set_source(cr, pattern);
    cairo_rectangle(cr, 0, 0, destRect.width(), destRect.height());

    if (alpha < 1) {
        cairo_clip(cr);
        cairo_paint_with_alpha(cr, alpha);
    } else
        cairo_fill(cr);
}

void PlatformContextCairo::drawSurfaceToContext(cairo_surface_t* surface, const FloatRect& destRect, const FloatRect& originalSrcRect, GraphicsContext* context)
{
    FloatRect srcRect = originalSrcRect;

    // We need to account for negative source dimensions by flipping the rectangle.
    if (originalSrcRect.width() < 0) {
        srcRect.setX(originalSrcRect.x() + originalSrcRect.width());
        srcRect.setWidth(std::fabs(originalSrcRect.width()));
    }
    if (originalSrcRect.height() < 0) {
        srcRect.setY(originalSrcRect.y() + originalSrcRect.height());
        srcRect.setHeight(std::fabs(originalSrcRect.height()));
    }

    // Cairo subsurfaces don't support floating point boundaries well, so we expand the rectangle.
    IntRect expandedSrcRect(enclosingIntRect(srcRect));

    // We use a subsurface here so that we don't end up sampling outside the originalSrcRect rectangle.
    // See https://bugs.webkit.org/show_bug.cgi?id=58309
    RefPtr<cairo_surface_t> subsurface = adoptRef(cairo_surface_create_for_rectangle(
        surface, expandedSrcRect.x(), expandedSrcRect.y(), expandedSrcRect.width(), expandedSrcRect.height()));
    RefPtr<cairo_pattern_t> pattern = adoptRef(cairo_pattern_create_for_surface(subsurface.get()));

    ASSERT(m_state);
    switch (m_state->m_imageInterpolationQuality) {
    case InterpolationNone:
    case InterpolationLow:
        cairo_pattern_set_filter(pattern.get(), CAIRO_FILTER_FAST);
        break;
    case InterpolationMedium:
    case InterpolationHigh:
        cairo_pattern_set_filter(pattern.get(), CAIRO_FILTER_BILINEAR);
        break;
    case InterpolationDefault:
        cairo_pattern_set_filter(pattern.get(), CAIRO_FILTER_BILINEAR);
        break;
    }
    cairo_pattern_set_extend(pattern.get(), CAIRO_EXTEND_PAD);

    // The pattern transformation properly scales the pattern for when the source rectangle is a
    // different size than the destination rectangle. We also account for any offset we introduced
    // by expanding floating point source rectangle sizes. It's important to take the absolute value
    // of the scale since the original width and height might be negative.
    float scaleX = std::fabs(srcRect.width() / destRect.width());
    float scaleY = std::fabs(srcRect.height() / destRect.height());
    float leftPadding = static_cast<float>(expandedSrcRect.x()) - floorf(srcRect.x());
    float topPadding = static_cast<float>(expandedSrcRect.y()) - floorf(srcRect.y());
    cairo_matrix_t matrix = { scaleX, 0, 0, scaleY, leftPadding, topPadding };
    cairo_pattern_set_matrix(pattern.get(), &matrix);

    ShadowBlur& shadow = context->platformContext()->shadowBlur();
    if (shadow.type() != ShadowBlur::NoShadow) {
        if (GraphicsContext* shadowContext = shadow.beginShadowLayer(context, destRect)) {
            drawPatternToCairoContext(shadowContext->platformContext()->cr(), pattern.get(), destRect, 1);
            shadow.endShadowLayer(context);
        }
    }

    cairo_save(m_cr.get());
    drawPatternToCairoContext(m_cr.get(), pattern.get(), destRect, globalAlpha());
    cairo_restore(m_cr.get());
}

void PlatformContextCairo::setImageInterpolationQuality(InterpolationQuality quality)
{
    ASSERT(m_state);
    m_state->m_imageInterpolationQuality = quality;
}

InterpolationQuality PlatformContextCairo::imageInterpolationQuality() const
{
    ASSERT(m_state);
    return m_state->m_imageInterpolationQuality;
}


float PlatformContextCairo::globalAlpha() const
{
    return m_state->m_globalAlpha;
}

void PlatformContextCairo::setGlobalAlpha(float globalAlpha)
{
    m_state->m_globalAlpha = globalAlpha;
}

static inline void reduceSourceByAlpha(cairo_t* cr, float alpha)
{
    if (alpha >= 1)
        return;
    cairo_push_group(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint_with_alpha(cr, alpha);
    cairo_pop_group_to_source(cr);
}

static void prepareCairoContextSource(cairo_t* cr, Pattern* pattern, Gradient* gradient, const Color& color, float globalAlpha)
{
    if (pattern) {
        RefPtr<cairo_pattern_t> cairoPattern(adoptRef(pattern->createPlatformPattern(AffineTransform())));
        cairo_set_source(cr, cairoPattern.get());
        reduceSourceByAlpha(cr, globalAlpha);
    } else if (gradient)
        cairo_set_source(cr, gradient->platformGradient(globalAlpha));
    else { // Solid color source.
        if (globalAlpha < 1)
            setSourceRGBAFromColor(cr, colorWithOverrideAlpha(color.rgb(), color.alpha() / 255.f * globalAlpha));
        else
            setSourceRGBAFromColor(cr, color);
    }
}

void PlatformContextCairo::prepareForFilling(const GraphicsContextState& state, PatternAdjustment patternAdjustment)
{
    cairo_set_fill_rule(m_cr.get(), state.fillRule == RULE_EVENODD ?  CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    prepareCairoContextSource(m_cr.get(),
                              state.fillPattern.get(),
                              state.fillGradient.get(),
                              state.fillColor,
                              patternAdjustment == AdjustPatternForGlobalAlpha ? globalAlpha() : 1);

    if (state.fillPattern)
        clipForPatternFilling(state);
}

void PlatformContextCairo::prepareForStroking(const GraphicsContextState& state, AlphaPreservation alphaPreservation)
{
    prepareCairoContextSource(m_cr.get(),
                              state.strokePattern.get(),
                              state.strokeGradient.get(),
                              state.strokeColor,
                              alphaPreservation == PreserveAlpha ? globalAlpha() : 1);
}

void PlatformContextCairo::clipForPatternFilling(const GraphicsContextState& state)
{
    ASSERT(state.fillPattern);

    // Hold current cairo path in a variable for restoring it after configuring the pattern clip rectangle.
    OwnPtr<cairo_path_t> currentPath = adoptPtr(cairo_copy_path(m_cr.get()));
    cairo_new_path(m_cr.get());

    // Initialize clipping extent from current cairo clip extents, then shrink if needed according to pattern.
    // Inspired by GraphicsContextQt::drawRepeatPattern.
    double x1, y1, x2, y2;
    cairo_clip_extents(m_cr.get(), &x1, &y1, &x2, &y2);
    FloatRect clipRect(x1, y1, x2 - x1, y2 - y1);

    Image* patternImage = state.fillPattern->tileImage();
    ASSERT(patternImage);
    const AffineTransform& patternTransform = state.fillPattern->getPatternSpaceTransform();
    FloatRect patternRect = patternTransform.mapRect(FloatRect(0, 0, patternImage->width(), patternImage->height()));

    bool repeatX = state.fillPattern->repeatX();
    bool repeatY = state.fillPattern->repeatY();

    if (!repeatX) {
        clipRect.setX(patternRect.x());
        clipRect.setWidth(patternRect.width());
    }
    if (!repeatY) {
        clipRect.setY(patternRect.y());
        clipRect.setHeight(patternRect.height());
    }
    if (!repeatX || !repeatY) {
        cairo_rectangle(m_cr.get(), clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
        cairo_clip(m_cr.get());
    }

    // Restoring cairo path.
    cairo_append_path(m_cr.get(), currentPath.get());
}

} // namespace WebCore
