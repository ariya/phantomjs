/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#include "config.h"

#if ENABLE(FILTERS)
#include "FEMorphology.h"

#include "Filter.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/ParallelJobs.h>
#include <wtf/Uint8ClampedArray.h>
#include <wtf/Vector.h>

using std::min;
using std::max;

namespace WebCore {

FEMorphology::FEMorphology(Filter* filter, MorphologyOperatorType type, float radiusX, float radiusY)
    : FilterEffect(filter)
    , m_type(type)
    , m_radiusX(radiusX)
    , m_radiusY(radiusY)
{
}

PassRefPtr<FEMorphology> FEMorphology::create(Filter* filter, MorphologyOperatorType type, float radiusX, float radiusY)
{
    return adoptRef(new FEMorphology(filter, type, radiusX, radiusY));
}

MorphologyOperatorType FEMorphology::morphologyOperator() const
{
    return m_type;
}

bool FEMorphology::setMorphologyOperator(MorphologyOperatorType type)
{
    if (m_type == type)
        return false;
    m_type = type;
    return true;
}

float FEMorphology::radiusX() const
{
    return m_radiusX;
}

bool FEMorphology::setRadiusX(float radiusX)
{
    if (m_radiusX == radiusX)
        return false;
    m_radiusX = radiusX;
    return true;
}

float FEMorphology::radiusY() const
{
    return m_radiusY;
}

void FEMorphology::determineAbsolutePaintRect()
{
    FloatRect paintRect = inputEffect(0)->absolutePaintRect();
    Filter* filter = this->filter();
    paintRect.inflateX(filter->applyHorizontalScale(m_radiusX));
    paintRect.inflateY(filter->applyVerticalScale(m_radiusY));
    if (clipsToBounds())
        paintRect.intersect(maxEffectRect());
    else
        paintRect.unite(maxEffectRect());
    setAbsolutePaintRect(enclosingIntRect(paintRect));
}

bool FEMorphology::setRadiusY(float radiusY)
{
    if (m_radiusY == radiusY)
        return false;
    m_radiusY = radiusY;
    return true;
}

void FEMorphology::platformApplyGeneric(PaintingData* paintingData, int yStart, int yEnd)
{
    Uint8ClampedArray* srcPixelArray = paintingData->srcPixelArray;
    Uint8ClampedArray* dstPixelArray = paintingData->dstPixelArray;
    const int width = paintingData->width;
    const int height = paintingData->height;
    const int effectWidth = width * 4;
    const int radiusX = paintingData->radiusX;
    const int radiusY = paintingData->radiusY;

    Vector<unsigned char> extrema;
    for (int y = yStart; y < yEnd; ++y) {
        int extremaStartY = max(0, y - radiusY);
        int extremaEndY = min(height - 1, y + radiusY);
        for (unsigned int clrChannel = 0; clrChannel < 4; ++clrChannel) {
            extrema.clear();
            // Compute extremas for each columns
            for (int x = 0; x <= radiusX; ++x) {
                unsigned char columnExtrema = srcPixelArray->item(extremaStartY * effectWidth + 4 * x + clrChannel);
                for (int eY = extremaStartY + 1; eY < extremaEndY; ++eY) {
                    unsigned char pixel = srcPixelArray->item(eY * effectWidth + 4 * x + clrChannel);
                    if ((m_type == FEMORPHOLOGY_OPERATOR_ERODE && pixel <= columnExtrema)
                        || (m_type == FEMORPHOLOGY_OPERATOR_DILATE && pixel >= columnExtrema)) {
                        columnExtrema = pixel;
                    }
                }

                extrema.append(columnExtrema);
            }

            // Kernel is filled, get extrema of next column
            for (int x = 0; x < width; ++x) {
                const int endX = min(x + radiusX, width - 1);
                unsigned char columnExtrema = srcPixelArray->item(extremaStartY * effectWidth + endX * 4 + clrChannel);
                for (int i = extremaStartY + 1; i <= extremaEndY; ++i) {
                    unsigned char pixel = srcPixelArray->item(i * effectWidth + endX * 4 + clrChannel);
                    if ((m_type == FEMORPHOLOGY_OPERATOR_ERODE && pixel <= columnExtrema)
                        || (m_type == FEMORPHOLOGY_OPERATOR_DILATE && pixel >= columnExtrema))
                        columnExtrema = pixel;
                }
                if (x - radiusX >= 0)
                    extrema.remove(0);
                if (x + radiusX <= width)
                    extrema.append(columnExtrema);

                unsigned char entireExtrema = extrema[0];
                for (unsigned kernelIndex = 1; kernelIndex < extrema.size(); ++kernelIndex) {
                    if ((m_type == FEMORPHOLOGY_OPERATOR_ERODE && extrema[kernelIndex] <= entireExtrema)
                        || (m_type == FEMORPHOLOGY_OPERATOR_DILATE && extrema[kernelIndex] >= entireExtrema))
                        entireExtrema = extrema[kernelIndex];
                }
                dstPixelArray->set(y * effectWidth + 4 * x + clrChannel, entireExtrema);
            }
        }
    }
}

void FEMorphology::platformApplyWorker(PlatformApplyParameters* param)
{
    param->filter->platformApplyGeneric(param->paintingData, param->startY, param->endY);
}

void FEMorphology::platformApply(PaintingData* paintingData)
{
    int optimalThreadNumber = (paintingData->width * paintingData->height) / s_minimalArea;
    if (optimalThreadNumber > 1) {
        WTF::ParallelJobs<PlatformApplyParameters> parallelJobs(&WebCore::FEMorphology::platformApplyWorker, optimalThreadNumber);
        int numOfThreads = parallelJobs.numberOfJobs();
        if (numOfThreads > 1) {
            // Split the job into "jobSize"-sized jobs but there a few jobs that need to be slightly larger since
            // jobSize * jobs < total size. These extras are handled by the remainder "jobsWithExtra".
            const int jobSize = paintingData->height / numOfThreads;
            const int jobsWithExtra = paintingData->height % numOfThreads;
            int currentY = 0;
            for (int job = numOfThreads - 1; job >= 0; --job) {
                PlatformApplyParameters& param = parallelJobs.parameter(job);
                param.filter = this;
                param.startY = currentY;
                currentY += job < jobsWithExtra ? jobSize + 1 : jobSize;
                param.endY = currentY;
                param.paintingData = paintingData;
            }
            parallelJobs.execute();
            return;
        }
        // Fallback to single thread model
    }

    platformApplyGeneric(paintingData, 0, paintingData->height);
}


void FEMorphology::platformApplySoftware()
{
    FilterEffect* in = inputEffect(0);

    Uint8ClampedArray* dstPixelArray = createPremultipliedImageResult();
    if (!dstPixelArray)
        return;

    setIsAlphaImage(in->isAlphaImage());
    if (m_radiusX <= 0 || m_radiusY <= 0) {
        dstPixelArray->zeroFill();
        return;
    }

    Filter* filter = this->filter();
    int radiusX = static_cast<int>(floorf(filter->applyHorizontalScale(m_radiusX)));
    int radiusY = static_cast<int>(floorf(filter->applyVerticalScale(m_radiusY)));

    IntRect effectDrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    RefPtr<Uint8ClampedArray> srcPixelArray = in->asPremultipliedImage(effectDrawingRect);

    PaintingData paintingData;
    paintingData.srcPixelArray = srcPixelArray.get();
    paintingData.dstPixelArray = dstPixelArray;
    paintingData.width = effectDrawingRect.width();
    paintingData.height = effectDrawingRect.height();
    paintingData.radiusX = min(effectDrawingRect.width() - 1, radiusX);
    paintingData.radiusY = min(effectDrawingRect.height() - 1, radiusY);

    platformApply(&paintingData);
}

void FEMorphology::dump()
{
}

static TextStream& operator<<(TextStream& ts, const MorphologyOperatorType& type)
{
    switch (type) {
    case FEMORPHOLOGY_OPERATOR_UNKNOWN:
        ts << "UNKNOWN";
        break;
    case FEMORPHOLOGY_OPERATOR_ERODE:
        ts << "ERODE";
        break;
    case FEMORPHOLOGY_OPERATOR_DILATE:
        ts << "DILATE";
        break;
    }
    return ts;
}

TextStream& FEMorphology::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feMorphology";
    FilterEffect::externalRepresentation(ts);
    ts << " operator=\"" << morphologyOperator() << "\" "
       << "radius=\"" << radiusX() << ", " << radiusY() << "\"]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
