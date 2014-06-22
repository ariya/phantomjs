/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Brent Fulgham <bfulgham@gmail.com>
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

#ifndef GraphicsContextPlatformPrivateCairo_h
#define GraphicsContextPlatformPrivateCairo_h

#include "GraphicsContext.h"

#include "PlatformContextCairo.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <wtf/MathExtras.h>

#if PLATFORM(GTK)
#include <pango/pango.h>
typedef struct _GdkExposeEvent GdkExposeEvent;
#elif PLATFORM(WIN)
#include <cairo-win32.h>
#endif

namespace WebCore {

class GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivate(PlatformContextCairo* newPlatformContext)
        : platformContext(newPlatformContext)
#if PLATFORM(GTK)
        , expose(0)
#endif
#if PLATFORM(WIN) || (PLATFORM(GTK) && OS(WINDOWS))
        // NOTE:  These may note be needed: review and remove once Cairo implementation is complete
        , m_hdc(0)
        , m_shouldIncludeChildWindows(false)
#endif
    {
    }

    virtual ~GraphicsContextPlatformPrivate()
    {
    }

#if PLATFORM(WIN)
    // On Windows, we need to update the HDC for form controls to draw in the right place.
    void save();
    void restore();
    void flush();
    void clip(const FloatRect&);
    void clip(const Path&);
    void scale(const FloatSize&);
    void rotate(float);
    void translate(float, float);
    void concatCTM(const AffineTransform&);
    void setCTM(const AffineTransform&);
    void syncContext(cairo_t* cr);
#else
    // On everything else, we do nothing.
    void save() {}
    void restore() {}
    void flush() {}
    void clip(const FloatRect&) {}
    void clip(const Path&) {}
    void scale(const FloatSize&) {}
    void rotate(float) {}
    void translate(float, float) {}
    void concatCTM(const AffineTransform&) {}
    void setCTM(const AffineTransform&) {}
    void syncContext(cairo_t*) { }
#endif

    PlatformContextCairo* platformContext;
    Vector<float> layers;

#if PLATFORM(GTK)
    GdkEventExpose* expose;
#endif
#if PLATFORM(WIN) || (PLATFORM(GTK) && OS(WINDOWS))
    HDC m_hdc;
    bool m_shouldIncludeChildWindows;
#endif
};

// This is a specialized private section for the Cairo GraphicsContext, which knows how
// to clean up the heap allocated PlatformContextCairo that we must use for the top-level
// GraphicsContext.
class GraphicsContextPlatformPrivateToplevel : public GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivateToplevel(PlatformContextCairo* platformContext)
        : GraphicsContextPlatformPrivate(platformContext)
    {
    }

    virtual ~GraphicsContextPlatformPrivateToplevel()
    {
        delete platformContext;
    }
};


} // namespace WebCore

#endif // GraphicsContextPlatformPrivateCairo_h
