/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
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

#ifndef FEMorphology_h
#define FEMorphology_h

#if ENABLE(FILTERS)
#include "FilterEffect.h"
#include "Filter.h"

namespace WebCore {

enum MorphologyOperatorType {
    FEMORPHOLOGY_OPERATOR_UNKNOWN = 0,
    FEMORPHOLOGY_OPERATOR_ERODE = 1,
    FEMORPHOLOGY_OPERATOR_DILATE = 2
};

class FEMorphology : public FilterEffect {
public:
    static PassRefPtr<FEMorphology> create(Filter*, MorphologyOperatorType, float radiusX, float radiusY);  
    MorphologyOperatorType morphologyOperator() const;
    bool setMorphologyOperator(MorphologyOperatorType);

    float radiusX() const;
    bool setRadiusX(float);

    float radiusY() const;
    bool setRadiusY(float);

    virtual void platformApplySoftware();
    virtual void dump();

    virtual void determineAbsolutePaintRect();

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

    struct PaintingData {
        Uint8ClampedArray* srcPixelArray;
        Uint8ClampedArray* dstPixelArray;
        int width;
        int height;
        int radiusX;
        int radiusY;
    };

    static const int s_minimalArea = (300 * 300); // Empirical data limit for parallel jobs

    struct PlatformApplyParameters {
        FEMorphology* filter;
        int startY;
        int endY;
        PaintingData* paintingData;
    };

    static void platformApplyWorker(PlatformApplyParameters*);

    inline void platformApply(PaintingData*);
    inline void platformApplyGeneric(PaintingData*, const int yStart, const int yEnd);
private:
    FEMorphology(Filter*, MorphologyOperatorType, float radiusX, float radiusY);
    
    MorphologyOperatorType m_type;
    float m_radiusX;
    float m_radiusY;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FEMorphology_h
