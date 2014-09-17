/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2010 Zoltan Herczeg <zherczeg@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef LightSource_h
#define LightSource_h

#if ENABLE(FILTERS)
#include "FloatPoint3D.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

enum LightType {
    LS_DISTANT,
    LS_POINT,
    LS_SPOT
};

class TextStream;

class LightSource : public RefCounted<LightSource> {
public:

    // Light vectors must be calculated for every pixel during
    // painting. It is expensive to pass all these arguments to
    // a frequently called function, especially because not all
    // light sources require all of them. Instead, we just pass
    // a reference to the following structure
    struct PaintingData {
        // SVGFELighting also use them
        FloatPoint3D lightVector;
        FloatPoint3D colorVector;
        float lightVectorLength;
        // Private members
        FloatPoint3D directionVector;
        FloatPoint3D privateColorVector;
        float coneCutOffLimit;
        float coneFullLight;
        int specularExponent;
    };

    LightSource(LightType type)
        : m_type(type)
    { }

    virtual ~LightSource() { }

    LightType type() const { return m_type; }
    virtual TextStream& externalRepresentation(TextStream&) const = 0;

    virtual void initPaintingData(PaintingData&) = 0;
    // z is a float number, since it is the alpha value scaled by a user
    // specified "surfaceScale" constant, which type is <number> in the SVG standard
    virtual void updatePaintingData(PaintingData&, int x, int y, float z) = 0;

    bool setAzimuth(float);
    bool setElevation(float);
    bool setX(float);
    bool setY(float);
    bool setZ(float);
    bool setPointsAtX(float);
    bool setPointsAtY(float);
    bool setPointsAtZ(float);
    bool setSpecularExponent(float);
    bool setLimitingConeAngle(float);

private:
    LightType m_type;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // LightSource_h
