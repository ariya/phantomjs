/*
 * Copyright (C) 2010 University of Szeged
 * Copyright (C) 2010 Zoltan Herczeg
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef FELighting_h
#define FELighting_h

#if ENABLE(FILTERS)
#include "Color.h"
#include "Filter.h"
#include "FilterEffect.h"
#include "LightSource.h"
#include "PointLightSource.h"
#include "SpotLightSource.h"
#include <wtf/ByteArray.h>
#include <wtf/Platform.h>

// Common base class for FEDiffuseLighting and FESpecularLighting

namespace WebCore {

class FELighting : public FilterEffect {
public:
    virtual void apply();

    virtual void determineAbsolutePaintRect() { setAbsolutePaintRect(maxEffectRect()); }

protected:
    enum LightingType {
        DiffuseLighting,
        SpecularLighting
    };

    struct LightingData {
        // This structure contains only read-only (SMP safe) data
        ByteArray* pixels;
        float surfaceScale;
        int widthMultipliedByPixelSize;
        int widthDecreasedByOne;
        int heightDecreasedByOne;

        inline void topLeft(int offset, IntPoint& normalVector);
        inline void topRow(int offset, IntPoint& normalVector);
        inline void topRight(int offset, IntPoint& normalVector);
        inline void leftColumn(int offset, IntPoint& normalVector);
        inline void interior(int offset, IntPoint& normalVector);
        inline void rightColumn(int offset, IntPoint& normalVector);
        inline void bottomLeft(int offset, IntPoint& normalVector);
        inline void bottomRow(int offset, IntPoint& normalVector);
        inline void bottomRight(int offset, IntPoint& normalVector);
    };

    FELighting(Filter*, LightingType, const Color&, float, float, float, float, float, float, PassRefPtr<LightSource>);

    bool drawLighting(ByteArray*, int, int);
    inline void inlineSetPixel(int offset, LightingData&, LightSource::PaintingData&,
                               int lightX, int lightY, float factorX, float factorY, IntPoint& normalVector);

    // Not worth to inline every occurence of setPixel.
    void setPixel(int offset, LightingData&, LightSource::PaintingData&,
                  int lightX, int lightY, float factorX, float factorY, IntPoint& normalVector);

    inline void platformApply(LightingData&, LightSource::PaintingData&);

    inline void platformApplyGeneric(LightingData&, LightSource::PaintingData&);
    static int getPowerCoefficients(float exponent);
    inline void platformApplyNeon(LightingData&, LightSource::PaintingData&);

    LightingType m_lightingType;
    RefPtr<LightSource> m_lightSource;

    Color m_lightingColor;
    float m_surfaceScale;
    float m_diffuseConstant;
    float m_specularConstant;
    float m_specularExponent;
    float m_kernelUnitLengthX;
    float m_kernelUnitLengthY;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FELighting_h
