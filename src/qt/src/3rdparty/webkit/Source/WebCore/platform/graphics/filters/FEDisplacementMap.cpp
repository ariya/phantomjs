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
#include "FEDisplacementMap.h"

#include "Filter.h"
#include "GraphicsContext.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/ByteArray.h>

namespace WebCore {

FEDisplacementMap::FEDisplacementMap(Filter* filter, ChannelSelectorType xChannelSelector, ChannelSelectorType yChannelSelector, float scale)
    : FilterEffect(filter)
    , m_xChannelSelector(xChannelSelector)
    , m_yChannelSelector(yChannelSelector)
    , m_scale(scale)
{
}

PassRefPtr<FEDisplacementMap> FEDisplacementMap::create(Filter* filter, ChannelSelectorType xChannelSelector,
    ChannelSelectorType yChannelSelector, float scale)
{
    return adoptRef(new FEDisplacementMap(filter, xChannelSelector, yChannelSelector, scale));
}

ChannelSelectorType FEDisplacementMap::xChannelSelector() const
{
    return m_xChannelSelector;
}

bool FEDisplacementMap::setXChannelSelector(const ChannelSelectorType xChannelSelector)
{
    if (m_xChannelSelector == xChannelSelector)
        return false;
    m_xChannelSelector = xChannelSelector;
    return true;
}

ChannelSelectorType FEDisplacementMap::yChannelSelector() const
{
    return m_yChannelSelector;
}

bool FEDisplacementMap::setYChannelSelector(const ChannelSelectorType yChannelSelector)
{
    if (m_yChannelSelector == yChannelSelector)
        return false;
    m_yChannelSelector = yChannelSelector;
    return true;
}

float FEDisplacementMap::scale() const
{
    return m_scale;
}

bool FEDisplacementMap::setScale(float scale)
{
    if (m_scale == scale)
        return false;
    m_scale = scale;
    return true;
}

void FEDisplacementMap::apply()
{
    if (hasResult())
        return;
    FilterEffect* in = inputEffect(0);
    FilterEffect* in2 = inputEffect(1);
    in->apply();
    in2->apply();
    if (!in->hasResult() || !in2->hasResult())
        return;

    if (m_xChannelSelector == CHANNEL_UNKNOWN || m_yChannelSelector == CHANNEL_UNKNOWN)
        return;

    ByteArray* dstPixelArray = createPremultipliedImageResult();
    if (!dstPixelArray)
        return;

    IntRect effectADrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    RefPtr<ByteArray> srcPixelArrayA = in->asPremultipliedImage(effectADrawingRect);

    IntRect effectBDrawingRect = requestedRegionOfInputImageData(in2->absolutePaintRect());
    RefPtr<ByteArray> srcPixelArrayB = in2->asUnmultipliedImage(effectBDrawingRect);

    ASSERT(srcPixelArrayA->length() == srcPixelArrayB->length());

    Filter* filter = this->filter();
    IntSize paintSize = absolutePaintRect().size();
    float scaleX = filter->applyHorizontalScale(m_scale / 255);
    float scaleY = filter->applyVerticalScale(m_scale / 255);
    float scaleAdjustmentX = filter->applyHorizontalScale(0.5f - 0.5f * m_scale);
    float scaleAdjustmentY = filter->applyVerticalScale(0.5f - 0.5f * m_scale);
    int stride = paintSize.width() * 4;
    for (int y = 0; y < paintSize.height(); ++y) {
        int line = y * stride;
        for (int x = 0; x < paintSize.width(); ++x) {
            int dstIndex = line + x * 4;
            int srcX = x + static_cast<int>(scaleX * srcPixelArrayB->get(dstIndex + m_xChannelSelector - 1) + scaleAdjustmentX);
            int srcY = y + static_cast<int>(scaleY * srcPixelArrayB->get(dstIndex + m_yChannelSelector - 1) + scaleAdjustmentY);
            for (unsigned channel = 0; channel < 4; ++channel) {
                if (srcX < 0 || srcX >= paintSize.width() || srcY < 0 || srcY >= paintSize.height())
                    dstPixelArray->set(dstIndex + channel, static_cast<unsigned char>(0));
                else {
                    unsigned char pixelValue = srcPixelArrayA->get(srcY * stride + srcX * 4 + channel);
                    dstPixelArray->set(dstIndex + channel, pixelValue);
                }
            }
        }
    }
}

void FEDisplacementMap::dump()
{
}

static TextStream& operator<<(TextStream& ts, const ChannelSelectorType& type)
{
    switch (type) {
    case CHANNEL_UNKNOWN:
        ts << "UNKNOWN";
        break;
    case CHANNEL_R:
        ts << "RED";
        break;
    case CHANNEL_G:
        ts << "GREEN";
        break;
    case CHANNEL_B:
        ts << "BLUE";
        break;
    case CHANNEL_A:
        ts << "ALPHA";
        break;
    }
    return ts;
}

TextStream& FEDisplacementMap::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feDisplacementMap";
    FilterEffect::externalRepresentation(ts);
    ts << " scale=\"" << m_scale << "\" "
       << "xChannelSelector=\"" << m_xChannelSelector << "\" "
       << "yChannelSelector=\"" << m_yChannelSelector << "\"]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    inputEffect(1)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
