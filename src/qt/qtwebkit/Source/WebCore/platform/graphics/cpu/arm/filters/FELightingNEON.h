/*
 * Copyright (C) 2011 University of Szeged
 * Copyright (C) 2011 Zoltan Herczeg
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

#ifndef FELightingNEON_h
#define FELightingNEON_h

#if CPU(ARM_NEON) && CPU(ARM_TRADITIONAL) && COMPILER(GCC)

#include "FELighting.h"
#include <wtf/Alignment.h>
#include <wtf/ParallelJobs.h>

namespace WebCore {

// Otherwise: Distant Light.
#define FLAG_POINT_LIGHT                 0x01
#define FLAG_SPOT_LIGHT                  0x02
#define FLAG_CONE_EXPONENT_IS_1          0x04

// Otherwise: Diffuse light.
#define FLAG_SPECULAR_LIGHT              0x10
#define FLAG_DIFFUSE_CONST_IS_1          0x20
#define FLAG_SPECULAR_EXPONENT_IS_1      0x40

// Must be aligned to 16 bytes.
struct FELightingFloatArgumentsForNeon {
    float surfaceScale;
    float minusSurfaceScaleDividedByFour;
    float diffuseConstant;
    float padding1;

    float coneCutOffLimit;
    float coneFullLight;
    float coneCutOffRange;
    float constOne;

    float lightX;
    float lightY;
    float lightZ;
    float padding2;

    float directionX;
    float directionY;
    float directionZ;
    float padding3;

    float colorRed;
    float colorGreen;
    float colorBlue;
    float padding4;
};

struct FELightingPaintingDataForNeon {
    unsigned char* pixels;
    float yStart;
    int widthDecreasedByTwo;
    int absoluteHeight;
    // Combination of FLAG constants above.
    int flags;
    int specularExponent;
    int coneExponent;
    FELightingFloatArgumentsForNeon* floatArguments;
    short* paintingConstants;
};

short* feLightingConstantsForNeon();

extern "C" {
void neonDrawLighting(FELightingPaintingDataForNeon*);
}

inline void FELighting::platformApplyNeon(LightingData& data, LightSource::PaintingData& paintingData)
{
    WTF_ALIGNED(FELightingFloatArgumentsForNeon, floatArguments, 16);

    FELightingPaintingDataForNeon neonData = {
        data.pixels->data(),
        1,
        data.widthDecreasedByOne - 1,
        data.heightDecreasedByOne - 1,
        0,
        0,
        0,
        &floatArguments,
        feLightingConstantsForNeon()
    };

    // Set light source arguments.
    floatArguments.constOne = 1;

    floatArguments.colorRed = m_lightingColor.red();
    floatArguments.colorGreen = m_lightingColor.green();
    floatArguments.colorBlue = m_lightingColor.blue();
    floatArguments.padding4 = 0;

    if (m_lightSource->type() == LS_POINT) {
        neonData.flags |= FLAG_POINT_LIGHT;
        PointLightSource* pointLightSource = static_cast<PointLightSource*>(m_lightSource.get());
        floatArguments.lightX = pointLightSource->position().x();
        floatArguments.lightY = pointLightSource->position().y();
        floatArguments.lightZ = pointLightSource->position().z();
        floatArguments.padding2 = 0;
    } else if (m_lightSource->type() == LS_SPOT) {
        neonData.flags |= FLAG_SPOT_LIGHT;
        SpotLightSource* spotLightSource = static_cast<SpotLightSource*>(m_lightSource.get());
        floatArguments.lightX = spotLightSource->position().x();
        floatArguments.lightY = spotLightSource->position().y();
        floatArguments.lightZ = spotLightSource->position().z();
        floatArguments.padding2 = 0;

        floatArguments.directionX = paintingData.directionVector.x();
        floatArguments.directionY = paintingData.directionVector.y();
        floatArguments.directionZ = paintingData.directionVector.z();
        floatArguments.padding3 = 0;

        floatArguments.coneCutOffLimit = paintingData.coneCutOffLimit;
        floatArguments.coneFullLight = paintingData.coneFullLight;
        floatArguments.coneCutOffRange = paintingData.coneCutOffLimit - paintingData.coneFullLight;
        neonData.coneExponent = getPowerCoefficients(spotLightSource->specularExponent());
        if (spotLightSource->specularExponent() == 1)
            neonData.flags |= FLAG_CONE_EXPONENT_IS_1;
    } else {
        ASSERT(m_lightSource->type() == LS_DISTANT);
        floatArguments.lightX = paintingData.lightVector.x();
        floatArguments.lightY = paintingData.lightVector.y();
        floatArguments.lightZ = paintingData.lightVector.z();
        floatArguments.padding2 = 1;
    }

    // Set lighting arguments.
    floatArguments.surfaceScale = data.surfaceScale;
    floatArguments.minusSurfaceScaleDividedByFour = -data.surfaceScale / 4;
    if (m_lightingType == FELighting::DiffuseLighting)
        floatArguments.diffuseConstant = m_diffuseConstant;
    else {
        neonData.flags |= FLAG_SPECULAR_LIGHT;
        floatArguments.diffuseConstant = m_specularConstant;
        neonData.specularExponent = getPowerCoefficients(m_specularExponent);
        if (m_specularExponent == 1)
            neonData.flags |= FLAG_SPECULAR_EXPONENT_IS_1;
    }
    if (floatArguments.diffuseConstant == 1)
        neonData.flags |= FLAG_DIFFUSE_CONST_IS_1;

    int optimalThreadNumber = ((data.widthDecreasedByOne - 1) * (data.heightDecreasedByOne - 1)) / s_minimalRectDimension;
    if (optimalThreadNumber > 1) {
        // Initialize parallel jobs
        ParallelJobs<FELightingPaintingDataForNeon> parallelJobs(&WebCore::FELighting::platformApplyNeonWorker, optimalThreadNumber);

        // Fill the parameter array
        int job = parallelJobs.numberOfJobs();
        if (job > 1) {
            int yStart = 1;
            int yStep = (data.heightDecreasedByOne - 1) / job;
            for (--job; job >= 0; --job) {
                FELightingPaintingDataForNeon& params = parallelJobs.parameter(job);
                params = neonData;
                params.yStart = yStart;
                params.pixels += (yStart - 1) * (data.widthDecreasedByOne + 1) * 4;
                if (job > 0) {
                    params.absoluteHeight = yStep;
                    yStart += yStep;
                } else
                    params.absoluteHeight = data.heightDecreasedByOne - yStart;
            }
            parallelJobs.execute();
            return;
        }
    }

    neonDrawLighting(&neonData);
}

} // namespace WebCore

#endif // CPU(ARM_NEON) && COMPILER(GCC)

#endif // FELightingNEON_h
