/*
 * Copyright (C) 2011 Igalia S.L.
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

#ifndef PlatformContextCairo_h
#define PlatformContextCairo_h

#include "GraphicsContext.h"
#include "RefPtrCairo.h"
#include "ShadowBlur.h"

namespace WebCore {

struct GraphicsContextState;

// Much like PlatformContextSkia in the Skia port, this class holds information that
// would normally be private to GraphicsContext, except that we want to allow access
// to it in Font and Image code. This allows us to separate the concerns of Cairo-specific
// code from the platform-independent GraphicsContext.

class PlatformContextCairo {
    WTF_MAKE_NONCOPYABLE(PlatformContextCairo);
public:
    PlatformContextCairo(cairo_t*);
    ~PlatformContextCairo();

    cairo_t* cr() { return m_cr.get(); }
    void setCr(cairo_t* cr) { m_cr = cr; }

    ShadowBlur& shadowBlur() { return m_shadowBlur; }

    void save();
    void restore();
    void setGlobalAlpha(float);
    float globalAlpha() const;

    void pushImageMask(cairo_surface_t*, const FloatRect&);
    void drawSurfaceToContext(cairo_surface_t*, const FloatRect& destRect, const FloatRect& srcRect, GraphicsContext*);

    void setImageInterpolationQuality(InterpolationQuality);
    InterpolationQuality imageInterpolationQuality() const;

    enum PatternAdjustment { NoAdjustment, AdjustPatternForGlobalAlpha };
    void prepareForFilling(const GraphicsContextState&, PatternAdjustment);

    enum AlphaPreservation { DoNotPreserveAlpha, PreserveAlpha };
    void prepareForStroking(const GraphicsContextState&, AlphaPreservation = PreserveAlpha);

private:
    void clipForPatternFilling(const GraphicsContextState&);

    RefPtr<cairo_t> m_cr;

    class State;
    State* m_state;
    WTF::Vector<State> m_stateStack;

    // GraphicsContext is responsible for managing the state of the ShadowBlur,
    // so it does not need to be on the state stack.
    ShadowBlur m_shadowBlur;

};

} // namespace WebCore

#endif // PlatformContextCairo_h
