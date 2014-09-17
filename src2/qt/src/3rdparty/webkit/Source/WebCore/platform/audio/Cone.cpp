/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "Cone.h"
#include <wtf/MathExtras.h>

namespace WebCore {

ConeEffect::ConeEffect()
    : m_innerAngle(360.0)
    , m_outerAngle(360.0)
    , m_outerGain(0.0)
{
}

double ConeEffect::gain(FloatPoint3D sourcePosition, FloatPoint3D sourceOrientation, FloatPoint3D listenerPosition)
{
    if (sourceOrientation.isZero() || ((m_innerAngle == 360.0) && (m_outerAngle == 360.0)))
        return 1.0; // no cone specified - unity gain

    // Normalized source-listener vector
    FloatPoint3D sourceToListener = listenerPosition - sourcePosition;
    sourceToListener.normalize();

    FloatPoint3D normalizedSourceOrientation = sourceOrientation;
    normalizedSourceOrientation.normalize();

    // Angle between the source orientation vector and the source-listener vector
    double dotProduct = sourceToListener.dot(normalizedSourceOrientation);
    double angle = 180.0 * acos(dotProduct) / piDouble;
    double absAngle = fabs(angle);

    // Divide by 2.0 here since API is entire angle (not half-angle)
    double absInnerAngle = fabs(m_innerAngle) / 2.0;
    double absOuterAngle = fabs(m_outerAngle) / 2.0;
    double gain = 1.0;

    if (absAngle <= absInnerAngle)
        // No attenuation
        gain = 1.0;
    else if (absAngle >= absOuterAngle)
        // Max attenuation
        gain = m_outerGain;
    else {
        // Between inner and outer cones
        // inner -> outer, x goes from 0 -> 1
        double x = (absAngle - absInnerAngle) / (absOuterAngle - absInnerAngle);
        gain = (1.0 - x) + m_outerGain * x;
    }

    return gain;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
