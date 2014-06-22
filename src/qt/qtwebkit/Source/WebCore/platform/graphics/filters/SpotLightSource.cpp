/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2010 Zoltan Herczeg <zherczeg@webkit.org>
 * Copyright (C) 2011 University of Szeged
 * Copyright (C) 2011 Renata Hodovan <reni@webkit.org>
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

#include "config.h"

#if ENABLE(FILTERS)
#include "SpotLightSource.h"

#include "TextStream.h"

namespace WebCore {

// spot-light edge darkening depends on an absolute treshold
// according to the SVG 1.1 SE light regression tests
static const float antiAliasTreshold = 0.016f;

void SpotLightSource::initPaintingData(PaintingData& paintingData)
{
    paintingData.privateColorVector = paintingData.colorVector;
    paintingData.directionVector.setX(m_direction.x() - m_position.x());
    paintingData.directionVector.setY(m_direction.y() - m_position.y());
    paintingData.directionVector.setZ(m_direction.z() - m_position.z());
    paintingData.directionVector.normalize();

    if (!m_limitingConeAngle) {
        paintingData.coneCutOffLimit = 0.0f;
        paintingData.coneFullLight = -antiAliasTreshold;
    } else {
        float limitingConeAngle = m_limitingConeAngle;
        if (limitingConeAngle < 0.0f)
            limitingConeAngle = -limitingConeAngle;
        if (limitingConeAngle > 90.0f)
            limitingConeAngle = 90.0f;
        paintingData.coneCutOffLimit = cosf(deg2rad(180.0f - limitingConeAngle));
        paintingData.coneFullLight = paintingData.coneCutOffLimit - antiAliasTreshold;
    }

    // Optimization for common specularExponent values
    if (!m_specularExponent)
        paintingData.specularExponent = 0;
    else if (m_specularExponent == 1.0f)
        paintingData.specularExponent = 1;
    else // It is neither 0.0f nor 1.0f
        paintingData.specularExponent = 2;
}

void SpotLightSource::updatePaintingData(PaintingData& paintingData, int x, int y, float z)
{
    paintingData.lightVector.setX(m_position.x() - x);
    paintingData.lightVector.setY(m_position.y() - y);
    paintingData.lightVector.setZ(m_position.z() - z);
    paintingData.lightVectorLength = paintingData.lightVector.length();

    float cosineOfAngle = (paintingData.lightVector * paintingData.directionVector) / paintingData.lightVectorLength;
    if (cosineOfAngle > paintingData.coneCutOffLimit) {
        // No light is produced, scanlines are not updated
        paintingData.colorVector.setX(0.0f);
        paintingData.colorVector.setY(0.0f);
        paintingData.colorVector.setZ(0.0f);
        return;
    }

    // Set the color of the pixel
    float lightStrength;
    switch (paintingData.specularExponent) {
    case 0:
        lightStrength = 1.0f; // -cosineOfAngle ^ 0 == 1
        break;
    case 1:
        lightStrength = -cosineOfAngle; // -cosineOfAngle ^ 1 == -cosineOfAngle
        break;
    default:
        lightStrength = powf(-cosineOfAngle, m_specularExponent);
        break;
    }

    if (cosineOfAngle > paintingData.coneFullLight)
        lightStrength *= (paintingData.coneCutOffLimit - cosineOfAngle) / (paintingData.coneCutOffLimit - paintingData.coneFullLight);

    if (lightStrength > 1.0f)
        lightStrength = 1.0f;

    paintingData.colorVector.setX(paintingData.privateColorVector.x() * lightStrength);
    paintingData.colorVector.setY(paintingData.privateColorVector.y() * lightStrength);
    paintingData.colorVector.setZ(paintingData.privateColorVector.z() * lightStrength);
}

bool SpotLightSource::setX(float x)
{
    if (m_position.x() == x)
        return false;
    m_position.setX(x);
    return true;
}

bool SpotLightSource::setY(float y)
{
    if (m_position.y() == y)
        return false;
    m_position.setY(y);
    return true;
}

bool SpotLightSource::setZ(float z)
{
    if (m_position.z() == z)
        return false;
    m_position.setZ(z);
    return true;
}

bool SpotLightSource::setPointsAtX(float pointsAtX)
{
    if (m_direction.x() == pointsAtX)
        return false;
    m_direction.setX(pointsAtX);
    return true;
}

bool SpotLightSource::setPointsAtY(float pointsAtY)
{
    if (m_direction.y() == pointsAtY)
        return false;
    m_direction.setY(pointsAtY);
    return true;
}

bool SpotLightSource::setPointsAtZ(float pointsAtZ)
{
    if (m_direction.z() == pointsAtZ)
        return false;
    m_direction.setZ(pointsAtZ);
    return true;
}

bool SpotLightSource::setSpecularExponent(float specularExponent)
{
    if (m_specularExponent == specularExponent)
        return false;
    m_specularExponent = specularExponent;
    return true;
}

bool SpotLightSource::setLimitingConeAngle(float limitingConeAngle)
{
    if (m_limitingConeAngle == limitingConeAngle)
        return false;
    m_limitingConeAngle = limitingConeAngle;
    return true;
}

static TextStream& operator<<(TextStream& ts, const FloatPoint3D& p)
{
    ts << "x=" << p.x() << " y=" << p.y() << " z=" << p.z();
    return ts;
}

TextStream& SpotLightSource::externalRepresentation(TextStream& ts) const
{
    ts << "[type=SPOT-LIGHT] ";
    ts << "[position=\"" << position() << "\"]";
    ts << "[direction=\"" << direction() << "\"]";
    ts << "[specularExponent=\"" << specularExponent() << "\"]";
    ts << "[limitingConeAngle=\"" << limitingConeAngle() << "\"]";
    return ts;
}

}; // namespace WebCore

#endif // ENABLE(FILTERS)
