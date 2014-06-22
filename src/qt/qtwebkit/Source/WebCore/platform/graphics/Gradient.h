/*
 * Copyright (C) 2006, 2007, 2008, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Torch Mobile, Inc.
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

#ifndef Gradient_h
#define Gradient_h

#include "AffineTransform.h"
#include "FloatPoint.h"
#include "GraphicsTypes.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

#if USE(CG)

typedef struct CGContext* CGContextRef;

typedef struct CGGradient* CGGradientRef;
typedef CGGradientRef PlatformGradient;

#elif PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QGradient;
QT_END_NAMESPACE
typedef QGradient* PlatformGradient;
#elif USE(CAIRO)
typedef struct _cairo_pattern cairo_pattern_t;
typedef cairo_pattern_t* PlatformGradient;
#elif PLATFORM(BLACKBERRY)
namespace BlackBerry {
namespace Platform {
namespace Graphics {
class Gradient;
}
}
}
typedef BlackBerry::Platform::Graphics::Gradient* PlatformGradient;
#else
typedef void* PlatformGradient;
#endif

namespace WebCore {

    class Color;
    class FloatRect;
    class GraphicsContext;

    class Gradient : public RefCounted<Gradient> {
    public:
        static PassRefPtr<Gradient> create(const FloatPoint& p0, const FloatPoint& p1)
        {
            return adoptRef(new Gradient(p0, p1));
        }
        static PassRefPtr<Gradient> create(const FloatPoint& p0, float r0, const FloatPoint& p1, float r1, float aspectRatio = 1)
        {
            return adoptRef(new Gradient(p0, r0, p1, r1, aspectRatio));
        }
        ~Gradient();

        struct ColorStop;
        void addColorStop(const ColorStop&);
        void addColorStop(float, const Color&);

        bool hasAlpha() const;

        bool isRadial() const { return m_radial; }
        bool isZeroSize() const { return m_p0.x() == m_p1.x() && m_p0.y() == m_p1.y() && (!m_radial || m_r0 == m_r1); }

        const FloatPoint& p0() const { return m_p0; }
        const FloatPoint& p1() const { return m_p1; }

        void setP0(const FloatPoint& p)
        {
            if (m_p0 == p)
                return;
            
            m_p0 = p;
            
            invalidateHash();
        }
        
        void setP1(const FloatPoint& p)
        {
            if (m_p1 == p)
                return;
            
            m_p1 = p;
            
            invalidateHash();
        }

        float startRadius() const { return m_r0; }
        float endRadius() const { return m_r1; }

        void setStartRadius(float r)
        {
            if (m_r0 == r)
                return;

            m_r0 = r;

            invalidateHash();
        }

        void setEndRadius(float r)
        {
            if (m_r1 == r)
                return;

            m_r1 = r;

            invalidateHash();
        }

        float aspectRatio() const { return m_aspectRatio; }

#if USE(WINGDI)
        const Vector<ColorStop, 2>& getStops() const;
#else
        PlatformGradient platformGradient();
#endif

        struct ColorStop {
            float stop;
            float red;
            float green;
            float blue;
            float alpha;

            ColorStop() : stop(0), red(0), green(0), blue(0), alpha(0) { }
            ColorStop(float s, float r, float g, float b, float a) : stop(s), red(r), green(g), blue(b), alpha(a) { }
        };

        void setStopsSorted(bool s) { m_stopsSorted = s; }
        
        void setSpreadMethod(GradientSpreadMethod);
        GradientSpreadMethod spreadMethod() { return m_spreadMethod; }
        void setGradientSpaceTransform(const AffineTransform& gradientSpaceTransformation);
        // Qt and CG transform the gradient at draw time
        AffineTransform gradientSpaceTransform() { return m_gradientSpaceTransformation; }

        void fill(GraphicsContext*, const FloatRect&);
        void adjustParametersForTiledDrawing(IntSize&, FloatRect&);

        void setPlatformGradientSpaceTransform(const AffineTransform& gradientSpaceTransformation);

        unsigned hash() const;
        void invalidateHash() { m_cachedHash = 0; }

#if USE(CG)
        void paint(CGContextRef);
        void paint(GraphicsContext*);
#elif USE(CAIRO)
        PlatformGradient platformGradient(float globalAlpha);
#endif

    private:
        Gradient(const FloatPoint& p0, const FloatPoint& p1);
        Gradient(const FloatPoint& p0, float r0, const FloatPoint& p1, float r1, float aspectRatio);

        void platformInit() { m_gradient = 0; }
        void platformDestroy();

        void sortStopsIfNecessary();

        // Keep any parameters relevant to rendering in sync with the structure in Gradient::hash().
        bool m_radial;
        FloatPoint m_p0;
        FloatPoint m_p1;
        float m_r0;
        float m_r1;
        float m_aspectRatio; // For elliptical gradient, width / height.
        mutable Vector<ColorStop, 2> m_stops;
        mutable bool m_stopsSorted;
        GradientSpreadMethod m_spreadMethod;
        AffineTransform m_gradientSpaceTransformation;

        mutable unsigned m_cachedHash;

        PlatformGradient m_gradient;

#if USE(CAIRO)
        float m_platformGradientAlpha;
#endif

    };

} //namespace

#endif
