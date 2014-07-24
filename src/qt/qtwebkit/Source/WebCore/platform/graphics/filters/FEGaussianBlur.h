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

#ifndef FEGaussianBlur_h
#define FEGaussianBlur_h

#if ENABLE(FILTERS)
#include "FilterEffect.h"
#include "Filter.h"

namespace WTF {
template<typename Type> class ParallelJobs;
}

namespace WebCore {

class FEGaussianBlur : public FilterEffect {
public:
    static PassRefPtr<FEGaussianBlur> create(Filter*, float, float);

    float stdDeviationX() const;
    void setStdDeviationX(float);

    float stdDeviationY() const;
    void setStdDeviationY(float);

    static float calculateStdDeviation(float);

    virtual void platformApplySoftware();
    virtual void dump();
    
    virtual void determineAbsolutePaintRect();
    static void calculateKernelSize(Filter*, unsigned& kernelSizeX, unsigned& kernelSizeY, float stdX, float stdY);
    static void calculateUnscaledKernelSize(unsigned& kernelSizeX, unsigned& kernelSizeY, float stdX, float stdY);

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    static const int s_minimalRectDimension = 100 * 100; // Empirical data limit for parallel jobs

    template<typename Type>
    friend class WTF::ParallelJobs;

    struct PlatformApplyParameters {
        FEGaussianBlur* filter;
        RefPtr<Uint8ClampedArray> srcPixelArray;
        RefPtr<Uint8ClampedArray> dstPixelArray;
        int width;
        int height;
        unsigned kernelSizeX;
        unsigned kernelSizeY;
    };

    static void platformApplyWorker(PlatformApplyParameters*);

    FEGaussianBlur(Filter*, float, float);

    static inline void kernelPosition(int boxBlur, unsigned& std, int& dLeft, int& dRight);
    inline void platformApply(Uint8ClampedArray* srcPixelArray, Uint8ClampedArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize);

    inline void platformApplyGeneric(Uint8ClampedArray* srcPixelArray, Uint8ClampedArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize);

    float m_stdX;
    float m_stdY;
};

inline void FEGaussianBlur::kernelPosition(int boxBlur, unsigned& std, int& dLeft, int& dRight)
{
    // check http://www.w3.org/TR/SVG/filters.html#feGaussianBlurElement for details
    switch (boxBlur) {
    case 0:
        if (!(std % 2)) {
            dLeft = std / 2 - 1;
            dRight = std - dLeft;
        } else {
            dLeft = std / 2;
            dRight = std - dLeft;
        }
        break;
    case 1:
        if (!(std % 2)) {
            dLeft++;
            dRight--;
        }
        break;
    case 2:
        if (!(std % 2)) {
            dRight++;
            std++;
        }
        break;
    }
}

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FEGaussianBlur_h
